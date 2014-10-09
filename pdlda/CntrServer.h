/*
 * CntrServer.h
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#ifndef CNTRSERVER_H_
#define CNTRSERVER_H_

#include "common.h"


#define TAG_FETCH 5
#define TAG_UPDATE 6

namespace pdlda {
class CntrServer {
public:
	typedef struct {
		word_id w;
		topic_id decU;
		topic_id incU;
	} updateInfo;

	static CntrServer* inst;
	CntrServer();
	~CntrServer();

	num** wordUDist;
	num opNum_l;
	num opNum_l_test;
	num opNum_s;

	void listen(num numFetch, num numUpdate);

private:
	num estimateOpNum_l();
	num estimateOpNum_s();
	num estimateOpNum_l_test();
};
}

#endif /* CNTRSERVER_H_ */
