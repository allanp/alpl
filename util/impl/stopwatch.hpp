#ifndef _ALPL_UTIL_STOPWATCH_IPP_
#define _ALPL_UTIL_STOPWATCH_IPP_

#include "../stopwatch.h"

namespace alpl{
namespace util{

Stopwatch::Stopwatch(): _start(0), _started(false){
	#ifdef _WIN32
	QueryPerformanceFrequency((LARGE_INTEGER *)&_freq);
	#endif
}

Stopwatch::~Stopwatch(){}

inline void Stopwatch::begin(){
	_started = true;
#ifdef _WIN32
	__int64 t;
	QueryPerformanceCounter((LARGE_INTEGER *)&t);
	_start = (double)t * 1000 / (double)_freq;
#else
	struct timeval t;
	gettimeofday( &t, (struct timezone *)0 );
	_start = t.tv_sec * 1.0e3 + t.tv_usec * 1.0e - 3;
#endif
}


inline void Stopwatch::stop(){
#ifdef _WIN32
	__int64 t;
	QueryPerformanceCounter((LARGE_INTEGER *)&t);
	_elapsed = ((double)t * 1000 / (double)_freq) - _start;
#else
	struct timeval t;
	gettimeofday( &t, (struct timezone *)0 );
	_elapsed = (t.tv_sec * 1.0e3 + t.tv_usec * 1.0e - 3) - _start;
#endif
	_started = false;
}

const char* Stopwatch::readable() {

	long msecValue = (long)(_elapsed + 0.5);

	int totalDays = msecValue / _day_factor_;

	int yearValue = totalDays / 365;
	int monthValue = convert(msecValue, _month_factor_);
	int dayValue = convert(msecValue, _day_factor_);
	int hourValue = convert(msecValue, _hour_factor_);
	int minValue = convert(msecValue, _min_factor_);
	int secValue = convert(msecValue, _sec_factor_);

	char str[32];

	if(yearValue)
		sprintf(str, "%dY%dM%dD%02d:%02d:%02d.%03d", yearValue, monthValue, dayValue, hourValue, minValue, secValue, msecValue);
	else if(monthValue)
		sprintf(str, "%dM%dD%02d:%02d:%02d.%03d", monthValue, dayValue, hourValue, minValue, secValue, msecValue);
	else if( dayValue )
		sprintf(str, "%dD%02d:%02d:%02d.%03d", dayValue, hourValue, minValue, secValue, msecValue);
	else if(hourValue)
		sprintf(str, "%d:%02d:%02d.%03d", hourValue, minValue, secValue, msecValue);
	else if(minValue)
		sprintf(str, "%d:%02d.%03d", minValue, secValue, msecValue);
	else if(secValue)
		sprintf(str, "%d.%03d", secValue, msecValue);
	else
		sprintf(str, "0.%03d", msecValue);

	return strdup(str);
}


const char* Stopwatch::readable2(){

	long msecValue = (long)_elapsed;

	int microValue = (int)((_elapsed - msecValue) * 1000 + 0.5);

	int totalDays = msecValue / _day_factor_;

	int yearValue = totalDays / 365;
	int monthValue = convert(msecValue, _month_factor_);
	int dayValue = convert(msecValue, _day_factor_);
	int hourValue = convert(msecValue, _hour_factor_);
	int minValue = convert(msecValue, _min_factor_);
	int secValue = convert(msecValue, _sec_factor_);

	char str[32];

	if(yearValue)
		sprintf(str, "%dY%dM%dD%02d:%02d:%02d.%03d%03d", yearValue, monthValue, dayValue, hourValue, minValue, secValue, msecValue, microValue);
	else if(monthValue)
		sprintf(str, "%dM%dD%02d:%02d:%02d.%03d%03d", monthValue, dayValue, hourValue, minValue, secValue, msecValue, microValue);
	else if( dayValue )
		sprintf(str, "%dD%02d:%02d:%02d.%03d%03d", dayValue, hourValue, minValue, secValue, msecValue, microValue);
	else if(hourValue)
		sprintf(str, "%d:%02d:%02d.%03d%03d", hourValue, minValue, secValue, msecValue, microValue);
	else if(minValue)
		sprintf(str, "%d:%02d.%03d%03d", minValue, secValue, msecValue, microValue);
	else if(secValue)
		sprintf(str, "%d.%03d%03d", secValue, msecValue, microValue);
	else
		sprintf(str, "0.%03d%03d", msecValue, microValue);

	return strdup(str);
}

inline Stopwatch& Stopwatch::start_new(){
	Stopwatch* sw = new Stopwatch();
	sw->begin();
	return *sw;
}

};
};

#endif // _ALPL_UTIL_STOPWATCH_IPP_
