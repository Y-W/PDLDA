/*
 * RandGen.hpp
 *
 *  Created on: Jul 24, 2014
 *      Author: yw
 */

#ifndef RANDGEN_HPP_
#define RANDGEN_HPP_

#include "common.h"
#include "mtrand.h"

namespace pdlda {

class RandGen {
public:
	RandGen();
	~RandGen();
	num getInt(num maxExclusive);
	double getDbl();
	static RandGen* inst;
private:
	static MTRand_int32* mtRand;
};

} /* namespace pdlda */

#endif /* RANDGEN_HPP_ */
