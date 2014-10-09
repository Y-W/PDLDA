/*
 * TaskAssigner.h
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#ifndef TASKASSIGNER_H_
#define TASKASSIGNER_H_

#include <set>
#include <vector>
#include <mpi/mpi.h>

#include "common.h"

using std::set;
using std::vector;

namespace pdlda {
class TaskAssigner {
public:
	static TaskAssigner* inst;
	TaskAssigner();
	task_id rank;
	task_id numTask;

	task_id numSamp;
	task_id numCntr;

	set<task_id> sampIds;
	vector<task_id> lstSampIds;
	MPI_Comm sampComm;

	set<task_id> cntrIds;
	vector<task_id> lstCntrIds;
	MPI_Comm cntrComm;

	bool isSamp;

	task_id getSampId(doc_id docId);
	task_id getCntrId(word_id wordId);
};
}

#endif /* TASKASSIGNER_H_ */
