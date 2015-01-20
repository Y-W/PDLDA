/*
 * alg.cpp
 *
 *  Created on: Jan 16, 2015
 *      Author: yw
 */

#include "alg.h"
#include "param.h"
#include "data.h"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>

using std::size_t;
using namespace pdlda;

static size_t sampDist(size_t length, double* dist, uint32_t rnd) {
	double sum = 0;
	for (size_t i = 0; i < length; i++) {
		sum += dist[i];
	}
	sum *= (rnd) / (double) UINT32_MAX;
	for (size_t i = 0; i < length; i++) {
		sum -= dist[i];
		if (sum < 0) {
			return i;
		}
	}
	throw std::runtime_error("Sum >= 0 till the end!");
}

static size_t sampDist(size_t length, num* dist, uint32_t rnd) {
	double sum = 0;
	for (size_t i = 0; i < length; i++) {
		sum += dist[i];
	}
	sum *= (rnd) / (double) UINT32_MAX;
	for (size_t i = 0; i < length; i++) {
		sum -= dist[i];
		if (sum < 0) {
			return i;
		}
	}
	throw std::runtime_error("Sum >= 0 till the end!");
}

#define INC_RND_ST_POS(X) X = (X == RND_LENGTH -1 ? 0 : X + 1)

static inline bool hasLabel(label_set v, label_id k) {
	return ((v & (1 << k)) != 0);
}

static inline label_id cntNumLabel(label_set v) {
	label_id ans = 0;
	while (v > 0) {
		ans += (v & 0x1);
		v >>= 1;
	}
	return ans;
}

static inline topic_id cntZNum(label_set v) {
	return Param::numBgZ + cntNumLabel(v) * Param::numLbZ;
}

//static inline topic_id getIthZ(label_set v, topic_id i) {
//	if (i < Param::numBgZ)
//		return i;
//	else
//		i -= Param::numBgZ;
//	topic_id ans = Param::numBgZ;
//	while (v > 0 && ans < Param::numZ) {
//		if ((v & 0x1) == 1) {
//			if (i < Param::numLbZ)
//				return ans + i;
//			else {
//				i -= Param::numLbZ;
//			}
//		}
//		ans += Param::numLbZ;
//		v >>= 1;
//	}
//	throw std::runtime_error("I-th Z is out of boundary!");
//}

static inline void listZ(label_set v, topic_id* arr) {
	for (topic_id i = 0; i < Param::numBgZ; i++) {
		*(arr++) = i;
	}
	topic_id next = Param::numBgZ;
	while (v > 0) {
		if ((v & 0x1) == 1) {
			for (topic_id i = 0; i < Param::numLbZ; i++) {
				*(arr++) = (next++);
			}
		} else {
			next += Param::numLbZ;
		}
		v >>= 1;
	}
}

