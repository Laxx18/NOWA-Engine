#include "NOWAPrecompiled.h"
#include "ShaderGeneratorTechniqueResolverListener.h"

namespace NOWA
{

	ShaderGeneratorTechniqueResolverListener::ShaderGeneratorTechniqueResolverListener(Ogre::RTShader::ShaderGenerator* shaderGenerator)
		: shaderGenerator(shaderGenerator)
	{

	}

	ShaderGeneratorTechniqueResolverListener::~ShaderGeneratorTechniqueResolverListener()
	{

	}

	/** This is the hook point where shader based technique will be created.
	It will be called whenever the material manager won't find appropriate technique
	that satisfy the target scheme name. If the scheme name is out target RT Shader System
	scheme name we will try to create shader generated technique for it.
	*/
	Ogre::Technique* ShaderGeneratorTechniqueResolverListener::handleSchemeNotFound(unsigned short schemeIndex,
		const Ogre::String& schemeName, Ogre::Material* originalMaterial, unsigned short lodIndex,
		const Ogre::Renderable* rend)
	{
		Ogre::Technique* generatedTech = nullptr;

		// Case this is the default shader generator scheme.
		if (schemeName == Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME)
		{
			
			MaterialRegisterIterator it = this->registeredMaterials.find(originalMaterial);
			bool techniqueCreated = false;

			// important skip materials that do not need RTS shader like trays, particles, backgrounds because these ones make trouble when the materials are invalidated when the engine
			// is shutdown
			if (Ogre::StringUtil::match(originalMaterial->getName(), "Backgroun*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "__OgreNewt*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "SdkTray*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "PUMediaPack*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "Materials/OverlayMateria*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "BaseWhit*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "consol*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "Font*", true))
			{
				return nullptr;
			}
			/*else if (Ogre::StringUtil::match(originalMaterial->getName(), "RedLin*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "GreenLin*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "BlueLin*", true))
			{
				return nullptr;
			}
			else if (Ogre::StringUtil::match(originalMaterial->getName(), "YellowLin*", true))
			{
				return nullptr;
			}*/
		
			// This material was not registered before.
			if (it == this->registeredMaterials.end())
			{
				techniqueCreated = this->shaderGenerator->createShaderBasedTechnique(originalMaterial->getName(), Ogre::MaterialManager::DEFAULT_SCHEME_NAME, schemeName);
				if (techniqueCreated)
				{
					// Force creating the shaders for the generated technique.
					this->shaderGenerator->validateMaterial(schemeName, originalMaterial->getName());
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ShaderModule] Validate material: " + originalMaterial->getName());
					// Grab the generated technique.
					Ogre::Material::TechniqueIterator itTech = originalMaterial->getTechniqueIterator();

					while (itTech.hasMoreElements())
					{
						Ogre::Technique* curTech = itTech.getNext();

						if (curTech->getSchemeName() == schemeName)
						{
							generatedTech = curTech;
							break;
						}
					}
				}
			}
			/*else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ShaderModule] Have already: " + originalMaterial->getName());
			}*/
			this->registeredMaterials[originalMaterial] = techniqueCreated;
		}

		return generatedTech;
	}

	ShaderGeneratorTechniqueResolverListener::MaterialRegisterMap ShaderGeneratorTechniqueResolverListener::getRegisteredMaterials(void)
	{
		return this->registeredMaterials;
	}

}; // namespace end