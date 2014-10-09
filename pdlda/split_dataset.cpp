/*
 * split_dataset.cc
 *
 *  Created on: Jul 3, 2014
 *      Author: yw
 */

#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>

#include "mtrand.h"

using namespace std;

static string inputFileName;
static string trainFilename;
static string testFilename;
static double testRatio = 0;

static MTRand_int32 randGen;

void parseArg(int argc, char **argv) {
	if (argc < 5) {
		cout << "Arguments parsing failed!";
		exit(EXIT_FAILURE);
	}

	int p = 1;
	while (p < argc) {
		if (strcmp(argv[p], "--test_ratio") == 0) {
			p++;
			testRatio = atof(argv[p]);
			p++;
			continue;
		}
		if (strcmp(argv[p], "--input") == 0) {
			p++;
			inputFileName = string(argv[p]);
			p++;
			continue;
		}
		if (strcmp(argv[p], "--train_output") == 0) {
			p++;
			trainFilename = string(argv[p]);
			p++;
			continue;
		}
		if (strcmp(argv[p], "--test_output") == 0) {
			p++;
			testFilename = string(argv[p]);
			p++;
			continue;
		}
		cout << "Arguments parsing failed!";
		exit(EXIT_FAILURE);
	}

	if (trainFilename.empty()) {
		string s1 = inputFileName.substr(0,
				inputFileName.find_last_not_of('.'));
		string s2 = inputFileName.substr(inputFileName.find_last_not_of('.'));
		trainFilename = s1 + "_train" + s2;
	}
	if (testFilename.empty()) {
		string s1 = inputFileName.substr(0,
				inputFileName.find_last_not_of('.'));
		string s2 = inputFileName.substr(inputFileName.find_last_not_of('.'));
		trainFilename = s1 + "_test" + s2;
	}
}

static int tmpRandFn(int i) {
	return randGen() % i;
}

int main(int argc, char **argv) {
	parseArg(argc, argv);
	ifstream in(inputFileName.c_str());
	ofstream train(trainFilename.c_str());
	ofstream test(testFilename.c_str());

	vector<string> lines;
	string line;
	while (getline(in, line)) {
		if (line.empty() || line[0] == '#'
				|| line.find_first_not_of(' ') == string::npos) {
			continue;
		}
		lines.push_back(line);
	}

	random_shuffle(lines.begin(), lines.end(), tmpRandFn);

	for (unsigned i = 0; i < lines.size(); i++) {
		if (i < lines.size() * testRatio) {
			test << lines[i] << "\n";
		} else {
			train << lines[i] << "\n";
		}
	}

	in.close();
	train.flush();
	train.close();
	test.flush();
	test.close();
	return EXIT_SUCCESS;
}
