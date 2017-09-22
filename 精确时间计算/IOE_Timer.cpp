// IOE_Timer.cpp

#include "IOE_Timer.h"

#if defined(IOE_OS_MS) && !defined(IOE_OS_MS_MOBILE)

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#endif

#if defined(IOE_OS_MS)

struct PerformanceTimer
{
	PerformanceTimer()
		: UsingVistaOrLater(false),
		TimeCS(),
		OldMMTimeMs(0),
		MMTimeWrapCounter(0),
		PerfFrequency(0),
		PerfFrequencyInverse(0),
		PerfFrequencyInverseNanos(0),
		PerfMinusTicksDeltaNanos(0),
		LastResultNanos(0)
	{ }

	enum {
		MMTimerResolutionNanos = 1000000
	};

	void    Initialize();
	void    Shutdown();

	uint64_t  GetTimeSeconds();
	double    GetTimeSecondsDouble();
	uint64_t  GetTimeNanos();

	UINT64 getFrequency()
	{
		if (PerfFrequency == 0)
		{
			LARGE_INTEGER freq;
			::QueryPerformanceFrequency(&freq);
			PerfFrequency = freq.QuadPart;
			PerfFrequencyInverse = 1.0 / (double)PerfFrequency;
			PerfFrequencyInverseNanos = 1000000000.0 / (double)PerfFrequency;
		}
		return PerfFrequency;
	}

	double GetFrequencyInverse()
	{
		IOE_ASSERT(PerfFrequencyInverse != 0.0);
		return PerfFrequencyInverse;
	}

	// In Vista+ we are able to use QPC exclusively.
	bool            UsingVistaOrLater;

	CRITICAL_SECTION TimeCS;
	// timeGetTime() support with wrap.
	uint32_t        OldMMTimeMs;
	uint32_t        MMTimeWrapCounter;
	// Cached performance frequency result.
	uint64_t        PerfFrequency;              // cycles per second, typically a large value like 3000000, but usually not the same as the CPU clock rate.
	double          PerfFrequencyInverse;       // seconds per cycle (will be a small fractional value).
	double          PerfFrequencyInverseNanos;  // nanoseconds per cycle.

												// Computed as (perfCounterNanos - ticksCounterNanos) initially,
												// and used to adjust timing.
	uint64_t        PerfMinusTicksDeltaNanos;
	// Last returned value in nanoseconds, to ensure we don't back-step in time.
	uint64_t        LastResultNanos;
};

static PerformanceTimer Win32PerfTimer;

void PerformanceTimer::Initialize()
{
	::InitializeCriticalSection(&this->TimeCS);
	MMTimeWrapCounter = 0;
	PerfFrequencyInverseNanos = getFrequency();

#if defined(IOE_OS_WIN32) // Desktop Windows only
	// Set Vista flag.  On Vista, we can just use QPC() without all the extra work
	OSVERSIONINFOEXW ver;
	ZeroMemory(&ver, sizeof(OSVERSIONINFOEXW));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	ver.dwMajorVersion = 6; // Vista+

	DWORDLONG condMask = 0;
	VER_SET_CONDITION(condMask, VER_MAJORVERSION, VER_GREATER_EQUAL);

	// VerifyVersionInfo returns true if the OS meets the conditions set above
	UsingVistaOrLater = ::VerifyVersionInfoW(&ver, VER_MAJORVERSION, condMask) != 0;
#else
	UsingVistaOrLater = true;
#endif

	if (!UsingVistaOrLater)
	{
#if defined(IOE_OS_WIN32) // Desktop Windows only
		// The following has the effect of setting the NT timer resolution (NtSetTimerResolution) to 1 millisecond.
		MMRESULT mmr = timeBeginPeriod(1);
		IOE_ASSERT(TIMERR_NOERROR == mmr);
#endif

#if defined(IOE_BUILD_DEBUG) && defined(IOE_OS_WIN32)
		HMODULE hNtDll = ::LoadLibraryW(L"NtDll.dll");
		if (hNtDll)
		{
			pNtQueryTimerResolution = (NtQueryTimerResolutionType)::GetProcAddress(hNtDll, "NtQueryTimerResolution");
			//pNtSetTimerResolution = (NtSetTimerResolutionType)::GetProcAddress(hNtDll, "NtSetTimerResolution");

			if (pNtQueryTimerResolution)
			{
				ULONG MinimumResolution; // in 100-ns units
				ULONG MaximumResolution;
				ULONG ActualResolution;
				pNtQueryTimerResolution(&MinimumResolution, &MaximumResolution, &ActualResolution);
				IOE_DEBUG_LOG(("NtQueryTimerResolution = Min %ld us, Max %ld us, Current %ld us", MinimumResolution / 10, MaximumResolution / 10, ActualResolution / 10));
			}

			::FreeLibrary(hNtDll);
		}
#endif
	}
}

