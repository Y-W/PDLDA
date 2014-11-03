/*
 * RandGen.cpp
 *
 *  Created on: Jul 24, 2014
 *      Author: yw
 */

#include "RandGen.h"
#include <stdexcept>
#include <cmath>
#include "ArgRegister.h"

static TCLAP::ValueArg<unsigned long int> seed("r", "rand_seed",
		"Seed for random generator", false, 0, "integer");
REGISTER_ARG(seed)

using namespace pdlda;

RandGen* RandGen::inst;
MTRand_int32* RandGen::mtRand;

RandGen::RandGen() {
	if (RandGen::mtRand == NULL) {
		if (seed.isSet()) {
			RandGen::mtRand = new MTRand(seed.getValue());
		} else {
			RandGen::mtRand = new MTRand();
		}
		RandGen::inst = this;
	} else {
		throw std::logic_error("RandGen initialized twice!");
	}
}

RandGen::~RandGen() {
	if (RandGen::mtRand != NULL) {
		delete RandGen::mtRand;
	}
}

num RandGen::getInt(num maxExclusive) {
	if (maxExclusive <= 1) {
		return 0;
	}
	num maxQ = MTRAND_INT32_MAX / maxExclusive;
	if (MTRAND_INT32_MAX % maxExclusive + 1 == maxExclusive) {
		maxQ++;
	}
	num rand = (*RandGen::mtRand)();
	while (rand / maxExclusive >= maxQ) {
		rand = (*RandGen::mtRand)();
	}
	return rand % maxExclusive;
}

double RandGen::getDbl() {
	return (*RandGen::mtRand)() / ((double) 1.0 + MTRAND_INT32_MAX);
}