void Alg::trainStateInit() {
#pragma omp parallel for
	for (size_t i = 0; i < Param::numWord * (size_t) Param::numU; i++) {
		Data::cntWU[i] = 0;
	}

#pragma omp parallel for schedule(dynamic)
	for (doc_id i = 0; i < Param::numTrainDoc; i++) {
		size_t rndPos = (Data::rndStPos + i * (size_t) RND_ST_INC2) % RND_LENGTH;
		Data::doc_state& ds = Data::trainDocStates[i];
		for (size_t j = 0; j < Param::numZ; j++) {
			ds.cntZ[j] = 0;
		}
		topic_id numAvailZ = cntZNum(ds.labels);
		topic_id* availZ = new topic_id[numAvailZ];
		listZ(ds.labels, availZ);

		for (size_t j = 0; j < ds.length; j++) {
			Data::zuw& tkn = ds.tokens[j];
			tkn.z = availZ[Data::rnd[rndPos] % numAvailZ];
			INC_RND_ST_POS(rndPos);
			tkn.u = sampDist(Param::numU, Data::getTmxRow(tkn.z),
					Data::rnd[rndPos]);
			INC_RND_ST_POS(rndPos);
			ds.cntZ[tkn.z]++;
#pragma omp atomic
			Data::getCntWURow(tkn.w)[tkn.u]++;
		}

		delete[] availZ;
	}

	Data::rndStPos = (Data::rndStPos + RND_ST_INC1) % RND_LENGTH;

#pragma omp parallel for
	for (topic_id u = 0; u < Param::numU; u++) {
		num sum = 0;
		for (word_id w = 0; w < Param::numWord; w++) {
			sum += Data::getCntWURow(w)[u];
		}
		Data::cntU[u] = sum;
	}
}
void Alg::testStateInit() {
#pragma omp parallel for schedule(dynamic)
	for (doc_id i = 0; i < Param::numTestDoc; i++) {
		size_t rndPos = (Data::rndStPos + i * (size_t) RND_ST_INC2) % RND_LENGTH;
		Data::doc_state& ds = Data::testDocStates[i];
		for (size_t j = 0; j < Param::numZ; j++) {
			ds.cntZ[j] = 0;
		}
		double* tmpDistZ = new double[Param::numZ];

		for (size_t j = 0; j < ds.length; j++) {
			Data::zuw& tkn = ds.tokens[j];
			tkn.u = sampDist(Param::numU, Data::getCntWURow(tkn.w),
					Data::rnd[rndPos]);
			INC_RND_ST_POS(rndPos);

			for (size_t k = 0; k < Param::numZ; k++) {
				tmpDistZ[k] = Data::getTmxRow(k)[tkn.u];
			}
			tkn.z = sampDist(Param::numZ, tmpDistZ, Data::rnd[rndPos]);
			INC_RND_ST_POS(rndPos);
			ds.cntZ[tkn.z]++;
		}

		delete[] tmpDistZ;
	}

	Data::rndStPos = (Data::rndStPos + RND_ST_INC1) % RND_LENGTH;
}
void Alg::trainGibbsSamp() {
#pragma omp parallel for schedule(dynamic)
	for (doc_id d = 0; d < Param::numTrainDoc; d++) {
		size_t rndPos = (Data::rndStPos + d * (size_t) RND_ST_INC2) % RND_LENGTH;
		Data::doc_state& ds = Data::trainDocStates[d];
		double* aU = new double[Param::numU];
		size_t distLen = (Param::numU > Param::numZ ? Param::numU : Param::numZ);
		double* dist = new double[distLen];
		double betaNumW = Param::beta * Param::numWord;
		for (size_t u = 0; u < Param::numU; u++) {
			aU[u] = 0;
			for (size_t z = 0; z < Param::numZ; z++) {
				aU[u] += (Data::getTmxRow(z)[u]) * (Param::alpha + ds.cntZ[z]);
			}
		}
		for (size_t p = 0; p < ds.length; p++) {
			Data::zuw cpy = ds.tokens[p];
			for (size_t u = 0; u < Param::numU; u++) {
				if (cpy.u == u) {
					dist[u] = (aU[u] - Data::getTmxRow(cpy.z)[u])
							* (Data::getCntWURow(cpy.w)[u] - 1 + Param::beta)
							/ (Data::cntU[u] - 1 + betaNumW);
				} else {
					dist[u] = (aU[u] - Data::getTmxRow(cpy.z)[u])
							* (Data::getCntWURow(cpy.w)[u] + Param::beta)
							/ (Data::cntU[u] + betaNumW);
				}
			}
			topic_id newU = sampDist(Param::numU, dist, Data::rnd[rndPos]);
			INC_RND_ST_POS(rndPos);
			for (size_t z = 0; z < Param::numZ; z++) {
				if (cpy.z == z) {
					dist[z] = (ds.cntZ[z] - 1 + Param::alpha)
							* Data::getTmxRow(z)[newU];
				} else {
					dist[z] = (ds.cntZ[z] + Param::alpha)
							* Data::getTmxRow(z)[newU];
				}
			}
			topic_id newZ = sampDist(Param::numZ, dist, Data::rnd[rndPos]);
			INC_RND_ST_POS(rndPos);
			ds.tokens[p].z = newZ;
			ds.tokens[p].u = newU;
			ds.cntZ[cpy.z]--;
			ds.cntZ[newZ]++;
			for (size_t u = 0; u < Param::numU; u++) {
				aU[u] += (Data::getTmxRow(newZ)[u])
						- (Data::getTmxRow(cpy.z)[u]);
			}
#pragma omp atomic
			Data::getCntWURow(cpy.w)[cpy.u]--;
#pragma omp atomic
			Data::cntU[cpy.u]--;
#pragma omp atomic
			Data::getCntWURow(cpy.w)[newU]++;
#pragma omp atomic
			Data::cntU[newU]++;
		}

		delete[] aU;
		delete[] dist;
	}

	Data::rndStPos = (Data::rndStPos + RND_ST_INC1) % RND_LENGTH;
}
void Alg::testGibbsSamp() {
#pragma omp parallel for schedule(dynamic)
	for (doc_id d = 0; d < Param::numTestDoc; d++) {
		size_t rndPos = (Data::rndStPos + d * (size_t) RND_ST_INC2) % RND_LENGTH;
		Data::doc_state& ds = Data::testDocStates[d];
		double* aU = new double[Param::numU];
		size_t distLen = (Param::numU > Param::numZ ? Param::numU : Param::numZ);
		double* dist = new double[distLen];
		double betaNumW = Param::beta * Param::numWord;
		for (size_t u = 0; u < Param::numU; u++) {
			aU[u] = 0;
			for (size_t z = 0; z < Param::numZ; z++) {
				aU[u] += (Data::getTmxRow(z)[u]) * (Param::alpha + ds.cntZ[z]);
			}
		}
		for (size_t p = 0; p < ds.length; p++) {
			Data::zuw cpy = ds.tokens[p];
			for (size_t u = 0; u < Param::numU; u++) {
				dist[u] = (aU[u] - Data::getTmxRow(cpy.z)[u])
						* (Data::getCntWURow(cpy.w)[u] + Param::beta)
						/ (Data::cntU[u] + betaNumW);
			}
			topic_id newU = sampDist(Param::numU, dist, Data::rnd[rndPos]);
			INC_RND_ST_POS(rndPos);
			for (size_t z = 0; z < Param::numZ; z++) {
				if (cpy.z == z) {
					dist[z] = (ds.cntZ[z] - 1 + Param::alpha)
							* Data::getTmxRow(z)[newU];
				} else {
					dist[z] = (ds.cntZ[z] + Param::alpha)
							* Data::getTmxRow(z)[newU];
				}
			}
			topic_id newZ = sampDist(Param::numZ, dist, Data::rnd[rndPos]);
			INC_RND_ST_POS(rndPos);
			ds.tokens[p].z = newZ;
			ds.tokens[p].u = newU;
			ds.cntZ[cpy.z]--;
			ds.cntZ[newZ]++;
			for (size_t u = 0; u < Param::numU; u++) {
				aU[u] += (Data::getTmxRow(newZ)[u])
						- (Data::getTmxRow(cpy.z)[u]);
			}
		}
		delete[] aU;
		delete[] dist;
	}

	Data::rndStPos = (Data::rndStPos + RND_ST_INC1) % RND_LENGTH;
}
void Alg::clearZUCnt() {
#pragma omp parallel for
	for (size_t i = 0; i < Param::numZ * (size_t) Param::numU; i++) {
		Data::trainZUDistAccum[i] = 0;
	}
}
void Alg::accumZUCnt() {
#pragma omp parallel for schedule(dynamic)
	for (size_t i = 0; i < Param::numTrainDoc; i++) {
		for (size_t j = 0; j < Data::trainDocStates[i].length; j++) {
			Data::zuw* tkn = Data::trainDocStates[i].tokens + j;
#pragma omp atomic
			Data::trainZUDistAccum[tkn->z * (size_t) Param::numU + tkn->u] += 1;
		}
	}
}

