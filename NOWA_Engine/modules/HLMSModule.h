#ifndef HLMS_MODULE_H
#define HLMS_MODULE_H

#if 0

#include "defines.h"

namespace NOWA
{
	class EXPORTED HlmsModule
	{
	public:
		/*
		* @brief		Initializes the compositor.
		* @param[in]	sceneManager			The scene manager to work with.
		*/
		void initialize(Ogre::SceneManager* sceneManager);

		void createHLMSMaterial(Ogre::v1::SubEntity* subEntity, bool dynamic);

		void ensureHasTangents(Ogre::v1::MeshPtr mesh);

		void destroyContent(void);

		/// Do not use this constructor!
		HlmsModule();
		~HlmsModule();
		
	public:
		/*
		* @brief		Gets an instance of the @HLMSModule.
		* @return		instance The instance of the HLMS module
		*/
		static HlmsModule* getInstance();
	private:
		/*
		* @brief		Adapts the given texture name with a postfix like "_norm.", "_mask.". See @generateLightShaders for further details.
		*				It also verifies, if the new texture name exists in the folder of the original texture name. If not an empty texture name will be delivered.
		* @param[in]	textureName The original texture name.
		* @param[in]	adaptTextureName The adapted texture name like "_norm.", "_mask.".
		* @param[in]	removeCharCount	 removes x-chars before concatenating the adaptTextureName like: "testD.png" will be "testN.png" when adaptTextureName is set to "N."
		*								 and removeCharCount = 1
		* @return		newTextureName
		*/
		Ogre::String adaptAndVerifyTexture(const Ogre::String& textureName, const Ogre::String& adaptTextureName, int removeCharCount = 0);

		bool hasNoTangentsAndCanGenerate(Ogre::v1::VertexDeclaration* vertexDecl);
	private:
		Ogre::SceneManager* sceneManager;
		Ogre::Hlms* hlms;
		Ogre::HlmsPbs* pbs;
		Ogre::HlmsUnlit* unlit;
		Ogre::HlmsManager* hlmsManager;
		Ogre::HlmsTextureManager* hlmsTextureManager;
		std::map<Ogre::String, Ogre::v1::MeshPtr> processedMeshes;
	};

}; // namespace end

#endif
#endif