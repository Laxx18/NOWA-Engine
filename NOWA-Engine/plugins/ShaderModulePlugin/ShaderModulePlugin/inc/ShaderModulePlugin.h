#ifndef SHADER_MODULE_PLUGIN_H
#define SHADER_MODULE_PLUGIN_H

//Shadersystem
#include <OgreRTShaderSystem.h>
#include <OGRE/OgrePlugin.h>
#include <defines.h>
#include "ShaderGeneratorTechniqueResolverListener.h"

namespace OBA
{

class EXPORTED ShaderModulePlugin : public Ogre::Plugin, Ogre::Singleton<ShaderModulePlugin>
{
public:
	ShaderModulePlugin();
	virtual ~ShaderModulePlugin();
	//calculate the tangets for normalmap shader
	//Param
	//		entity: The Entity to calculate the tangents
	void substractOutTangentsForShader(Ogre::Entity *entity);
	
	/*
	RTShaderSystem initialisieren
	Paramater:
	sceneManager: SceneManager auf welchen die Shader ausgefuerht werden sollen
	viewport: Viewport der die Shader anwendet
	usePerPixelLightning: Pixel Lichtbeleuchtung nutzen
	useNormalMapping: NormalMapping nutzen
	useFog: Nebel nutzen
	useShadows: Schatten nutzen
	*/
	void initializeRTShaderSystem(Ogre::SceneManager *sceneManager, Ogre::Viewport *viewport, bool usePerPixelLightning, bool useNormalMapping, bool useFog, bool useShadows);
	void deleteRTShaderSystem(void);
	void generateRTShaders(Ogre::SubEntity *pSubEntity, const Ogre::String &strNormalMap);
	void applyShadowType(Ogre::SceneManager *sceneManager, Ogre::Camera *camera, Ogre::Real viewRange);
	void applyPerPixelFog(Ogre::SceneManager *sceneManager);
	Ogre::MaterialPtr buildDepthShadowMaterial(const Ogre::String& materialName);
	void configureShadows(Ogre::SceneManager *sceneManager, Ogre::Camera *camera, bool enabled, Ogre::Real viewRange);
	bool getUseRTShaderSystem(void) const;
	bool getUseShadows(void) const;
	bool getUsePerPixelLightning(void) const;
	bool getUseNormalMapping(void) const;
	bool getUseFog(void) const;
	//Override
	const Ogre::String& getName() const;
	//Override
	void install();
	//Override
	void initialise();
	//Override
	void shutdown();
	//Override
	void uninstall();
	//Override
	static ShaderModulePlugin& getSingleton(void);
	//Override
	static ShaderModulePlugin* getSingletonPtr(void);
public:
	Ogre::ShadowCameraSetupPtr					pPSSMSetup;
private:
	Ogre::RTShader::ShaderGenerator*			pRTShaderGenerator;
	ShaderGeneratorTechniqueResolverListener*	pRTShaderMaterialListener;
	bool										useRTShaderSystem;
	bool										usePerPixelLightning;
	bool										useNormalMapping;
	bool										useFog;
	bool										useShadows;
	Ogre::String								name;
	
};

}; //namespace end

#ifdef WIN32
template<> OBA::ShaderModulePlugin* Ogre::Singleton<OBA::ShaderModulePlugin>::msSingleton = 0;
#endif

#endif
