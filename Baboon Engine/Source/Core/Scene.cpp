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

void Scene::Init()
{
	m_Camera.Init();
	loadAssets("Scenes/sponza/");
}

void Scene::UpdateUniforms()
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	m_SceneUniforms.view = m_Camera.GetViewMatrix();
	m_SceneUniforms.proj = m_Camera.GetProjMatrix();
	
	

	
	//m_Models[0].Translate(glm::vec3(0, -0.001f*time,0));
	//m_Models[1].Translate(glm::vec3(0.0001f*time, 0, 0));

}

void Scene::OnWindowResize()
{
	m_Camera.UpdateProjectionMatrix();
}


void Scene::loadAssets(const std::string i_ScenePath)
{
	RendererAbstract* renderer = ServiceLocator::GetRenderer();

	const aiScene* aScene;
	

	Assimp::Importer Importer;
	int flags =   aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_GenNormals;
	aScene = Importer.ReadFile(i_ScenePath + "sponza.obj", flags);//TODO: Iterate filesystem and read all .obj files

	///////////TODO: This has to be done before material loading breaking the nice order of materials first, models after, probably
	//since scene geometry is static we don't need to do this
	int numberOfModelsInScene = aScene->mNumMeshes;
	int dynamicAlignment = 256;//TODO: This should come from somewhere...
	size_t dynamicBufferSize = numberOfModelsInScene * dynamicAlignment;
	m_InstanceUniforms = (InstanceUBO*)alignedAlloc(dynamicBufferSize, dynamicAlignment);
	renderer->createInstancedUniformBuffer(nullptr, dynamicBufferSize);
	/////////////////////////////////

	renderer->createStaticUniformBuffer(nullptr, sizeof(SceneUniforms));
	UpdateUniforms();


	loadMaterials(aScene, i_ScenePath);
	loadModels(aScene);

	
	renderer->SetupRenderCalls();
}

void Scene::loadMaterials(const aiScene* i_aScene, const std::string i_SceneTexturesPath)
{
	RendererAbstract* renderer = ServiceLocator::GetRenderer();

	int numberOfMaterialsInScene =i_aScene->mNumMaterials;
	m_Materials.resize(numberOfMaterialsInScene);

	for (int i = 0; i < numberOfMaterialsInScene; i++)
	{
		aiMaterial* pMaterial = i_aScene->mMaterials[i];

		std::vector<int> i_TexIndices;

	


		aiString texturefile;
		// Diffuse
		unsigned char* pPixels = nullptr;
		int width, height;
		int texChannels;
		
		
		
		if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturefile);
			std::string fileName = i_SceneTexturesPath + std::string(texturefile.C_Str());
			pPixels = stbi_load(fileName.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
		}
		if(pPixels == nullptr)
		{
			
			// todo : separate pipeline and layout
			pPixels = stbi_load("Textures/baboon.jpg", &width, &height, &texChannels, STBI_rgb_alpha);
		}
		if (pPixels == nullptr)
			throw std::runtime_error("Texture not found, I will handle this properly at some point shouldn't just break!");

		int iTexIndex = renderer->createTexture((void*)pPixels, width, height);
		i_TexIndices.push_back(iTexIndex);
		
		stbi_image_free(pPixels);


		aiString matName;
		pMaterial->Get(AI_MATKEY_NAME, matName);


		renderer->createMaterial(std::string(matName.C_Str()), i_TexIndices.data(),i_TexIndices.size());
		m_Materials[i].Init(std::string(matName.C_Str()));
	}


}
void Scene::loadModels(const aiScene* i_aScene)
{
	int numberOfModelsInScene = i_aScene->mNumMeshes;

	int dynamicAlignment = 256;//TODO: This should come from somewhere...
	

	m_Models.resize(numberOfModelsInScene);
	m_Meshes.resize(numberOfModelsInScene);//THIS IS NOT NECESSARILY TRUE DIFFERENT MODELS COULD POINT TO THE SAME MESH, TODO

	int iCurrentIndex = 0;
	int iVertexCount = 0;
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

		m_Meshes[i].SetMeshIndicesInfo(iCurrentIndex, iNIndices, iVertexCount);
		m_Models[i].SetMesh(&m_Meshes[i]);
		m_Models[i].SetInstanceUniforms((InstanceUBO*)((uint64_t)m_InstanceUniforms + (i * dynamicAlignment)), i);
		m_Models[i].SetMaterial(&m_Materials[aMesh->mMaterialIndex]);

		iCurrentIndex += iNIndices;
		iVertexCount += aMesh->mNumVertices;
	}

	RendererAbstract* renderer = ServiceLocator::GetRenderer();
	

	renderer->createVertexBuffer((void*)(GetVerticesData()), GetVerticesSize());
	renderer->createIndexBuffer((void*)(GetIndicesData()), GetIndicesSize());
}