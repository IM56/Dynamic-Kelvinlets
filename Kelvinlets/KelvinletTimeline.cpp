#include "KelvinletTimeline.h"

struct TimelineSort
{
	bool operator()(const Kelvinlet& lhs, const Kelvinlet& rhs) const
	{
		return lhs.startTime < rhs.startTime;
	}
};

void KelvinletTimeline::init()
{}

void KelvinletTimeline::update(float dt)
{
	if (m_ticking)
	{
		// Advance the timeline
		m_timePoint += dt*m_playSpeed;
		if ((m_timePoint < 0.0f) || (m_timePoint > m_endPoint))
		{
			stop();
			return;
		}
		update_kelvinlets();
	}
}

void KelvinletTimeline::play()
{
	m_ticking = true;
}

void KelvinletTimeline::pause()
{
	m_ticking = false;
}

void KelvinletTimeline::stop()
{
	pause();				// Stop ticking
	for (auto it = begin(); it != end(); ++it)
		it->age = 0.0f;
	m_timePoint = 0.0f;		// Go back to the start
}

void KelvinletTimeline::update_kelvinlets()
{
	float S, L;
	// Advance all active Kelvinlets
	for (auto it = m_kelvinlets.begin(); it != m_kelvinlets.end(); ++it)
	{
		// Ignore null Kelvinlets
		if (it->type == 0)
			continue;
		else
		{
			S = it->startTime;
			L = it->lifespan;
			// If the timepoint is in the Kelvinlet's lifetime, determine its age
			if ((m_timePoint > S) && (m_timePoint < (S + L)))
				it->age = m_timePoint - S;
		}
	}
}

void KelvinletTimeline::insert_kelvinlet(Kelvinlet k)
{
	for(auto it = begin(); it != end(); ++it)
	{
		// Find the first available slot and occupy it
		if (it->type == 0)
		{
			*it = k;
			float kEnd = it->startTime + it->lifespan;
			if((kEnd > m_endPoint))
				m_endPoint = kEnd;
			return;
		}
	}
}

void KelvinletTimeline::remove_kelvinlet(Kelvinlet& k)
{
	// Return the Kelvinlet's slot by making it null
	k.type = 0;
	// Find the new end point of the timeline
	float kEnd;
	m_endPoint = 0;
	for (auto it = begin(); it != end(); ++it)
	{
		if (it->type == 0)
			continue;
		kEnd = it->startTime + it->lifespan;
		if (kEnd > m_endPoint)
			m_endPoint = kEnd;
	}
}