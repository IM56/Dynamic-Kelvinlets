#include "KelvinletsApp.h"

// Helper function for dispatching compute shader threads
s32 align(s32 num, s32 alignment)
{
	return (num + (alignment - 1)) & ~(alignment - 1);
}

KelvinletsApp::~KelvinletsApp()
{
	on_release();
}

void KelvinletsApp::on_init(SystemsInterface& systems)
{
	init_camera(systems);
	// Create Per Frame Constant Buffer
	m_pPerFrameCB = create_constant_buffer<PerFrameCBData>(systems.pD3DDevice);
	// Create Per Frame Constant Buffer
	m_pPerInstanceCB = create_constant_buffer<PerInstanceCBData>(systems.pD3DDevice);
	// Create Point Light Constant Buffer
	m_pPointLightCB = create_constant_buffer<PointLight>(systems.pD3DDevice);
	// Compile shaders
	init_shaders(systems);
	// Load meshes from file or otherwise
	m_meshManager.init(systems);
	// Load textures from file or otherwise
	m_texture.init_from_dds(systems.pD3DDevice, "Assets/Textures/brick.dds");
	// Create a mesh instance
	//Mesh& mesh = m_meshManager.get_mesh("LP_Sphere");		// Uncomment this line for low-poly sphere
	Mesh& mesh = m_meshManager.get_mesh("Sphere");		// Uncomment this line for high-poly sphere
	//Mesh& mesh = m_meshManager.get_mesh("Cube");			// Uncomment this line for high-poly cube
	//Mesh& mesh = m_meshManager.get_mesh("C_Shape");			// Uncomment this line for a C-shaped mesh
	m_meshInstanceManager.add_mesh_instance(systems, v3(0.0f, 0.0f, 0.0f), mesh, m_texture, 10);
	// Add a Kelvinlet
	MeshInstance& mi = *m_meshInstanceManager.begin();
	KelvinletTimeline& kt = mi.get_kelvinlet_manager().get_timeline();

	// Initialize point light
	m_pointLightCBData.lightPos = v3(-7.0f, 7.0f, 7.0f);
	m_pointLightCBData.lightCol = v3(1.0f, 1.0f, 1.0f);
	m_pointLightCBData.power = 65.0f;
	m_pointLightCBData.angularFrequency = 0.2f * DirectX::XM_2PI;
}

