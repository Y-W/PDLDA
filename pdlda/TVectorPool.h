/*
 * TVectorPool.h
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#ifndef TVECTORPOOL_H_
#define TVECTORPOOL_H_

#include "common.h"
#include "BookLoader.h"
#include <vector>

using std::vector;

namespace pdlda {
class TVectorPool {
public:
	static TVectorPool* inst;
	TVectorPool();
	~TVectorPool();

	topic_id numBgZ;
	topic_id numZPerLb;
	topic_id numZ;
	double klRegulatorCoeff;
	num klsIter;

	double* tVecs;
	inline double* getTVec(topic_id z) {
		return tVecs + (size_t) z * BookLoader::inst->numU;
	}

	void cacheCurrentDocState4Update();
	void clearDocStateCache();
	void updateTVec();
	void getZ4Doc(doc_id docId, vector<topic_id>& newList);
	void getZ4Lb_test(label_id wordId, vector<topic_id>& newList);

	void getZ4Doc_all(vector<topic_id>& newList);

private:
	num* distZUCache;
};
}


#endif /* TVECTORPOOL_H_ */
