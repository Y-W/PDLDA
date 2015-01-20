/*
 * data.cpp
 *
 *  Created on: Jan 16, 2015
 *      Author: yw
 */

#include "data.h"
#include <cstring>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>
#include "mtrand.h"

using namespace pdlda;
using std::string;
using std::set;
using std::vector;

Data::doc_state* Data::trainDocStates;
Data::doc_state* Data::testDocStates;
double* Data::tmx;
num* Data::cntWU;
num* Data::cntU;
uint32_t* Data::rnd;
std::size_t Data::rndStPos = 0;
std::map<std::string, word_id> Data::wordStrMap;
std::map<std::string, label_id> Data::labelStrMap;
num* Data::testLabelDistAccum, *Data::trainZUDistAccum;

#ifdef DATALOADER_SHUFFLE_WORD_ORDER
static int rndP = 0;
static std::ptrdiff_t tmpRandFn(std::ptrdiff_t i) {
	return Data::rnd[rndP] % i;
}
#endif

class Doc {
public:
	set<label_id> labels;
	vector<word_id> words;
};

static label_set compactLabelSet(set<label_id> labels) {
	label_set ans = 0;
	for (set<label_id>::iterator it = labels.begin(); it != labels.end();
			it++) {
		ans |= (1 << *it);
	}
	return ans;
}

void Data::loadTrainData() {
	Param::numLabel = 0;
	Param::numWord = 0;
	num totalTokens = 0;

	std::ifstream bookFile(Param::trainDataPath.c_str());
	vector<Doc> tmpDocs;
	string line;

	while (std::getline(bookFile, line)) {
		if (line[0] == '#')
			continue;
		Doc doc;

		string labelS;
		if (line.find(':') == string::npos) {
			labelS = line.substr(0, line.find_first_of(' '));
			line = line.substr(line.find_first_of(' ') + 1);
		} else {
			labelS = line.substr(0, line.find_first_of(':'));
			line = line.substr(line.find_first_of(':') + 1);
		}
		std::istringstream labelStream(labelS);
		string labelStr;
		while (labelStream >> labelStr) {
			if (labelStrMap.count(labelStr) > 0) {
				doc.labels.insert(labelStrMap.find(labelStr)->second);
			} else {
				label_id newId = Param::numLabel++;
				doc.labels.insert(newId);
				labelStrMap.insert(
						std::pair<string, label_id>(labelStr, newId));
			}
		}

		string wordStr;
		std::istringstream wordStream(line);
		word_id lastId;
		bool lastValid = false;
		while (wordStream >> wordStr) {
			num n = atol(wordStr.c_str());
			if (n > 0) {
				if (!lastValid) {
					throw std::runtime_error(
							"Parsing book file failed: two numbers in a row!");
				}
				lastValid = false;
				for (num i = 0; i < n - 1; i++) { // n-1 not n!!
					doc.words.push_back(lastId);
				}
			} else {
				lastValid = true;
				if (wordStrMap.count(wordStr) > 0) {
					lastId = wordStrMap.find(wordStr)->second;
				} else {
					lastId = Param::numWord++;
					wordStrMap.insert(
							std::pair<string, word_id>(wordStr, lastId));
				}
				doc.words.push_back(lastId);
			}
		}

#ifdef DATALOADER_SHUFFLE_WORD_ORDER
		std::random_shuffle(doc.words.begin(), doc.words.end(), tmpRandFn);
#endif

		std::vector<word_id>(doc.words).swap(doc.words); // to shrink the capacity of words
		totalTokens += doc.words.size();

		tmpDocs.push_back(doc);
	}

	bookFile.close();

	Param::numTrainDoc = tmpDocs.size();
	Param::numZ = Param::numBgZ + Param::numLbZ * Param::numLabel;

	zuw* tokenSpace = new zuw[totalTokens];
	num* cntZSpace = new num[Param::numZ * (std::size_t) Param::numTrainDoc];
	trainDocStates = new doc_state[Param::numTrainDoc];

	for (std::size_t i = 0; i < Param::numTrainDoc; i++) {
		trainDocStates[i].length = tmpDocs[i].words.size();
		trainDocStates[i].labels = compactLabelSet(tmpDocs[i].labels);
		trainDocStates[i].tokens = tokenSpace;
		tokenSpace += trainDocStates[i].length;
		trainDocStates[i].cntZ = cntZSpace;
		cntZSpace += Param::numZ;
#pragma omp parallel for
		for (std::size_t j = 0; j < trainDocStates[i].length; j++) {
			trainDocStates[i].tokens[j].w = tmpDocs[i].words[j];
		}
	}
}
void Data::loadTestData() {

	std::ifstream bookFile_test(Param::testDataPath.c_str());
	string line;
	vector<Doc> tmpDocs;
	num totalTokens = 0;

	while (std::getline(bookFile_test, line)) {
		if (line[0] == '#')
			continue;
		Doc doc;

		string labelS;
		if (line.find(':') == string::npos) {
			labelS = line.substr(0, line.find_first_of(' '));
			line = line.substr(line.find_first_of(' ') + 1);
		} else {
			labelS = line.substr(0, line.find_first_of(':'));
			line = line.substr(line.find_first_of(':') + 1);
		}
		std::istringstream labelStream(labelS);
		string labelStr;
		while (labelStream >> labelStr) {
			if (labelStrMap.count(labelStr) > 0) {
				doc.labels.insert(labelStrMap.find(labelStr)->second);
			} else {
				// ignore
			}
		}

		string wordStr;
		std::istringstream wordStream(line);
		word_id lastId;
		bool lastValid = false;
		while (wordStream >> wordStr) {
			num n = atol(wordStr.c_str());
			if (n > 0) {
				if (!lastValid) {
					throw std::runtime_error(
							"Parsing book file failed: two numbers in a row!");
				}
				lastValid = false;
				if (lastId < WORD_ID_MAX) {
					for (num i = 0; i < n - 1; i++) { // n-1 not n!!
						doc.words.push_back(lastId);
					}
				}
			} else {
				lastValid = true;
				if (wordStrMap.count(wordStr) > 0) {
					lastId = wordStrMap.find(wordStr)->second;
					doc.words.push_back(lastId);
				} else {
					lastId = WORD_ID_MAX;
					// ignore
				}
			}
		}

#ifdef DATALOADER_SHUFFLE_WORD_ORDER
		std::random_shuffle(doc.words.begin(), doc.words.end(), tmpRandFn);
#endif

		std::vector<word_id>(doc.words).swap(doc.words); // to shrink the capacity of words

		totalTokens += doc.words.size();
		tmpDocs.push_back(doc);

	}

	bookFile_test.close();

	Param::numTestDoc = tmpDocs.size();

	zuw* tokenSpace = new zuw[totalTokens];
	num* cntZSpace = new num[Param::numZ * (std::size_t) Param::numTestDoc];
	testDocStates = new doc_state[Param::numTestDoc];

	for (std::size_t i = 0; i < Param::numTestDoc; i++) {
		testDocStates[i].length = tmpDocs[i].words.size();
		testDocStates[i].labels = compactLabelSet(tmpDocs[i].labels);
		testDocStates[i].tokens = tokenSpace;
		tokenSpace += testDocStates[i].length;
		testDocStates[i].cntZ = cntZSpace;
		cntZSpace += Param::numZ;
#pragma omp parallel for
		for (std::size_t j = 0; j < testDocStates[i].length; j++) {
			testDocStates[i].tokens[j].w = tmpDocs[i].words[j];
		}
	}
}

