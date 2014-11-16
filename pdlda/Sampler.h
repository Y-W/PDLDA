/*
 * Sampler.h
 *
 *  Created on: Jul 30, 2014
 *      Author: yw
 */

#ifndef SAMPLER_H_
#define SAMPLER_H_

#include "common.h"
#include "RequestPool.h"

#define REQUEST_POOL_MAX_SIZE_COEFF (size_t)10

namespace pdlda {
class Sampler {
public:
	typedef struct {
			doc_id docId;
			num wordPos;
		} doc_word;

	static Sampler* inst;

	double alpha;
	double beta;

	Sampler();
	~Sampler();
	void initDocStates();
	void sampleAllDocOnce();
	void updateDistUCache();

	void sampleDocWord(doc_id docId, num wordPos, void* replyContent);
	void updateDistUCache(void* replyContent);

	void initDocStates_test();
	void sampleAllDocOnce_test();
	void sampleDocWord_test(doc_id docId, num wordPos, void* replyContent);

	num* docTopic;

	void clearDocLabelCnt();
	void accumDocLabelCnt();

	void judgeTest();

private:

	num* distUCache;
	RequestPool<bool> updatePool;
	RequestPool<bool> fetchSendPool;
	RequestPool<doc_word> fetchPool;

};
}

#endif /* SAMPLER_H_ */
