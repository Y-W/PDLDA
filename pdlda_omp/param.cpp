/*
 * Param.cpp
 *
 *  Created on: Jan 16, 2015
 *      Author: yw
 */

#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "param.h"

using namespace pdlda;

double Param::alpha, Param::beta;
num Param::trainEmIter, Param::trainGibbsIter, Param::testGibbsIter,
		Param::testEmFreq;
num Param::trainGibbsAccum, Param::testGibbsAccum;
std::string Param::trainDataPath, Param::testDataPath, Param::testResultPath;
doc_id Param::numTrainDoc, Param::numTestDoc;
word_id Param::numWord;
topic_id Param::numZ, Param::numU, Param::numBgZ, Param::numLbZ;
label_id Param::numLabel;
num Param::rndSeed;
num Param::tmxRegIter;
double Param::tmxRegCoef;

#define PARAM_REQUESTED(X) if(p==0){cntRequired++;}else{\
	if(*(argv[p])=='-' && strcmp(#X,argv[p]+1)==0){\
		cntFilled++; std::stringstream ss; ss.str(argv[++p]); ss >> X;\
		std::cout<<"Parameter "<<#X<<" = "<<X<<"\n";}}

#define PARAM_DEFAULT(X, DEF) if(p==0){X=DEF;}else{\
	if(*(argv[p])=='-' && strcmp(#X,argv[p]+1)==0){\
		std::stringstream ss; ss.str(argv[++p]); ss >> X;\
		std::cout<<"Parameter "<<#X<<" = "<<X<<"\n";}}

void Param::parseArgs(int argc, char **argv) {
	int cntFilled = 0, cntRequired = 0;
	for (int p = 0; p < argc; p++) {
		PARAM_REQUESTED(alpha);
		PARAM_REQUESTED(beta);
		PARAM_REQUESTED(trainEmIter);
		PARAM_REQUESTED(trainGibbsIter);
		PARAM_REQUESTED(testGibbsIter);
		PARAM_REQUESTED(testEmFreq);
		PARAM_REQUESTED(trainGibbsAccum);
		PARAM_REQUESTED(testGibbsAccum);
		PARAM_REQUESTED(trainDataPath);
		PARAM_REQUESTED(testDataPath);
		PARAM_REQUESTED(numU);
		PARAM_REQUESTED(numBgZ);
		PARAM_REQUESTED(numLbZ);
		PARAM_DEFAULT(rndSeed, 43);
		PARAM_DEFAULT(tmxRegIter, 0);
		PARAM_DEFAULT(tmxRegCoef, 0.0);
		PARAM_DEFAULT(testResultPath, "");
	}
	if (cntFilled != cntRequired) {
		throw std::runtime_error(
				"Required number of arguments does not match what is provided.");
	}
	if (!testResultPath.empty()) {
		std::ofstream testResultHeader(testResultPath.c_str(),
				std::ofstream::out | std::ofstream::app);
		testResultHeader
				<< "iter_num, all, all_correct, first_k_correct_percent, last_correct_pos,"
						" first_correct_1rank, pair_percent, correct_lb, wrong_lb,"
						" correct_lb_hit, wrong_lb_hit"
				<< std::endl;
		testResultHeader.flush();
		testResultHeader.close();
	}
}
