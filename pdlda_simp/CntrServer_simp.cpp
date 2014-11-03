/*
 * CntrServer_simp.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#include "CntrServer_simp.h"
#include "TaskAssigner.h"
#include "BookLoader.h"
#include "DocState.h"

#include <mpi/mpi.h>
#include <cstring>
#include <stdexcept>

using namespace pdlda;

CntrServer_simp* CntrServer_simp::inst;

CntrServer_simp::CntrServer_simp() {
	wordUDist = new num[((size_t) BookLoader::inst->numWord)
			* BookLoader::inst->numU];
	memset(wordUDist, 0,
			sizeof(num) * ((size_t) BookLoader::inst->numWord)
					* BookLoader::inst->numU);
	uDist = new num[BookLoader::inst->numU];
	memset(uDist, 0, sizeof(num) * BookLoader::inst->numU);

	CntrServer_simp::inst = this;
}

CntrServer_simp::~CntrServer_simp() {
	delete[] wordUDist;
	delete[] uDist;
}

void CntrServer_simp::sync() {
	memset(wordUDist, 0,
			sizeof(num) * ((size_t) BookLoader::inst->numWord)
					* BookLoader::inst->numU);
	memset(uDist, 0, sizeof(num) * BookLoader::inst->numU);

	for (size_t i = 0; i < BookLoader::inst->numDoc; i++) {
		if (DocState::inst->docStates[i] != NULL) {
			for (size_t j = 0; j < DocState::inst->docStates[i]->length; j++) {
				topic_id u = DocState::inst->docStates[i]->tokens[j].u;
				word_id w = DocState::inst->docStates[i]->tokens[j].w;
				getWordEntry(w)[u]++;
				uDist[u]++;
			}
		}
	}

	MPI_Allreduce(MPI_IN_PLACE, wordUDist, ((size_t) BookLoader::inst->numWord)
			* BookLoader::inst->numU, NUM_MPI_TYPE, MPI_SUM, MPI_COMM_WORLD);
}

void CntrServer_simp::update(updateInfo d) {
	if (d.decU < BookLoader::inst->numU) {
		getWordEntry(d.w)[d.decU]--;
		uDist[d.decU]--;
	}
	if (d.incU < BookLoader::inst->numU) {
		getWordEntry(d.w)[d.incU]++;
		uDist[d.incU]++;
	}
}

num* CntrServer_simp::fetch(word_id w) {
	return getWordEntry(w);
}
