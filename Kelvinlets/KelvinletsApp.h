#pragma once
#include "Framework.h"
#include "MeshManager.h"
#include "MeshInstanceManager.h"
#include "ShaderSet.h"
#include "Texture.h"

#include <deque>

class KelvinletsApp : public FrameworkApp
{
public:
	struct PerFrameCBData	// Used in VS/PS shader every frame
	{
		m4x4 m_matProjection;
		m4x4 m_matView;
		m4x4 m_matProjInv;
		m4x4 m_matViewInv;
		v3 camPos;
		f32	deltaTime;		// deltaTime is frame time in seconds
		f32 elapsedTime;
		bool normalCorrection;
		v2 padding;
	};

	struct PerInstanceCBData	// Used in both CS and VS/PS shaders
	{
		m4x4 m_matModel;		// Model matrix for mesh instance
		u32 numVertices;		// Number of vertices in this instance's mesh
		u32 numKelvinlets;	    // Number of kelvinlets attached to instance
		f32 alpha;				// Material parameter : pressure wave speed
		f32 beta;				// Material parameter : shear wave speed
	};

	struct PointLight
	{
		v3 lightPos;
		f32 power;
		v3 lightCol;
		f32 angularFrequency;
	};

	struct VertexDisplacement
	{
		v3 vertexDisplacement;
		v3 neighbourDisplacement1;
		v3 neighbourDisplacement2;
	};

	~KelvinletsApp();

	void on_init(SystemsInterface& systems) override;
	void on_update(SystemsInterface& systems) override;
	void on_render(SystemsInterface& systems) override;
	void on_release();
	void on_resize(SystemsInterface& systems) override
	{
		// Need this to flush memory leaks on Intel Integrated GPUs
		systems.pD3DContext->Flush();
	}

private:
	void init_camera(SystemsInterface& systems);
	void init_shaders(SystemsInterface& systems);
	void calculate_kelvinlet_displacements(SystemsInterface& systems);
	void extract_per_instance_data(MeshInstance&);
	
private:
	MeshManager m_meshManager;
	MeshInstanceManager m_meshInstanceManager;
	ShaderSet m_kelvinletShader;	// Compute shader for calculating Kelvinlet displacements
	ShaderSet m_renderShader;		// VS/PS pair to render each mesh
	Texture m_texture;
	Timer m_timer;

	ID3D11SamplerState* m_pLinearMipSamplerState = nullptr;
	PerFrameCBData m_perFrameCBData;
	ID3D11Buffer* m_pPerFrameCB = nullptr;
	PerInstanceCBData m_perInstanceCBData;
	ID3D11Buffer* m_pPerInstanceCB = nullptr;
	PointLight m_pointLightCBData;
	ID3D11Buffer* m_pPointLightCB = nullptr;

	int m_editorMode = 0;		// 0 = EDIT, 1 = PLAY
	bool m_ticking = false;
	bool m_correctNormals = true;

	float m_elapsedTime = 0.0f;	// Total running time in seconds
	float m_frameTime;
};
