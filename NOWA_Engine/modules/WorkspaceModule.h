#ifndef WORKSPACE_MODULE_H
#define WORKSPACE_MODULE_H

#include "defines.h"

namespace NOWA
{
	
	class WorkspaceBaseComponent;

	class EXPORTED WorkspaceModule
	{
	public:
		WorkspaceModule* getPrimaryWorkspaceModule(void);

		WorkspaceModule* getAdditionalWorkspaceModule(Ogre::Camera* camera);

		void destroyContent(void);

		Ogre::Hlms* getHlms(void) const;

		Ogre::HlmsPbs* getHlmsPbs(void) const;

		Ogre::HlmsUnlit* getHlmsUnlit(void) const;

		Ogre::HlmsManager* getHlmsManager(void) const;

		Ogre::CompositorManager2* getCompositorManager(void) const;

		/*
		 * @brief Sets the shadow quality filter.
		 * @param[in] shadowFilter The shadow filter to set. Possible values:
		 * 	PCF_2x2: Standard quality. Very fast.
		 * 	PCF_3x3: Good quality. Still quite fast on most modern hardware.
		 * 	PCF_4x4: High quality. Very slow in old hardware (i.e. DX10 level hw and below). Use RSC_TEXTURE_GATHER to check whether it will be slow or not.
		 * 	ExponentialShadowMaps: High quality. Produces soft shadows. It's much more expensive but given its blurry results, you can reduce resolution and/or use less PSSM splits
		 * 	which gives you very competing performance with great results. ESM stands for Exponential Shadow Maps.
		 * @param[in] recreateWorkspace Whether the workspace should be recreated (should be set to true, when shadows quality is changed during runtime).
		 */
		void setShadowQuality(Ogre::HlmsPbs::ShadowFilter shadowFilter, bool recreateWorkspace);

		Ogre::HlmsPbs::ShadowFilter getShadowQuality(void) const;

		/*
		 * @brief Sets the ambient light mode.
		 * @param[in] ambientLightMode The ambient light mode. Possible values:
		 * 	AmbientAuto: Use fixed-color ambient lighting when upper hemisphere = lower hemisphere, use hemisphere lighting when they don't match. Disables ambient lighting if the colours are black
		 * 	AmbientFixed: Force fixed-color ambient light. Only uses the upper hemisphere paramter.
		 * 	AmbientHemisphere: Force hemisphere ambient light. Useful if you plan on adjusting the colours dynamically very often and this might cause swapping shaders.
		 * 	AmbientNone: Disable ambient lighting.
		 */

		void setAmbientLightMode(Ogre::HlmsPbs::AmbientLightMode ambientLightMode);

		Ogre::HlmsPbs::AmbientLightMode getAmbientLightMode(void) const;

		void setPrimaryWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, WorkspaceBaseComponent* workspaceBaseComponent);

		void addNthWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, WorkspaceBaseComponent* workspaceBaseComponent);

		Ogre::CompositorWorkspace* getPrimaryWorkspace(Ogre::Camera* camera);

		WorkspaceBaseComponent* getPrimaryWorkspaceComponent(void);

		Ogre::CompositorWorkspace* getWorkspace(Ogre::Camera* camera);

		WorkspaceBaseComponent* getWorkspaceComponent(void);

		void removeWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera);

		void removeCamera(Ogre::Camera* camera);

		bool hasAnyWorkspace(void) const;

		bool hasMoreThanOneWorkspace(void) const;

		void setUseSplitScreen(bool useSplitScreen);

		bool getUseSplitScreen(void) const;

		Ogre::uint8 getLastExecutionMask(void) const;

		Ogre::uint8 getCountCameras(void);
	public:
		static WorkspaceModule* getInstance();
		const Ogre::String workspaceNamePbs = "NOWAPbsWorkspace";
		const Ogre::String workspaceNameSky = "NOWASkyWorkspace";
		const Ogre::String workspaceNameSkyReflection = "NOWASkyReflectionWorkspace";
		const Ogre::String workspaceNamePbsPlanarReflection = "NOWAPbsPlanarReflectionWorkspace";
		const Ogre::String workspaceNameSkyPlanarReflection = "NOWASkyPlanarReflectionWorkspace";
		const Ogre::String workspaceNameBackgroundPlanarReflection = "NOWABackgroundPlanarReflectionWorkspace";
		const Ogre::String workspaceNameTerra = "NOWATerraWorkspace";
		const Ogre::String workspaceNameBackground = "NOWABackgroundWorkspace";
		Ogre::String shadowNodeName = "NOWAShadowNode";
	private:
		WorkspaceModule();
		~WorkspaceModule();

		//////////////////////////ESM Shadow optimizations////////////////////////////////////////
		int retrievePreprocessorParameter(const Ogre::String& preprocessDefines, const Ogre::String& paramName);
		/**
		@param job
			The compute job to change.
		@param kernelRadius
			The kernel radius. A radius of 8 means there's 17x17 taps (8x2 + 1).
			Changing this parameter will trigger a recompile.
		@param gaussianDeviationFactor
			The std. deviation of a gaussian filter.
		@param K
			A big K (K > 20) means softer, blurrier shadows; but may reveal some artifacts on the
			edge cases ESM does not deal well, while small K (< 8) may hide those artifacts,
			but makes the shadows fat (fatter than normal, still blurry) and very dark.
			Small K combined with large radius (radius > 8) may cause self shadowing artifacts.
			A K larger than 80 may run into floating point precision issues (which means light
			bleeding or weird shadows appearing caused by NaNs).
			Changing this parameter will trigger a recompile.
		*/
		void setGaussianLogFilterParams(Ogre::HlmsComputeJob* job, Ogre::uint8 kernelRadius, Ogre::Real gaussianDeviationFactor, Ogre::uint16 K);

		/// Adjusts the material (pixel shader variation). See other overload for param description.
		void setGaussianLogFilterParams(const Ogre::String& materialName, Ogre::uint8 kernelRadius, Ogre::Real gaussianDeviationFactor, Ogre::uint16 K);

		Ogre::CompositorWorkspace* createDummyWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera);
	private:
		Ogre::Hlms* hlms;
		Ogre::HlmsPbs* pbs;
		Ogre::HlmsUnlit* unlit;

		Ogre::HlmsManager* hlmsManager;
		Ogre::CompositorManager2* compositorManager;
		Ogre::HlmsPbs::ShadowFilter shadowFilter;
		Ogre::HlmsPbs::AmbientLightMode ambientLightMode;

		bool useSplitScreen;
		Ogre::uint8 executionMask;
		Ogre::uint8 viewportModifierMask;

		struct WorkspaceData
		{
			Ogre::CompositorWorkspace* workspace = nullptr;
			WorkspaceBaseComponent* workspaceBaseComponent = nullptr;
			bool isDummy = false;
			bool isPrimary = false;
		};

		std::map<Ogre::Camera*, WorkspaceData> workspaceMap;
	};

}; //namespace end

#endif
