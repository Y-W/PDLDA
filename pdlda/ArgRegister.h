/*
 * ArgRegister.hpp
 *
 *  Created on: Jul 24, 2014
 *      Author: yw
 */

#ifndef ARGREGISTER_HPP_
#define ARGREGISTER_HPP_

#include "tclap/CmdLine.h"

namespace pdlda {

class ArgRegister {
public:
	static bool addArg(TCLAP::Arg& arg);
	void parseArg(int argc, const char * const *argv);
	~ArgRegister();
private:
	static TCLAP::CmdLine* cmdLine;
};
}

#define REGISTER_ARG(A) bool __reg_arg_ ## A = pdlda::ArgRegister::addArg(A);

#endif /* ARGREGISTER_HPP_ */
