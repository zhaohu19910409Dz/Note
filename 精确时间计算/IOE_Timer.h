// IOE_Timer.h

#ifndef __IOE_Timer_H__
#define __IOE_Timer_H__

#include <stdint.h>
#include "IOE_Types.h"

class Timer
{
public:
	static double GetSeconds();
	static uint64_t GetTicksNanos();

#ifdef IOE_OS_MS
	static double GetPerfFrequencyInverse();
#endif

	static uint32_t GetTicksMs()
	{
		return  uint32_t(GetTicksNanos() / 1000000);
	}

	static void InitializeTimerSystem();
	static void ShutdownTimerSystem();
};
#endif // __IOE_Timer_H__
