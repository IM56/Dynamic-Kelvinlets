#pragma once

#include "Timer.h"

//////////////////////////////////////////////////////////////////////////
// Common C/C++ headers from the standard
//////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <functional>

//////////////////////////////////////////////////////////////////////////
// Common Windows and directX Headers
//////////////////////////////////////////////////////////////////////////
#ifndef NOIME
	#define NOIME
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wrl.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>

//////////////////////////////////////////////////////////////////////////
// Maths related headers
//////////////////////////////////////////////////////////////////////////
#include <DirectXMath.h>
#include "DirectXTK/SimpleMath.h"

// ComPtr is useful for simplifying release of Com objects.
// see : https://github.com/Microsoft/DirectXTK/wiki/ComPtr

using Microsoft::WRL::ComPtr;

//////////////////////////////////////////////////////////////////////////
// Debug drawing library
//////////////////////////////////////////////////////////////////////////
#define DEBUG_DRAW_EXPLICIT_CONTEXT
#include "debug_draw/debug_draw.hpp"

//////////////////////////////////////////////////////////////////////////
// Imgui library
//////////////////////////////////////////////////////////////////////////
#include "imgui/imgui.h"

//////////////////////////////////////////////////////////////////////////
// Common game industry typedefs
//  * Very compact when used in expressions.
//  * Express the size in bytes.
//////////////////////////////////////////////////////////////////////////

// Unsigned
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

// Signed
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

// Floating point
using f32 = float;
using f64 = double;

// Memory
using memtype_t = u8;
constexpr u64 KB = 1024;
constexpr u64 MB = 1024 * KB;

// Vector maths.
using v2 = DirectX::SimpleMath::Vector2;
using v3 = DirectX::SimpleMath::Vector3;
using v4 = DirectX::SimpleMath::Vector4;
using m4x4 = DirectX::SimpleMath::Matrix;
using m3x3 = DirectX::XMFLOAT3X3;
using quat = DirectX::SimpleMath::Quaternion;

//////////////////////////////////////////////////////////////////////////
// Useful assertion macro
//////////////////////////////////////////////////////////////////////////

#define ASSERT(x) if(!(x)){ __debugbreak(); }

#define SAFE_RELEASE(ptr) if(ptr){ ptr->Release(); }

// ========================================================
// Debug printing functions
// ========================================================

// Prints error to standard error stream.
void errorF(const char * format, ...);

// Printf to message box and abort
void panicF(const char * format, ...);

// Printf to console and debug output.
void debugF(const char * format, ...);


// ========================================================
// Frequently used maths
// ========================================================

constexpr f32 kfPI = 3.1415926535897931f;
constexpr f32 kfHalfPI = 0.5f * kfPI;
constexpr f32 kfTwoPI = 2.0f * kfPI;

// Angle in degrees to angle in radians
constexpr f32 degToRad(const f32 degrees)
{
	return degrees * kfPI / 180.0f;
}

// Angle in radians to angle in degrees
constexpr f32 radToDeg(const f32 radians)
{
	return radians * 180.0f / kfPI;
}

// Random numbers (0, 1) and (-1, 1) for floats and vectors.
inline f32 randf_norm() { return (float)rand() / RAND_MAX; }
inline f32 randf() { return randf_norm() * 2.0f - 1.0f; }
inline v2 randv2() { return v2(randf(),randf()); }
inline v3 randv3() { return v3(randf(), randf(), randf()); }
inline v4 randv4() { return v4(randf(), randf(), randf(), randf()); }


// Helper for packing float3x3 matrices.
// These are tricky because HLSL packs them as 3 * float4 with alignment.
inline void pack_upper_float3x3(const m4x4& m, v4* v)
{
	v[0].x = m._11;
	v[0].y = m._12;
	v[0].z = m._13;

	v[1].x = m._21;
	v[1].y = m._22;
	v[1].z = m._23;

	v[2].x = m._31;
	v[2].y = m._32;
	v[2].z = m._33;
}

//////////////////////
// Camera structure //
//////////////////////

struct Camera
{
	//
	// Camera Axes:
	//
	//    (up)
	//    +Y   +Z (forward)
	//    |   /
	//    |  /
	//    | /
	//    + ------ +X (right)
	//  (eye)
	//
	v3 right;
	v3 up;
	v3 forward;
	v3 eye;
	m4x4 viewMatrix;
	m4x4 projMatrix;
	m4x4 vpMatrix;
	f32 fovY;
	f32 aspect;
	f32 nearClip;
	f32 farClip;

	// Frustum planes for clipping:
	enum { A, B, C, D };
	v4 planes[6];

	// Tunable values:
	float movementSpeed = 10.0f;
	float lookSpeed = 10.0f;

	enum MoveDir
	{
		Forward, // Move forward relative to the camera's space
		Back,    // Move backward relative to the camera's space
		Left,    // Move left relative to the camera's space
		Right    // Move right relative to the camera's space
	};

	Camera();

	void pitch(const float angle);

	void rotate(const float angle);

	void move(const MoveDir dir, const float amount);

	void checkKeyboardMovement();

	void checkMouseRotation();

	void resizeViewport(u32 width, u32 height);

	void updateMatrices();

	void look_at(const v3& vTarget);