void KelvinletsApp::on_update(SystemsInterface& systems)
{
	m_timer.Start();
	
	// Update per-frame data on the CPU
	m4x4 matView = systems.pCamera->viewMatrix.Transpose();
	m4x4 matProj = systems.pCamera->projMatrix.Transpose();
	m_perFrameCBData.m_matView = matView;
	m_perFrameCBData.m_matProjection = matProj;
	m_perFrameCBData.m_matViewInv = matView.Invert();
	m_perFrameCBData.m_matProjInv = matProj.Invert();
	m_perFrameCBData.camPos = systems.pCamera->eye;
	m_perFrameCBData.deltaTime = m_frameTime * 0.001f;			// Delta time is given in seconds
	m_perFrameCBData.elapsedTime = m_elapsedTime * 0.001f;	   // Elapsed time is given in seconds
	m_perFrameCBData.normalCorrection = m_correctNormals;

	//ImGui::ShowDemoWindow();
	
	// Edit the point light properties
	ImGui::Begin("Point Light");
	static float colour[3] = { m_pointLightCBData.lightCol.x, m_pointLightCBData.lightCol.y, m_pointLightCBData.lightCol.z };
	ImGui::ColorEdit3("Light Colour", colour);
	ImGui::SliderFloat("Power", &m_pointLightCBData.power, 0.0f, 100.0f);
	ImGui::SliderFloat("Angular Frequency", &m_pointLightCBData.angularFrequency, 0.0f, 2.0f*DirectX::XM_2PI);
	ImGui::Checkbox("Normal Correction", &m_correctNormals);
	m_pointLightCBData.lightCol = v3(colour[0], colour[1], colour[2]);
	ImGui::End();
	
	update_constant_buffer(systems.pD3DContext, m_pPointLightCB, m_pointLightCBData);
	// Update per-frame data on the GPU
	update_constant_buffer(systems.pD3DContext, m_pPerFrameCB, m_perFrameCBData);

	ImGui::Begin("Kelvinlet Timeline");

	ImGui::Text("Frame time: %f ms", m_frameTime);
	ImGui::Dummy({ 20.0f, 20.0f });
	ImGui::RadioButton("Edit Mode", &m_editorMode, 0); ImGui::SameLine();
	ImGui::RadioButton("Play Mode", &m_editorMode, 1);

	static Kelvinlet k;
	static float lc[3] = { 0.0f, 0.0f, 0.0f };
	static float fp[3] = { 0.0f, 0.0f, 0.0f };

	// EDIT MODE
	if (m_editorMode == 0)
	{
		if (m_ticking)
		{
			m_meshInstanceManager.stop(systems.pD3DContext);
			m_ticking = false;
		}
		// List each active mesh instance as a dropdown menu \/ List all Kelvinlets as drop-down menus
		// At the bottom, have an add Kelvinlet dropdown button, which will list parameters and have an add button
		int instanceNum = 0;	
		// Iterate over each mesh instance
		std::string instanceName;
		std::string kName;
		for (auto mi_it = m_meshInstanceManager.begin(); mi_it != m_meshInstanceManager.end(); ++mi_it)
		{
			// List the mesh instance as a drop-down menu
			instanceName = std::string("Instance##") + std::to_string(instanceNum);
			if (ImGui::TreeNode(instanceName.c_str()))
			{
				KelvinletManager& km = mi_it->get_kelvinlet_manager();
				KelvinletTimeline& kt = km.get_timeline();
				// Provide an interface to edit physical parameters
				ImGui::SliderFloat("Alpha", km.get_alpha_ptr(), 0.2f, 10.0f);
				ImGui::SliderFloat("Beta", km.get_beta_ptr(), 0.1f, 0.7f*km.get_alpha());
				// Iterate over all of the mesh instance's Kelvinlets
				int kNum = 0;
				for (auto it = kt.begin(); it != kt.end(); ++it)
				{
					if (it->type == 0)
						continue;
					kName = std::string("Kelvinlet##") + std::to_string(kNum);
					if (ImGui::TreeNode(kName.c_str()))
					{
						Kelvinlet& k = *it;
						ImGui::Text("Load Centre: %f, %f, %f", k.loadCentre.x, k.loadCentre.y, k.loadCentre.z);
						ImGui::Text("Force Params: %f, %f, %f", k.forceParams.x, k.forceParams.y, k.forceParams.z);
						ImGui::Text("Regularization: %f", k.epsilon);
						switch (k.type)
						{
						case 1:
							ImGui::Text("Type: Impulse"); break;
						case 2:
							ImGui::Text("Type: Pinch"); break;
						case 3:
							ImGui::Text("Type: Scale"); break;
						default:
							ImGui::Text("Type: ?"); break;
						}
						ImGui::Text("Start Time: %f", k.startTime);
						ImGui::Text("Lifespan: %f", k.lifespan);

						if (ImGui::Button("Remove Kelvinlet"))
						{
							kt.remove_kelvinlet(k);
							ImGui::TreePop();
							continue;
						}
						ImGui::TreePop();
					}
					++kNum;
				}	

				if (ImGui::TreeNode("New Kelvinlet"))
				{
					ImGui::InputFloat3("Load Centre", lc);
					ImGui::InputFloat3("Force Params", fp);
					ImGui::SliderFloat("Regularization", &k.epsilon, 0.01f, 10.0f);
					ImGui::RadioButton("Impulse", &k.type, 1); ImGui::SameLine();
					ImGui::RadioButton("Pinch", &k.type, 2); ImGui::SameLine();
					ImGui::RadioButton("Scale", &k.type, 3);
					ImGui::InputFloat("Start Time", &k.startTime);
					ImGui::InputFloat("Life Span", &k.lifespan);

					if (ImGui::Button("Add"))
					{
						k.loadCentre = v3(lc[0], lc[1], lc[2]);
						k.forceParams = v3(fp[0], fp[1], fp[2]);
						kt.insert_kelvinlet(k);
						ImGui::TreePop();
						ImGui::TreePop();
						continue;
					}
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			++instanceNum;	
		}
		
	}
	// PLAY MODE
	else if (m_editorMode == 1)
	{
		static int pause = 1;
	
		ImGui::RadioButton("Play", &pause, 0); ImGui::SameLine();
		ImGui::RadioButton("Pause", &pause, 1); ImGui::SameLine();
		if (ImGui::Button("Reset")) { m_meshInstanceManager.stop(systems.pD3DContext); }

		m_ticking = !static_cast<bool>(pause);

		int instanceNum = 0;
		// Iterate over each mesh instance
		std::string instanceName;
		for (auto mi_it = m_meshInstanceManager.begin(); mi_it != m_meshInstanceManager.end(); ++mi_it)
		{
			// List the mesh instance as a drop-down menu
			instanceName = std::string("Instance##") + std::to_string(instanceNum);
			if (ImGui::TreeNode(instanceName.c_str()))
			{
				KelvinletTimeline& kt = mi_it->get_kelvinlet_manager().get_timeline();
				ImGui::SliderFloat("Playback Speed", kt.get_play_speed_ptr(), -1.0f, 1.0f);
				ImGui::SliderFloat("Time", kt.get_timepoint_ptr(), 0.0f, kt.get_endpoint());

				ImGui::TreePop();
			}
			++instanceNum;
		}

		if (m_ticking)
		{
			m_meshInstanceManager.play();
			// Update each mesh instance
			m_meshInstanceManager.update(systems, m_frameTime);
			// Perform Kelvinlet calculations for animation
			calculate_kelvinlet_displacements(systems);
		}
		else
			m_meshInstanceManager.pause();
	}
	ImGui::End();
	
	// This function displays some useful debugging values, camera positions etc.
	DemoFeatures::editorHud(systems.pDebugDrawContext);
}

void KelvinletsApp::on_render(SystemsInterface& systems)
{
	// Grid from -50 to +50 in both X & Z
	auto ctx = systems.pDebugDrawContext;

	//dd::xzSquareGrid(ctx, -50.0f, 50.0f, 0.0f, 1.f, dd::colors::DimGray);
	dd::axisTriad(ctx, (const float*)& m4x4::Identity, 0.1f, 15.0f);

	// Bind the VS/PS rendering pair
	m_renderShader.bind(systems.pD3DContext);

	// Bind per-frame data to Slot 0 of the VS/PS rendering pair
	ID3D11Buffer* ppPerFrameCB[] = { m_pPerFrameCB };
	ID3D11Buffer* ppPointLightCB[] = { m_pPointLightCB };
	systems.pD3DContext->VSSetConstantBuffers(0, 1, ppPerFrameCB);
	systems.pD3DContext->PSSetConstantBuffers(0, 1, ppPerFrameCB);
	systems.pD3DContext->PSSetConstantBuffers(2, 1, ppPointLightCB);

	// Bind a sampler state to pixel shader
	ID3D11SamplerState* samplers[] = { m_pLinearMipSamplerState };
	systems.pD3DContext->PSSetSamplers(0, 1, samplers);

	// Draw each mesh instance
	for (auto it = m_meshInstanceManager.begin(); it != m_meshInstanceManager.end(); ++it)
	{
		// Bind the instance's mesh and texture to the device context
		it->get_mesh().bind(systems.pD3DContext);
		it->get_texture().bind(systems.pD3DContext, ShaderStage::kPixel, 0);
		// Update per-instance data on CPU
		extract_per_instance_data(*it);
		// Update per-instance data on GPU
		update_constant_buffer(systems.pD3DContext, m_pPerInstanceCB, m_perInstanceCBData);
		// Bind per-instance data to Slot 1 of the VS/PS rendering pair
		ID3D11Buffer* ppPerInstanceCB[] = { m_pPerInstanceCB };
		systems.pD3DContext->VSSetConstantBuffers(1, 1, ppPerInstanceCB);
		systems.pD3DContext->PSSetConstantBuffers(1, 1, ppPerInstanceCB);
		// Bind displacement buffer SRV to shader slot t1 
		it->get_displacement_manager().bind_displacements_SRV_to_VS(systems.pD3DContext, 1);
		// Render the mesh instance 
		it->render(systems);
	}

	// Unbind resources from the VS/PS rendering pair
	ID3D11ShaderResourceView* nullSRVs[] = { nullptr };
	systems.pD3DContext->VSSetShaderResources(1, 1, nullSRVs);

	ID3D11Buffer* nullCBs[] = { nullptr, nullptr };
	ID3D11Buffer* nullPSCBs[] = { nullptr, nullptr, nullptr };
	systems.pD3DContext->VSSetConstantBuffers(0, 2, nullCBs);
	systems.pD3DContext->PSSetConstantBuffers(0, 3, nullPSCBs);

	ID3D11SamplerState* nullSamplers[] = { nullptr };
	systems.pD3DContext->PSSetSamplers(0, 1, nullSamplers);

	m_timer.Stop();
	m_frameTime = static_cast<float>(m_timer.ElapsedMicroseconds() * 0.001);
	m_elapsedTime += m_frameTime;
	m_timer.Reset();
}

void KelvinletsApp::on_release()
{
	SAFE_RELEASE(m_pLinearMipSamplerState);
	SAFE_RELEASE(m_pPerFrameCB);
	SAFE_RELEASE(m_pPerInstanceCB);
	SAFE_RELEASE(m_pPointLightCB);
}

void KelvinletsApp::init_camera(SystemsInterface& systems)
{
	// Initialize camera
	systems.pCamera->eye = v3(0.0f, 30.0f, 60.0f);
	systems.pCamera->look_at(v3(0.0f, 0.0f, 0.0f));
}

void KelvinletsApp::init_shaders(SystemsInterface& systems)
{
	// Compile compute shader
	m_kelvinletShader.init(systems.pD3DDevice,
		ShaderSetDesc::Create_CS("Assets/Shaders/KelvinletShader.fx", "CS_Kelvinlet"),
		{ VertexFormatTraits<Vertex_Pos3fColour4ubNormal3fTangent3fTex2f>::desc,
		VertexFormatTraits<Vertex_Pos3fColour4ubNormal3fTangent3fTex2f>::size }, false);

	// Compile rendering shader
	m_renderShader.init(systems.pD3DDevice,
		ShaderSetDesc::Create_VS_PS("Assets/Shaders/RenderShader.fx", "VS_Main", "PS_Main"),
		{ VertexFormatTraits<Vertex_Pos3fColour4ubNormal3fTangent3fTex2f>::desc,
		VertexFormatTraits<Vertex_Pos3fColour4ubNormal3fTangent3fTex2f>::size }, true);
}

void KelvinletsApp::calculate_kelvinlet_displacements(SystemsInterface& systems)
{
	if (m_editorMode == 0)
		return;

	// Bind compute shader to device context
	m_kelvinletShader.bind(systems.pD3DContext);

	// Bind per-frame data to shader slot b0
	ID3D11Buffer* ppPerFrameCB[] = { m_pPerFrameCB };
	systems.pD3DContext->CSSetConstantBuffers(0, 1, ppPerFrameCB);

	ID3D11Buffer* ppPerInstanceCB[] = { m_pPerInstanceCB };

	// Iterate over each mesh instance, calculating displacements due to their Kelvinlets
	for (auto it = m_meshInstanceManager.begin(); it != m_meshInstanceManager.end(); ++it)
	{
		// Update per-instance data on the CPU
		extract_per_instance_data(*it);
		// Push per-instance data to the GPU
		update_constant_buffer(systems.pD3DContext, m_pPerInstanceCB, m_perInstanceCBData);
		// Bind per-instance constant buffer to shader slot b1
		ppPerInstanceCB[0] = m_pPerInstanceCB;
		systems.pD3DContext->CSSetConstantBuffers(1, 1, ppPerInstanceCB);
		// Bind per-instance displacements buffer to shader slot u0
		it->get_displacement_manager().bind_displacements_UAV_to_CS(systems.pD3DContext, 0);
		// Bind the instance's mesh vertex data SRV to shader slot t0
		it->get_mesh().bind_structured_VB_SRV(systems.pD3DContext, 0);
		// Bind the instance's Kelvinlet data SRV to shader slot t1;
		it->get_kelvinlet_manager().bind_kelvinlet_data_SRV_to_CS(systems.pD3DContext, 1);
		
		// Launch 1D thread groups, one thread per vertex
		u32 numVertices = it->get_mesh().num_vertices();
		u32 numThreads = align(numVertices, 256);
		systems.pD3DContext->Dispatch(numThreads / 256, 1, 1);
	}

	// Unbind SRVs from compute shader
	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
	systems.pD3DContext->CSSetShaderResources(0, 2, nullSRVs);
	// Unbind UAVS from compute shader
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };
	systems.pD3DContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, NULL);
	// Unbind cbuffers from compute shader
	ID3D11Buffer* nullCBs[] = { nullptr, nullptr };
	systems.pD3DContext->CSSetConstantBuffers(0, 2, nullCBs);
}

void KelvinletsApp::extract_per_instance_data(MeshInstance& mi)
{
	m4x4 matModel = m4x4::CreateTranslation(mi.get_position());
	const KelvinletManager& km = mi.get_kelvinlet_manager();
	m_perInstanceCBData.m_matModel = matModel.Transpose();
	m_perInstanceCBData.numVertices = mi.get_mesh().num_vertices();
	m_perInstanceCBData.numKelvinlets = km.get_num_kelvinlets();
	m_perInstanceCBData.alpha = km.get_alpha();
	m_perInstanceCBData.beta = km.get_beta();
}

KelvinletsApp g_app;

FRAMEWORK_IMPLEMENT_MAIN(g_app, "Kelvinlets Timeline")