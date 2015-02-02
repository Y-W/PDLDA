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
#include <sstream>
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
	static doc_state* unlabelledDocStates;
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
	static void loadUnlabelledData();
	static void initCounts();
	static void initAccum();
	static void initTmx();
	static void init();
	static void destroy();

	template<typename T>
	static std::string get1dArrayStr(size_t s, T* arr){
		std::stringstream tmp;
		for (size_t i = 0; i < s - 1; i++) {
			tmp << arr[i] << ", ";
		}
		tmp << arr[s - 1];
		return tmp.str();
	}

	template<typename T>
	static std::string get2dArrayStr(size_t r, size_t c, T* arr){
		std::stringstream tmp;
		for (size_t i = 0; i < r; i++) {
			tmp << get1dArrayStr(c, arr+i*c) << std::endl;
		}
		return tmp.str();
	}

};

} /* namespace pdlda */

#endif /* DATA_H_ */
