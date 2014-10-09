/*
 * CntrServer.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#include "CntrServer.h"
#include "TaskAssigner.h"
#include "RequestPool.h"
#include "BookLoader.h"

#include <mpi/mpi.h>
#include <cstring>
#include <stdexcept>

//#include <iostream>
//#include <sstream>

using namespace pdlda;

CntrServer* CntrServer::inst;

CntrServer::CntrServer() {
	wordUDist = new num*[BookLoader::inst->numWord];
	for (word_id i = 0; i < BookLoader::inst->numWord; i++) {
		if (TaskAssigner::inst->getCntrId(i) == TaskAssigner::inst->rank) {
			wordUDist[i] = new num[BookLoader::inst->numU];
			memset(wordUDist[i], 0, sizeof(num) * BookLoader::inst->numU);
		} else {
			wordUDist[i] = NULL;
		}
	}

	opNum_l = estimateOpNum_l();
	opNum_s = estimateOpNum_s();
	opNum_l_test = estimateOpNum_l_test();

	CntrServer::inst = this;
}

CntrServer::~CntrServer() {
	for (word_id i = 0; i < BookLoader::inst->numWord; i++) {
		if (wordUDist[i] != NULL) {
			delete[] wordUDist[i];
		}
	}
	delete[] wordUDist;
}

static void callbackFunc(bool& i, void* buf, MPI_Status* status) {
	delete[] (num*) buf;
}

void CntrServer::listen(num numFetch, num numUpdate) {
	RequestPool<bool> sendReqs(TaskAssigner::inst->numSamp, &callbackFunc);

	// fetch
	word_id fetchBuf;
	MPI_Request fetchRequest;
	if (numFetch > 0) {
		MPI_Recv_init(&fetchBuf, 1, WORD_ID_MPI_TYPE, MPI_ANY_SOURCE, TAG_FETCH,
		MPI_COMM_WORLD, &fetchRequest);
		MPI_Start(&fetchRequest);
	}

	// update
	updateInfo updateBuf;
	MPI_Request updateRequest;
	if (numUpdate > 0) {
		MPI_Recv_init(&updateBuf, sizeof(updateInfo), MPI_BYTE, MPI_ANY_SOURCE,
		TAG_UPDATE, MPI_COMM_WORLD, &updateRequest);
		MPI_Start(&updateRequest);
	}

	while (numFetch > 0 || numUpdate > 0) {
		sendReqs.checkReq();

		int flag;
		if (numUpdate > 0) {
			MPI_Test(&updateRequest, &flag, MPI_STATUS_IGNORE);
			if (flag) {
				if (wordUDist[updateBuf.w] == NULL) {
					throw std::runtime_error(
							"Update Request sent to wrong server!");
				}
				if (updateBuf.decU < BookLoader::inst->numU) {
					wordUDist[updateBuf.w][updateBuf.decU]--;
				}
				if (updateBuf.incU < BookLoader::inst->numU) {
					wordUDist[updateBuf.w][updateBuf.incU]++;
				}
				numUpdate--;
				if (numUpdate > 0) {
					MPI_Start(&updateRequest);
				} else {
					MPI_Request_free(&updateRequest);
				}
				continue;
			}
		}
		if (numFetch > 0) {

			MPI_Status status;
			MPI_Test(&fetchRequest, &flag, &status);
			if (flag) {
//				std::stringstream tmpss;
//				tmpss << "Task " << TaskAssigner::inst->rank
//						<< ": received fetch request from " << status.MPI_SOURCE
//						<< "\n";
//				std::cout << tmpss.str();

				if (fetchBuf < BookLoader::inst->numWord) {
					if (wordUDist[fetchBuf] == NULL) {
						throw std::runtime_error(
								"Fetch Request sent to wrong server!");
					}
					num* tmp = new num[BookLoader::inst->numU + 1];
					tmp[0] = fetchBuf;
					memcpy(tmp + 1, wordUDist[fetchBuf],
							sizeof(num) * BookLoader::inst->numU);

					MPI_Request* req = new MPI_Request;
					MPI_Issend(tmp, BookLoader::inst->numU + 1, NUM_MPI_TYPE,
							status.MPI_SOURCE, TAG_FETCH, MPI_COMM_WORLD, req);
					bool tmpVal = false;
					sendReqs.addRequest(req, tmp, tmpVal);
				} else {
					num* tmp = new num[BookLoader::inst->numU + 1];
					tmp[0] = fetchBuf;
					memset(tmp + 1, 0, sizeof(num) * BookLoader::inst->numU);
					for (word_id i = 0; i < BookLoader::inst->numWord; i++) {
						if (wordUDist[i] != NULL) {
							for (topic_id j = 0; j < BookLoader::inst->numU;
									j++) {
								tmp[j + 1] += wordUDist[i][j];
							}
						}
					}

					MPI_Request* req = new MPI_Request;
					MPI_Issend(tmp, BookLoader::inst->numU + 1, NUM_MPI_TYPE,
							status.MPI_SOURCE, TAG_FETCH, MPI_COMM_WORLD, req);

//					std::stringstream tmpss;
//					tmpss << "Task " << TaskAssigner::inst->rank
//							<< ": replied count sum to " << status.MPI_SOURCE
//							<< "\n";
//					std::cout << tmpss.str();

					bool tmpVal = false;
					sendReqs.addRequest(req, tmp, tmpVal);
				}

				numFetch--;
				if (numFetch > 0) {
					MPI_Start(&fetchRequest);
				} else {
					MPI_Request_free(&fetchRequest);
				}
				continue;
			}
		}

	}




	sendReqs.waitTillClear();
}

num CntrServer::estimateOpNum_l() {
	num sum = 0;
	for (word_id i = 0; i < BookLoader::inst->numWord; i++) {
		if (TaskAssigner::inst->getCntrId(i) == TaskAssigner::inst->rank) {
			sum += BookLoader::inst->wordDist[i];
		}
	}
	return sum;
}

num CntrServer::estimateOpNum_l_test() {
	num sum = 0;
	for (word_id i = 0; i < BookLoader::inst->numWord; i++) {
		if (TaskAssigner::inst->getCntrId(i) == TaskAssigner::inst->rank) {
			sum += BookLoader::inst->wordDist_test[i];
		}
	}
	return sum;
}

num CntrServer::estimateOpNum_s() {
	num sum = 0;
	for (word_id i = 0; i < BookLoader::inst->numWord; i++) {
		if (TaskAssigner::inst->getCntrId(i) == TaskAssigner::inst->rank) {
			sum += 1;
		}
	}
	return sum;
}
