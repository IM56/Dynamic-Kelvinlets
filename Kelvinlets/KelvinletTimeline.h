#pragma once
#include "Kelvinlet.h"

#include <vector>
#include <set>

class KelvinletTimeline
{
public:
	KelvinletTimeline(const u32 max_ks) : m_maxKelvinlets(max_ks), m_kelvinlets(max_ks)
	{}
	~KelvinletTimeline() 
	{}

	const u32 get_max_num_kelvinlets() const { return m_maxKelvinlets; }
	const Kelvinlet* get_kelvinlet_array() const { return m_kelvinlets.data(); }
	const float get_endpoint() const { return m_endPoint; }
	float* get_play_speed_ptr() { return &m_playSpeed; }
	float* get_timepoint_ptr() { return &m_timePoint; }

	std::vector<Kelvinlet>::iterator begin() { return m_kelvinlets.begin(); }
	std::vector<Kelvinlet>::const_iterator cbegin() const { return m_kelvinlets.cbegin(); }
	std::vector<Kelvinlet>::iterator end() { return m_kelvinlets.end(); }
	std::vector<Kelvinlet>::const_iterator cend() const { return m_kelvinlets.cend(); }

	void init();
	void update(float);
	void play();
	void pause();
	void stop();

	void insert_kelvinlet(Kelvinlet k);
	void remove_kelvinlet(Kelvinlet& k);

private:
	void update_kelvinlets();	// Updates Kelvinlets

private:
	u32 m_maxKelvinlets;
	std::vector<Kelvinlet> m_kelvinlets;	// Kelvinlet data for updating and sending to GPU
	u32 m_currentKIndex = 0;	// Tracks along which Kelvinlets to update
	float m_timePoint = 0.0f;	// Current point along the timeline
	float m_endPoint = 0.0f;	// End of the timeline
	float m_playSpeed = 0.005f;	// Time increment every frame
	bool m_ticking = false;		// Is the timeline ticking?
};