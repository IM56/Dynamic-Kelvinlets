#include "Timer.h"


Timer::Timer()
{
	m_startPoint.QuadPart = 0;
	m_stopPoint.QuadPart = 0;
	QueryPerformanceFrequency(&m_frequency);
}

Timer::~Timer()
{}

void Timer::Start()
{
	QueryPerformanceCounter(&m_startPoint);
}

void Timer::Stop()
{
	QueryPerformanceCounter(&m_stopPoint);
}

void Timer::Reset()
{
	m_startPoint.QuadPart = 0;
	m_stopPoint.QuadPart = 0;
}

double Timer::toMicroseconds(LARGE_INTEGER& L) const
{
	return (static_cast<double>(L.QuadPart * 1000000) / static_cast<double>(m_frequency.QuadPart));
}

double Timer::ElapsedMicroseconds() const
{
	LARGE_INTEGER time;
	time.QuadPart = m_stopPoint.QuadPart - m_startPoint.QuadPart;
	return toMicroseconds(time);
}