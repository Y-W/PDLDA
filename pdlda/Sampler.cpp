/*
 * Sampler.cpp
 *
 *  Created on: Jul 30, 2014
 *      Author: yw
 */

#include "Sampler.h"

#include <mpi/mpi.h>
#include <stddef.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "ArgRegister.h"
#include "BookLoader.h"
#include "CntrServer.h"
#include "DocState.h"
#include "RandGen.h"
#include "tclap/ValueArg.h"
#include "TaskAssigner.h"
#include "TVectorPool.h"

using namespace pdlda;

static TCLAP::ValueArg<double> alphaArg("a", "alpha", "Alpha value", true, 0,
		"(0, 1)");
REGISTER_ARG(alphaArg)

static TCLAP::ValueArg<double> betaArg("b", "beta", "Beta value", true, 0,
		"(0, 1)");
REGISTER_ARG(betaArg)

Sampler* Sampler::inst;

static void updateCallBack(bool& arg0, void* buff, MPI_Status* status) {
	delete (CntrServer::updateInfo*) buff;
}

static void fetchSendCallBack(bool& arg0, void* buff, MPI_Status* status) {
	delete (word_id*) buff;
}

static void fetchCallBack(Sampler::doc_word& docWord, void* buff,
		MPI_Status* status) {
	if (docWord.docId < BookLoader::inst->numDoc) {
		Sampler::inst->sampleDocWord(docWord.docId, docWord.wordPos, buff);
	} else {
		if (docWord.docId == BookLoader::inst->numDoc) {
			Sampler::inst->updateDistUCache(buff);
		} else {
			Sampler::inst->sampleDocWord_test(docWord.docId, docWord.wordPos,
					buff);
		}
	}
	delete[] (num*) buff;
}

Sampler::Sampler() :
		updatePool(REQUEST_POOL_MAX_SIZE_COEFF * TaskAssigner::inst->numCntr,
				&updateCallBack), fetchSendPool(
		REQUEST_POOL_MAX_SIZE_COEFF * TaskAssigner::inst->numCntr,
				&fetchSendCallBack), fetchPool(
		REQUEST_POOL_MAX_SIZE_COEFF * TaskAssigner::inst->numCntr,
				&fetchCallBack) {
	distUCache = new num[BookLoader::inst->numU];
	alpha = alphaArg.getValue();
	beta = betaArg.getValue();
	Sampler::inst = this;
}

Sampler::~Sampler() {
	updatePool.waitTillClear();
	fetchSendPool.waitTillClear();
	fetchPool.waitTillClear();
	delete[] distUCache;
}

static size_t randDist(size_t size, double* prob) {
	double sum = 0;
	for (size_t i = 0; i < size; i++) {
		if (prob[i] < 0) {
			throw new std::runtime_error("Prob smaller than 0!");
		}
		sum += prob[i];
	}
	sum *= RandGen::inst->getDbl();
	for (size_t i = 0; i < size; i++) {
		sum -= prob[i];
		if (sum < 0) {
			return i;
		}
	}
	throw new std::runtime_error("Sum >= 0 till the end!");
}

void Sampler::initDocStates() {
	for (doc_id i = 0; i < BookLoader::inst->numDoc; i++) {
		if (TaskAssigner::inst->getSampId(i) != TaskAssigner::inst->rank) {
			continue;
		}
		DocState::doc& d = DocState::inst->getDocState(i);
		std::memset(d.distDocZ, 0, sizeof(num) * TVectorPool::inst->numZ);
		vector<topic_id> docZ;
		TVectorPool::inst->getZ4Doc(i, docZ);

		if (docZ.size() == 0) {
			throw new std::runtime_error("DocZ empty: NO z to choose!");
		}

		for (num wordPos = 0; wordPos < d.length; wordPos++) {

			size_t zIndex = RandGen::inst->getInt(docZ.size());
			topic_id z = docZ[zIndex];
			d.tokens[wordPos].z = z;
			d.distDocZ[z]++;
			topic_id u = randDist(BookLoader::inst->numU,
					TVectorPool::inst->getTVec(z));
			d.tokens[wordPos].u = u;
			CntrServer::updateInfo* buf = new CntrServer::updateInfo;
			buf->w = d.tokens[wordPos].w;
			buf->incU = u;
			buf->decU = BookLoader::inst->numU;
			MPI_Request* req = new MPI_Request;

			MPI_Issend(buf, sizeof(CntrServer::updateInfo), MPI_BYTE,
					TaskAssigner::inst->getCntrId(buf->w), TAG_UPDATE,
					MPI_COMM_WORLD, req);

			bool useless = true;

			updatePool.addRequest(req, buf, useless);
		}

	}

	updatePool.waitTillClear();
}

