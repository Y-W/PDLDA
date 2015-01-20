/*
 * pdlda_omp.cpp
 *
 *  Created on: Jan 16, 2015
 *      Author: yw
 */

#include <iostream>
#include "common.h"
#include "param.h"
#include "data.h"
#include "alg.h"

using namespace pdlda;

void init(int argc, char **argv) {
	Param::parseArgs(argc, argv);
	Data::init();
}

void destroy() {
	Data::destroy();
}

int main(int argc, char **argv) {
	init(argc, argv);
	Alg::trainStateInit();
//	Alg::testWhileTrain();
	destroy();
}


