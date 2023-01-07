/*
 * ShadowcasterHandler.cpp
 *
 *  Created on: 07.07.2014
 *      Author: marvin
 */
#include "NOWAPrecompiled.h"
#include "ShadowcasterHandler.h"
#include "OgreTechnique.h"

Shadow_casterHandler::Shadow_casterHandler()
{
	// TODO Auto-generated constructor stub

}

Shadow_casterHandler::~Shadow_casterHandler()
{
	// TODO Auto-generated destructor stub
}

Ogre::Technique* Shadow_casterHandler::handleSchemeNotFound(unsigned short schemeIndex, const Ogre::String& schemeName, Ogre::Material* originalMaterial, 
	unsigned short lodIndex, const Ogre::Renderable* rend)
{
	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "------>Shadow_casterHandler::handleSchemeNotFound MaterialName " + originalMaterial->getName());
	Ogre::Technique* tech = originalMaterial->createTechnique();
	Ogre::MaterialPtr materialPtr = Ogre::MaterialManager::getSingleton().getByName("DeferredShading/Shadows/CasterOGRE", "General");

	tech = materialPtr.get()->getTechnique(0);
	//	std::cout<<"sadf"<<tech->getPass(0)->getNumTextureUnitStates()<< "çççççççççççç"<<"\n";

	Ogre::Pass* pass = tech->getPass(0);

	tech->setSchemeName(schemeName);
	//
	if (originalMaterial->getTechnique(0)->getPass(0)->getNumTextureUnitStates() != 0)
	{
		//const GpuProgramParametersSharedPtr& params = tech->getPass(0)->getVertexProgram()->getDefaultParameters();

			//params->setNamedAutoConstant("cTextureMatrix", GpuProgramParameters::ACT_TEXTURE_MATRIX);

		Ogre::TextureUnitState* texunit = originalMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0);

		pass->createTextureUnitState();
		*pass->getTextureUnitState(0) = *texunit;
	}
	//return tech;
	return originalMaterial->getTechnique(0);
	//    	return Ogre::MaterialManager::getSingleton().getByName("DeferredShading/Shadows/Caster","General").get()->getBestTechnique();

}