void Sampler::updateDistUCache() {
	std::memset(distUCache, 0, sizeof(num) * BookLoader::inst->numU);

	for (size_t i = 0; i < TaskAssigner::inst->numCntr; i++) {
		MPI_Request* req = new MPI_Request;
		word_id* buf = new word_id;
		*buf = BookLoader::inst->numWord;
		MPI_Issend(buf, 1, WORD_ID_MPI_TYPE, TaskAssigner::inst->lstCntrIds[i],
		TAG_FETCH, MPI_COMM_WORLD, req);
		bool useless = false;
		fetchSendPool.addRequest(req, buf, useless);
		req = new MPI_Request;
		num* buf2 = new num[BookLoader::inst->numU + 1];
		MPI_Irecv(buf2, BookLoader::inst->numU + 1, NUM_MPI_TYPE,
				TaskAssigner::inst->lstCntrIds[i],
				TAG_FETCH, MPI_COMM_WORLD, req);
		Sampler::doc_word dw;
		dw.docId = BookLoader::inst->numDoc;
		dw.wordPos = 0;
		fetchPool.addRequest(req, buf2, dw);
	}
	fetchPool.waitTillClear();
}

void Sampler::updateDistUCache(void* replyContent) {
	num* arr = (num*) replyContent;
	if (arr[0] != BookLoader::inst->numWord) {
		throw std::runtime_error(
				"Update Dist U Cache failed: not what's requested!");
	}
	for (size_t i = 0; i < BookLoader::inst->numU; i++) {
		distUCache[i] += arr[i + 1];
	}
}

void Sampler::sampleAllDocOnce() {
	for (doc_id i = 0; i < BookLoader::inst->numDoc; i++) {

		if (TaskAssigner::inst->getSampId(i) != TaskAssigner::inst->rank) {
			continue;
		}

		DocState::doc& d = DocState::inst->getDocState(i);
		for (num wordPos = 0; wordPos < d.length; wordPos++) {

			word_id* buf = new word_id;
			*buf = d.tokens[wordPos].w;
			MPI_Request* req = new MPI_Request;
			MPI_Issend(buf, 1, WORD_ID_MPI_TYPE,
					TaskAssigner::inst->getCntrId(d.tokens[wordPos].w),
					TAG_FETCH,
					MPI_COMM_WORLD, req);
			bool useless = false;
			fetchSendPool.addRequest(req, buf, useless);
			req = new MPI_Request;
			num* buf2 = new num[BookLoader::inst->numU + 1];
			MPI_Irecv(buf2, BookLoader::inst->numU + 1, NUM_MPI_TYPE,
					TaskAssigner::inst->getCntrId(d.tokens[wordPos].w),
					TAG_FETCH, MPI_COMM_WORLD, req);
			Sampler::doc_word dw;
			dw.docId = i;
			dw.wordPos = wordPos;
			fetchPool.addRequest(req, buf2, dw);
		}
	}
	fetchSendPool.waitTillClear();
	fetchPool.waitTillClear();
	updatePool.waitTillClear();
}

void Sampler::sampleDocWord(doc_id docId, num wordPos, void* replyContent) {
	num* arr = (num*) replyContent;
	DocState::doc& d = DocState::inst->getDocState(docId);
	DocState::zuw* tk = d.tokens + wordPos;
	if (arr[0] != d.tokens[wordPos].w) {
		std::stringstream tmp;
		tmp << "Sample Doc Word failed: not what's requested! " << arr[0] << " "
				<< d.tokens[wordPos].w << " " << docId << " " << wordPos;
		throw std::runtime_error(tmp.str());
	}
	vector<topic_id> docZ;
	TVectorPool::inst->getZ4Doc(docId, docZ);
	double* probs = new double[docZ.size() * BookLoader::inst->numU];
	for (size_t z_i = 0; z_i < docZ.size(); z_i++) {
		topic_id z = docZ[z_i];
		for (topic_id u = 0; u < BookLoader::inst->numU; u++) {
			num cntz = d.distDocZ[z];
			if (tk->z == z)
				cntz -= 1;
			num cntwu = arr[u + 1];
			num cntu = distUCache[u];
			if (tk->u == u) {
				cntwu -= 1;
				cntu -= 1;
			}

			probs[z_i * BookLoader::inst->numU + u] =
					TVectorPool::inst->getTVec(z)[u] * (alpha + cntz)
							* (beta + cntwu)
							/ (beta * BookLoader::inst->numWord + cntu);
		}
	}
	size_t pick = randDist(docZ.size() * BookLoader::inst->numU, probs);
	topic_id nu = pick % BookLoader::inst->numU;
	topic_id nz = docZ[pick / BookLoader::inst->numU];

	distUCache[tk->u]--;
	distUCache[nu]++;
	d.distDocZ[tk->z]--;
	d.distDocZ[nz]++;

	CntrServer::updateInfo* buf = new CntrServer::updateInfo;
	buf->w = tk->w;
	buf->incU = nu;
	buf->decU = tk->u;
	MPI_Request* req = new MPI_Request;
	MPI_Issend(buf, sizeof(CntrServer::updateInfo), MPI_BYTE,
			TaskAssigner::inst->getCntrId(buf->w), TAG_UPDATE,
			MPI_COMM_WORLD, req);
	bool useless = true;
	updatePool.addRequest(req, buf, useless);

	tk->u = nu;
	tk->z = nz;

	delete[] probs;
}

