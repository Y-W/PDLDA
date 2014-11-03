/*
 * TaskAssigner.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#include "TaskAssigner.h"
#include "ArgRegister.h"

using namespace pdlda;

TaskAssigner* TaskAssigner::inst;

TaskAssigner::TaskAssigner() {
	int tmp;
	MPI_Comm_size(MPI_COMM_WORLD, &tmp);
	this->numTask = tmp;

	MPI_Comm_rank(MPI_COMM_WORLD, &tmp);
	this->rank = tmp;
	TaskAssigner::inst = this;
}

task_id TaskAssigner::getSampId(doc_id docId) {
	return docId % numTask;
}
