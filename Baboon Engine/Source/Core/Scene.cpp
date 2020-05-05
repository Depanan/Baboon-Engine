#define NOMINMAX
#include "Scene.h"
#include "Renderer/Common/GLMInclude.h"

#include <glm/gtc/type_ptr.hpp>
#include "Core\ServiceLocator.h"
#include "Renderer\Common\Buffer.h"
#include <chrono>
#include <cstdlib>
#include <cstdio>

#include <limits>

#include "assimp\Importer.hpp"

#include "assimp\scene.h"
#include "assimp\postprocess.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb\stb_image.h"

//Util functions for aligned memalloc, maybe move them at some point
void* alignedAlloc(size_t size, size_t alignment)
{
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else 
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void alignedFree(void* data)
{
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else 
	free(data);
#endif
}


Scene::Scene()
{

}

Scene::~Scene()
{

}

void Scene::Init(const std::string i_ScenePath)
{

  Free();
  loadAssets(i_ScenePath);

}

void Scene::Free()
{
  if (!m_bIsInit)
   return;
	RendererAbstract* renderer = ServiceLocator::GetRenderer();

  renderer->WaitToDestroy();
	
  m_OpaqueModels.clear();
  m_TransparentModels.clear();
  m_OpaqueBatch.clear();
  m_TransparentBatch.clear();
	m_Models.clear();
	m_Materials.clear();


	m_Meshes.clear();
	
  m_Indices.clear();
	m_Vertices.clear();
  for (auto texture : m_Textures)
  {
      renderer->DeleteTexture(texture);
      delete texture;
  }
  m_Textures.clear();
	renderer->DeleteBuffer(m_IndicesBuffer);
	renderer->DeleteBuffer(m_VerticesBuffer);
	renderer->DeleteInstancedUniformBuffer();

	m_bIsInit = false;
}

void  Scene::getBatches(std::vector<RenderBatch>& batchList, BatchType batchType)//TODO: Don't do this sorting every frame. Only when relevant stuff changes. Probably can get away doing it onSceneLoad
{
    auto camera = ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main);

    auto& listToBatch = batchType == BatchType::BatchType_Opaque ? m_OpaqueModels : m_TransparentModels;

    std::multimap<const std::string, std::reference_wrapper<Model>> sortedByMaterial;
    for (Model& model : listToBatch)
    {
        sortedByMaterial.insert(std::pair(model.GetMaterial()->GetMaterialName(), std::ref(model)));
    }

    for (auto material : m_Materials)
    {
        batchList.emplace_back(RenderBatch());
        RenderBatch* batch = &batchList.back();
        batch->m_Name = std::string("batch_") + material.GetMaterialName();
        auto byMaterial = sortedByMaterial.equal_range(material.GetMaterialName());//retrieve the whole list of models using that material
        for (auto it = byMaterial.first; it != byMaterial.second; ++it)
        {
            Model& model = it->second;
            float distance = glm::length(camera->GetPosition() - model.getAABB().get_center());
            batch->m_ModelsByDistance.emplace(distance,model);
        }
          
       
    }


}





void Scene::OnWindowResize()
{
	if (!m_bIsInit)
		return;
	RendererAbstract* renderer = ServiceLocator::GetRenderer();
	renderer->SetupRenderCalls();
}

void Scene::setLightPosition(glm::vec3 position)
{
    m_Light.lightPos = glm::vec4(position.x, position.y, position.z, m_Light.lightPos.w);//w component of the light is the type of light so we don't touch it when positioning
}

void Scene::setLightColor(glm::vec3 color)
{
    m_Light.lightColor = glm::vec4(color.x, color.y, color.z, m_Light.lightColor.w);//w component of the light is the type of light so we don't touch it when positioning
   
}

void Scene::updateLightsBuffer()
{
    m_LightsUniformBuffer->update(&m_Light, sizeof(UBOLight));
}


void Scene::loadAssets(const std::string i_ScenePath)
{
	RendererAbstract* renderer = ServiceLocator::GetRenderer();

	const aiScene* aScene;

  LOGINFO("\n-----------------Attempting to open scene : "+ i_ScenePath + "-----------------------");

	Assimp::Importer Importer;
	/*int flags =   aiProcess_Triangulate | aiProcess_SortByPType|
     
      aiProcess_GenSmoothNormals |aiProcess_JoinIdenticalVertices
      ;*/
  int flags =   aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;
	aScene = Importer.ReadFile(i_ScenePath, flags);//TODO: Iterate filesystem and read all .obj files

	if (aScene == nullptr)
		throw std::runtime_error("Scene model not found, I will handle this properly at some point shouldn't just break!");

	
	
	std::string iRootScenePath = i_ScenePath.substr(0, i_ScenePath.find_last_of("\\/")) + "\\";
	

	loadMaterials(aScene, iRootScenePath);
	loadMeshes(aScene);
  //loadSceneRecursive(aScene->mRootNode);
  m_Light.lightPos = glm::vec4(100, 100, 100,0);
  m_Light.lightColor = glm::vec4(1.0f, 1.0f, 1.0f,0.0f);
  m_LightsUniformBuffer = ServiceLocator::GetRenderer()->CreateStaticUniformBuffer(&m_Light, sizeof(UBOLight));

  getBatches(m_OpaqueBatch, BatchType::BatchType_Opaque);
  getBatches(m_TransparentBatch, BatchType::BatchType_Transparent);
	renderer->SetupRenderCalls();



  m_bIsInit = true;
 
	
}

