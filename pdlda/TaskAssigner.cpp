/*
 * TaskAssigner.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#include "TaskAssigner.h"
#include "ArgRegister.h"

using namespace pdlda;

static TCLAP::ValueArg<unsigned int> numSampArg("s", "num_samp",
		"Number of sampling processes to run", true, 0, "integer");
REGISTER_ARG(numSampArg)

TaskAssigner* TaskAssigner::inst;

TaskAssigner::TaskAssigner() {
	int tmp;
	MPI_Comm_size(MPI_COMM_WORLD, &tmp);
	this->numTask = tmp;

	MPI_Comm_rank(MPI_COMM_WORLD, &tmp);
	this->rank = tmp;

	this->numSamp = numSampArg.getValue();
	this->numCntr = numTask - numSamp;

	task_id tmpSamp = numSamp;
	task_id tmpCntr = numCntr;
	task_id p = 0;
	while (tmpSamp > 0 && tmpCntr > 0) {
		sampIds.insert(p);
		lstSampIds.push_back(p);
		p++;
		tmpSamp--;
		cntrIds.insert(p);
		lstCntrIds.push_back(p);
		p++;
		tmpCntr--;
	}
	while (tmpSamp > 0) {
		sampIds.insert(p);
		lstSampIds.push_back(p);
		p++;
		tmpSamp--;
	}
	while (tmpCntr > 0) {
		cntrIds.insert(p);
		lstCntrIds.push_back(p);
		p++;
		tmpCntr--;
	}

	MPI_Group worldGroup, sampGroup, cntrGroup;
	MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
	int* rankTmp = new int[numSamp];
	for (int i = 0; i < numSamp; i++) {
		rankTmp[i] = lstSampIds[i];
	}

	MPI_Group_incl(worldGroup, numSamp, rankTmp, &sampGroup);
	MPI_Comm_create(MPI_COMM_WORLD, sampGroup, &sampComm);
	MPI_Group_excl(worldGroup, numSamp, rankTmp, &cntrGroup);
	MPI_Comm_create(MPI_COMM_WORLD, cntrGroup, &cntrComm);
	delete[] rankTmp;
	MPI_Group_free(&sampGroup);
	MPI_Group_free(&cntrGroup);

	isSamp = (sampIds.count(rank) > 0);

	TaskAssigner::inst = this;
}

task_id TaskAssigner::getSampId(doc_id docId) {
	task_id tmp = docId % numSamp;
	return lstSampIds[tmp];
}

task_id TaskAssigner::getCntrId(word_id wordId) {
	task_id tmp = wordId % numCntr;
	return lstCntrIds[tmp];
}
