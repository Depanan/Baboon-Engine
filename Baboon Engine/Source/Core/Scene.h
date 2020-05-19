#pragma once
#include "Renderer\RendererAbstract.h"
#include <vector>
#include "Core\Model.h"
#include "Core\Material.h"
#include <map>
#include <functional>
#include "Observer.h"


struct aiScene;
struct aiNode;
class SceneManager;

struct UBOLight {
    glm::vec4 lightPos;
    glm::vec4 lightColor;
};

enum class BatchType {
    BatchType_Opaque = 0,
    BatchType_Transparent = 1
};
struct RenderBatch {
    BatchType m_BatchType;
    std::string m_Name;
    std::multimap<float, std::reference_wrapper<Model>> m_ModelsByDistance;
};

class Scene{
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

  
  void SelectModel(glm::vec2 clickPoint);
  void Update();
	

	bool IsInit() { return m_bIsInit; }

  Buffer* getLightsUniformBuffer() const { return m_LightsUniformBuffer; }

	std::vector <std::unique_ptr<Model>>* GetModels() { return &m_Models; }
  std::vector <std::reference_wrapper<Model>>* GetOpaqueModels() { return &m_OpaqueModels; }
  std::vector < std::reference_wrapper<Model>>* GetTransparentModels() { return &m_TransparentModels; }
	
  
  std::vector<RenderBatch>& GetTransparentBatches() { return m_TransparentBatch; }
  std::vector<RenderBatch>& GetOpaqueBatches() { return m_OpaqueBatch; }
  const AABB& getSceneAABB()const { return m_SceneAABB; }

  const UBOLight& getLight()const { return m_Light; }
  void setLightPosition(glm::vec3 position);
  void setLightColor(glm::vec3 position);
  void updateLightsBuffer();


  void createBox(const glm::vec3& position);

  //UI functions
  void DoLightsUI(bool* pOpen);
  void DoModelsUI(bool* pOpen);

 
  void SetDirty() { m_bIsDirty = true; }

private:

    typedef std::pair<Mesh*, MeshView> MeshWithView;
    std::unordered_map<uint32_t, MeshWithView> m_MeshMap;

	bool m_bIsInit = false;
  bool m_bIsDirty = true;

  glm::vec3 m_SceneBoundMin;
  glm::vec3 m_SceneBoundMax;
  AABB m_SceneAABB;
	
  std::vector <Texture*> m_Textures;
	std::vector <std::unique_ptr<Model>> m_Models;

  std::vector <std::reference_wrapper<Model>> m_OpaqueModels;
  std::vector <std::reference_wrapper<Model>> m_TransparentModels;
  std::vector<RenderBatch> m_OpaqueBatch;
  std::vector<RenderBatch> m_TransparentBatch;


	std::vector <std::unique_ptr<Mesh>> m_Meshes;
	std::vector <Material> m_Materials;

	//Global data for indexed meshes
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
  

  Buffer* m_LightsUniformBuffer;
  UBOLight m_Light;

  void prepareBatches();
  void getBatches(std::vector<RenderBatch>& batchList, BatchType batchType);
	void loadAssets(const std::string i_ScenePath);
	void loadMaterials(const aiScene* i_aScene, const std::string i_SceneTexturesPath);
	void loadMeshes(const aiScene* i_aScene);
  void loadSceneRecursive(const aiNode* i_Node);
  void Init(const std::string i_ScenePath);
  void SetInit(){ m_bIsInit = true; }
  void Free();

};

class SceneManager {

    

public:
    SceneManager();
    void Update() { GetCurrentScene()->Update(); }
    void LoadScene(const std::string i_ScenePath);
    void FreeScene();
	Scene* GetCurrentScene() {
		return &m_SceneData[m_CurrentSceneIndex];
	}
  Subject& GetSubject() { return m_SceneSubject; }
private:
  Subject m_SceneSubject;
	Scene m_SceneData[2];
  int m_CurrentSceneIndex = 0;
  int m_LoadingSceneIndex = 1;

};

