/*
 * common.hpp
 *
 *  Created on: Jul 24, 2014
 *      Author: yw
 */

#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <stdint.h>
#include <ctime>
#include <string>

namespace pdlda {

typedef uint8_t topic_id;
typedef uint32_t word_id;
typedef uint8_t label_id;
typedef uint16_t doc_id;
typedef uint32_t num;
typedef uint8_t task_id;

#define NUM_MPI_TYPE MPI_UINT32_T
#define WORD_ID_MPI_TYPE MPI_UINT32_T
#define WORD_ID_MAX UINT32_MAX

#ifndef UINT32_MAX
#define UINT32_MAX (0xFFFFFFFFU)
#endif

#ifndef GET_CURRENT_TIME_STRING_
#define GET_CURRENT_TIME_STRING_

inline std::string getCurrentTimeString() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 80, "%d-%m-%Y %I:%M:%S", timeinfo);
	std::string str(buffer);
	return str;
}

#endif

}

#endif /* COMMON_HPP_ */
