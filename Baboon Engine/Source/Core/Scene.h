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

#define MAX_DEFERRED_POINT_LIGHTS 15
#define MAX_DEFERRED_SPOT_LIGHTS 15
#define MAX_DEFERRED_DIR_LIGHTS 2

struct alignas(16)Light {
    glm::vec4 lightPos;
    glm::vec4 lightColor;
};
struct alignas(16) UBODeferredLights
{
    Light pointLights[MAX_DEFERRED_POINT_LIGHTS];
    Light spotLights[MAX_DEFERRED_SPOT_LIGHTS];
    Light dirLights[MAX_DEFERRED_DIR_LIGHTS];
} ;
enum class LightType {
    LightType_Point = 0,
    LightType_Spot = 1,
    LightType_Directional = 2
};

enum class BatchType {
    BatchType_Opaque = 0,
    BatchType_Transparent = 1
};
struct RenderBatch {
    BatchType m_BatchType;
    std::string m_Name;
    uint8_t m_MaterialIndex;
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
  Buffer* getMaterialsUniformBuffer() const { return m_MaterialsUniformBuffer; }

	std::vector <std::unique_ptr<Model>>* GetModels() { return &m_Models; }
  std::vector <std::reference_wrapper<Model>>* GetOpaqueModels() { return &m_OpaqueModels; }
  std::vector < std::reference_wrapper<Model>>* GetTransparentModels() { return &m_TransparentModels; }
	
  
  std::vector<RenderBatch>& GetTransparentBatches() { return m_TransparentBatch; }
  std::vector<RenderBatch>& GetOpaqueBatches() { return m_OpaqueBatch; }
  const AABB& getSceneAABB()const { return m_SceneAABB; }

  //const Light& getLight(size_t index)const { return m_DeferredLights.lights[index]; }
  size_t getDirLightCount() { return m_DirLightCount; }
  size_t getSpotLightCount() { return m_SpotLightCount; }
  size_t getPointLightCount() { return m_PointLightCount; }
  /*void setLightPosition(size_t index,glm::vec3 position);
  void setLightColor(size_t index,glm::vec3 position);
  void setLightAttenuation(size_t index, float attenuation);*/
  void updateLightsBuffer();
  void updateMaterialsBuffer();


  void createBox(const glm::vec3& position);
  void createLight(const glm::vec3& position, const glm::vec3& color,float attenuation, LightType lightType);

  //UI functions
  void DoLightUI(Light& light, std::string& lightName);
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
	std::vector <Material*> m_Materials;
  UBOMaterial m_MaterialParametersUBO;
  Buffer* m_MaterialsUniformBuffer;


	//Global data for indexed meshes
	std::vector<Vertex> m_Vertices;
	std::vector<uint32_t> m_Indices;
  

  Buffer* m_LightsUniformBuffer;
  UBODeferredLights m_DeferredLights;
  size_t m_DirLightCount = 0;
  size_t m_SpotLightCount = 0;
  size_t m_PointLightCount = 0;



  void prepareBatches();
  void getBatches(std::vector<RenderBatch>& batchList, BatchType batchType);
	void loadAssets(const std::string i_ScenePath);
	void loadMaterials(const aiScene* i_aScene, const std::string i_SceneTexturesPath);
	void loadMeshes(const aiScene* i_aScene);
  void loadSceneRecursive(const aiNode* i_Node);
  void Init(const std::string i_ScenePath);
  void SetInit(){ m_bIsInit = true; }
  void Free();

  void createPointLight(const glm::vec3& position, const glm::vec3& color, float attenuation);
  void createSpotLight(const glm::vec3& position, const glm::vec3& color, float attenuation);
  void createDirLight(const glm::vec3& position, const glm::vec3& color);

  Material* createMaterial(std::string i_sMaterialName, std::vector<std::pair<std::string, Texture*>>* i_Textures, bool isTransparent, glm::vec4 diffuse, glm::vec4  ambient, glm::vec4 specular, bool updateBuffer = true);

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

