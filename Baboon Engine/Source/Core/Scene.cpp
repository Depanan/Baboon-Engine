#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Core\ServiceLocator.h"
#include <chrono>
#include <cstdlib>
#include <cstdio>

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
	m_InstanceUniforms = nullptr;
}

Scene::~Scene()
{
	if (m_InstanceUniforms)
		alignedFree(m_InstanceUniforms);
}

void Scene::Init(const std::string i_ScenePath)
{
	if (m_bIsInit)
		Free();

	loadAssets(i_ScenePath);
	//TODO: Make this work multithread
	//ServiceLocator::GetThreadPool()->threads[0]->addJob([=] { loadAssets(i_ScenePath); });
	m_bIsInit = true;
	
}

void Scene::Free()
{
	RendererAbstract* renderer = ServiceLocator::GetRenderer();
	renderer->DeleteMaterials();
  m_OpaqueModels.clear();
  m_TransparentModels.clear();
	m_Models.clear();
	m_Materials.clear();
	m_Meshes.clear();
	m_Indices.clear();
	m_Vertices.clear();
	renderer->DeleteIndexBuffer();
	renderer->DeleteVertexBuffer();
	renderer->DeleteInstancedUniformBuffer();

	m_bIsInit = false;
}

void Scene::GetSortedOpaqueAndTransparent(std::multimap<float, Model*>& opaque, std::multimap<float, Model*>& transparent)
{
    auto camera = ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main);
   
    for (auto model : m_OpaqueModels)
    {
        float distance = glm::length(camera->GetPosition() - model->getAABB().get_center());
        opaque.emplace(distance, model);
    }
    for (auto model : m_TransparentModels)
    {
        float distance = glm::length(camera->GetPosition() - model->getAABB().get_center());
        transparent.emplace(distance, model);
    }
}


void Scene::UpdateUniforms()
{
	
	//Maybe here moving lights and stuff....

	
	//m_Models[0].Translate(glm::vec3(0, -0.001f*time,0));
	//m_Models[1].Translate(glm::vec3(0.0001f*time, 0, 0));

}

void Scene::OnWindowResize()
{
	if (!m_bIsInit)
		return;
	RendererAbstract* renderer = ServiceLocator::GetRenderer();
	renderer->SetupRenderCalls();
}


void Scene::loadAssets(const std::string i_ScenePath)
{
	RendererAbstract* renderer = ServiceLocator::GetRenderer();

	const aiScene* aScene;

  LOGINFO("\n-----------------Attempting to open scene : "+ i_ScenePath + "-----------------------");

	Assimp::Importer Importer;
	int flags =   aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals;
	aScene = Importer.ReadFile(i_ScenePath, flags);//TODO: Iterate filesystem and read all .obj files

	if (aScene == nullptr)
		throw std::runtime_error("Scene model not found, I will handle this properly at some point shouldn't just break!");

	///////////TODO: This has to be done before material loading breaking the nice order of materials first, models after, probably
	//since scene geometry is static we don't need to do this
	int numberOfModelsInScene = aScene->mNumMeshes;
	int dynamicAlignment = 256;//TODO: This should come from somewhere...
	size_t dynamicBufferSize = numberOfModelsInScene * dynamicAlignment;
	m_InstanceUniforms = (InstanceUBO*)alignedAlloc(dynamicBufferSize, dynamicAlignment);
	//renderer->CreateInstancedUniformBuffer(nullptr, dynamicBufferSize);
	/////////////////////////////////
	UpdateUniforms();

	
	std::string iRootScenePath = i_ScenePath.substr(0, i_ScenePath.find_last_of("\\/")) + "\\";
	

	loadMaterials(aScene, iRootScenePath);
	loadModels(aScene);

	
	renderer->SetupRenderCalls();
	
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
        break;
    case aiTextureType_EMISSIVE:
        break;
    case aiTextureType_HEIGHT:
        break;
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
void Scene::loadModels(const aiScene* i_aScene)
{
	int numberOfModelsInScene = i_aScene->mNumMeshes;

	int dynamicAlignment = 256;//TODO: This should come from somewhere...
	

	m_Models.resize(numberOfModelsInScene);
	m_Meshes.resize(numberOfModelsInScene);//THIS IS NOT NECESSARILY TRUE DIFFERENT MODELS COULD POINT TO THE SAME MESH, TODO

	int iCurrentIndex = 0;
	int iVertexGeneralCount = 0;
	for (uint32_t i = 0; i < numberOfModelsInScene; i++)
	{
		aiMesh *aMesh = i_aScene->mMeshes[i];

		bool hasUV = aMesh->HasTextureCoords(0);
		bool hasColor = aMesh->HasVertexColors(0);
		bool hasNormals = aMesh->HasNormals();


		for (uint32_t v = 0; v < aMesh->mNumVertices; v++)
		{
			Vertex vertex;
			vertex.pos = glm::vec3(aMesh->mVertices[v].x, aMesh->mVertices[v].y, aMesh->mVertices[v].z);
			vertex.texCoord = hasUV ? glm::vec2(aMesh->mTextureCoords[0][v].x, 1.0f - aMesh->mTextureCoords[0][v].y) : glm::vec2(0.0f);
			vertex.normal = hasNormals ? glm::make_vec3(&aMesh->mNormals[v].x) : glm::vec3(0.0f);
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

		m_Meshes[i].SetMeshIndicesInfo(iCurrentIndex, iNIndices, iVertexGeneralCount, aMesh->mNumVertices);
    m_Models[i].SetInstanceUniforms((InstanceUBO*)((uint64_t)m_InstanceUniforms + (i * dynamicAlignment)), i);//TODO: Get rid of the instance ubo thing and let the model just have a Model matrix for now
    m_Models[i].SetMesh(&m_Meshes[i]);
		m_Models[i].SetMaterial(&m_Materials[aMesh->mMaterialIndex]);

    if (m_Materials[aMesh->mMaterialIndex].isTransparent())
    {
        m_TransparentModels.push_back(&m_Models[i]);
    }
    else
    {
        m_OpaqueModels.push_back(&m_Models[i]);
    }


		iCurrentIndex += iNIndices;
		iVertexGeneralCount += aMesh->mNumVertices;
	}

	RendererAbstract* renderer = ServiceLocator::GetRenderer();
	

	m_VerticesBuffer = renderer->CreateVertexBuffer((void*)(GetVerticesData()), GetVerticesSize());
	m_IndicesBuffer = renderer->CreateIndexBuffer((void*)(GetIndicesData()), GetIndicesSize());
}