#pragma once
#include "Renderer\RendererAbstract.h"
#include <vector>
#include "Core\Model.h"
#include "Core\Material.h"
#include <map>



struct aiScene;

class Scene {
public:
	Scene();
	~Scene();
   std::vector<Vertex>& GetVertices() { return m_Vertices; }
	const Vertex* GetVerticesData() { return m_Vertices.data(); }
	const int GetVerticesNumber() { return m_Vertices.size(); }
	const size_t GetVerticesSize() { return sizeof(m_Vertices[0]) * m_Vertices.size(); }

	const uint16_t* GetIndicesData() { return m_Indices.data(); }
	const int GetIndicesNumber() { return m_Indices.size(); }
	const size_t GetIndicesSize() { return sizeof(m_Indices[0]) * m_Indices.size(); }

  Buffer* GetIndicesBuffer() { return m_IndicesBuffer; }
  Buffer* GetVerticesBuffer() { return m_VerticesBuffer; }
	
	void OnWindowResize();

	void UpdateUniforms();
	
	
	InstanceUBO* GetInstanceUniforms() { return m_InstanceUniforms; }


	
	void Init(const std::string i_ScenePath);
	void Free();


	bool IsInit() { return m_bIsInit; }

	

	std::vector <Model>* GetModels() { return &m_Models; }
  std::vector <Model*>* GetOpaqueModels() { return &m_OpaqueModels; }
  std::vector <Model*>* GetTransparentModels() { return &m_TransparentModels; }
	
  void GetSortedOpaqueAndTransparent(std::multimap<float, Model*>& opaque, std::multimap<float, Model*>& transparent);

private:

	bool m_bIsInit = false;

	
	InstanceUBO* m_InstanceUniforms =nullptr;

	std::vector <Model> m_Models;

  std::vector <Model*> m_OpaqueModels;
  std::vector <Model*> m_TransparentModels;


	std::vector <Mesh> m_Meshes;
	std::vector <Material> m_Materials;

	//Global data for indexed meshes
	std::vector<Vertex> m_Vertices;
	std::vector<uint16_t> m_Indices;
  Buffer* m_VerticesBuffer;
  Buffer* m_IndicesBuffer;


	void loadAssets(const std::string i_ScenePath);
	void loadMaterials(const aiScene* i_aScene, const std::string i_SceneTexturesPath);
	void loadModels(const aiScene* i_aScene);

};

class SceneManager {

public:
	Scene* GetScene() {
		return &m_SceneData;
	}
private:
	Scene m_SceneData;

};