	v3 getTarget() const;

	bool pointInFrustum(const v3& v) const;

	static v3 rotateAroundAxis(const v3 & vec, const v3 & axis, const float angle);
};

// ========================================================
// The SystemsInterface provide access to
// systems and device contexts.
// ========================================================
struct SystemsInterface
{
	ID3D11Device* pD3DDevice;
	ID3D11DeviceContext* pD3DContext;
	ID3D11RenderTargetView* pSwapRenderTarget; // 
	dd::ContextHandle pDebugDrawContext;
	Camera* pCamera;
	u32 width;
	u32 height;
};

//================================================================================
// Resource Creation Helpers
//================================================================================


// template to create a constant buffer from CPU structure.
template<typename ConstantBufferType>
ID3D11Buffer* create_constant_buffer(ID3D11Device* pDevice)
{
	ID3D11Buffer* pBuffer = nullptr;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(ConstantBufferType);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	HRESULT hr = pDevice->CreateBuffer(&desc, NULL, &pBuffer);
	ASSERT(!FAILED(hr) && pBuffer);

	return pBuffer;
}

// template to update entire constant buffer from CPU structure.
template<typename ConstantBufferType>
void update_constant_buffer(ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer, const ConstantBufferType& rData)
{
	D3D11_MAPPED_SUBRESOURCE subresource;
	if (!FAILED(pContext->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource)))
	{
		memcpy(subresource.pData, &rData, sizeof(ConstantBufferType));
		pContext->Unmap(pBuffer, 0);
	}
}

// template to create a structure buffer from CPU structure.
template<typename Element>
ID3D11Buffer* create_dynamic_structured_buffer(ID3D11Device* pDevice, const u32 elements, D3D11_SUBRESOURCE_DATA* pData)
{
	ID3D11Buffer* pBuffer = nullptr;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(Element) * elements;
	desc.StructureByteStride = sizeof(Element);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	HRESULT hr = pDevice->CreateBuffer(&desc, pData, &pBuffer);
	ASSERT(!FAILED(hr) && pBuffer);

	return pBuffer;
}

template <typename Element>
void update_dynamic_structured_buffer(ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer, Element* pNewArray, u32 numElements)
{
	// Create new mapped subresource
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	ZeroMemory(&mappedSubresource, sizeof(D3D11_MAPPED_SUBRESOURCE));

	// Disable GPU access to the structured buffer
	if (!FAILED(pContext->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource)))
	{
		// Copy the new data into the mapped subresource
		memcpy(mappedSubresource.pData, pNewArray, numElements * sizeof(Element));
		// Enable GPU access to the updated structured buffer
		pContext->Unmap(pBuffer, 0);
	}
}

template <typename Element>
void zero_dynamic_structured_buffer(ID3D11DeviceContext* pContext, ID3D11Buffer* pBuffer, const u32 numElements)
{
	Element* pEmptyArray = static_cast<Element*>(std::calloc(numElements, sizeof(Element)));
	update_dynamic_structured_buffer<Element>(pContext, pBuffer, pEmptyArray, numElements);
	std::free(pEmptyArray);
}

template<typename Element>
ID3D11Buffer* create_default_structured_buffer(ID3D11Device* pDevice, const u32 numElements, D3D11_SUBRESOURCE_DATA* pData)
{
	ID3D11Buffer* pBuffer = nullptr;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(Element) * numElements;
	desc.StructureByteStride = sizeof(Element);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	HRESULT hr = pDevice->CreateBuffer(&desc, pData, &pBuffer);
	ASSERT(!FAILED(hr) && pBuffer);

	return pBuffer;
}

inline ID3D11ShaderResourceView* create_structured_buffer_SRV(ID3D11Device* pDevice, const u32 numElements, ID3D11Buffer* pBuffer)
{
	ID3D11ShaderResourceView* pView = nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	desc.Buffer.ElementOffset = 0;
	desc.Buffer.NumElements = numElements;

	HRESULT hr = pDevice->CreateShaderResourceView(pBuffer, &desc, &pView);
	ASSERT(!FAILED(hr) && pView);
	return pView;
}


inline ID3D11UnorderedAccessView* create_structured_buffer_UAV(ID3D11Device* pDevice, const u32 numElements, ID3D11Buffer* pBuffer)
{
	ID3D11UnorderedAccessView* pBuffer_UAV = nullptr;

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = 0;
	desc.Buffer.NumElements = numElements;
	desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;

	HRESULT hr = pDevice->CreateUnorderedAccessView(pBuffer, &desc, &pBuffer_UAV);
	ASSERT(!FAILED(hr) && pBuffer_UAV);

	return pBuffer_UAV;
}


// helper to create a sampler state
inline ID3D11SamplerState* create_basic_sampler(ID3D11Device* pDevice, D3D11_TEXTURE_ADDRESS_MODE mode)
{
	ID3D11SamplerState* pSampler = nullptr;

	D3D11_SAMPLER_DESC desc = {};
	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU = mode;
	desc.AddressV = mode;
	desc.AddressW = mode;
	desc.MinLOD = 0.f;
	desc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = pDevice->CreateSamplerState(&desc, &pSampler);
	ASSERT(!FAILED(hr) && pSampler);

	return pSampler;
}