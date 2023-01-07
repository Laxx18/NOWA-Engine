#include "NOWAPrecompiled.h"
#include "HlmsModule.h"
#include "main/Core.h"

#if 0

namespace NOWA
{
	
	HlmsModule::HlmsModule()
		: sceneManager(nullptr),
		hlms(nullptr),
		pbs(nullptr),
		unlit(nullptr),
		hlmsManager(nullptr),
		hlmsTextureManager(nullptr)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[HlmsModule] Module created");
	}

	HlmsModule::~HlmsModule()
	{
		
	}

	void HlmsModule::initialize(Ogre::SceneManager* sceneManager)
	{
		this->sceneManager = sceneManager;
		// Get hlms data
		this->hlms = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
		assert(dynamic_cast<Ogre::HlmsPbs*>(hlms));
		this->pbs = static_cast<Ogre::HlmsPbs*>(hlms);
		assert(dynamic_cast<Ogre::HlmsUnlit*>(hlms));
		this->unlit = static_cast<Ogre::HlmsUnlit*>(hlms);
		this->hlmsManager = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager();
		this->hlmsTextureManager = hlmsManager->getTextureManager();
	}

	void HlmsModule::createHLMSMaterial(Ogre::v1::SubEntity* subEntity, bool dynamic)
	{

		Ogre::v1::MeshPtr v1MeshPtr = Ogre::v1::MeshManager::getSingleton().load(subEntity->getm, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
		
	}

	Ogre::String HlmsModule::adaptAndVerifyTexture(const Ogre::String& textureName, const Ogre::String& adaptTextureName, int removeCharCount)
	{
		std::string tempTextureName = textureName;
		// 4 because e.g. ".png"
		// e.g. case1.png -> case1N.png
		int index = tempTextureName.length() - 4 - removeCharCount;
		std::string resultTextureName = tempTextureName.substr(0, index);
		resultTextureName += adaptTextureName + tempTextureName.substr(index + 1 + removeCharCount, tempTextureName.length());

		if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(resultTextureName))
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HlmsModule]: Creating normal map shader with texture: " + resultTextureName);
			return resultTextureName;
		}
		else
		{
			// Try a second time an delete one char more
			removeCharCount++;
			// e.g. case1D.png -> case1N.png and not case1DN.png
			std::string tempTextureName = textureName;
			// 4 because e.g. ".png"
			int index = tempTextureName.length() - 4 - removeCharCount;
			std::string resultTextureName = tempTextureName.substr(0, index);
			resultTextureName += adaptTextureName + tempTextureName.substr(index + 1 + removeCharCount, tempTextureName.length());
			if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(resultTextureName))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HlmsModule]: Creating normal map shader with texture: " + resultTextureName);
				return resultTextureName;
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[HlmsModule]: Texture does not exist: " + resultTextureName);
				return "";
			}
		}
	}

	void HlmsModule::ensureHasTangents(Ogre::v1::MeshPtr mesh)
	{
		bool generateTangents = false;
		if (mesh->sharedVertexData)
		{
			Ogre::v1::VertexDeclaration* vertexDecl = mesh->sharedVertexData[0]->vertexDeclaration;
			generateTangents |= hasNoTangentsAndCanGenerate(vertexDecl);
		}

		for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
		{
			Ogre::v1::SubMesh* subMesh = mesh->getSubMesh(i);
			if (subMesh->vertexData)
			{
				Ogre::v1::VertexDeclaration* vertexDecl = subMesh->vertexData[0]->vertexDeclaration;
				generateTangents |= hasNoTangentsAndCanGenerate(vertexDecl);
			}
		}

		try
		{
			if (generateTangents)
			{
				mesh->buildTangentVectors();
			}
		}
		catch (...)
		{
		}
	}

	bool HlmsModule::hasNoTangentsAndCanGenerate(Ogre::v1::VertexDeclaration* vertexDecl)
	{
		bool hasTangents = false;
		bool hasUVs = false;
		const Ogre::v1::VertexDeclaration::VertexElementList& elementList = vertexDecl->getElements();
		Ogre::v1::VertexDeclaration::VertexElementList::const_iterator itor = elementList.begin();
		Ogre::v1::VertexDeclaration::VertexElementList::const_iterator end = elementList.end();

		while (itor != end && !hasTangents)
		{
			const Ogre::v1::VertexElement& vertexElem = *itor;
			if (vertexElem.getSemantic() == Ogre::VES_TANGENT)
			{
				hasTangents = true;
			}
			if (vertexElem.getSemantic() == Ogre::VES_TEXTURE_COORDINATES)
			{
				hasUVs = true;
			}
			++itor;
		}

		return !hasTangents && hasUVs;
	}

	void HlmsModule::destroyContent(void)
	{
		this->hlms = nullptr;
		this->pbs = nullptr;
		this->unlit = nullptr;
		this->hlmsManager = nullptr;
		this->hlmsTextureManager = nullptr;
		this->processedMeshes.clear();
	}

	HlmsModule* HlmsModule::getInstance()
	{
		static HlmsModule instance;

		return& instance;
	}

}; // namespace end

#endif