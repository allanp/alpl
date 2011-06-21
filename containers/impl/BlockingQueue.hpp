#ifndef _ALPL_COLLECTIONS_BLOCKING_QUEUE_IPP_
#define _ALPL_COLLECTIONS_BLOCKING_QUEUE_IPP_

namespace alpl{
namespace containers{

template<typename T>
blocking_queue<T>::blocking_queue(int max_size) :  q_(max_size), max_size_((size_t)max_size), priority_(0) {

	init_sync(&sync_);

#if _WIN32_WINNT > _WIN32_WINNT_WINXP // if vista or later
	cond_init(&queue_is_full);
	cond_init(&queue_is_empty);
#else // if win 2000 or win xp
	queue_is_full = cond_init(FALSE);
	queue_is_empty = cond_init(FALSE);
#endif
	memset(name_, 0, sizeof(char)*max_name_length);
}


template<typename T>
blocking_queue<T>::blocking_queue(const char* name, int max_size) : q_(max_size),  max_size_((size_t)max_size), priority_(0) {
	
	init_sync(&sync_);
#if _WIN32_WINNT > _WIN32_WINNT_WINXP // if vista or later
	cond_init(&queue_is_full);
	cond_init(&queue_is_empty);
#else // if win 2000 or win xp
	queue_is_full = cond_init(FALSE);
	queue_is_empty = cond_init(FALSE);
#endif
	int len = strlen(name);
	name_ = malloc(sizeof(char) * (len+1));
	strncpy(name_, name, sizeof(char)*len);
	name_[len] = '\0';
}

template<typename T>
blocking_queue<T>::~blocking_queue(){
	DeleteCriticalSection(&sync_);
#if _WIN32_WINNT > _WIN32_WINNT_WINXP // if vista or later
#else
	CloseHandle(queue_is_full);
	CloseHandle(queue_is_empty);
#endif
}


template<typename T>
bool blocking_queue<T>::try_push_back(const T& item, long milliseconds){

	sync(sync_){
		while( q_.size() >= max_size_ ){
#if _WIN32_WINNT > _WIN32_WINNT_WINXP // if vista or later
			if(cond_try_sig_wait(&sync_, &queue_is_full, milliseconds) == 0){

				DWORD error = GetLastError();
				// MSDN states returns WAIT_TIMEOUT, but in fact, it returns ERROR_TIMEOUT
				if( WAIT_TIMEOUT == error || ERROR_TIMEOUT == error){
					return false;	// timout
				}
			}
#else
			LeaveCriticalSection(&sync_);
			DWORD dw = WaitForSingleObject(queue_is_full, milliseconds);
			switch( dw ){
				case WAIT_TIMEOUT: // timeout
					return false;
				case WAIT_OBJECT_0: // signaled
					EnterCriticalSection(&sync_);
					break;
				case WAIT_FAILED:
					fprintf(stderr, "concurrent queue push back sync failed, error code: %d\n", GetLastError());
					break;
				default:
					break;
			}
#endif
		}
		q_.push_back(item);

		cond_wake(queue_is_empty);
	}
	return true;
}


template<typename T>
bool blocking_queue<T>::try_pop_front(T& item, long milliseconds){

	sync(sync_){
		while( q_.size() == 0 ){
#if _WIN32_WINNT > _WIN32_WINNT_WINXP // if vista or later
			if(cond_try_sig_wait(&sync_, &queue_is_empty, milliseconds) == 0){

				DWORD error = GetLastError();
				// MSDN states returns WAIT_TIMEOUT, but in fact, it returns ERROR_TIMEOUT
				if( WAIT_TIMEOUT == error || ERROR_TIMEOUT == error){
					return false;	// timout
				}
			}
#else
			LeaveCriticalSection(&sync_);

			DWORD dw = WaitForSingleObject(queue_is_empty, milliseconds);
			switch( dw){
				case WAIT_TIMEOUT:  // timeout
					return false;
				case WAIT_OBJECT_0: // signaled
					EnterCriticalSection(&sync_);
					break;
				case WAIT_FAILED:
					fprintf(stderr, "concurrent queue pop front sync failed, error code: %d\n", GetLastError());
					break;
				default:
					break;
			}
#endif
		}

		item = q_.front();
		q_.pop_front();
		cond_wake(queue_is_full);
	}
	return true;
};


template<typename T>
void blocking_queue<T>::push_back(const T& item){
	try_push_back(item, INFINITE);
}


template<typename T>
void blocking_queue<T>::pop_front(T& item){
	try_pop_front(item, INFINITE);
}

}; // namespace containers
}; // namespace alpl

#endif