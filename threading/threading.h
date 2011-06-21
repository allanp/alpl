#ifndef _ALPL_THREADING_THREADING_H_
#define _ALPL_THREADING_THREADING_H_

#include <deque>
#include "assert.h"

#ifdef _MSC_VER

#include "Windows.h"

//#ifdef _WIN32_WINNT
//#undef _WIN32_WINNT
//#define _WIN32_WINNT _WIN32_WINNT_NT4
//#define _WIN32_WINNT _WIN32_WINNT_WIN2K
//#define _WIN32_WINNT _WIN32_WINNT_WINXP
//#define _WIN32_WINNT _WIN32_WINNT_WS03
//#define _WIN32_WINNT _WIN32_WINNT_VISTA
//#define _WIN32_WINNT _WIN32_WINNT_WS08
//#define _WIN32_WINNT _WIN32_WINNT_WIN7
//#endif


#include "../util/errno.h"

#ifdef _ALPL_HEADER_ONLY_
#define _ALPL_THREADING_HEADER_ONLY_
#endif

namespace alpl{
namespace threading{

typedef HANDLE thread_t;

#define thread(ref_thread_proc, ref_param, ref_thread_id) \
	CreateThread(0, 0, ref_thread_proc, ref_param, 0, ref_thread_id)

template <typename T>
struct ThreadInfo {
	// we have an object
	T* obj;
	// and that object has a function that takes void* and returns DWORD, and so is suitable for a threadproc (except for it being a member function)
	DWORD (T::* function)(void*);
	
	// and we have any amount of extra data that we might need.
	void* data;
	
	// default copy c-tor, d-tor and operator= are fine
	ThreadInfo(T* o, DWORD (T::*func)(void*), void* d) : obj(o), function(func), data(d) { }
};

template <typename T>
DWORD WINAPI createThreadFunction(void* data){
	std::auto_ptr< ThreadInfo<T> > ti((ThreadInfo<T>*)data);
	return ((ti->obj)->*(ti->function))(ti->data);
}

template <typename T>
void* createThreadInfo(T* obj, DWORD(T::* fun)(void*), void* data){
	return (void*)new ThreadInfo<T>(obj, fun, data);
}


//// sync structs
//
typedef CRITICAL_SECTION sync_point;

typedef struct sync_t{
	sync_point *cs_;
	bool entered;

	sync_t(sync_point& cs) : cs_(&cs), entered(false) {
		EnterCriticalSection(cs_);
		entered = true;
	}

	~sync_t(void) {
		LeaveCriticalSection(cs_);
	}
}sync_t;

#define init_sync(ref_cs_)	InitializeCriticalSection(ref_cs_)
#define sync(cs_)			for(sync_t s(cs_); s.entered; s.entered = false)
#define del_sync(ref_cs_)	DeleteCriticalSection(ref_cs_)

//// conditional variables
//

#if _WIN32_WINNT > _WIN32_WINNT_WINXP	// if Win vista or later

typedef CONDITION_VARIABLE cond_var;

#define cond_init(ref_cond_var) InitializeConditionVariable(ref_cond_var)

#define cond_wait(ref_cond_var) void(NULL)

#define cond_sig_wait(ref_cs_, ref_cv_) SleepConditionVariableCS(ref_cv_, ref_cs_, INFINITE)

#define cond_try_sig_wait(ref_cs_, ref_cv_, milliseconds) \
	SleepConditionVariableCS(ref_cv_, ref_cs_, milliseconds)

#define cond_wake(cond_var) WakeConditionVariable(& cond_var)

#else

typedef HANDLE cond_var;

#define cond_init(init) \
	CreateEvent(NULL, FALSE, init, NULL);

#define cond_wake(cond_var) \
	SetEvent(cond_var)

#define cond_reset(cond_var) \
	ResetEvent(cond_var)

#define cond_wait(cond_var, milliseconds) \
	WaitForSingleObject(e, milliseconds)

#define cond_try_sig_wait(to_sig, to_wait, milliseconds) \
	SignalObjectAndWait(to_sig, to_wait, milliseconds, FALSE)

#endif

};
};

#endif // _MSC_VER
#endif // _THREADING_H_
