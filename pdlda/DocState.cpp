/*
 * DocState.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#include "DocState.h"
#include "BookLoader.h"
#include "TaskAssigner.h"
#include "TVectorPool.h"

#include <stdexcept>

using namespace pdlda;

DocState* DocState::inst;

DocState::DocState() {
	docStates = new doc*[BookLoader::inst->numDoc];
	num sumLen = 0;
	for (doc_id i = 0; i < BookLoader::inst->numDoc; i++) {
		if (TaskAssigner::inst->getSampId(i) == TaskAssigner::inst->rank) {
			docStates[i] = new doc;
			docStates[i]->length = BookLoader::inst->documents[i].words.size();
			sumLen += docStates[i]->length;
			docStates[i]->distDocZ = new num[TVectorPool::inst->numZ];
//			docStates[i]->tokens = new zuw[docStates[i]->length];
//			for(size_t j=0; j<docStates[i]->length; j++) {
//				docStates[i]->tokens[j].w = BookLoader::inst->documents[i].words[j];
//			}
		} else {
			docStates[i] = NULL;
		}

//		BookLoader::inst->documents[i].words.clear();
	}

	zuw* space = new zuw[sumLen];
	for (doc_id i = 0; i < BookLoader::inst->numDoc; i++) {
		if (TaskAssigner::inst->getSampId(i) == TaskAssigner::inst->rank) {
			docStates[i]->tokens = space;
			space += docStates[i]->length;
			for (size_t j = 0; j < docStates[i]->length; j++) {
				docStates[i]->tokens[j].w =
						BookLoader::inst->documents[i].words[j];
			}
		}

		BookLoader::inst->documents[i].words.clear();
	}

	docStates_test = new doc*[BookLoader::inst->numDoc_test];
	sumLen = 0;
	for (doc_id i = 0; i < BookLoader::inst->numDoc_test; i++) {
		if (TaskAssigner::inst->getSampId(i) == TaskAssigner::inst->rank) {
			docStates_test[i] = new doc;
			docStates_test[i]->length =
					BookLoader::inst->documents_test[i].words.size();
			sumLen += docStates_test[i]->length;
//				docStates_test[i]->tokens = new zuw[docStates_test[i]->length];
			docStates_test[i]->distDocZ = new num[TVectorPool::inst->numZ];
//				for(size_t j=0; j<docStates_test[i]->length; j++) {
//					docStates_test[i]->tokens[j].w = BookLoader::inst->documents_test[i].words[j];
//				}
		} else {
			docStates_test[i] = NULL;
		}

//			BookLoader::inst->documents_test[i].words.clear();
	}
	space = new zuw[sumLen];
	for (doc_id i = 0; i < BookLoader::inst->numDoc_test; i++) {
		if (TaskAssigner::inst->getSampId(i) == TaskAssigner::inst->rank) {
			docStates_test[i]->tokens = space;
			space += docStates_test[i]->length;

			for (size_t j = 0; j < docStates_test[i]->length; j++) {
				docStates_test[i]->tokens[j].w =
						BookLoader::inst->documents_test[i].words[j];
			}
		}

		BookLoader::inst->documents_test[i].words.clear();
	}
	DocState::inst = this;
}

DocState::~DocState() {
	bool fstTokens = true;
	for (doc_id i = 0; i < BookLoader::inst->numDoc; i++) {
		if (docStates[i] != NULL) {
			if (fstTokens) {
				delete[] docStates[i]->tokens;
				fstTokens = false;
			}
			delete[] docStates[i]->distDocZ;
			delete docStates[i];
		}
	}
	delete[] docStates;

	fstTokens = true;
		for (doc_id i = 0; i < BookLoader::inst->numDoc_test; i++) {
			if (docStates_test[i] != NULL) {
				if (fstTokens) {
					delete[] docStates_test[i]->tokens;
					fstTokens = false;
				}
				delete[] docStates_test[i]->distDocZ;
				delete docStates_test[i];
			}
		}
		delete[] docStates_test;
}

DocState::doc& DocState::getDocState(doc_id docId) {
	if (docStates[docId] == NULL) {
		throw std::runtime_error("Doc not stored on this process!");
	}
	return *(docStates[docId]);
}

DocState::doc& DocState::getDocState_test(doc_id docId) {
	if (docStates_test[docId] == NULL) {
		throw std::runtime_error("Doc not stored on this process!");
	}
	return *(docStates_test[docId]);
}
