/*
 * TVectorPool.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#include "TVectorPool.h"
#include "ArgRegister.h"
#include "TaskAssigner.h"
#include "DocState.h"
#include "BookLoader.h"

#include <cstring>
#include <set>

using namespace pdlda;

static TCLAP::ValueArg<unsigned int> numBgZArg("Z", "bg_z",
		"Number of background Z", true, 0, "integer");
REGISTER_ARG(numBgZArg)

static TCLAP::ValueArg<unsigned int> numZPerLbArg("z", "lb_z",
		"Number of Z per label", true, 0, "integer");
REGISTER_ARG(numZPerLbArg)

static TCLAP::ValueArg<double> klArg("d", "kl_div_coeff",
		"Coefficient for KL divergence regulator, asymmetric or symmetric",
		true, 0.0, "real number");
REGISTER_ARG(klArg)

static TCLAP::ValueArg<unsigned int> numKlsArg("D", "kl_div_sym_iter",
		"Number of KL-symmetric iterations", true, 0, "integer");
REGISTER_ARG(numKlsArg)

TVectorPool* TVectorPool::inst;
TVectorPool::TVectorPool() {
	numBgZ = numBgZArg.getValue();
	numZPerLb = numZPerLbArg.getValue();
	numZ = numBgZ + BookLoader::inst->numLabel * numZPerLb;
	klRegulatorCoeff = klArg.getValue();
	klsIter = numKlsArg.getValue();

	if (TaskAssigner::inst->isSamp) {
		distZUCache = new num[(size_t) numZ * BookLoader::inst->numU];
		memset(distZUCache, 0, sizeof(num) * numZ * BookLoader::inst->numU);

		tVecs = new double[(size_t) numZ * BookLoader::inst->numU];
		double tmp = 1.0 / BookLoader::inst->numU;
		for (size_t i = 0; i < (size_t) numZ * BookLoader::inst->numU; i++) {
			tVecs[i] = tmp;
		}
	} else {
		tVecs = NULL;
		distZUCache = NULL;
	}
	TVectorPool::inst = this;
}

TVectorPool::~TVectorPool() {
	if (distZUCache != NULL) {
		delete[] distZUCache;
	}
	if (tVecs != NULL) {
		delete[] tVecs;
	}
}

void TVectorPool::clearDocStateCache() {
	if (!TaskAssigner::inst->isSamp) {
		return;
	}
	memset(distZUCache, 0, sizeof(num) * numZ * BookLoader::inst->numU);
}

void TVectorPool::getZ4Doc(doc_id docId, vector<topic_id>& newList) {
	newList.clear();
	if (!TaskAssigner::inst->isSamp) {
		return;
	}
	for (topic_id i = 0; i < numBgZ; i++) {
		newList.push_back(i);
	}
	for (std::set<label_id>::iterator it =
			BookLoader::inst->documents[docId].labels.begin();
			it != BookLoader::inst->documents[docId].labels.end(); it++) {
		for (topic_id i = 0; i < numZPerLb; i++) {
			newList.push_back((topic_id) (numBgZ + (*it) * numZPerLb + i));
		}
	}
}

void TVectorPool::getZ4Lb_test(label_id label, vector<topic_id>& newList) {
	newList.clear();
	if (!TaskAssigner::inst->isSamp) {
		return;
	}
	for (topic_id i = 0; i < numBgZ; i++) {
		newList.push_back(i);
	}

	for (topic_id i = 0; i < numZPerLb; i++) {
		newList.push_back((topic_id) (numBgZ + (label) * numZPerLb + i));
	}

}

void TVectorPool::getZ4Doc_all(vector<topic_id>& newList) {
	newList.clear();
	if (!TaskAssigner::inst->isSamp) {
		return;
	}
	for (topic_id i = 0; i < numZ; i++) {
		newList.push_back(i);
	}
}

void TVectorPool::cacheCurrentDocState4Update() {
	if (!TaskAssigner::inst->isSamp) {
		return;
	}
	for (size_t i = 0; i < BookLoader::inst->numDoc; i++) {
		if (DocState::inst->docStates[i] != NULL) {
			for (size_t j = 0; j < DocState::inst->docStates[i]->length; j++) {
				size_t z = DocState::inst->docStates[i]->tokens[j].z;
				size_t u = DocState::inst->docStates[i]->tokens[j].u;
				distZUCache[z * BookLoader::inst->numU + u]++;
			}
		}
	}
}

static void normalizeVector(double* arr, size_t size) {
	double sum = 0;
	for (size_t i = 0; i < size; i++) {
		if (arr[i] <= 0) {
			arr[i] = 0;
		} else {
			sum += arr[i];
		}
	}
	sum = 1.0 / sum;
	for (size_t i = 0; i < size; i++) {
		arr[i] *= sum;
	}
}

static void vectorMinus(double* source, double* dest, double coeff,
		size_t size) {
	for (size_t i = 0; i < size; i++) {
		dest[i] -= source[i] * coeff;
	}
}

void TVectorPool::updateTVec() {
	if (!TaskAssigner::inst->isSamp) {
		return;
	}
	num* tmpSum = new num[(size_t) numZ * BookLoader::inst->numU];
	MPI_Allreduce(distZUCache, tmpSum, (size_t) numZ * BookLoader::inst->numU,
	NUM_MPI_TYPE,
	MPI_SUM, TaskAssigner::inst->sampComm);
	for (size_t i = 0; i < (size_t) numZ * BookLoader::inst->numU; i++) {
		tVecs[i] = tmpSum[i];
	}

	if (klsIter == 0) {

		for (topic_id i = 0; i < numBgZ; i++) {
			for (topic_id j = 0; j < i; j++) {
				vectorMinus(getTVec(j), getTVec(i), klRegulatorCoeff,
						BookLoader::inst->numU);
			}
			normalizeVector(getTVec(i), BookLoader::inst->numU);
		}

		for (topic_id i = 0; i < BookLoader::inst->numLabel; i++) {
			for (topic_id j = 0; j < numZPerLb; j++) {
				topic_id destZ = numBgZ + i * numZPerLb + j;
				for (topic_id k = 0; k < numBgZ; k++) {
					vectorMinus(getTVec(k), getTVec(destZ), klRegulatorCoeff,
							BookLoader::inst->numU);
				}
				for (topic_id k = 0; k < j; k++) {
					vectorMinus(getTVec(numBgZ + i * numZPerLb + k),
							getTVec(destZ), klRegulatorCoeff,
							BookLoader::inst->numU);
				}
				normalizeVector(getTVec(destZ), BookLoader::inst->numU);
			}
		}
	} else {
		double* colSum = new double[BookLoader::inst->numU];
		memset(colSum, 0, sizeof(double) * BookLoader::inst->numU);

		num* countSum = new num[numZ];
		memset(countSum, 0, sizeof(num) * numZ);

		for (topic_id i = 0; i < numZ; i++) {
			normalizeVector(getTVec(i), BookLoader::inst->numU);
			for (size_t j = 0; j < BookLoader::inst->numU; j++) {
				colSum[j] += getTVec(i)[j];
				countSum[i] += tmpSum[(size_t) i
						* ((size_t) BookLoader::inst->numU) + j];
			}
		}

		for(num itr=0; itr<klsIter; itr++) {
			for(topic_id r=0; r<numZ; r++) {
				for(size_t c=0; c<BookLoader::inst->numU; c++) {
					colSum[c] -= getTVec(r)[c];
					getTVec(r)[c] = (tmpSum[r * (size_t) BookLoader::inst->numU + c]
					                        - klRegulatorCoeff * (colSum[c])) /
					                        (countSum[r] - klRegulatorCoeff * (numZ - 1));
					if(getTVec(r)[c] < 0) {
						getTVec(r)[c] = 0;
					}
					colSum[c] += getTVec(r)[c];
				}
			}
		}

		delete[] countSum;
		delete[] colSum;
	}

	delete[] tmpSum;
}
