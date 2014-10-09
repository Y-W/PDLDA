/*
 * DocState.h
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#ifndef DOCSTATE_H_
#define DOCSTATE_H_

#include <vector>
#include "common.h"

using std::vector;

namespace pdlda {
class DocState {
public:
	typedef struct {
		topic_id z;
		topic_id u;
		word_id w;
	} zuw;
	typedef struct {
		zuw* tokens;
		num length;
		num* distDocZ;
	} doc;
	static DocState* inst;
	DocState();
	~DocState();

	doc& getDocState(doc_id docId);

	doc** docStates;

	doc& getDocState_test(doc_id docId);
	doc** docStates_test;
};
}

#endif /* DOCSTATE_H_ */
