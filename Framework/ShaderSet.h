#pragma once

// ========================================================
// Shader stage enum
// ========================================================
namespace ShaderStage
{
	enum ShaderStageEnum
	{
		kVertex,
		kHull,
		kDomain,
		kGeometry,
		kPixel,
		kCompute,

		kMaxStages
	};
}

// ========================================================
// ShaderSet
// ========================================================

// Describes the entry points for a given set of shaders
// Fill in the filename then multiple entry points.
struct ShaderSetDesc
{
	const char* filename;
	const char* entryPoints[ShaderStage::kMaxStages];

	static ShaderSetDesc Create_VS_PS(const char* fName, const char* vsEntry, const char* psEntry)
	{
		ShaderSetDesc desc = {};
		desc.filename = fName;
		desc.entryPoints[ShaderStage::kVertex] = vsEntry;
		desc.entryPoints[ShaderStage::kPixel] = psEntry;
		return desc;
	}

	static ShaderSetDesc Create_CS(const char* fName, const char* csEntry)
	{
		ShaderSetDesc desc = {};
		desc.filename = fName;
		desc.entryPoints[ShaderStage::kCompute] = csEntry;
		return desc;
	}
};

struct ShaderSet
{
	using InputLayoutDesc = std::tuple<const D3D11_INPUT_ELEMENT_DESC *, int>;
	ShaderSet();
	void init(ID3D11Device* device, const ShaderSetDesc& desc, const InputLayoutDesc & layout, bool layoutEnabled);

	void bind(ID3D11DeviceContext* pContext) const;

	ComPtr<ID3D11InputLayout>  inputLayout;
	ComPtr<ID3D11VertexShader> vs;
	ComPtr<ID3D11HullShader> hs;
	ComPtr<ID3D11DomainShader> ds;
	ComPtr<ID3D11GeometryShader> gs;
	ComPtr<ID3D11PixelShader>  ps;
	ComPtr<ID3D11ComputeShader>  cs;
};


