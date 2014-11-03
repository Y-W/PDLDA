/*
 * CntrServer_simp.h
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#ifndef CntrServer_simp_H_
#define CntrServer_simp_H_

#include "common.h"
#include "BookLoader.h"

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

	num* wordUDist;
	num* uDist;

	void sync();
	void update(updateInfo d);
	num* fetch(word_id w);
private:
	inline num* getWordEntry(word_id w) {
		return wordUDist + ((size_t) w) * BookLoader::inst->numU;
	}
};
}

#endif /* CntrServer_simp_H_ */
