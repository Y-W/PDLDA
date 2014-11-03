/*
 * book_loader.cc
 *
 *  Created on: Jun 24, 2014
 *      Author: yw
 */

#ifndef BOOK_LOADER_CC_
#define BOOK_LOADER_CC_

#include <vector>
#include <set>
#include <string>

#include "common.h"

using std::vector;
using std::string;
using std::set;

namespace pdlda {

class BookLoader {
	class Doc {
	public:
		set<label_id> labels;
		vector<word_id> words;
	};
public:
	static BookLoader* inst;

	BookLoader();
	vector<string> wordStrList;
	word_id numWord;
	vector<string> labelStrList;
	label_id numLabel;
	vector<Doc> documents;
	num numDoc;

	vector<Doc> documents_test;
	num numDoc_test;

	topic_id numU;

	vector<num> wordDist;
	vector<num> wordDist_test;
private:
	void loadBook();
};

}

#endif /* BOOK_LOADER_CC_ */