void PerformanceTimer::Shutdown()
{
	::DeleteCriticalSection(&TimeCS);

	if (!UsingVistaOrLater)
	{
#if defined(IOE_OS_WIN32) // Desktop Windows only
		MMRESULT mmr = timeEndPeriod(1);
		IOE_ASSERT(TIMERR_NOERROR == mmr);
#endif
	}
}


uint64_t PerformanceTimer::GetTimeSeconds()
{
	if (UsingVistaOrLater)
	{
		LARGE_INTEGER li;
		::QueryPerformanceCounter(&li);
		IOE_ASSERT(PerfFrequencyInverse != 0); // Initialize should have been called earlier.
		return (uint64_t)(li.QuadPart * PerfFrequencyInverse);
	}

	return (uint64_t)(GetTimeNanos() * .0000000001);
}


double PerformanceTimer::GetTimeSecondsDouble()
{
	if (UsingVistaOrLater)
	{
		LARGE_INTEGER li;
		::QueryPerformanceCounter(&li);
		IOE_ASSERT(PerfFrequencyInverse != 0);
		return (li.QuadPart * PerfFrequencyInverse);
	}

	return (GetTimeNanos() * .0000000001);
}


uint64_t PerformanceTimer::GetTimeNanos()
{
	uint64_t      resultNanos;
	LARGE_INTEGER li;

	IOE_ASSERT(PerfFrequencyInverseNanos != 0); // Initialize should have been called earlier.

	if (UsingVistaOrLater) // Includes non-desktop platforms
	{
		// Then we can use QPC() directly without all that extra work
		::QueryPerformanceCounter(&li);
		resultNanos = (uint64_t)(li.QuadPart * PerfFrequencyInverseNanos);
		return resultNanos;
	}

	// Pre-Vista computers:
	// Note that the Oculus SDK does not run on PCs before Windows 7 SP1
	// so this code path should never be taken in practice.  We keep it here
	// since this is a nice reusable timing library that can be useful for
	// other projects.

	// On Win32 QueryPerformanceFrequency is unreliable due to SMP and
	// performance levels, so use this logic to detect wrapping and track
	// high bits.
	
	//::EnterCriticalSection(&TimeCS);

	// Get raw value and perf counter "At the same time".
	::QueryPerformanceCounter(&li);

	DWORD mmTimeMs = timeGetTime();
	if (OldMMTimeMs > mmTimeMs)
		MMTimeWrapCounter++;
	OldMMTimeMs = mmTimeMs;

	// Normalize to nanoseconds.
	uint64_t  perfCounterNanos = (uint64_t)(li.QuadPart * PerfFrequencyInverseNanos);
	uint64_t  mmCounterNanos = ((uint64_t(MMTimeWrapCounter) << 32) | mmTimeMs) * 1000000;
	if (PerfMinusTicksDeltaNanos == 0)
		PerfMinusTicksDeltaNanos = perfCounterNanos - mmCounterNanos;

	// Compute result before snapping. 
	//
	// On first call, this evaluates to:
	//          resultNanos = mmCounterNanos.    
	// Next call, assuming no wrap:
	//          resultNanos = prev_mmCounterNanos + (perfCounterNanos - prev_perfCounterNanos).        
	// After wrap, this would be:
	//          resultNanos = snapped(prev_mmCounterNanos +/- 1ms) + (perfCounterNanos - prev_perfCounterNanos).
	//
	resultNanos = perfCounterNanos - PerfMinusTicksDeltaNanos;

	// Snap the range so that resultNanos never moves further apart then its target resolution.
	// It's better to allow more slack on the high side as timeGetTime() may be updated at sporadically 
	// larger then 1 ms intervals even when 1 ms resolution is requested.
	if (resultNanos > (mmCounterNanos + MMTimerResolutionNanos * 2))
	{
		resultNanos = mmCounterNanos + MMTimerResolutionNanos * 2;
		if (resultNanos < LastResultNanos)
			resultNanos = LastResultNanos;
		PerfMinusTicksDeltaNanos = perfCounterNanos - resultNanos;
	}
	else if (resultNanos < (mmCounterNanos - MMTimerResolutionNanos))
	{
		resultNanos = mmCounterNanos - MMTimerResolutionNanos;
		if (resultNanos < LastResultNanos)
			resultNanos = LastResultNanos;
		PerfMinusTicksDeltaNanos = perfCounterNanos - resultNanos;
	}

	LastResultNanos = resultNanos;
	
	//::LeaveCriticalSection(&TimeCS);

	return resultNanos;
}

double Timer::GetSeconds()
{
	return Win32PerfTimer.GetTimeSecondsDouble();
}

uint64_t Timer::GetTicksNanos()
{
	return Win32PerfTimer.GetTimeNanos();
}

double Timer::GetPerfFrequencyInverse()
{
	return Win32PerfTimer.GetFrequencyInverse();
}

void Timer::InitializeTimerSystem()
{
	Win32PerfTimer.Initialize();
}

void Timer::ShutdownTimerSystem()
{
	Win32PerfTimer.Shutdown();
}

#endif