void Sampler::initDocStates_test() {
	for (doc_id i = 0; i < BookLoader::inst->numDoc_test; i++) {
		if (TaskAssigner::inst->getSampId(i) != TaskAssigner::inst->rank) {
			continue;
		}
		DocState::doc& d = DocState::inst->getDocState_test(i);
		std::memset(d.distDocZ, 0, sizeof(num) * TVectorPool::inst->numZ);
		vector<topic_id> docZ;
		TVectorPool::inst->getZ4Doc_all(docZ);

		for (num wordPos = 0; wordPos < d.length; wordPos++) {

			size_t zIndex = RandGen::inst->getInt(docZ.size());
			topic_id z = docZ[zIndex];
			d.tokens[wordPos].z = z;
			d.distDocZ[z]++;
			topic_id u = randDist(BookLoader::inst->numU,
					TVectorPool::inst->getTVec(z));
			d.tokens[wordPos].u = u;
		}
	}
}

void Sampler::sampleAllDocOnce_test() {
	for (doc_id i = 0; i < BookLoader::inst->numDoc_test; i++) {

		if (TaskAssigner::inst->getSampId(i) != TaskAssigner::inst->rank) {
			continue;
		}

		DocState::doc& d = DocState::inst->getDocState_test(i);
		for (num wordPos = 0; wordPos < d.length; wordPos++) {

			word_id* buf = new word_id;
			*buf = d.tokens[wordPos].w;
			MPI_Request* req = new MPI_Request;
			MPI_Issend(buf, 1, WORD_ID_MPI_TYPE,
					TaskAssigner::inst->getCntrId(d.tokens[wordPos].w),
					TAG_FETCH,
					MPI_COMM_WORLD, req);
			bool useless = false;
			fetchSendPool.addRequest(req, buf, useless);
			req = new MPI_Request;
			num* buf2 = new num[BookLoader::inst->numU + 1];
			MPI_Irecv(buf2, BookLoader::inst->numU + 1, NUM_MPI_TYPE,
					TaskAssigner::inst->getCntrId(d.tokens[wordPos].w),
					TAG_FETCH, MPI_COMM_WORLD, req);
			Sampler::doc_word dw;
			dw.docId = i + 1 + BookLoader::inst->numDoc;
			dw.wordPos = wordPos;
			fetchPool.addRequest(req, buf2, dw);
		}
	}
	fetchSendPool.waitTillClear();
	fetchPool.waitTillClear();
}

