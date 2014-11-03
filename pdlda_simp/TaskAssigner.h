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

namespace pdlda {
class TaskAssigner {
public:
	static TaskAssigner* inst;
	TaskAssigner();
	task_id rank;
	task_id numTask;
	task_id getSampId(doc_id docId);
};
}

#endif /* TASKASSIGNER_H_ */
