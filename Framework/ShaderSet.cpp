#include "CommonHeader.h"
#include "ShaderSet.h"

#include <d3dcompiler.h>


// ========================================================
// ShaderSet
// ========================================================


void compileShaderFromFile(const char * fileName, const char * entryPoint, const char * shaderModel, ID3DBlob ** ppBlobOut)
{
	UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows
	// the shaders to be optimized and to run exactly the way they will run in
	// the release configuration.
#if defined(DEBUG) || defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
#endif // DEBUG

	wchar_t fileNameW[MAX_PATH];
	size_t numChars;
	mbstowcs_s(&numChars, fileNameW, MAX_PATH, fileName, MAX_PATH);

	D3D_SHADER_MACRO macros[] = { {NULL, NULL} };

	ComPtr<ID3DBlob> pErrorBlob;
	HRESULT hr = D3DCompileFromFile(fileNameW, macros, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, shaderModel,
		shaderFlags, 0, ppBlobOut, pErrorBlob.GetAddressOf());
	if (FAILED(hr))
	{
		auto * details = (pErrorBlob ? static_cast<const char *>(pErrorBlob->GetBufferPointer()) : "<no info>");
		panicF("Failed to compile shader '%s'!\nError info:\n%s", fileName, details);
	}
}


ShaderSet::ShaderSet()
{
}

void ShaderSet::init(ID3D11Device* device, const ShaderSetDesc& desc, const InputLayoutDesc & layout, bool layoutEnabled)
{
	ComPtr<ID3DBlob> blobs[ShaderStage::kMaxStages];
	static const char* profiles[ShaderStage::kMaxStages] = { "vs_5_0", "hs_5_0" ,"ds_5_0" ,"gs_5_0" ,"ps_5_0" ,"cs_5_0" };

	// Compile each stage we set an entry point for.
	for (u32 i = 0; i < ShaderStage::kMaxStages; ++i)
	{
		if (desc.entryPoints[i])
			compileShaderFromFile(desc.filename, desc.entryPoints[i], profiles[i], blobs[i].GetAddressOf());
	}

	// check we have either (compute) or (vertex + pixel)
	ASSERT(
		(blobs[ShaderStage::kCompute] != nullptr)
		|| (blobs[ShaderStage::kVertex] != nullptr && blobs[ShaderStage::kPixel] != nullptr)
	);

	HRESULT hr;

	// Create the vertex shader:
	if (blobs[ShaderStage::kVertex])
	{
		hr = device->CreateVertexShader(blobs[ShaderStage::kVertex]->GetBufferPointer(), blobs[ShaderStage::kVertex]->GetBufferSize(), nullptr, vs.GetAddressOf());
		if (FAILED(hr))
		{
			panicF("Failed to create vertex shader");
		}
	}

	// Create the hull shader:
	if (blobs[ShaderStage::kHull])
	{
		hr = device->CreateHullShader(blobs[ShaderStage::kHull]->GetBufferPointer(), blobs[ShaderStage::kHull]->GetBufferSize(), nullptr, hs.GetAddressOf());
		if (FAILED(hr))
		{
			panicF("Failed to create hull shader");
		}
	}

	// Create the domain shader:
	if (blobs[ShaderStage::kDomain])
	{
		hr = device->CreateDomainShader(blobs[ShaderStage::kDomain]->GetBufferPointer(), blobs[ShaderStage::kDomain]->GetBufferSize(), nullptr, ds.GetAddressOf());
		if (FAILED(hr))
		{
			panicF("Failed to create hull shader");
		}
	}

	// Create the geometry shader:
	if (blobs[ShaderStage::kGeometry])
	{
		hr = device->CreateGeometryShader(blobs[ShaderStage::kGeometry]->GetBufferPointer(), blobs[ShaderStage::kGeometry]->GetBufferSize(), nullptr, gs.GetAddressOf());
		if (FAILED(hr))
		{
			panicF("Failed to create geometry shader");
		}
	}

	// Create the pixel shader:
	if (blobs[ShaderStage::kPixel])
	{
		hr = device->CreatePixelShader(blobs[ShaderStage::kPixel]->GetBufferPointer(), blobs[ShaderStage::kPixel]->GetBufferSize(), nullptr, ps.GetAddressOf());
		if (FAILED(hr))
		{
			panicF("Failed to create pixel shader");
		}
	}

	// Create the compute shader:
	if (blobs[ShaderStage::kCompute])
	{
		hr = device->CreateComputeShader(blobs[ShaderStage::kCompute]->GetBufferPointer(), blobs[ShaderStage::kCompute]->GetBufferSize(), nullptr, cs.GetAddressOf());
		if (FAILED(hr))
		{
			panicF("Failed to create compute shader");
		}
	}

	if (layoutEnabled)
	{
		// Create vertex input layout:
		hr = device->CreateInputLayout(std::get<0>(layout), std::get<1>(layout),
			blobs[ShaderStage::kVertex]->GetBufferPointer(),
			blobs[ShaderStage::kVertex]->GetBufferSize(),
			inputLayout.GetAddressOf());
		if (FAILED(hr))
		{
			panicF("Failed to create vertex layout!");
		}
	}
}

void ShaderSet::bind(ID3D11DeviceContext* pContext) const
{

	// otherwise make sure we bind vs and ps
	// all others are optional.

	// Vertex
	if (vs)
	{
		pContext->IASetInputLayout(inputLayout.Get());
		pContext->VSSetShader(vs.Get(), NULL, 0);
	}
	else
	{
		pContext->IASetInputLayout(NULL);
		pContext->VSSetShader(NULL, NULL, 0);
	}

	// Hull Shader
	if (hs)
	{
		pContext->HSSetShader(hs.Get(), NULL, 0);
	}
	else
	{
		pContext->HSSetShader(NULL, NULL, 0);
	}

	// Domain Shader
	if (ds)
	{
		pContext->DSSetShader(ds.Get(), NULL, 0);
	}
	else
	{
		pContext->DSSetShader(NULL, NULL, 0);
	}

	// Geometry Shader
	if (gs)
	{
		pContext->GSSetShader(gs.Get(), NULL, 0);
	}
	else
	{
		pContext->GSSetShader(NULL, NULL, 0);
	}


	// Geometry Shader
	if (ps)
	{
		pContext->PSSetShader(ps.Get(), NULL, 0);
	}
	else
	{
		pContext->PSSetShader(NULL, NULL, 0);
	}

	// Compute Shader
	if (cs)
	{
		pContext->CSSetShader(cs.Get(), NULL, 0);
	}
	else
	{
		pContext->CSSetShader(NULL, NULL, 0);
	}

}

