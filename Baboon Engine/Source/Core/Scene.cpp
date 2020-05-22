#define NOMINMAX
#include "Scene.h"
#include "Renderer/Common/GLMInclude.h"
#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include "Core\ServiceLocator.h"
#include "Renderer\Common\Buffer.h"
#include <chrono>
#include <cstdlib>
#include <cstdio>

#include <limits>

#include "assimp\Importer.hpp"
#include "assimp\DefaultLogger.hpp"
#include "assimp\scene.h"
#include "assimp\postprocess.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb\stb_image.h"




static Mesh* s_BoxMesh = nullptr;
static Material* s_DefaultMaterial;
static const std::vector<glm::vec3> s_VerticesBox = {
{-10.0f, -10.0f, 10.0f} ,//0//TODO: Normals are not gonna look good here, start fetching the vertex attributes to use different variants depending on that
{10.0f, -10.0f, 10.0f}  , //1
{10.0f, 10.0f, 10.0f}   ,  //2
{-10.0f, 10.0f, 10.0f}  , //3
{-10.0f, -10.0f, -10.0f}, //4
{10.0f, -10.0f, -10.0f} , //5
{10.0f, 10.0f, -10.0f}  , //6
{-10.0f, 10.0f, -10.0f} //7
};
static  std::vector<glm::vec3> s_ColorsBox = {
{1.0f, 1.0f, 0.0f} ,//0//TODO: Normals are not gonna look good here, start fetching the vertex attributes to use different variants depending on that
{1.0f, 1.0f, 0.0f}  , //1
{1.0f, 1.0f, 0.0f}   ,  //2
{1.0f, 1.0f, 0.0f}  , //3
{1.0f, 1.0f, 0.0f}, //4
{1.0f, 1.0f, 0.0f} , //5
{1.0f, 1.0f, 0.0f}  , //6
{1.0f, 1.0f, 0.0f} //7
};

static const  std::vector<uint32_t> s_IndicesBox = {
     0, 1, 2, 2, 3, 0,//front
     0, 1, 5, 5, 4, 0,//bottom
     1, 5, 6, 6, 2, 1,//right
     4, 0, 3, 3, 7, 4,//left
     4, 5, 6, 6, 7, 4,//back
     3, 2, 6, 6, 7, 3 //top
};





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
	
	renderer->DeleteInstancedUniformBuffer();

	m_bIsInit = false;
}

void Scene::prepareBatches()
{
    /*for (auto& model : m_Models)
    {
        if (model->IsVisible())
        {
            if (model->GetMaterial()->isTransparent())
            {

            }
            else
            {

            }
        }

    }*/
    getBatches(m_OpaqueBatch, BatchType::BatchType_Opaque);
    getBatches(m_TransparentBatch, BatchType::BatchType_Transparent);
}

void  Scene::getBatches(std::vector<RenderBatch>& batchList, BatchType batchType)//TODO: Don't do this sorting every frame. Only when relevant stuff changes. Probably can get away doing it onSceneLoad
{
    batchList.clear();
    auto camera = ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main);

    auto& listToBatch = batchType == BatchType::BatchType_Opaque ? m_OpaqueModels : m_TransparentModels;

    std::multimap<const std::string, std::reference_wrapper<Model>> sortedByMaterial;
    for (Model& model : listToBatch)
    {
        sortedByMaterial.insert(std::pair(model.GetMaterial()->GetMaterialName(), std::ref(model)));
    }

    for (auto material : m_Materials)
    {
        bool needsInsertion = (batchType == BatchType::BatchType_Transparent && material.isTransparent()) || (batchType == BatchType::BatchType_Opaque && !material.isTransparent());
        if(!needsInsertion)
            continue;
        batchList.emplace_back(RenderBatch());
        RenderBatch* batch = &batchList.back();
        batch->m_BatchType = batchType;
        batch->m_Name = std::string("batch_") + material.GetMaterialName();
        auto byMaterial = sortedByMaterial.equal_range(material.GetMaterialName());//retrieve the whole list of models using that material
        for (auto it = byMaterial.first; it != byMaterial.second; ++it)
        {
            Model& model = it->second;
            float distance = glm::length(camera->GetPosition() - model.getAABB().get_center());
           
            batch->m_ModelsByDistance.emplace(distance, model);
        }
          
       
    }
    if(s_DefaultMaterial && ((s_DefaultMaterial->isTransparent() && batchType == BatchType::BatchType_Transparent)|| (!s_DefaultMaterial->isTransparent() && batchType == BatchType::BatchType_Opaque)))
    {
        batchList.emplace_back(RenderBatch());
        RenderBatch* batch = &batchList.back();
        batch->m_BatchType = batchType;

        batch->m_Name = std::string("batch_") + s_DefaultMaterial->GetMaterialName();
        auto byMaterial = sortedByMaterial.equal_range(s_DefaultMaterial->GetMaterialName());//retrieve the whole list of models using that material
        for (auto it = byMaterial.first; it != byMaterial.second; ++it)
        {
            Model& model = it->second;
            float distance = glm::length(camera->GetPosition() - model.getAABB().get_center());
     
            batch->m_ModelsByDistance.emplace(distance,model);
        }
    }

}





