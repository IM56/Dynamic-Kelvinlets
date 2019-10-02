#pragma once

#include "CommonHeader.h"
#include "ShaderSet.h"

class Texture
{

public:

	Texture();
	~Texture();

	// Initialize from a DDS file.
	void init_from_dds(ID3D11Device* pDevice, const char* pFilename);

	// Initialize from a non-dds image files such as JPEG, or PNG
	void init_from_image(ID3D11Device* pDevice, const char* pFilename, bool bGenerateMips);

	// bind to the pipeline on a particular shader and slot
	void bind(ID3D11DeviceContext* pDeviceContext, ShaderStage::ShaderStageEnum stage, u32 slot) const;

private:
	ID3D11Resource* m_pTexture;
	ID3D11ShaderResourceView* m_pTextureView;
};

