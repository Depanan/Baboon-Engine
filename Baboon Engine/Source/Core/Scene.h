#pragma once
#include "Renderer\RendererAbstract.h"
#include <vector>
#include "Core\Model.h"
#include "Core\Material.h"
#include <map>



struct aiScene;
struct aiNode;
class SceneManager;
class Scene {
    friend class SceneManager;
public:
	Scene();
	~Scene();
   std::vector<Vertex>& GetVertices() { return m_Vertices; }
	const Vertex* GetVerticesData() const { return m_Vertices.data(); }
	const int GetVerticesNumber() { return m_Vertices.size(); }
	const size_t GetVerticesSize() { return sizeof(m_Vertices[0]) * m_Vertices.size(); }

	const uint32_t* GetIndicesData()const { return m_Indices.data(); }
	const int GetIndicesNumber() { return m_Indices.size(); }
	const size_t GetIndicesSize() { return sizeof(m_Indices[0]) * m_Indices.size(); }

  Buffer* GetIndicesBuffer() { return m_IndicesBuffer; }
  Buffer* GetVerticesBuffer() { return m_VerticesBuffer; }
	
	void OnWindowResize();
	


	
	


	bool IsInit() { return m_bIsInit; }

	

	std::vector <Model>* GetModels() { return &m_Models; }
  std::vector <Model*>* GetOpaqueModels() { return &m_OpaqueModels; }
  std::vector <Model*>* GetTransparentModels() { return &m_TransparentModels; }
	
  void GetSortedOpaqueAndTransparent(std::multimap<float, Model*>& opaque, std::multimap<float, Model*>& transparent);

private:

	bool m_bIsInit = false;

	
  std::vector <Texture*> m_Textures;
	std::vector <Model> m_Models;

  std::vector <Model*> m_OpaqueModels;
  std::vector <Model*> m_TransparentModels;


	std::vector <Mesh> m_Meshes;
	std::vector <Material> m_Materials;

	//Global data for indexed meshes
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
  Buffer* m_VerticesBuffer;
  Buffer* m_IndicesBuffer;


	void loadAssets(const std::string i_ScenePath);
	void loadMaterials(const aiScene* i_aScene, const std::string i_SceneTexturesPath);
	void loadMeshes(const aiScene* i_aScene);
  void loadSceneRecursive(const aiNode* i_Node);
  void Init(const std::string i_ScenePath);
  void Free();

};

class SceneManager {

    

public:

    void LoadScene(const std::string i_ScenePath);
    void FreeScene();
	Scene* GetCurrentScene() {
		return &m_SceneData[m_CurrentSceneIndex];
	}


private:
	Scene m_SceneData[2];
  int m_CurrentSceneIndex = 0;
  int m_LoadingSceneIndex = 1;

};

