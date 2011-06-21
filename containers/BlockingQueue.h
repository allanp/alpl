#ifndef _ALPL_COLLECTIONS_BLOCKING_QUEUE_H_
#define _ALPL_COLLECTIONS_BLOCKING_QUEUE_H_

#include "../threading/threading.h"

#ifdef _ALPL_HEADER_ONLY_
#define _ALPL_CONTAINERS_HEADER_ONLY_
#endif

namespace alpl{
namespace containers{

//// blocking queue
//
template<typename T>
class blocking_queue{
private:
	const static int max_queue_size = 1024;
	const static int max_name_length = 32;

	alpl::threading::sync_point sync_;
	char priority_;
	char name_[max_name_length];
	std::deque<T> q_;
	size_t max_size_;

	alpl::threading::cond_var queue_is_full;
	alpl::threading::cond_var queue_is_empty;

public:
	//concurrent_queue();
	blocking_queue(int max_size = max_queue_size);

	blocking_queue(const char* name, int max_size);

	~blocking_queue();

	inline const char* name() const { return name_; }
	inline const char priority() const { return priority_; }

	bool empty() const {
		sync(sync_){
			return q_.empty();
		}
	}

	const size_t size() const {
		sync(sync_){
			return q_.size();
		}
	}

	bool try_push_back(const T& item, long milliseconds);
	bool try_pop_front(T& item, long milliseconds);

	void push_back(const T& item);
	void pop_front(T& item);
};

};
};

#endif // _ALPL_COLLECTIONS_BLOCKING_QUEUE_

#ifdef _ALPL_CONTAINERS_HEADER_ONLY_
# include "impl/BlockingQueue.hpp"
#endif // ifdef _ALPL_CONTAINERS_HEADER_ONLY_