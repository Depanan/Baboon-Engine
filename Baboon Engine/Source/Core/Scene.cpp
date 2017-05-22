#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Core\ServiceLocator.h"
#include <chrono>
#include <cstdlib>
#include <cstdio>

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

	const aiScene* aScene;
	Assimp::Importer Importer;
	int flags = aiProcess_PreTransformVertices | aiProcess_Triangulate | aiProcess_GenNormals;
	aScene = Importer.ReadFile("Models/sibenik/sibenik.dae", flags);
	
	const int numberTexturesInScene = aScene->mNumTextures;
	const int numberOfModelsInScene = aScene->mNumMeshes;

	m_SceneUniforms.view = glm::mat4();
	m_SceneUniforms.proj = glm::mat4();
	int dynamicAlignment = 256;//TODO: This should come from somewhere...
	size_t dynamicBufferSize = numberOfModelsInScene * dynamicAlignment;
	m_InstanceUniforms = (InstanceUBO*)alignedAlloc(dynamicBufferSize, dynamicAlignment);


	//if(numberTexturesInScene>0)
	//{ 	
		m_Textures.resize(1);
		m_Textures[0].Init("Textures/baboon.jpg");
	//}

	if (numberOfModelsInScene > 0)
	{
		m_Models.resize(numberOfModelsInScene);
		m_Meshes.resize(numberOfModelsInScene);//THIS IS NOT NECESSARILY TRUE DIFFERENT MODELS COULD POINT TO THE SAME MESH, TODO

		int iCurrentIndex = 0;
		int iVertexCount = 0;
		for (uint32_t i = 0; i < numberOfModelsInScene; i++)
		{
			aiMesh *aMesh = aScene->mMeshes[i];

			bool hasUV = aMesh->HasTextureCoords(0);
			bool hasColor = aMesh->HasVertexColors(0);
			bool hasNormals = aMesh->HasNormals();

			
			for (uint32_t v = 0; v < aMesh->mNumVertices; v++)
			{
				Vertex vertex;
				vertex.pos = glm::make_vec3(&aMesh->mVertices[v].x);
				vertex.pos.y = -vertex.pos.y;
				vertex.texCoord = hasUV ? glm::make_vec2(&aMesh->mTextureCoords[0][v].x) : glm::vec2(0.0f);
				vertex.normal = hasNormals ? glm::make_vec3(&aMesh->mNormals[v].x) : glm::vec3(0.0f);
				vertex.normal.y = -vertex.normal.y;
				vertex.color = hasColor ? glm::make_vec3(&aMesh->mColors[0][v].r) : glm::vec3(1.0f);
				m_Vertices.push_back(vertex);
			}

			int iNIndices = 0;
			for (uint32_t f = 0; f < aMesh->mNumFaces; f++)
			{
				for (uint32_t j = 0; j < 3; j++)
				{
					m_Indices.push_back(aMesh->mFaces[f].mIndices[j]);
					iNIndices++;
				}
			}

			m_Meshes[i].SetMeshIndicesInfo(iCurrentIndex, iNIndices);
			m_Models[i].SetMesh(&m_Meshes[i]);
			m_Models[i].SetInstanceUniforms((InstanceUBO*)((uint64_t)m_InstanceUniforms + (i * dynamicAlignment)), i);

			iCurrentIndex += iNIndices;
		}
	}
	
	
	

	
	
	//m_Models[0].Scale(glm::vec3(0.1f, 0.1f, 0.1f));
	//m_Models[0].Translate(glm::vec3(0.0f, 0.0f, 50.0f));

	RendererAbstract* renderer = ServiceLocator::GetRenderer();
	

	for (int i = 0; i < m_Textures.size(); i++)
	{
		renderer->createTexture((void*)(m_Textures[i].GetData()), m_Textures[i].GetWidht(), m_Textures[i].GetHeight());
	}

	renderer->createVertexBuffer((void*)(GetVerticesData()), GetVerticesSize());
	renderer->createIndexBuffer((void*)(GetIndicesData()), GetIndicesSize());

	

	
	
	renderer->createStaticUniformBuffer(nullptr, sizeof(SceneUniforms));
	renderer->createInstancedUniformBuffer(nullptr, dynamicBufferSize);
	
	UpdateUniforms();
	renderer->SetupRenderCalls();
		
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