void Scene::SelectModel(glm::vec2 clickPoint)
{
    glm::vec3 worldPoint;
    for (auto& model : m_Models)
    {
        const AABB& box = model->getAABB();
        if (box.pointInside(worldPoint))//TODO: WE NEED A RAY NOT A POINT!
        {
            model->SetSelection(true);
            
        }
        model->SetSelection(false);
    }
}

void Scene::Update()
{
    if (!m_bIsInit)
        return;
    if (!m_bIsDirty)
        return;
    bool hasBeenNotified = false;
    for (auto& model : m_Models)
    {
        if (model->GetDirty())
        {
            model->computeModelMatrix();
            if(!hasBeenNotified)
            {
                ServiceLocator::GetSceneManager()->GetSubject().Notify(Subject::Message::SCENEDIRTY);
                hasBeenNotified = true;
            }
        }
    }
    if(hasBeenNotified)
        prepareBatches();//Reordering geometry

    m_bIsDirty = false;
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

void Scene::DoLightsUI(bool* pOpen)
{
    if (!m_bIsInit)
        return;

    if (ImGui::Begin("Scene lights", pOpen, ImGuiWindowFlags_MenuBar))
    {

        if (ImGui::TreeNode("Light"))
        {
            glm::vec3 lPos = m_Light.lightPos;
            bool bUpdateLightsBuffer = false;
            const char* labelsTrans[3]{ "X","Y","Z" };
            if (GUI::ImguiVec3Controller(lPos, labelsTrans))
            {
                setLightPosition(lPos);
                bUpdateLightsBuffer = true;
            }

            glm::vec3 lCol = m_Light.lightColor;
            ImGui::ColorPicker3("Light color", &lCol.x);
            if (lCol != glm::vec3(m_Light.lightColor))
            {
                setLightColor(lCol);
                bUpdateLightsBuffer = true;
            }
            if (bUpdateLightsBuffer)
            {
                updateLightsBuffer();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();

    
    
}


 void Scene::createBox(const glm::vec3& position)
{

     if (s_BoxMesh == nullptr)
     {
         s_BoxMesh = new Mesh();
         std::unordered_map<std::string, AttributeDescription> descriptions;
         AttributeDescription a{VK_FORMAT_R32G32B32_SFLOAT,12,0};
         descriptions.emplace("inPosition", a);
         descriptions.emplace("inColor", a);
        
         s_BoxMesh->setData(descriptions,s_VerticesBox, s_IndicesBox, &s_ColorsBox);
     }
         
     MeshView meshView{ 0,s_IndicesBox.size(),0,s_VerticesBox.size(),-1 };

     m_Models.emplace_back(std::make_unique<Model>(*s_BoxMesh, meshView,*this, "Box!!"));
     auto& model = m_Models.back();
     if (s_DefaultMaterial == nullptr)
     {
         s_DefaultMaterial = new Material();
         s_DefaultMaterial->Init("daniel_default_mat", nullptr, true);
     }
     model->SetMaterial(s_DefaultMaterial);

     //if (m_Materials[0].isTransparent())
     //{
         m_TransparentModels.push_back(*model);
     //}
     //else
     //{
         //m_OpaqueModels.push_back(*model);
     //}

     glm::mat4 transform;
     transform = glm::translate(transform, -position);
     model->SetTransform(transform);

    

}


void Scene::DoModelsUI(bool* pOpen)
{
    if (!m_bIsInit)
        return;



    if (ImGui::Begin("Scene models", pOpen, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::TreeNode("Models"))
        {
            for (int i = 0; i < m_Models.size(); i++)
            {
                auto& model = m_Models[i];
                if (ImGui::TreeNode((void*)(intptr_t)i, "%s", model->getName().c_str()))
                {
                    glm::vec3 trans = model->GetTranslation();
                    glm::vec3 position = -trans;
                    const char* labelsTrans[3]{ "tX","tY","tZ" };
                    if (GUI::ImguiVec3Controller(position, labelsTrans))
                    {
                        model->Translate(-position);
                        
                    }
                    glm::vec3 rotation = model->GetRotation();
                    const char* labelsRot[3]{ "Yaw","Pitch","Roll" };
                    if (GUI::ImguiVec3Controller(rotation, labelsRot))
                    {
                        model->Rotate(rotation);
                        
                    }
                    glm::vec3 scale = model->GetScale();
                    const char* labelsScale[3]{ "sX","sY","sZ" };
                    if (GUI::ImguiVec3Controller(scale, labelsScale))
                    {
                        model->Scale(scale);
                        
                    }
                    if (ImGui::Button("Goto.."))
                    {
                        ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main)->CenterAt(model->getAABB().get_center());
                    }
                   
                  
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::Button("Create Box!"))
        {
            createBox(ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main)->GetPosition());
        }
    }
    ImGui::End();
  
}

void myCallback(const char* msg, char* userData) {
    LOGINFO(msg);
}
void Scene::loadAssets(const std::string i_ScenePath)
{

	const aiScene* aScene;

  LOGINFO("\n-----------------Attempting to open scene : "+ i_ScenePath + "-----------------------");

	Assimp::Importer Importer;
  aiString supported;
  Importer.GetExtensionList(supported);

  
  Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, aiDefaultLogStream_STDOUT);

  int flags =   aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;
	aScene = Importer.ReadFile(i_ScenePath, flags);//TODO: Iterate filesystem and read all .obj files

	if (aScene == nullptr)
		throw std::runtime_error("Scene model not found, I will handle this properly at some point shouldn't just break!");

 
  Assimp::DefaultLogger::kill();
	
	std::string iRootScenePath = i_ScenePath.substr(0, i_ScenePath.find_last_of("\\/")) + "\\";
	

	loadMaterials(aScene, iRootScenePath);
	loadMeshes(aScene);

  m_SceneBoundMin = glm::vec3(std::numeric_limits<float>::max());
  m_SceneBoundMax = glm::vec3(std::numeric_limits<float>::min());
  loadSceneRecursive(aScene->mRootNode);
  m_SceneAABB = AABB(m_SceneBoundMin, m_SceneBoundMax);

  m_Light.lightPos = glm::vec4(100, 100, 100,0);
  m_Light.lightColor = glm::vec4(1.0f, 1.0f, 1.0f,0.0f);
  m_LightsUniformBuffer = ServiceLocator::GetRenderer()->CreateStaticUniformBuffer(&m_Light, sizeof(UBOLight));

  

  //Only need this during creation
  m_MeshMap.clear();
 
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
		m_Materials[i].Init(std::string(matName.C_Str()),&texturesInMaterial,isTransparent);
	}
}

void Scene::loadMeshes(const aiScene* i_aScene)
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> biTangents;
    std::vector<uint32_t>  indices;
    std::unordered_map<std::string, AttributeDescription> descriptions;
    AttributeDescription avec3{ VK_FORMAT_R32G32B32_SFLOAT,12,0 };
    AttributeDescription avec2{ VK_FORMAT_R32G32_SFLOAT,8,0 };
    descriptions.emplace("inPosition", avec3);

  m_Meshes.emplace_back(std::make_unique<Mesh>());
  Mesh& mesh = *m_Meshes.back();
  
	int numberOfMeshesInScene = i_aScene->mNumMeshes;


	uint32_t iCurrentIndex = 0;
  uint32_t iVertexGeneralCount = 0;
	for (uint32_t i = 0; i < numberOfMeshesInScene; i++)
	{
		aiMesh *aMesh = i_aScene->mMeshes[i];

		bool hasUV = aMesh->HasTextureCoords(0);
    if (hasUV)
        descriptions.emplace("inTexCoord", avec2);
		bool hasColor = aMesh->HasVertexColors(0);
    if (hasUV)
        descriptions.emplace("inColor", avec3);
		bool hasNormals = aMesh->HasNormals();
    if (hasUV)
        descriptions.emplace("inNormal", avec3);
    bool hasTangentsAndBitangents = aMesh->HasTangentsAndBitangents();
    if (hasUV)
    {
        descriptions.emplace("inTangent", avec3);
        descriptions.emplace("inBiTangent", avec3);
    }
        


    


		for (uint32_t v = 0; v < aMesh->mNumVertices; v++)
		{
        positions.push_back(glm::vec3(aMesh->mVertices[v].x, aMesh->mVertices[v].y, aMesh->mVertices[v].z));

        if (hasColor)
        {
            colors.push_back(glm::make_vec3(&aMesh->mColors[0][v].r));
        }
        if (hasUV)
        {
            texCoords.push_back(glm::vec2(aMesh->mTextureCoords[0][v].x, 1.0f - aMesh->mTextureCoords[0][v].y));
        }
        if (hasNormals)
        {
            normals.push_back(glm::make_vec3(&aMesh->mNormals[v].x));
        }
        if (hasTangentsAndBitangents)
        {
            tangents.push_back(glm::make_vec3(&aMesh->mTangents[v].x));
            biTangents.push_back(glm::make_vec3(&aMesh->mBitangents[v].x));
        }



			/*Vertex vertex;
			vertex.pos = glm::vec3(aMesh->mVertices[v].x, aMesh->mVertices[v].y, aMesh->mVertices[v].z);
			vertex.texCoord = hasUV ? glm::vec2(aMesh->mTextureCoords[0][v].x, 1.0f - aMesh->mTextureCoords[0][v].y) : glm::vec2(0.0f);
			vertex.normal = hasNormals ? glm::make_vec3(&aMesh->mNormals[v].x) : glm::vec3(0.0f);
      vertex.tangent = hasTangentsAndBitangents ? glm::make_vec3(&aMesh->mTangents[v].x) : glm::vec3(0.0f);
      vertex.biTangent = hasTangentsAndBitangents ? glm::make_vec3(&aMesh->mBitangents[v].x) : glm::vec3(0.0f);
			vertex.color = hasColor ? glm::make_vec3(&aMesh->mColors[0][v].r) : glm::vec3(1.0f);
      mesh.pushVertex(vertex);*/
		}
		

    uint32_t iNIndices = 0;
		for (uint32_t f = 0; f < aMesh->mNumFaces; f++)
		{
			aiFace* pFace = &aMesh->mFaces[f];
			if (pFace->mNumIndices != 3)
			{
				throw std::runtime_error("Error loading model! Face is not a triangle");
			}
			for (uint32_t j = 0; j < pFace->mNumIndices; j++)
			{
          //mesh.pushIndex(pFace->mIndices[j]);
          indices.push_back(pFace->mIndices[j]);
				  iNIndices++;
			}
		}

    m_MeshMap.emplace(i, MeshWithView(&mesh, { iCurrentIndex, iNIndices, iVertexGeneralCount, aMesh->mNumVertices,aMesh->mMaterialIndex }));
    
  
    


		iCurrentIndex += iNIndices;
		iVertexGeneralCount += aMesh->mNumVertices;
	}

  mesh.setData(descriptions,positions, indices, &colors, &texCoords, &normals, &tangents, &biTangents);
  
}

glm::mat4 getNodeTransformation(const aiNode* i_Node, glm::mat4 mat)
{
    if (i_Node->mParent)
        return getNodeTransformation(i_Node->mParent, mat * glm::mat4(i_Node->mParent->mTransformation[0][0]));
    else
        return mat ;
}
void Scene::loadSceneRecursive(const aiNode* i_Node)
{
    LOGINFO("Loading node: " + std::string(i_Node->mName.C_Str()));
    if (i_Node->mNumMeshes)
    {
        assert(i_Node->mNumMeshes == 1,"WOOPS, What we do with more than one mesh per node");
        for (int i = 0;i< i_Node->mNumMeshes;i++)//TODO: What happens for nodes with more than one mesh :/
        {
            auto meshWithView = m_MeshMap[i_Node->mMeshes[i]];
            m_Models.emplace_back(std::make_unique<Model>(*meshWithView.first, meshWithView.second,*this, std::string(i_Node->mName.C_Str())));
            auto& model = m_Models.back();
            model->SetMaterial(&m_Materials[meshWithView.second.m_MaterialIndex]);

            if (m_Materials[meshWithView.second.m_MaterialIndex].isTransparent())
            {
                m_TransparentModels.push_back(*model);
            }
            else
            {
                m_OpaqueModels.push_back(*model);
            }

            glm::mat4 transform = getNodeTransformation(i_Node, glm::mat4(i_Node->mTransformation[0][0]));//Recursing up to get the right hierarchical transform. If we have a tree we won't need this
            glm::mat4 checkIdentity = glm::mat4();
            if (transform != checkIdentity)
                LOGINFO("LLAFALW");
           
            model->SetTransform(transform);
            //////////
            model->computeModelMatrix();
            model->updateAABB();//Set transform handles this normally but we need the AABBs for computing the scene AABB
            model->SetDirty();
            ////////
            m_SceneBoundMin = glm::min(m_SceneBoundMin, model->getAABB().get_min());
            m_SceneBoundMax = glm::max(m_SceneBoundMax, model->getAABB().get_max());
           
        }
       
    }
    for (int i = 0; i < i_Node->mNumChildren; i++)
    {
        const aiNode* child = i_Node->mChildren[i];
        loadSceneRecursive(child);
    }
}

std::string BasicFileOpen()
{
    std::string fileOpen = "";
    char filename[MAX_PATH];

    OPENFILENAME ofn;
    ZeroMemory(&filename, sizeof(filename));
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
    ofn.lpstrFilter = "OBJ Files\0*.obj\0Any File\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select a File, yo!";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
    {
        fileOpen = std::string(filename);
    }

    return fileOpen;
}

SceneManager::SceneManager()
{
    ServiceLocator::GetGUI()->AddUIFunction([=]() {
        
        static bool bLoadScene = false;
        static bool bLightsMenu = false;
        static bool bModelsMenu = false;
        
        if (bLoadScene)
        {
            std::string filePath = BasicFileOpen();
            if (filePath.size() > 0)
            {
                ServiceLocator::GetSceneManager()->LoadScene(filePath);
            }
            bLoadScene = false;
        }


        if(bLightsMenu)
            GetCurrentScene()->DoLightsUI(&bLightsMenu);
        if (bModelsMenu)
            GetCurrentScene()->DoModelsUI(&bModelsMenu);
        
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Scene"))
            {
                ImGui::MenuItem("Load Scene", NULL, &bLoadScene);
                ImGui::MenuItem("Lights", NULL, &bLightsMenu);
                ImGui::MenuItem("Models", NULL, &bModelsMenu);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    
    
    
    });
}

void SceneManager::LoadScene(const std::string i_ScenePath)
{

    ServiceLocator::GetThreadPool()->threads[0]->addJob([=] {  
        m_SceneData[m_LoadingSceneIndex].Init(i_ScenePath);
        //swapScenes
        int oldCurrentSceneIndex = m_CurrentSceneIndex;
        m_CurrentSceneIndex = m_LoadingSceneIndex;
        m_LoadingSceneIndex = oldCurrentSceneIndex;

        ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main)->CenterAt(GetCurrentScene()->getSceneAABB().get_center());
        LOGINFO("Scene finished loading!");
        m_SceneSubject.Notify(Subject::Message::SCENELOADED);
        GetCurrentScene()->SetInit();
       
    });
    

   
}

void SceneManager::FreeScene()
{
}
