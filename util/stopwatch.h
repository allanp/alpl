#ifndef _ALPL_UTIL_STOPWATCH_H_
#define _ALPL_UTIL_STOPWATCH_H_

#define _CRT_SECURE_NO_WARNINGS


#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <stdio.h>

namespace alpl{

namespace util{

	class Stopwatch{
	private:
#ifdef _WIN32
		__int64 _freq;
#endif

		double _start;
		double _elapsed;
		bool _started;

		static const unsigned int _sec_factor_	= 1000;
		static const unsigned int _min_factor_	= 1000 * 60;
		static const unsigned int _hour_factor_	= 1000 * 60 * 60;
		static const unsigned int _day_factor_	= 1000 * 60 * 60 * 24;
		static const unsigned int _month_factor_ = 1000 * 60 * 60 * 24 * 30;

		inline int convert(long& msecValue, const unsigned int factor){

			int value = msecValue / factor;
			msecValue -= value * factor;
			return value;
		}

	public:

		Stopwatch();
		~Stopwatch();
		inline void begin();
		inline void stop();

		const char* readable();
		const char* readable2();
		
		/* helper method */
		inline static Stopwatch& start_new();

		inline bool started() const { return _started; }

		/* milliseconds */ 
		inline const double elapsedmilliseconds() const { return _elapsed; }

		/* seconds */
		inline const double elapsedseconds() const { return _elapsed / 1000; }

	}; // class timer


}; // namespace timing
}; //namespace alp

#endif // _ALPL_UTIL_STOPWATCH_H_

#if defined(_ALPL_THREADING_HEADER_ONLY_)
# include "impl/stopwatch.hpp"
#endif // defined(_ALPL_NET_HEADER_ONLY_)