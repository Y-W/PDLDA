/*
 * ArgRegister.cpp
 *
 *  Created on: Jul 24, 2014
 *      Author: yw
 */

#include "ArgRegister.h"

using namespace pdlda;

TCLAP::CmdLine* ArgRegister::cmdLine;

bool ArgRegister::addArg(TCLAP::Arg& arg) {
	if(ArgRegister::cmdLine == NULL) {
		ArgRegister::cmdLine = new TCLAP::CmdLine("PDLDA by Yijie Wang (wyijie93@gmail.com)");
	}
	ArgRegister::cmdLine->add(arg);
	return true;
}

void ArgRegister::parseArg(int argc, const char * const *argv) {
	ArgRegister::cmdLine->parse(argc, argv);
}

ArgRegister::~ArgRegister() {
	if(ArgRegister::cmdLine != NULL) {
		delete ArgRegister::cmdLine;
	}
}
