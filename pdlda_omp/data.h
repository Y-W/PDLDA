/*
 * data.h
 *
 *  Created on: Jan 16, 2015
 *      Author: yw
 */

#ifndef DATA_H_
#define DATA_H_

#include <string>
#include <map>
#include "common.h"
#include "param.h"

namespace pdlda {

class Data {
public:
	typedef struct {
		topic_id z;
		topic_id u;
		word_id w;
	} zuw;
	typedef struct {
		zuw* tokens;
		num* cntZ;
		size_t length;
		label_set labels;
	} doc_state;

	static doc_state* trainDocStates;
	static doc_state* testDocStates;
	static double* tmx;
	static num* cntWU;
	static num* cntU;

	static uint32_t* rnd;
	static std::size_t rndStPos;

	static std::map<std::string, word_id> wordStrMap;
	static std::map<std::string, label_id> labelStrMap;

	static num *testLabelDistAccum, *trainZUDistAccum;

	static inline double* getTmxRow(topic_id z) {
		return tmx + Param::numU * (std::size_t) z;
	}
	static inline num* getCntWURow(word_id w) {
		return cntWU + Param::numU * (std::size_t) w;
	}

	static void initRnd();
	static void loadTrainData();
	static void loadTestData();
	static void initCounts();
	static void initAccum();
	static void initTmx();
	static void init();
	static void destroy();
};

} /* namespace pdlda */

#endif /* DATA_H_ */