static void normalizeVector(double* arr, size_t size) {
	double sum = 0;
	for (size_t i = 0; i < size; i++) {
		sum += arr[i];
	}
	sum = 1.0 / sum;
	for (size_t i = 0; i < size; i++) {
		arr[i] *= sum;
	}
}

static void vectorMinus(double* source, double* dest, double coeff,
		size_t size) {
#pragma omp parallel for
	for (size_t i = 0; i < size; i++) {
		dest[i] -= source[i] * coeff;
	}
}

void Alg::updateTmx() {
#pragma omp parallel for
	for (size_t i = 0; i < (size_t) Param::numZ * Param::numU; i++) {
		Data::tmx[i] = Data::trainZUDistAccum[i];
	}
#pragma omp parallel for
	for (size_t z = 0; z < Param::numZ; z++) {
		normalizeVector(Data::getTmxRow(z), Param::numU);
	}
	if (Param::tmxRegIter == 0 && Param::tmxRegCoef == 0) {
		return;
	}
	if (Param::tmxRegIter == 0) {

		for (topic_id i = 0; i < Param::numBgZ; i++) {
			for (topic_id j = 0; j < i; j++) {
				vectorMinus(Data::getTmxRow(j), Data::getTmxRow(i),
						Param::tmxRegCoef, Param::numU);
			}
			normalizeVector(Data::getTmxRow(i), Param::numU);
		}

		for (topic_id i = 0; i < Param::numLabel; i++) {
			for (topic_id j = 0; j < Param::numLbZ; j++) {
				topic_id destZ = Param::numBgZ + i * Param::numLbZ + j;
				for (topic_id k = 0; k < Param::numBgZ; k++) {
					vectorMinus(Data::getTmxRow(k), Data::getTmxRow(destZ),
							Param::tmxRegCoef, Param::numU);
				}
				for (topic_id k = 0; k < j; k++) {
					vectorMinus(
							Data::getTmxRow(
									Param::numBgZ + i * Param::numLbZ + k),
							Data::getTmxRow(destZ), Param::tmxRegCoef,
							Param::numU);
				}
				normalizeVector(Data::getTmxRow(destZ), Param::numU);
			}
		}
	} else {
		double* colSum = new double[Param::numU];
#pragma omp parallel for
		for (size_t j = 0; j < Param::numU; j++) {
			colSum[j] = 0;
		}

		num* countSum = new num[Param::numZ];

#pragma omp parallel for
		for (topic_id i = 0; i < Param::numZ; i++) {
			countSum[i] = 0;
			for (size_t j = 0; j < Param::numU; j++) {
#pragma omp atomic
				colSum[j] += Data::getTmxRow(i)[j];
				countSum[i] += Data::trainZUDistAccum[(size_t) i
						* ((size_t) Param::numU) + j];
			}
		}

		for (num itr = 0; itr < Param::tmxRegIter; itr++) {
			for (topic_id r = 0; r < Param::numZ; r++) {
#pragma omp parallel for
				for (size_t c = 0; c < Param::numU; c++) {
					colSum[c] -= Data::getTmxRow(r)[c];
					Data::getTmxRow(r)[c] = (Data::trainZUDistAccum[r
							* (size_t) Param::numU + c]
							- Param::tmxRegCoef * (colSum[c]))
							/ (countSum[r]
									- Param::tmxRegCoef * (Param::numZ - 1));
					if (Data::getTmxRow(r)[c] < 0) {
						Data::getTmxRow(r)[c] = 0;
					}
					colSum[c] += Data::getTmxRow(r)[c];
				}
			}
		}

		delete[] countSum;
		delete[] colSum;
	}
}
void Alg::clearTestLabelCnt() {
#pragma omp parallel for
	for (size_t i = 0; i < Param::numTestDoc * (size_t) Param::numLabel; i++) {
		Data::testLabelDistAccum[i] = 0;
	}
}
void Alg::accumTestLabelCnt() {
#pragma omp parallel for
	for (size_t i = 0; i < Param::numTestDoc; i++) {
		for (size_t j = 0; j < Param::numLabel; j++) {
			for (size_t k = 0; k < Param::numLbZ; k++) {
#pragma omp atomic
				Data::testLabelDistAccum[i * (size_t) Param::numLabel + j] +=
						Data::testDocStates[i].cntZ[Param::numBgZ
								+ j * Param::numLbZ + k];
			}
		}
	}
}

