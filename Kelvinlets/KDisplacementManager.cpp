#include "KDisplacementManager.h"

KDisplacementManager::KDisplacementManager(KDisplacementManager&& other) :
	m_displacements(other.m_displacements),
	m_pDisplacementBuffer(nullptr),
	m_pDisplacementBufferSRV(nullptr),
	m_pDisplacementBufferUAV(nullptr)
{
	m_pDisplacementBuffer = other.m_pDisplacementBuffer;
	m_pDisplacementBufferSRV = other.m_pDisplacementBufferSRV;
	m_pDisplacementBufferUAV = other.m_pDisplacementBufferUAV;
	other.m_pDisplacementBuffer = nullptr;
	other.m_pDisplacementBufferSRV = nullptr;
	other.m_pDisplacementBufferUAV = nullptr;
}

KDisplacementManager& KDisplacementManager::operator=(KDisplacementManager&& other)
{
	if (this != &other)
	{
		release();
		m_displacements = other.m_displacements;
		m_pDisplacementBuffer = other.m_pDisplacementBuffer;
		m_pDisplacementBufferSRV = other.m_pDisplacementBufferSRV;
		m_pDisplacementBufferUAV = other.m_pDisplacementBufferUAV;
		other.m_pDisplacementBuffer = nullptr;
		other.m_pDisplacementBufferSRV = nullptr;
		other.m_pDisplacementBufferUAV = nullptr;
	}

	return *this;
}

KDisplacementManager::~KDisplacementManager()
{
	release();
}

void KDisplacementManager::init(SystemsInterface& systems)
{
	// Fill displacement buffer with zeros
	KDisplacement kd;
	for (size_t i = 0; i < m_numVertices; ++i)
		m_displacements.push_back(kd);
	create_displacement_buffer(systems);
}

void KDisplacementManager::update(SystemsInterface& systems)
{
}

void KDisplacementManager::release()
{
	SAFE_RELEASE(m_pDisplacementBuffer);
	SAFE_RELEASE(m_pDisplacementBufferSRV);
	SAFE_RELEASE(m_pDisplacementBufferUAV);
}

void KDisplacementManager::zero_displacement_buffer(ID3D11DeviceContext* pContext)
{
	zero_dynamic_structured_buffer<KDisplacement>(pContext, m_pDisplacementBuffer, m_displacements.size());
}

void KDisplacementManager::resize_displacement_buffer(SystemsInterface& systems, size_t numVertices)
{
	// Resize displacement buffers to accomodate a new mesh
	m_displacements.resize(numVertices);
	release();
	create_displacement_buffer(systems);
}

void KDisplacementManager::create_displacement_buffer(SystemsInterface& systems)
{
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = m_displacements.data();
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;

	m_pDisplacementBuffer =
		create_default_structured_buffer<KDisplacement>(systems.pD3DDevice, m_displacements.size(), &data);
	m_pDisplacementBufferSRV =
		create_structured_buffer_SRV(systems.pD3DDevice, m_displacements.size(), m_pDisplacementBuffer);
	m_pDisplacementBufferUAV =
		create_structured_buffer_UAV(systems.pD3DDevice, m_displacements.size(), m_pDisplacementBuffer);
}

void KDisplacementManager::bind_displacements_SRV_to_VS(ID3D11DeviceContext* pContext, u32 slot) const
{
	pContext->VSSetShaderResources(slot, 1, &m_pDisplacementBufferSRV);
}

void KDisplacementManager::bind_displacements_UAV_to_CS(ID3D11DeviceContext* pContext, u32 slot) const
{
	pContext->CSSetUnorderedAccessViews(slot, 1, &m_pDisplacementBufferUAV, nullptr);
}