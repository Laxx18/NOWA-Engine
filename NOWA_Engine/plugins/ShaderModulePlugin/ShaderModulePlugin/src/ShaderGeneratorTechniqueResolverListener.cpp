#include "ShaderGeneratorTechniqueResolverListener.h"

namespace OBA
{

ShaderGeneratorTechniqueResolverListener::ShaderGeneratorTechniqueResolverListener(Ogre::RTShader::ShaderGenerator *pShaderGenerator)
:
pShaderGenerator(pShaderGenerator)
{
	// Set shader cache path.
    //mShaderGenerator->setShaderCachePath("../../Samples/Media/RTShaderLib/");
	//mViewport->setMaterialScheme(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
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
	Ogre::Technique* generatedTech = NULL;

	// Case this is the default shader generator scheme.
	if (schemeName == Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME)
	{
		bool techniqueCreated;

		// Create shader generated technique for this material.
		techniqueCreated = this->pShaderGenerator->createShaderBasedTechnique(
			originalMaterial->getName(), 
			Ogre::MaterialManager::DEFAULT_SCHEME_NAME, 
			schemeName);	

		// Case technique registration succeeded.
		if (techniqueCreated)
		{
			// Force creating the shaders for the generated technique.
			if (!this->pShaderGenerator->validateMaterial(schemeName, originalMaterial->getName()))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage("[ShaderGeneratorTechniqueResolverListener] Error in validateMaterial, there is something wrong with shaders?");
			}

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

	return generatedTech;
}

}; //Ende Namensraum