void Sampler::sampleDocWord_test(doc_id docId, num wordPos,
		void* replyContent) {
	docId -= (1 + BookLoader::inst->numDoc);
	num* arr = (num*) replyContent;
	DocState::doc& d = DocState::inst->getDocState_test(docId);
	DocState::zuw* tk = d.tokens + wordPos;
	if (arr[0] != d.tokens[wordPos].w) {
		std::stringstream tmp;
		tmp << "Sample Doc Word failed: not what's requested! " << arr[0] << " "
				<< d.tokens[wordPos].w << " " << docId << " " << wordPos;
		throw std::runtime_error(tmp.str());
	}
	vector<topic_id> docZ;
	TVectorPool::inst->getZ4Doc_all(docZ);
	double* probs = new double[docZ.size() * BookLoader::inst->numU];
	for (size_t z_i = 0; z_i < docZ.size(); z_i++) {
		topic_id z = docZ[z_i];
		for (topic_id u = 0; u < BookLoader::inst->numU; u++) {
			num cntz = d.distDocZ[z];
			if (tk->z == z)
				cntz -= 1;
			num cntwu = arr[u + 1];
			num cntu = distUCache[u];
//			if (tk->u == u) {
//				cntwu -= 1;
//				cntu -= 1;
//			}

			probs[z_i * BookLoader::inst->numU + u] =
					TVectorPool::inst->getTVec(z)[u] * (alpha + cntz)
							* (beta + cntwu)
							/ (beta * BookLoader::inst->numWord + cntu);
		}
	}
	size_t pick = randDist(docZ.size() * BookLoader::inst->numU, probs);
	topic_id nu = pick % BookLoader::inst->numU;
	topic_id nz = docZ[pick / BookLoader::inst->numU];

//	distUCache[tk->u]--;
//	distUCache[nu]++;
	d.distDocZ[tk->z]--;
	d.distDocZ[nz]++;

//	CntrServer::updateInfo* buf = new CntrServer::updateInfo;
//	buf->w = tk->w;
//	buf->incU = nu;
//	buf->decU = tk->u;
//	MPI_Request* req = new MPI_Request;
//	MPI_Issend(buf, sizeof(CntrServer::updateInfo), MPI_BYTE,
//			TaskAssigner::inst->getCntrId(buf->w), TAG_UPDATE,
//			MPI_COMM_WORLD, req);
//	bool useless = true;
//	updatePool.addRequest(req, buf, useless);

	tk->u = nu;
	tk->z = nz;

	delete[] probs;
}

static bool cmpResult(std::pair<label_id, num> a, std::pair<label_id, num> b) {
	return (a.second > b.second);
}

