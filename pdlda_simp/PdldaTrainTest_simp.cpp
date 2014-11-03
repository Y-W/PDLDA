/*
 * PdldaTrain.cpp
 *
 *  Created on: Sep 9, 2014
 *      Author: yw
 */

#include <mpi/mpi.h>
#include <iostream>
#include <sstream>

#include "common.h"
#include "RandGen.h"
#include "ArgRegister.h"
#include "BookLoader.h"
#include "DocState.h"
#include "TaskAssigner.h"
#include "TVectorPool.h"
#include "CntrServer_simp.h"
#include "Sampler.h"

using namespace pdlda;

static TCLAP::ValueArg<num> em_iterArg("e", "em_iter",
		"Number of EM iterations", true, 0, "integer");
REGISTER_ARG(em_iterArg)

static TCLAP::ValueArg<num> gibbs_iterArg("g", "gibbs_iter",
		"Number of Gibbs iterations", true, 0, "integer");
REGISTER_ARG(gibbs_iterArg)

static TCLAP::ValueArg<num> gibbs_iterArg_test("G", "gibbs_iter_test",
		"Number of Gibbs iterations for testing", true, 0, "integer");
REGISTER_ARG(gibbs_iterArg_test)

static TCLAP::ValueArg<num> em_iterArg_test("E", "em_iter_test",
		"Frequency of testing by number of EM iterations", true, 0, "integer");
REGISTER_ARG(em_iterArg_test)

int main(int argc, char **argv) {
	MPI_Init(&argc, &argv);
	ArgRegister argR;
	argR.parseArg(argc, argv);
	RandGen randG;
	TaskAssigner taskAssigner;
	BookLoader bookL;
	TVectorPool tVectorPool;
	DocState docState;
	CntrServer_simp CntrServer_simp;
	Sampler sampler;

	std::stringstream ss;

	ss << getCurrentTimeString() << " @ " << (int) TaskAssigner::inst->rank
			<< " - All init finished.\n";
	std::cout << ss.str();
	ss.str("");

	num em_iter = em_iterArg.getValue();
	num gibbs_iter = gibbs_iterArg.getValue();
	num gibbs_iter_test = gibbs_iterArg_test.getValue();
	num em_iter_test = em_iterArg_test.getValue();

	MPI_Barrier(MPI_COMM_WORLD);

	Sampler::inst->initDocStates();
	CntrServer_simp::inst->sync();

	ss << getCurrentTimeString() << " @ " << (int) TaskAssigner::inst->rank
			<< " - Finished Init Doc. \n";
	std::cout << ss.str();
	ss.str("");

	for (size_t em = 0; em < em_iter; em++) {
//		ss << getCurrentTimeString() << " @ " << TaskAssigner::inst->rank
//				<< " - Started EM Iter " << em << ".\n";
//		std::cout << ss.str();
//		ss.str("");
		TVectorPool::inst->clearDocStateCache();
		for (size_t gibbs = 0; gibbs < gibbs_iter; gibbs++) {
//			ss << getCurrentTimeString() << " @ " << TaskAssigner::inst->rank
//					<< " - Started Gibbs Iter " << gibbs << ".\n";
//			std::cout << ss.str();
//			ss.str("");


				Sampler::inst->sampleAllDocOnce();
				CntrServer_simp::inst->sync();

				if (gibbs + 1 == gibbs_iter) {
					TVectorPool::inst->cacheCurrentDocState4Update();
				}

			ss << getCurrentTimeString() << " @ "
					<< (int) TaskAssigner::inst->rank
					<< " - Finished Gibbs Iter " << gibbs << ".\n";
			std::cout << ss.str();
			ss.str("");

		}

			TVectorPool::inst->updateTVec();

//			if (TaskAssigner::inst->rank == 0) {
//				ss << getCurrentTimeString() << " @ "
//						<< (int) TaskAssigner::inst->rank
//						<< " - Output T matrix:\n";
//
//				for (topic_id z = 0; z < TVectorPool::inst->numZ; z++) {
//					for (topic_id u = 0; u < BookLoader::inst->numU; u++) {
//						ss
//								<< TVectorPool::inst->tVecs[z
//										* BookLoader::inst->numU + u] << ' ';
//					}
//					ss << "\n";
//				}
//
//				std::cout << ss.str();
//				ss.str("");
//			}

		ss << getCurrentTimeString() << " @ " << (int) TaskAssigner::inst->rank
				<< " - Finished EM Iter " << em << ".\n";
		std::cout << ss.str();
		ss.str("");

		if ((em + 1) % em_iter_test == 0) {
			ss << getCurrentTimeString() << " @ "
					<< (int) TaskAssigner::inst->rank << " - Start Testing.\n";
			std::cout << ss.str();
			ss.str("");


				Sampler::inst->initDocStates_test();
				for (num gbs = 0; gbs < gibbs_iter_test; gbs++) {
					Sampler::inst->sampleAllDocOnce_test();
				}
				Sampler::inst->judgeTest();


//			ss << getCurrentTimeString() << " @ " << TaskAssigner::inst->rank
//					<< " - Finished Testing.\n";
//			std::cout << ss.str();
//			ss.str("");
		}

//		MPI_Barrier(MPI_COMM_WORLD);
	}

	MPI_Finalize();
	return 0;
}
