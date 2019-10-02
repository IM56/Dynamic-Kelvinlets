#include "Texture.h"
#include "DirectXTK/DDSTextureLoader.h"
#include "DirectXTK/WICTextureLoader.h"

Texture::Texture()
	: m_pTexture(nullptr)
	, m_pTextureView(nullptr)
{

}

Texture::~Texture()
{
	SAFE_RELEASE(m_pTextureView);
	SAFE_RELEASE(m_pTexture);
}

void Texture::init_from_dds(ID3D11Device* pDevice, const char* pFilename)
{
	wchar_t fileNameW[MAX_PATH];
	size_t numChars;
	mbstowcs_s(&numChars, fileNameW, MAX_PATH, pFilename, MAX_PATH);

	HRESULT hr = DirectX::CreateDDSTextureFromFile(pDevice, fileNameW, &m_pTexture, &m_pTextureView);
	if (FAILED(hr))
	{
		panicF("Could not load texture : %s ", pFilename);
	}
}

void Texture::init_from_image(ID3D11Device* pDevice, const char* pFilename, bool bGenerateMips)
{
	wchar_t fileNameW[MAX_PATH];
	size_t numChars;
	mbstowcs_s(&numChars, fileNameW, MAX_PATH, pFilename, MAX_PATH);

	HRESULT hr = DirectX::CreateWICTextureFromFile(pDevice, fileNameW, &m_pTexture, &m_pTextureView);
	if (FAILED(hr))
	{
		panicF("Could not load texture : %s ", pFilename);
	}
}

void Texture::bind(ID3D11DeviceContext* pDeviceContext, ShaderStage::ShaderStageEnum stage, u32 slot) const
{
	// This is not very efficient.
	// We could bind multiple in a single call.
	// An opportunity for you to change ;)

	switch (stage)
	{
	case ShaderStage::kVertex:
		pDeviceContext->VSSetShaderResources(slot, 1, &m_pTextureView);
		break;
	case ShaderStage::kHull:
		pDeviceContext->HSSetShaderResources(slot, 1, &m_pTextureView);
		break;
	case ShaderStage::kDomain:
		pDeviceContext->DSSetShaderResources(slot, 1, &m_pTextureView);
		break;
	case ShaderStage::kGeometry:
		pDeviceContext->GSSetShaderResources(slot, 1, &m_pTextureView);
		break;
	case ShaderStage::kPixel:
		pDeviceContext->PSSetShaderResources(slot, 1, &m_pTextureView);
		break;
	case ShaderStage::kCompute:
		pDeviceContext->CSSetShaderResources(slot, 1, &m_pTextureView);
		break;
	}
}