void Sampler::judgeTest() {
	num all = 0;
	num all_correct = 0;
	double first_k_percent = 0;
	double last_crLb_percent = 0;
	double first_crLb_1rank = 0;
	double pair_percent = 0;

	num lbCorrect = 0;
	num lbWrong = 0;
	num sumCrLb = 0;
	num sumWrLb = 0;
	for (doc_id i = 0; i < BookLoader::inst->numDoc_test; i++) {

		if (TaskAssigner::inst->getSampId(i) != TaskAssigner::inst->rank) {
			continue;
		}

		std::vector<std::pair<label_id, num> > results;
		DocState::doc& d = DocState::inst->getDocState_test(i);
		for (label_id l = 0; l < BookLoader::inst->numLabel; l++) {
			vector<topic_id> docZ;
			TVectorPool::inst->getZ4Lb_test(l, docZ);
			num cnt = 0;
			for (size_t pos = 0; pos < docZ.size(); pos++) {
				topic_id z = docZ[pos];
				cnt += d.distDocZ[z];
			}

			results.push_back(std::pair<label_id, num>(l, cnt));

			if (BookLoader::inst->documents_test[i].labels.count(l) > 0) {
				sumCrLb += cnt;
				lbCorrect++;
			} else {
				sumWrLb += cnt;
				lbWrong++;
			}
		}

		std::sort(results.begin(), results.end(), cmpResult);

		all += 1;

		label_id numCrLb = BookLoader::inst->documents_test[i].labels.size();
		label_id cntCrlb = 0;
		for (label_id p = 0; p < numCrLb; p++) {
			if (BookLoader::inst->documents_test[i].labels.count(
					results[p].first) > 0) {
				cntCrlb += 1;
			}
		}

		if (cntCrlb == numCrLb) {
			all_correct += 1;
		}

		first_k_percent += (cntCrlb * 1.0) / numCrLb;

		label_id fstPos = BookLoader::inst->numLabel;
		label_id lstPos = BookLoader::inst->numLabel;
		for (label_id p = 0; p < BookLoader::inst->numLabel; p++) {
			if (BookLoader::inst->documents_test[i].labels.count(
					results[p].first) > 0) {
				lstPos = p;
				if (fstPos == BookLoader::inst->numLabel) {
					fstPos = p;
				}
			}
		}

		last_crLb_percent += (lstPos + 1 - numCrLb) * 1.0 / (BookLoader::inst->numLabel - numCrLb);
		first_crLb_1rank += 1.0 / (fstPos + 1.0);

		num cntCrPr = 0;
		num cntWrPr = 0;

		for (label_id p = 0; p < BookLoader::inst->numLabel; p++) {
			if (BookLoader::inst->documents_test[i].labels.count(
					results[p].first) > 0) {
				for (label_id q = p + 1; q < BookLoader::inst->numLabel; q++) {
					if (BookLoader::inst->documents_test[i].labels.count(
							results[q].first) == 0) {
						cntCrPr += 1;
					}
				}
			} else {
				for (label_id q = p + 1; q < BookLoader::inst->numLabel; q++) {
					if (BookLoader::inst->documents_test[i].labels.count(
							results[q].first) > 0) {
						cntWrPr += 1;
					}
				}
			}
		}

		pair_percent += (cntCrPr) / (0.0 + cntCrPr + cntWrPr);
	}

	std::stringstream tmp;
	tmp << " @ " << (int) TaskAssigner::inst->rank << " - Test result:\n"
			<< "all_correct: " << all_correct << " / " << all << "\n"
			<< "first_k_correct_percent: " << first_k_percent << " / " << all
			<< "\n" << "last_correct_pos: " << last_crLb_percent << " / " << all
			<< "\n" << "first_correct_1rank: " << first_crLb_1rank << " / "
			<< all << "\n" << "pair_percent: " << pair_percent << " / " << all
			<< "\n" << "C/W lb: " << lbCorrect << " / " << lbWrong << "\n"
			<< "C/W lb_count: " << sumCrLb << " / " << sumWrLb << "\n";
	std::cout << tmp.str();

	num sum_all = 0;
	num sum_all_correct = 0;
	double sum_first_k_percent = 0;
	double sum_last_crLb_percent = 0;
	double sum_first_crLb_1rank = 0;
	double sum_pair_percent = 0;

	num sum_lbCorrect = 0;
	num sum_lbWrong = 0;
	num sum_sumCrLb = 0;
	num sum_sumWrLb = 0;

	MPI_Allreduce(&all, &sum_all, 1, NUM_MPI_TYPE, MPI_SUM,
			TaskAssigner::inst->sampComm);
	MPI_Allreduce(&all_correct, &sum_all_correct, 1, NUM_MPI_TYPE, MPI_SUM,
			TaskAssigner::inst->sampComm);

	MPI_Allreduce(&first_k_percent, &sum_first_k_percent, 1, MPI_DOUBLE,
	MPI_SUM, TaskAssigner::inst->sampComm);
	MPI_Allreduce(&last_crLb_percent, &sum_last_crLb_percent, 1, MPI_DOUBLE,
	MPI_SUM, TaskAssigner::inst->sampComm);
	MPI_Allreduce(&first_crLb_1rank, &sum_first_crLb_1rank, 1, MPI_DOUBLE,
	MPI_SUM, TaskAssigner::inst->sampComm);
	MPI_Allreduce(&pair_percent, &sum_pair_percent, 1, MPI_DOUBLE, MPI_SUM,
			TaskAssigner::inst->sampComm);

	MPI_Allreduce(&lbCorrect, &sum_lbCorrect, 1, NUM_MPI_TYPE, MPI_SUM,
			TaskAssigner::inst->sampComm);
	MPI_Allreduce(&lbWrong, &sum_lbWrong, 1, NUM_MPI_TYPE, MPI_SUM,
			TaskAssigner::inst->sampComm);
	MPI_Allreduce(&sumCrLb, &sum_sumCrLb, 1, NUM_MPI_TYPE, MPI_SUM,
			TaskAssigner::inst->sampComm);
	MPI_Allreduce(&sumWrLb, &sum_sumWrLb, 1, NUM_MPI_TYPE, MPI_SUM,
			TaskAssigner::inst->sampComm);

	if (TaskAssigner::inst->rank == TaskAssigner::inst->lstSampIds[0]) {
		std::stringstream tmp;
		tmp << " ALL - Test result:\n" << "all_correct: " << sum_all_correct
				<< " / " << sum_all << "\n" << "first_k_correct_percent: "
				<< sum_first_k_percent << " / " << sum_all << "\n"
				<< "last_correct_pos: " << sum_last_crLb_percent << " / "
				<< sum_all << "\n" << "first_correct_1rank: "
				<< sum_first_crLb_1rank << " / " << sum_all << "\n"
				<< "pair_percent: " << sum_pair_percent << " / " << sum_all
				<< "\n" << "C/W lb: " << sum_lbCorrect << " / " << sum_lbWrong
				<< "\n" << "C/W lb_count: " << sum_sumCrLb << " / "
				<< sum_sumWrLb << "\n";
		std::cout << tmp.str();
	}
}
