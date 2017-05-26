#pragma once
#include "Renderer\RendererAbstract.h"
#include <vector>
#include "Core\Model.h"
#include "Core\Material.h"




struct aiScene;

class Scene {
public:
	Scene();
	~Scene();
	

	
	void OnWindowResize();

	void UpdateUniforms();
	
	
	InstanceUBO* GetInstanceUniforms() { return m_InstanceUniforms; }


	
	void Init(const std::string i_ScenePath);
	void Free();


	bool IsInit() { return m_bIsInit; }

	

	std::vector <Model>* GetModels() { return &m_Models; }

	int GetVertexBufferIndex() { return m_iVertexBufferIndex; }

private:

	bool m_bIsInit = false;

	
	InstanceUBO* m_InstanceUniforms =nullptr;


	std::vector <Model> m_Models;
	std::vector <Mesh> m_Meshes;
	std::vector <Material> m_Materials;
	int m_iVertexBufferIndex = -1;//In the renderer internal storage

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

