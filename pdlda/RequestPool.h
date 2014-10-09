/*
 * RequestPool.h
 *
 *  Created on: Jul 28, 2014
 *      Author: yw
 */

#ifndef REQUESTPOOL_H_
#define REQUESTPOOL_H_

#include <mpi/mpi.h>
#include <stddef.h>
#include <string>
#include <vector>

#include "common.h"

using std::vector;

namespace pdlda {
template<typename T>
class RequestPool {
private:
	size_t _maxSize;
	void (*_callback)(T&, void *, MPI_Status *);
	typedef struct {
		T val;
		void* buf;
		MPI_Request* req;
	} _store;
	vector<_store> _reqs;
public:
	RequestPool(size_t maxSize, void (*callback)(T&, void *, MPI_Status *)) :
			_maxSize(maxSize), _callback(callback) {
	}
	;
	void checkReq() {
		size_t p = 0;
		while (p < _reqs.size()) {
			MPI_Status status;
			int flag;
			MPI_Test(_reqs[p].req, &flag, &status);
			if (flag) {
				delete _reqs[p].req;
				_callback(_reqs[p].val, _reqs[p].buf, &status);
				_reqs[p] = _reqs[_reqs.size() - 1];
				_reqs.pop_back();
			} else {
				p++;
			}
		}
	}
	;
	size_t getSize() {
		checkReq();
		return _reqs.size();
	}
	;
	void addRequest(MPI_Request* req, void * buf, T& val) {
		_store tmp;
		tmp.req = req;
		tmp.buf = buf;
		tmp.val = val;
		_reqs.push_back(tmp);

//		if (sizeof(val) == 1 && *(bool*) &val == false) {
//			std::cout << buf << " " << *(word_id*) buf << "#2.1\n";
//		}

		waitTillSizeSmaller(_maxSize);

//		if (sizeof(val) == 1 && *(bool*) &val == false) {
//			std::cout << buf << " " << *(word_id*) buf << "#2.4\n";
//		}
	}
	;
	void waitTillClear() {
		waitTillSizeSmaller(0);
	}
	;
	void waitTillSizeSmaller(size_t threshold) {
		while (getSize() > threshold)
			;
	}
	;
	~RequestPool() {
		waitTillClear();
	}
};
}

#endif /* REQUESTPOOL_H_ */
