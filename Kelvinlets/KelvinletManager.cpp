#include "KelvinletManager.h"

// Move constructor
KelvinletManager::KelvinletManager(KelvinletManager&& kmOther) :
	m_timeline(kmOther.m_timeline),
	m_pKelvinletBuffer(nullptr),
	m_pKelvinletBufferSRV(nullptr)
{
	// Transfer ownership of resources
	m_pKelvinletBuffer = kmOther.m_pKelvinletBuffer;
	m_pKelvinletBufferSRV = kmOther.m_pKelvinletBufferSRV;
	// Prevent resources being released
	kmOther.m_pKelvinletBuffer = nullptr;
	kmOther.m_pKelvinletBufferSRV = nullptr;
}

// Move assignment operator
KelvinletManager& KelvinletManager::operator=(KelvinletManager&& kmOther)
{
	// Only perform operations if we are assigning a different object
	if (this != &kmOther)
	{
		// Release this object's old resources
		release();
		// Copy the other object into this one
		m_maxKelvinlets = kmOther.m_maxKelvinlets;
		m_alpha = kmOther.m_alpha;
		m_beta = kmOther.m_beta;
		m_pKelvinletBuffer = kmOther.m_pKelvinletBuffer;
		m_pKelvinletBufferSRV = kmOther.m_pKelvinletBufferSRV;
		m_timeline = std::move(kmOther.m_timeline);
		// Prevent multiple attempts to release resources
		kmOther.m_pKelvinletBuffer = nullptr;
		kmOther.m_pKelvinletBufferSRV = nullptr;
	}

	return *this;
}

KelvinletManager::~KelvinletManager()
{
	release();
}

void KelvinletManager::init(SystemsInterface& systems)
{
	m_timeline.init();

	D3D11_SUBRESOURCE_DATA kData;
	kData.pSysMem = m_timeline.get_kelvinlet_array();
	kData.SysMemPitch = 0;
	kData.SysMemSlicePitch = 0;

	m_pKelvinletBuffer = create_dynamic_structured_buffer<Kelvinlet>(systems.pD3DDevice, m_maxKelvinlets, &kData);
	m_pKelvinletBufferSRV = create_structured_buffer_SRV(systems.pD3DDevice, m_maxKelvinlets, m_pKelvinletBuffer);
}

// Called every frame to update Kelvinlets
void KelvinletManager::update(SystemsInterface& systems, float dt)
{
	m_timeline.update(dt);
	update_dynamic_structured_buffer(systems.pD3DContext, m_pKelvinletBuffer, m_timeline.get_kelvinlet_array(), m_maxKelvinlets);
}

void KelvinletManager::release()
{
	SAFE_RELEASE(m_pKelvinletBuffer);
	SAFE_RELEASE(m_pKelvinletBufferSRV);
}

void KelvinletManager::play()
{
	m_timeline.play();
}

void KelvinletManager::pause()
{
	m_timeline.pause();
}

void KelvinletManager::stop()
{
	m_timeline.stop();
}

void KelvinletManager::bind_kelvinlet_data_SRV_to_CS(ID3D11DeviceContext* pContext, u32 slot) const
{
	ID3D11ShaderResourceView* pSRV[] = { m_pKelvinletBufferSRV };
	pContext->CSSetShaderResources(slot, 1, pSRV);
}