#pragma once
#include "Manager.h"
#include "KDisplacement.h"

#include <vector>

class KDisplacementManager : public Manager<KDisplacementManager>
{
public:
	KDisplacementManager(u32 numVs) : m_numVertices(numVs) {}
	KDisplacementManager(const KDisplacementManager&) = delete;
	KDisplacementManager& operator=(KDisplacementManager&) = delete;
	KDisplacementManager(KDisplacementManager&&);
	KDisplacementManager& operator=(KDisplacementManager&&);
	~KDisplacementManager();

	void init(SystemsInterface&);
	void update(SystemsInterface&);
	void release();

	void resize_displacement_buffer(SystemsInterface&, size_t);
	void bind_displacements_SRV_to_VS(ID3D11DeviceContext* pContext, u32 slot) const;
	void bind_displacements_UAV_to_CS(ID3D11DeviceContext* pContext, u32 slot) const;
	void zero_displacement_buffer(ID3D11DeviceContext* pContext);

private:
	void create_displacement_buffer(SystemsInterface&);
	
private:
	u32 m_numVertices;
	std::vector<KDisplacement> m_displacements;
	ID3D11Buffer* m_pDisplacementBuffer = nullptr;
	ID3D11UnorderedAccessView* m_pDisplacementBufferUAV = nullptr;
	ID3D11ShaderResourceView* m_pDisplacementBufferSRV = nullptr;
};