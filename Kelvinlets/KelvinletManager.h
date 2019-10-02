#pragma once

#include "Manager.h"
#include "KelvinletTimeline.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
// KelvinletManager : public Manager			
//
// Responsibility : Manages Kelvinlets for a mesh instance. It has a timeline which schedules Kelvinlets
//                  and contains the material properties alpha (P-wave speed) and beta (S-wave speed).

class KelvinletManager : public Manager<KelvinletManager>
{
public:
	KelvinletManager(const u32 maxNum) : m_maxKelvinlets(maxNum), m_timeline(maxNum) {}
	KelvinletManager(const KelvinletManager&) = delete;
	KelvinletManager& operator=(const KelvinletManager&) = delete;
	KelvinletManager(KelvinletManager&&);					// Move constructor
	KelvinletManager& operator=(KelvinletManager&&);		// Move assignment operator
	~KelvinletManager();

	void init(SystemsInterface&);
	void update(SystemsInterface&, float);
	void release();

	void play();
	void pause();
	void stop();

	const float get_alpha() const { return m_alpha; }
	const float get_beta() const { return m_beta; }
	float* get_alpha_ptr() { return &m_alpha; }
	float* get_beta_ptr() { return &m_beta; }
	const u32 get_num_kelvinlets() const { return m_maxKelvinlets; }
	KelvinletTimeline& get_timeline() { return m_timeline; }

	void bind_kelvinlet_data_SRV_to_CS(ID3D11DeviceContext*, u32) const;

private:
	u32 m_maxKelvinlets = 10;	// Maximum number allowed in the timeline
	KelvinletTimeline m_timeline;
	float m_alpha = 2.0f;
	float m_beta = 1.3f;

	ID3D11Buffer* m_pKelvinletBuffer = nullptr;
	ID3D11ShaderResourceView* m_pKelvinletBufferSRV = nullptr;
};