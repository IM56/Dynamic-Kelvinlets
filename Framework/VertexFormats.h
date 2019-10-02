#pragma once

//////////////////////////////////////////////////////////////////////
// Vertex Formats.
//////////////////////////////////////////////////////////////////////

using VertexColour = u32;

//////////////////////////////////////////////////////////////////////////
// vertex descriptors are described using type traits.
// redefine the following for each vertex type.
// see examples for further info.
//////////////////////////////////////////////////////////////////////////

template <typename T> struct VertexFormatTraits {};

//////////////////////////////////////////////////////////////////////////
// Position and Colour
//////////////////////////////////////////////////////////////////////////
struct Vertex_Pos3fColour4ub
{
	DirectX::XMFLOAT3 pos;
	VertexColour colour;

	Vertex_Pos3fColour4ub();
	Vertex_Pos3fColour4ub(const DirectX::XMFLOAT3 &pos, VertexColour colour);
};

template <> struct VertexFormatTraits<Vertex_Pos3fColour4ub> {
	static const D3D11_INPUT_ELEMENT_DESC desc[];
	static const u32 size = 2;
};

//////////////////////////////////////////////////////////////////////////
// Position, Colour and Texture Coordinate
//////////////////////////////////////////////////////////////////////////
struct Vertex_Pos3fTex2fColour4ub
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 tex;
	VertexColour colour;

	Vertex_Pos3fTex2fColour4ub();
	Vertex_Pos3fTex2fColour4ub(const DirectX::XMFLOAT3 &pos, const DirectX::XMFLOAT2 &tex, VertexColour colour);
};

template <> struct VertexFormatTraits<Vertex_Pos3fTex2fColour4ub> {
	static const D3D11_INPUT_ELEMENT_DESC desc[];
	static const u32 size = 3;
};

//////////////////////////////////////////////////////////////////////////
// Position, Colour and Normal
//////////////////////////////////////////////////////////////////////////
struct Vertex_Pos3fColour4ubNormal3f
{
	DirectX::XMFLOAT3 pos;
	VertexColour colour;
	DirectX::XMFLOAT3 normal;

	Vertex_Pos3fColour4ubNormal3f();
	Vertex_Pos3fColour4ubNormal3f(const DirectX::XMFLOAT3 &pos, VertexColour colour, const DirectX::XMFLOAT3 &normal);
};

template <> struct VertexFormatTraits<Vertex_Pos3fColour4ubNormal3f> {
	static const D3D11_INPUT_ELEMENT_DESC desc[];
	static const u32 size = 3;
};

//////////////////////////////////////////////////////////////////////////
// Position, Colour, Normal and Texture 
//////////////////////////////////////////////////////////////////////////
struct Vertex_Pos3fColour4ubNormal3fTex2f
{
	DirectX::XMFLOAT3 pos;
	VertexColour colour;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 tex;

	Vertex_Pos3fColour4ubNormal3fTex2f();
	Vertex_Pos3fColour4ubNormal3fTex2f(const DirectX::XMFLOAT3 &pos, VertexColour colour, const DirectX::XMFLOAT3 &normal, const DirectX::XMFLOAT2 &tex);
};

template <> struct VertexFormatTraits<Vertex_Pos3fColour4ubNormal3fTex2f> {
	static const D3D11_INPUT_ELEMENT_DESC desc[];
	static const u32 size = 4;
};

//////////////////////////////////////////////////////////////////////////
// Position, Colour, Normal, Tangent (+sign) and Texture 
//////////////////////////////////////////////////////////////////////////
struct Vertex_Pos3fColour4ubNormal3fTangent3fTex2f
{
	DirectX::XMFLOAT3 pos;
	VertexColour colour;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT4 tangent;
	DirectX::XMFLOAT2 tex;

	Vertex_Pos3fColour4ubNormal3fTangent3fTex2f();
	Vertex_Pos3fColour4ubNormal3fTangent3fTex2f(const DirectX::XMFLOAT3 &pos, VertexColour colour, const DirectX::XMFLOAT3 &normal, const DirectX::XMFLOAT2 &tex);
	Vertex_Pos3fColour4ubNormal3fTangent3fTex2f(const DirectX::XMFLOAT3 &pos, VertexColour colour, const DirectX::XMFLOAT3 &normal, const DirectX::XMFLOAT4 &tangent, const DirectX::XMFLOAT2 &tex);
};

template <> struct VertexFormatTraits<Vertex_Pos3fColour4ubNormal3fTangent3fTex2f> {
	static const D3D11_INPUT_ELEMENT_DESC desc[];
	static const u32 size = 5;
};