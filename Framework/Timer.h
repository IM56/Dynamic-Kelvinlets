#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

class Timer
{
public:
	Timer();
	~Timer();
	void Start();
	void Stop();
	void Reset();
	double ElapsedMicroseconds() const;

private:
	LARGE_INTEGER m_startPoint;
	LARGE_INTEGER m_stopPoint;
	LARGE_INTEGER m_frequency;

	double toMicroseconds(LARGE_INTEGER &L) const;
};


