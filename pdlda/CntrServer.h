/*
 * CntrServer_simp.h
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#ifndef CntrServer_simp_H_
#define CntrServer_simp_H_

#include "common.h"


#define TAG_FETCH 5
#define TAG_UPDATE 6

namespace pdlda {
class CntrServer_simp {
public:
	typedef struct {
		word_id w;
		topic_id decU;
		topic_id incU;
	} updateInfo;

	static CntrServer_simp* inst;
	CntrServer_simp();
	~CntrServer_simp();

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

#endif /* CntrServer_simp_H_ */
