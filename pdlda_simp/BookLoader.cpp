/*
 * book_loader.cc
 *
 *  Created on: Jun 24, 2014
 *      Author: yw
 */

#include "BookLoader.h"

#include <stddef.h>
//#include <algorithm>
#include <cstdlib>
#include <fstream>
//#include <iostream>
#include <map>
//#include <sstream>
#include <stdexcept>
#include <utility>

#include "ArgRegister.h"
#include "RandGen.h"
#include "tclap/ValueArg.h"
#include "TaskAssigner.h"

using namespace pdlda;

static TCLAP::ValueArg<string> books("i", "books",
		"File containing books to train on", true, "", "path to file");
REGISTER_ARG(books)

static TCLAP::ValueArg<string> books_test("I", "books_test",
		"File containing books to test on", true, "", "path to file");
REGISTER_ARG(books_test)

static TCLAP::ValueArg<unsigned int> numUArg("u", "num_u", "Number of U", true,
		0, "integer");
REGISTER_ARG(numUArg)

BookLoader* BookLoader::inst;

BookLoader::BookLoader() {
	numU = numUArg.getValue();
	loadBook();
	BookLoader::inst = this;
}

#ifdef BOOKLOADER_SHUFFLE_WORD_ORDER
static std::ptrdiff_t tmpRandFn(std::ptrdiff_t i) {
	return RandGen::inst->getInt(i);
}
#endif

void BookLoader::loadBook() {
	std::map<string, word_id> wordStrMap;
	std::map<string, label_id> labelStrMap;

	std::ifstream bookFile(books.getValue().c_str());
	string line;
//
//	num sumTokens = 0;

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
				label_id newId = labelStrList.size();
				doc.labels.insert(newId);
				labelStrMap.insert(
						std::pair<string, label_id>(labelStr, newId));
				labelStrList.push_back(labelStr);
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
					wordDist[lastId]++;
				}
			} else {
				lastValid = true;
				if (wordStrMap.count(wordStr) > 0) {
					lastId = wordStrMap.find(wordStr)->second;
				} else {
					lastId = wordStrList.size();
					wordStrMap.insert(
							std::pair<string, word_id>(wordStr, lastId));
					wordStrList.push_back(wordStr);
					wordDist.push_back(0);
				}
				doc.words.push_back(lastId);
				wordDist[lastId]++;
			}
		}

#ifdef BOOKLOADER_SHUFFLE_WORD_ORDER
		std::random_shuffle(doc.words.begin(), doc.words.end(),
				tmpRandFn);
#endif

		std::vector<word_id>(doc.words).swap(doc.words); // to shrink the capacity of words

		if (TaskAssigner::inst->getSampId(documents.size())
				!= TaskAssigner::inst->rank) {
			doc.words.clear();
		}

		documents.push_back(doc);
//
//		sumTokens += doc.words.size();
//		std::cout<<"Current Token count: "<<sumTokens<<"\n";

	}

	bookFile.close();

	numDoc = documents.size();
	numLabel = labelStrList.size();
	numWord = wordStrList.size();


//	for (size_t i = 0; i < numDoc; i++) {
//		for (size_t j = 0; j < documents[i].words.size(); j++) {
//			wordDist[documents[i].words[j]]++;
//		}
//	}

	// Load test files.

	for (word_id i = 0; i < numWord; i++) {
		wordDist_test.push_back(0);
	}

	std::ifstream bookFile_test(books_test.getValue().c_str());

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
						wordDist_test[lastId]++;
					}
				}
			} else {
				lastValid = true;
				if (wordStrMap.count(wordStr) > 0) {
					lastId = wordStrMap.find(wordStr)->second;
					doc.words.push_back(lastId);
					wordDist_test[lastId]++;
				} else {
					lastId = WORD_ID_MAX;
					// ignore
				}
			}
		}

#ifdef BOOKLOADER_SHUFFLE_WORD_ORDER
		std::random_shuffle(doc.words.begin(), doc.words.end(),
				tmpRandFn);
#endif

		std::vector<word_id>(doc.words).swap(doc.words); // to shrink the capacity of words

		if (TaskAssigner::inst->getSampId(documents_test.size())
				!= TaskAssigner::inst->rank) {
			doc.words.clear();
		}

		documents_test.push_back(doc);

	}

	bookFile_test.close();

	numDoc_test = documents_test.size();

//	for (size_t i = 0; i < numDoc_test; i++) {
//		for (size_t j = 0; j < documents_test[i].words.size(); j++) {
//			wordDist_test[documents_test[i].words[j]]++;
//		}
//	}

#ifdef BOOKLOADER_SHRINK_SPACE
	wordStrList.clear();
	labelStrList.clear();
#endif
}

