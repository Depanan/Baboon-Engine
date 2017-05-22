#pragma once
#include "Renderer\RendererAbstract.h"
#include <vector>
#include "Core\Model.h"
#include "Image.h"
#include "Camera.h"

struct SceneUniforms {
	glm::mat4 view;
	glm::mat4 proj;
};




class Scene {
public:
	Scene();
	~Scene();
	const Vertex* GetVerticesData() { return m_Vertices.data(); }
	const int GetVerticesNumber() { return m_Vertices.size(); }
	const size_t GetVerticesSize() { return sizeof(m_Vertices[0]) * m_Vertices.size(); }

	const uint16_t* GetIndicesData() { return m_Indices.data(); }
	const int GetIndicesNumber() { return m_Indices.size(); }
	const size_t GetIndicesSize() { return sizeof(m_Indices[0]) * m_Indices.size(); }

	const Camera& GetCamera() { return m_Camera; }


	void UpdateUniforms();
	
	SceneUniforms* GetSceneUniforms() { return &m_SceneUniforms; }
	InstanceUBO* GetInstanceUniforms() { return m_InstanceUniforms; }


	//Init function likely to read from a scene file or whatever
	void Init();
	

	std::vector <Model>* GetModels() { return &m_Models; }

	std::vector <Image>* GetTextures() { return &m_Textures; }

private:

	Camera m_Camera;
	SceneUniforms m_SceneUniforms;
	InstanceUBO* m_InstanceUniforms =nullptr;

	std::vector <Model> m_Models;
	std::vector <Mesh> m_Meshes;


	//Global data for indexed meshes
	std::vector<Vertex> m_Vertices;
	std::vector<uint16_t> m_Indices;


	//Textures
	std::vector<Image> m_Textures;

};

class SceneManager {

public:
	Scene* GetScene() {
		return &m_SceneData;
	}
private:
	Scene m_SceneData;

};