static bool cmpResult(std::pair<label_id, num> a, std::pair<label_id, num> b) {
	return (a.second > b.second);
}

void Alg::judgeTest(num trainIterNum) {
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

#pragma omp parallel for schedule(dynamic)
	for (doc_id i = 0; i < Param::numTestDoc; i++) {
		num* p = Data::testLabelDistAccum + Param::numLabel * (size_t) i;
		Data::doc_state& ds = Data::testDocStates[i];
		std::vector<std::pair<label_id, num> > results;
		num deltaSumCrLb = 0;
		num deltaCrLb = 0;
		num deltaSumWrLb = 0;
		num deltaWrLb = 0;
		for (label_id l = 0; l < Param::numLabel; l++) {
			results.push_back(std::pair<label_id, num>(l, *(p + l)));

			if (hasLabel(ds.labels, l)) {
				deltaSumCrLb += *(p + l);
				deltaCrLb++;
			} else {
				deltaSumWrLb += *(p + l);
				deltaWrLb++;
			}
		}

		std::sort(results.begin(), results.end(), cmpResult);

#pragma omp atomic
		all += 1;
#pragma omp atomic
		sumCrLb += deltaSumCrLb;
#pragma omp atomic
		lbCorrect += deltaCrLb;
#pragma omp atomic
		sumWrLb += deltaSumWrLb;
#pragma omp atomic
		lbWrong += deltaWrLb;

		label_id numCrLb = cntNumLabel(ds.labels);
		label_id cntCrlb = 0;
		for (label_id p = 0; p < numCrLb; p++) {
			if (hasLabel(ds.labels, results[p].first)) {
				cntCrlb += 1;
			}
		}

		if (cntCrlb == numCrLb) {
#pragma omp atomic
			all_correct += 1;
		}
#pragma omp atomic
		first_k_percent += (cntCrlb * 1.0) / numCrLb;

		label_id fstPos = Param::numLabel;
		label_id lstPos = Param::numLabel;
		for (label_id p = 0; p < Param::numLabel; p++) {
			if (hasLabel(ds.labels, results[p].first)) {
				lstPos = p;
				if (fstPos == Param::numLabel) {
					fstPos = p;
				}
			}
		}
#pragma omp atomic
		last_crLb_percent += (lstPos + 1 - numCrLb) * 1.0
				/ (Param::numLabel - numCrLb);
#pragma omp atomic
		first_crLb_1rank += 1.0 / (fstPos + 1.0);

		num cntCrPr = 0;
		num cntWrPr = 0;

		for (label_id p = 0; p < Param::numLabel; p++) {
			if (hasLabel(ds.labels, results[p].first)) {
				for (label_id q = p + 1; q < Param::numLabel; q++) {
					if (!hasLabel(ds.labels, results[q].first)) {
						cntCrPr += 1;
					}
				}
			} else {
				for (label_id q = p + 1; q < Param::numLabel; q++) {
					if (hasLabel(ds.labels, results[q].first)) {
						cntWrPr += 1;
					}
				}
			}
		}
#pragma omp atomic
		pair_percent += (cntCrPr) / (0.0 + cntCrPr + cntWrPr);
	}

	std::cout << "Test result @ Iter " << trainIterNum << ":\n"
			<< "all_correct: " << all_correct << " / " << all << "\n"
			<< "first_k_correct_percent: " << first_k_percent << " / " << all
			<< "\n" << "last_correct_pos: " << last_crLb_percent << " / " << all
			<< "\n" << "first_correct_1rank: " << first_crLb_1rank << " / "
			<< all << "\n" << "pair_percent: " << pair_percent << " / " << all
			<< "\n" << "C/W lb: " << lbCorrect << " / " << lbWrong << "\n"
			<< "C/W lb_count: " << sumCrLb << " / " << sumWrLb << "\n";

	if (!Param::testResultPath.empty()) {
		std::ofstream testResultHeader(Param::testResultPath.c_str(),
				std::ofstream::out | std::ofstream::app);
		testResultHeader << trainIterNum << ", " << all << ", " << all_correct
				<< ", " << first_k_percent << ", " << last_crLb_percent << ", "
				<< first_crLb_1rank << ", " << pair_percent << ", " << lbCorrect
				<< ", " << lbWrong << ", " << sumCrLb << ", " << sumWrLb
				<< std::endl;
		testResultHeader.flush();
		testResultHeader.close();
	}
}

void Alg::trainEmItr() {
	clearZUCnt();
	for (int i = 0; i < Param::trainGibbsIter; i++) {
		trainGibbsSamp();
		if (i + Param::trainGibbsAccum >= Param::trainGibbsIter) {
			accumZUCnt();
		}
	}
	updateTmx();
}

void Alg::testItr(num trainIterNum) {
	testStateInit();
	clearTestLabelCnt();
	for (int i = 0; i < Param::testGibbsIter; i++) {
		testGibbsSamp();
		if (i + Param::testGibbsAccum >= Param::testGibbsIter) {
			accumTestLabelCnt();
		}
	}
	judgeTest(trainIterNum);
}

void Alg::testWhileTrain() {
	trainStateInit();
	for (int i = 0; i < Param::trainEmIter; i++) {
		trainEmItr();
		if((i+1) % Param::testEmFreq == 0) {
			testItr(i+1);
		}
	}
}
