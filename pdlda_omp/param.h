/*
 * Param.h
 *
 *  Created on: Jan 16, 2015
 *      Author: yw
 */

#ifndef PARAM_H_
#define PARAM_H_

#include <string>
#include "common.h"

namespace pdlda {

class Param {
public:
	static double alpha, beta;
	static num trainEmIter, trainGibbsIter, testGibbsIter;
	static num trainGibbsAccum, testGibbsAccum;
	static std::string trainDataPath, testDataPath, testResultPath, unlabelledPath, modelTmxPath, modelTopicPath;
	static doc_id numTrainDoc, numTestDoc, numUnlabelledDoc;
	static word_id numWord;
	static topic_id numZ, numU, numBgZ, numLbZ;
	static label_id numLabel;
	static num rndSeed;
	static num tmxRegIter;
	static double tmxRegCoef;
	static double unlabelledUpdateTmx;

	static void parseArgs(int argc, char **argv);
};

} /* namespace pdlda */

#endif /* PARAM_H_ */