void Data::initCounts() {
	cntWU = new num[Param::numWord * (std::size_t) Param::numU];
	cntU = new num[Param::numU];
}

void Data::initAccum() {
	testLabelDistAccum = new num[Param::numTestDoc
			* (std::size_t) Param::numLabel];
	trainZUDistAccum = new num[Param::numZ * (std::size_t) Param::numU];
}

void Data::initTmx() {
	tmx = new double[Param::numZ * (std::size_t) Param::numU];
	for (std::size_t i = 0; i < Param::numZ * (std::size_t) Param::numU; i++) {
		tmx[i] = 1.0 / Param::numU;
	}
}

void Data::initRnd() {
	rnd = new uint32_t[RND_LENGTH];
	MTRand_int32 mtRand(Param::rndSeed);
	for (std::size_t i = 0; i < RND_LENGTH; i++) {
		rnd[i] = mtRand();
	}
}

void Data::init() {
	Data::initRnd();
	Data::loadTrainData();
	Data::loadTestData();
	Data::initCounts();
	Data::initTmx();
	Data::initAccum();
}

void Data::destroy() {
	delete[] rnd;
	delete[] tmx;
	delete[] cntWU;
	delete[] cntU;
	delete[] testLabelDistAccum;
	delete[] trainZUDistAccum;
	delete[] trainDocStates[0].tokens;
	delete[] trainDocStates[0].cntZ;
	delete[] trainDocStates;
	delete[] testDocStates[0].tokens;
	delete[] testDocStates[0].cntZ;
	delete[] testDocStates;
}