const char* fromAiTexureTypesToShaderName(aiTextureType texType)
{
    switch (texType)
    {
    case aiTextureType_NONE:
        break;
    case aiTextureType_DIFFUSE:
        return "baseTexture";
    case aiTextureType_SPECULAR:
        return "specularTexture";
    case aiTextureType_AMBIENT:
        return "ambientTexture";
        break;
    case aiTextureType_EMISSIVE:
        break;
    case aiTextureType_HEIGHT:
        return "normalTexture";
    case aiTextureType_NORMALS:
        return "normalTexture";
    case aiTextureType_SHININESS:
        break;
    case aiTextureType_OPACITY:
        return "opacityTexture";
    case aiTextureType_DISPLACEMENT:
        break;
    case aiTextureType_LIGHTMAP:
        break;
    case aiTextureType_REFLECTION:
        break;
    case aiTextureType_UNKNOWN:
        break;
    case _aiTextureType_Force32Bit:
        break;
    default:
        break;
    }
    return " ";

}


void Scene::loadMaterials(const aiScene* i_aScene, const std::string i_SceneTexturesPath)
{
	RendererAbstract* renderer = ServiceLocator::GetRenderer();
  std::unordered_map<std::string, Texture*> fileNameImages;//Map to avoid loading the same texture more than once
	int numberOfMaterialsInScene =i_aScene->mNumMaterials;
	m_Materials.resize(numberOfMaterialsInScene);

	for (int i = 0; i < numberOfMaterialsInScene; i++)
	{

    std::vector<std::pair<std::string,Texture*>> texturesInMaterial;
		aiMaterial* pMaterial = i_aScene->mMaterials[i];
		aiString matName;
		pMaterial->Get(AI_MATKEY_NAME, matName);

    LOGINFO("\n Creating material: " + matName.C_Str());

    float opacity;
    pMaterial->Get(AI_MATKEY_OPACITY, opacity);
    bool isTransparent = opacity < 1.0f || pMaterial->GetTextureCount(aiTextureType_OPACITY);

		aiString texturefile;
		// Diffuse
		unsigned char* pPixels = nullptr;
		int width, height;
		int texChannels;
		
    int aiTexureTypes = aiTextureType_UNKNOWN - 1;
   
    for (int i = 0; i < aiTexureTypes; i++)
    {
        aiTextureType texType = (aiTextureType)i;
        if(texType == aiTextureType_AMBIENT)//we will be using the diffuse for that
            continue;

        if (pMaterial->GetTextureCount(texType) > 0)
        {
            Texture* texture = nullptr;
            pMaterial->GetTexture(texType, 0, &texturefile);
            std::string fileName = i_SceneTexturesPath + std::string(texturefile.C_Str());
            auto texIterator = fileNameImages.find(fileName);
            if (texIterator != fileNameImages.end())
            {
                texture = fileNameImages[fileName];
            }
            else
            {
                
                LOGINFO("\n Loading image: " + fileName);

                pPixels = stbi_load(fileName.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);

                if (pPixels == nullptr)
                {

                    // todo : separate pipeline and layout
                    pPixels = stbi_load("Textures/baboon.jpg", &width, &height, &texChannels, STBI_rgb_alpha);
                }
                if (pPixels == nullptr)
                    throw std::runtime_error("Texture not found, I will handle this properly at some point shouldn't just break!");

                texture = renderer->CreateTexture((void*)pPixels, width, height);
                m_Textures.push_back(texture);
                fileNameImages[fileName] = texture;
                stbi_image_free(pPixels);
            }
            if (texture)
            {
                texturesInMaterial.push_back({ fromAiTexureTypesToShaderName(texType),texture });
            }
        }
    }
		
	
		
		m_Materials[i].Init(std::string(matName.C_Str()),texturesInMaterial,isTransparent);
	}


}
void Scene::loadMeshes(const aiScene* i_aScene)
{
	int numberOfModelsInScene = i_aScene->mNumMeshes;


	int iCurrentIndex = 0;
	int iVertexGeneralCount = 0;
	for (uint32_t i = 0; i < numberOfModelsInScene; i++)
	{
		aiMesh *aMesh = i_aScene->mMeshes[i];

		bool hasUV = aMesh->HasTextureCoords(0);
		bool hasColor = aMesh->HasVertexColors(0);
		bool hasNormals = aMesh->HasNormals();
    bool hasTangentsAndBitangents = aMesh->HasTangentsAndBitangents();

		for (uint32_t v = 0; v < aMesh->mNumVertices; v++)
		{
			Vertex vertex;
			vertex.pos = glm::vec3(aMesh->mVertices[v].x, aMesh->mVertices[v].y, aMesh->mVertices[v].z);
			vertex.texCoord = hasUV ? glm::vec2(aMesh->mTextureCoords[0][v].x, 1.0f - aMesh->mTextureCoords[0][v].y) : glm::vec2(0.0f);
			vertex.normal = hasNormals ? glm::make_vec3(&aMesh->mNormals[v].x) : glm::vec3(0.0f);
      vertex.tangent = hasTangentsAndBitangents ? glm::make_vec3(&aMesh->mTangents[v].x) : glm::vec3(0.0f);
      vertex.biTangent = hasTangentsAndBitangents ? glm::make_vec3(&aMesh->mBitangents[v].x) : glm::vec3(0.0f);
			vertex.color = hasColor ? glm::make_vec3(&aMesh->mColors[0][v].r) : glm::vec3(1.0f);
			m_Vertices.push_back(vertex);
		}
		

		int iNIndices = 0;
		for (uint32_t f = 0; f < aMesh->mNumFaces; f++)
		{
			aiFace* pFace = &aMesh->mFaces[f];
			if (pFace->mNumIndices != 3)
			{
				throw std::runtime_error("Error loading model! Face is not a triangle");
			}
			for (uint32_t j = 0; j < pFace->mNumIndices; j++)
			{
				m_Indices.push_back(pFace->mIndices[j]);
				iNIndices++;
			}
		}

    m_Meshes.emplace_back(std::make_unique<Mesh>(*this,&m_Vertices[iVertexGeneralCount], iCurrentIndex, iNIndices, iVertexGeneralCount, aMesh->mNumVertices));
    Mesh& mesh = *m_Meshes[i];
    m_Models.emplace_back(std::make_unique<Model>(mesh));
    Model& model = *m_Models[i];
    model.SetMaterial(&m_Materials[aMesh->mMaterialIndex]);

    if (m_Materials[aMesh->mMaterialIndex].isTransparent())
    {
        m_TransparentModels.push_back(*m_Models[i]);
    }
    else
    {
        m_OpaqueModels.push_back(*m_Models[i]);
    }


		iCurrentIndex += iNIndices;
		iVertexGeneralCount += aMesh->mNumVertices;
	}

	RendererAbstract* renderer = ServiceLocator::GetRenderer();
	

	m_VerticesBuffer = renderer->CreateVertexBuffer((void*)(GetVerticesData()), GetVerticesSize());
	m_IndicesBuffer = renderer->CreateIndexBuffer((void*)(GetIndicesData()), GetIndicesSize());

  glm::vec3 scenemin = glm::vec3(std::numeric_limits<float>::max());
  glm::vec3 scenemax = glm::vec3(std::numeric_limits<float>::min());

  for (auto& model : m_Models)
  {
      
      model->updateAABB();
      scenemin = glm::min(scenemin, model->getAABB().get_min());
      scenemax = glm::max(scenemax, model->getAABB().get_max());
  }
  m_SceneAABB = AABB(scenemin, scenemax);
  ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main)->Teleport(m_SceneAABB.get_center() - glm::vec3(100.0,0,100.0),m_SceneAABB.get_center());
}

void Scene::loadSceneRecursive(const aiNode* i_Node)
{
    LOGINFO("Loading node: " + std::string(i_Node->mName.C_Str()));
    for (int i = 0; i < i_Node->mNumChildren; i++)
    {
        const aiNode* child = i_Node->mChildren[i];
        loadSceneRecursive(child);
    }
}

void SceneManager::LoadScene(const std::string i_ScenePath)
{

    ServiceLocator::GetThreadPool()->threads[0]->addJob([=] {  
        m_SceneData[m_LoadingSceneIndex].Init(i_ScenePath);
        ServiceLocator::GetRenderer()->SetupRenderCalls();
 
        //swapScenes
        int oldCurrentSceneIndex = m_CurrentSceneIndex;
        m_CurrentSceneIndex = m_LoadingSceneIndex;
        m_LoadingSceneIndex = oldCurrentSceneIndex;
        
       
       
    });

   
}

void SceneManager::FreeScene()
{
}
