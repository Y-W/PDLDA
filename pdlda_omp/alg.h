/*
 * alg.h
 *
 *  Created on: Jan 16, 2015
 *      Author: yw
 */

#ifndef ALG_H_
#define ALG_H_

#include "common.h"

namespace pdlda {

class Alg {
public:
	static void trainStateInit();
	static void testStateInit();
	static void unlabelledStateInit();
	static void trainGibbsSamp();
	static void testGibbsSamp();
	static void unlabelledGibbsSamp();
	static void clearTrainAccum();
	static void accumTrain();
	static void updateTmx();
	static void clearTestLabelCnt();
	static void accumTestLabelCnt();
	static void judgeTest(num trainIterNum);

	static void trainEmItr();
	static void testItr(num trainIterNum);
	static void testWhileTrain();
};

}

#endif /* ALG_H_ */
