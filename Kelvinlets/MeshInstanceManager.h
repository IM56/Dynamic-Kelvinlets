#pragma once

#include "Manager.h"
#include "MeshInstance.h"

#include <vector>

class MeshInstanceManager : public Manager<MeshInstanceManager>
{
public:
	MeshInstanceManager() {}
	~MeshInstanceManager();

	void init(SystemsInterface&);
	void update(SystemsInterface&, float);
	void release();
	
	void play();
	void pause();
	void stop(ID3D11DeviceContext*);

	void add_mesh_instance(SystemsInterface& systems, v3, Mesh&, Texture&, u32);
	void remove_mesh_instance(size_t x);

	std::vector<MeshInstance>::iterator begin();
	std::vector<MeshInstance>::const_iterator cbegin() const;
	std::vector<MeshInstance>::iterator end();
	std::vector<MeshInstance>::const_iterator cend() const;

private:
	void sort_instances_by_mesh();

private:
	std::vector<MeshInstance> m_meshInstances;
	static size_t m_id;
};