#include "NOWAPrecompiled.h"
#include "WorkspaceModule.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/CameraComponent.h"
#include "main/Core.h"
#include "main/AppStateManager.h"
#include "gameObject/WorkspaceComponents.h"
#include "camera/CameraManager.h"
#include "GraphicsModule.h"

#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "OgrePixelFormatGpu.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreLwString.h"

namespace NOWA
{
	WorkspaceModule::WorkspaceModule()
		: hlms(nullptr),
		pbs(nullptr),
		unlit(nullptr),
		hlmsManager(nullptr),
		compositorManager(nullptr),
		shadowFilter(Ogre::HlmsPbs::PCF_4x4),
		ambientLightMode(Ogre::HlmsPbs::AmbientAuto),
		splitScreenScenarioActive(false)
	{
		// Get hlms data
		this->hlms = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
		// assert(dynamic_cast<Ogre::HlmsPbs*>(hlms));
		this->pbs = static_cast<Ogre::HlmsPbs*>(hlms);
		// Attention: Just a Test!
		this->pbs->setDefaultBrdfWithDiffuseFresnel(true);
		// assert(dynamic_cast<Ogre::HlmsUnlit*>(hlms));
		this->unlit = static_cast<Ogre::HlmsUnlit*>(hlms);
		this->hlmsManager = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager();
		this->compositorManager = Core::getSingletonPtr()->getOgreRoot()->getCompositorManager2();
	}

	WorkspaceModule::~WorkspaceModule()
	{

	}

	void WorkspaceModule::destroyContent(void)
	{
		for (auto& it : this->workspaceMap)
		{
			if (it.second.isDummy)
			{
				auto workspace = it.second.workspace;
				auto compositorManager = this->compositorManager;

				ENQUEUE_RENDER_COMMAND_MULTI_WAIT_NO_THIS("RemoveDummyWorkspace", _2(workspace, compositorManager),
				{
					// Optionally unset listener if needed (uncomment if required)
					// workspace->setListener(nullptr);
					// workspace->removeListener();

					compositorManager->removeWorkspace(workspace);
				});
			}
		}

		this->workspaceMap.clear();
		this->splitScreenScenarioActive = false;
	}

	WorkspaceModule* WorkspaceModule::getInstance()
	{
		static WorkspaceModule instance;

		return &instance;
	}

	Ogre::Hlms* WorkspaceModule::getHlms(void) const
	{
		return this->hlms;
	}

	Ogre::HlmsPbs* WorkspaceModule::getHlmsPbs(void) const
	{
		return this->pbs;
	}

	Ogre::HlmsUnlit* WorkspaceModule::getHlmsUnlit(void) const
	{
		return this->unlit;
	}

	Ogre::HlmsManager* WorkspaceModule::getHlmsManager(void) const
	{
		return this->hlmsManager;
	}

	Ogre::CompositorManager2* WorkspaceModule::getCompositorManager(void) const
	{
		return this->compositorManager;
	}

	void WorkspaceModule::setShadowQuality(Ogre::HlmsPbs::ShadowFilter shadowFilter, bool recreateWorkspace)
	{
		ENQUEUE_RENDER_COMMAND_MULTI("WorkspaceModule::setShadowQuality", _1(shadowFilter),
		{
			this->shadowFilter = shadowFilter;
			if (nullptr == this->pbs)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceModule] Cannot set shadow quality, because there is no valid workspace. Please create first a workspace!");
				return;
			}
			//#ifndef _DEBUG
			//		this->pbs->setShadowSettings(shadowFilter);
			//#else
			//		this->pbs->setShadowSettings(Ogre::HlmsPbs::PCF_2x2);
			//#endif

					this->pbs->setShadowSettings(shadowFilter);

					// Causes with Ogre2.2 ugly memory crash with OgreHlmsPbs and mIrradianceVolume
			//		if (Ogre::HlmsPbs::ExponentialShadowMaps == shadowFilter)
			//		{
			//			/** Sets the 'K' parameter of ESM filter. Defaults to 600.
			//            Small values will give weak shadows, and light bleeding (specially if the
			//            caster is close to the receiver; particularly noticeable at contact points).
			//            It also gives the chance of over darkening to appear (the shadow of a small
			//            caster in front of a large caster looks darker; thus the large caster appers
			//            like if it were made of glass instead of being solid).
			//            Large values give strong, dark shadows; but the higher the value, the more
			//            you push floating point limits.
			//            This value is related to K in MiscUtils::setGaussianLogFilterParams. You don't
			//            have to set them to the same value; but you'll notice that if you change this
			//            value here, you'll likely have to change the log filter's too.
			//        @param K
			//            In range (0; infinite).
			//        */
			//// Attention: Make this configurable?
			//			this->pbs->setEsmK(10000);
			//			this->unlit->setEsmK(10000);
			//			this->unlit->setShadowSettings(true); // use exponential shadow map
			//			// "QUALITY WARNING: It is highly recommended that you call mHlmsManager->setShadowMappingUseBackFaces( false ) when using Exponential Shadow Maps (HlmsUnlit::setShadowSettings)
			//			this->hlmsManager->setShadowMappingUseBackFaces(false);
			//		}
			//		else
			//		{
			//			this->pbs->setEsmK(600);
			//			this->unlit->setEsmK(600);
			//			this->unlit->setShadowSettings(false); // use exponential shadow map
			//			// "QUALITY WARNING: It is highly recommended that you call mHlmsManager->setShadowMappingUseBackFaces( false ) when using Exponential Shadow Maps (HlmsUnlit::setShadowSettings)
			//			this->hlmsManager->setShadowMappingUseBackFaces(true);
			//		}

				// When set to exponential, set use back faces false was recommended in function setShadowSettings
				/*if (3 == shadowFilter)
				{
				// Does crash
					this->hlmsManager->setShadowMappingUseBackFaces(false);
				}*/

				if (Ogre::HlmsPbs::ExponentialShadowMaps == shadowFilter)
				{
					/*
					Note: Exponential shadows and outdoors aren't a good fit.
					However sometimes it works well. Assuming this is the case:
					The exact problem is caused by Terra not casting regular shadows; which works fine with most solutions like PCF, but it doesn't with exponential shadow maps dislike large depth discontinuities (which are caused by having no 'floor' in the shadow map)
					To workaround the problems you're having, you could try drawing a very large plane (or multiples ones if you have lots of mountains) below the terrain, casting shadows but invisible to screen (use visibility masks or render queues to have it only cast shadows without being on screen).
					That should solve the exponential shadow map issues; but I can't guarantee exp. shadow maps will look good on exteriors.
					*/

					Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
					Ogre::HlmsCompute* hlmsCompute = hlmsManager->getComputeHlms();

					// hlmsManager->setShadowMappingUseBackFaces(false);

					/*if(hlmsManager->getShadowMappingUseBackFaces())
					{
						Ogre::LogManager::getSingleton().logMessage("[WorkspaceModule]: QUALITY WARNING: It is highly recommended that you call "
									"mHlmsManager->setShadowMappingUseBackFaces( false ) when using Exponential "
									"Shadow Maps (HlmsPbs::setShadowSettings)" );
					}*/

					//For ESM, setup the filter settings (radius and gaussian deviation).
					//It controls how blurry the shadows will look.


					Ogre::uint8 kernelRadius = 8;
					float gaussianDeviationFactor = 0.5f;
					Ogre::uint16 K = 80;
					Ogre::HlmsComputeJob* job = 0;

					//Setup compute shader filter (faster for large kernels; but
					//beware of mobile hardware where compute shaders are slow)
					//For reference large kernels means kernelRadius > 2 (approx)
					job = hlmsCompute->findComputeJob("ESM/GaussianLogFilterH");
					this->setGaussianLogFilterParams(job, kernelRadius, gaussianDeviationFactor, K);
					job = hlmsCompute->findComputeJob("ESM/GaussianLogFilterV");
					this->setGaussianLogFilterParams(job, kernelRadius, gaussianDeviationFactor, K);

					//Setup pixel shader filter (faster for small kernels, also to use as a fallback
					//on GPUs that don't support compute shaders, or where compute shaders are slow).
					this->setGaussianLogFilterParams("ESM/GaussianLogFilterH", kernelRadius, gaussianDeviationFactor, K);
					this->setGaussianLogFilterParams("ESM/GaussianLogFilterV", kernelRadius, gaussianDeviationFactor, K);

					Ogre::RenderSystem* renderSystem = Core::getSingletonPtr()->getOgreRoot()->getRenderSystem();

					const Ogre::RenderSystemCapabilities* capabilities = renderSystem->getCapabilities();
					bool hasCompute = capabilities->hasCapability(Ogre::RSC_COMPUTE_PROGRAM);

					if (!hasCompute)
					{
						//There's no choice.
						this->shadowNodeName = "NOWAESMShadowNodePixelShader";
					}
					else
					{
						this->shadowNodeName = "NOWAESMShadowNodeCompute";
					}
				}
				else
				{
					this->shadowNodeName = "NOWAShadowNode";
				}
		});

		if (true == recreateWorkspace)
		{
			auto workspaceComponent = this->getPrimaryWorkspaceComponent();
			if (nullptr != workspaceComponent)
			{
				workspaceComponent->createWorkspace();
				workspaceComponent->connect();
			}
		}
	}

	Ogre::HlmsPbs::ShadowFilter WorkspaceModule::getShadowQuality(void) const
	{
		return this->shadowFilter;
	}

	void WorkspaceModule::setAmbientLightMode(Ogre::HlmsPbs::AmbientLightMode ambientLightMode)
	{
		this->ambientLightMode = ambientLightMode;
		if (nullptr == this->pbs)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceModule] cannot set ambient light mode, because there is no valid workspace. Please create first a workspace!");
			return;
		}
		ENQUEUE_RENDER_COMMAND_MULTI("WorkspaceModule::setAmbientLightMode", _1(ambientLightMode),
		{
			this->pbs->setAmbientLightMode(ambientLightMode);
		});
	}

	Ogre::HlmsPbs::AmbientLightMode WorkspaceModule::getAmbientLightMode(void) const
	{
		return this->ambientLightMode;
	}

	void WorkspaceModule::setGaussianLogFilterParams(Ogre::HlmsComputeJob* job, Ogre::uint8 kernelRadius, Ogre::Real gaussianDeviationFactor, Ogre::uint16 K)
	{
		assert(!(kernelRadius & 0x01) && "kernelRadius must be even!");

		if (job->getProperty("kernel_radius") != kernelRadius)
			job->setProperty("kernel_radius", kernelRadius);
		if (job->getProperty("K") != K)
			job->setProperty("K", K);

		Ogre::ShaderParams& shaderParams = job->getShaderParams("default");

		std::vector<Ogre::Real> weights(kernelRadius + 1u);

		const Ogre::Real fKernelRadius = kernelRadius;
		const Ogre::Real gaussianDeviation = fKernelRadius * gaussianDeviationFactor;

		//It's 2.0f if using the approximate filter (sampling between two pixels to
		//get the bilinear interpolated result and cut the number of samples in half)
		const Ogre::Real stepSize = 1.0f;

		//Calculate the weights
		Ogre::Real fWeightSum = 0;
		for (Ogre::uint32 i = 0; i < kernelRadius + 1u; ++i)
		{
			const Ogre::Real _X = i - fKernelRadius + (1.0f - 1.0f / stepSize);
			Ogre::Real fWeight = 1.0f / sqrt(2.0f * Ogre::Math::PI * gaussianDeviation * gaussianDeviation);
			fWeight *= exp(-(_X * _X) / (2.0f * gaussianDeviation * gaussianDeviation));

			fWeightSum += fWeight;
			weights[i] = fWeight;
		}

		fWeightSum = fWeightSum * 2.0f - weights[kernelRadius];

		//Normalize the weights
		for (Ogre::uint32 i = 0; i < kernelRadius + 1u; ++i)
			weights[i] /= fWeightSum;

		//Remove shader constants from previous calls (needed in case we've reduced the radius size)
		Ogre::ShaderParams::ParamVec::iterator itor = shaderParams.mParams.begin();
		Ogre::ShaderParams::ParamVec::iterator end = shaderParams.mParams.end();

		while (itor != end)
		{
			Ogre::String::size_type pos = itor->name.find("c_weights[");

			if (pos != Ogre::String::npos)
			{
				itor = shaderParams.mParams.erase(itor);
				end = shaderParams.mParams.end();
			}
			else
			{
				++itor;
			}
		}

		const bool bIsMetal = job->getCreator()->getShaderProfile() == "metal";
		//Set the shader constants, 16 at a time (since that's the limit of what ManualParam can hold)
		char tmp[32];
		Ogre::LwString weightsString(Ogre::LwString::FromEmptyPointer(tmp, sizeof(tmp)));
		const Ogre::uint32 floatsPerParam = sizeof(Ogre::ShaderParams::ManualParam().dataBytes) / sizeof(float);
		for (Ogre::uint32 i = 0; i < kernelRadius + 1u; i += floatsPerParam)
		{
			weightsString.clear();
			if( bIsMetal )
                weightsString.a( "c_weights[", i, "]" );
            else
                weightsString.a( "c_weights[", ( i >> 2u ), "]" );

			Ogre::ShaderParams::Param p;
			p.isAutomatic = false;
			p.isDirty = true;
			p.name = weightsString.c_str();
			shaderParams.mParams.push_back(p);
			Ogre::ShaderParams::Param* param = &shaderParams.mParams.back();

			param->setManualValue(&weights[i], std::min<Ogre::uint32>(floatsPerParam, weights.size() - i));
		}

		shaderParams.setDirty();
	}

	int WorkspaceModule::retrievePreprocessorParameter(const Ogre::String& preprocessDefines, const Ogre::String& paramName)
	{
		size_t startPos = preprocessDefines.find(paramName + '=');
		if (startPos == Ogre::String::npos)
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE,
				"Corrupted material? Expected: " + paramName +
				" but preprocessor defines are: " + preprocessDefines,
				"MiscUtils::retrievePreprocessorParameter");
		}

		startPos += paramName.size() + 1u;

		size_t endPos = preprocessDefines.find_first_of(";,", startPos);

		Ogre::String valuePart = preprocessDefines.substr(startPos, endPos - startPos);
		const int retVal = Ogre::StringConverter::parseInt(valuePart);
		return retVal;
	}

	void WorkspaceModule::setGaussianLogFilterParams(const Ogre::String& materialName, Ogre::uint8 kernelRadius, Ogre::Real gaussianDeviationFactor, Ogre::uint16 K)
	{
		assert(!(kernelRadius & 0x01) && "kernelRadius must be even!");

		Ogre::MaterialPtr material;
		Ogre::GpuProgram* psShader = 0;
		Ogre::GpuProgramParametersSharedPtr oldParams;
		Ogre::Pass* pass = 0;

		material = Ogre::MaterialManager::getSingleton().load(materialName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

		pass = material->getTechnique(0)->getPass(0);
		//Save old manual & auto params
		oldParams = pass->getFragmentProgramParameters();
		//Retrieve the HLSL/GLSL/Metal shader and rebuild it with new kernel radius
		psShader = pass->getFragmentProgram()->_getBindingDelegate();

		Ogre::String preprocessDefines = psShader->getParameter("preprocessor_defines");
		int oldNumWeights = retrievePreprocessorParameter(preprocessDefines, "NUM_WEIGHTS");
		int oldK = retrievePreprocessorParameter(preprocessDefines, "K");
		if (oldNumWeights != (kernelRadius + 1) || oldK != K)
		{
			int horizontalStep = retrievePreprocessorParameter(preprocessDefines, "HORIZONTAL_STEP");
			int verticalStep = retrievePreprocessorParameter(preprocessDefines, "VERTICAL_STEP");

			char tmp[64];
			Ogre::LwString preprocessString(Ogre::LwString::FromEmptyPointer(tmp, sizeof(tmp)));

			preprocessString.a("NUM_WEIGHTS=", kernelRadius + 1u);
			preprocessString.a(",K=", K);
			preprocessString.a(",HORIZONTAL_STEP=", horizontalStep);
			preprocessString.a(",VERTICAL_STEP=", verticalStep);

			psShader->setParameter("preprocessor_defines", preprocessString.c_str());
			pass->getFragmentProgram()->reload();
			//Restore manual & auto params to the newly compiled shader
			pass->getFragmentProgramParameters()->copyConstantsFrom(*oldParams);
		}

		std::vector<Ogre::Real> weights(kernelRadius + 1u);

		const Ogre::Real fKernelRadius = kernelRadius;
		const Ogre::Real gaussianDeviation = fKernelRadius * gaussianDeviationFactor;

		//It's 2.0f if using the approximate filter (sampling between two pixels to
		//get the bilinear interpolated result and cut the number of samples in half)
		const Ogre::Real stepSize = 1.0f;

		//Calculate the weights
		Ogre::Real fWeightSum = 0;
		for (Ogre::uint32 i = 0; i < kernelRadius + 1u; ++i)
		{
			const Ogre::Real _X = i - fKernelRadius + (1.0f - 1.0f / stepSize);
			float fWeight = 1.0f / sqrt(2.0f * Ogre::Math::PI * gaussianDeviation * gaussianDeviation);
			fWeight *= exp(-(_X * _X) / (2.0f * gaussianDeviation * gaussianDeviation));

			fWeightSum += fWeight;
			weights[i] = fWeight;
		}

		fWeightSum = fWeightSum * 2.0f - weights[kernelRadius];

		//Normalize the weights
		for (Ogre::uint32 i = 0; i < kernelRadius + 1u; ++i)
			weights[i] /= fWeightSum;

		Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
		psParams->setNamedConstant("weights", &weights[0], kernelRadius + 1u, 1);
	}

	void WorkspaceModule::setPrimaryWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, WorkspaceBaseComponent* workspaceBaseComponent)
	{
		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			// If there is no external workspace, create a dummy one
			if (nullptr == workspaceBaseComponent)
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::createDummyWorkspace", _3(found, sceneManager, camera),
				{
					found->second.workspace = this->createDummyWorkspace(sceneManager, camera);
					found->second.workspaceBaseComponent = nullptr;
					// Its a dummy workspace
					found->second.isDummy = true;
				});
			}
			else
			{
				// If there is a dummy workspace, it must be first removed
				if (true == found->second.isDummy)
				{
					// cannot be used anymore, still necessary?
					// found->second.workspace->setListener(nullptr);
					// found->second.workspace->removeListener()

					ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::setPrimaryWorkspace removeWorkspace", _1(found),
					{
						this->compositorManager->removeWorkspace(found->second.workspace);
					
						found->second.workspace = nullptr;
						found->second.workspaceBaseComponent->setWorkspace(nullptr);
					});
				}

				found->second.workspaceBaseComponent = workspaceBaseComponent;
				found->second.workspace = workspaceBaseComponent->getWorkspace();
				// If its the main camera, or an active one, set as primary
				unsigned int countCameras = AppStateManager::getSingletonPtr()->getCameraManager()->getCountCameras();
				if (1 == countCameras && AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera() == camera)
				{
					found->second.isPrimary = true;
				}
				else if (workspaceBaseComponent->getOwner()->getId() == GameObjectController::MAIN_CAMERA_ID)
				{
					found->second.isPrimary = true;
				}

				// No dummy workspace
				found->second.isDummy = false;
			}
		}
		else
		{
			// If there is no external workspace, create a dummy one
			if (nullptr == workspaceBaseComponent)
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::createDummyWorkspace", _2(sceneManager, camera),
				{
					WorkspaceData workspaceData;
					workspaceData.workspace = this->createDummyWorkspace(sceneManager, camera);
					workspaceData.isDummy = true;
					this->workspaceMap.emplace(camera, workspaceData);
				});
			}
			else
			{
				// Remove an existing current workspace before another one is added, because only one workspace can be running actively!
				for (auto& it = this->workspaceMap.cbegin(); it != this->workspaceMap.cend(); ++it)
				{
					if (false == it->second.isDummy)
					{
						it->second.workspaceBaseComponent->removeWorkspace();
					}
					else
					{
						ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::setPrimaryWorkspace removeWorkspace", _2(it, found),
						{
							this->compositorManager->removeWorkspace(it->second.workspace);
							found->second.workspace = nullptr;
							// it->second.workspaceBaseComponent->setWorkspace(nullptr);
						});
					}
				}

				WorkspaceData workspaceData;
				workspaceData.workspaceBaseComponent = workspaceBaseComponent;
				workspaceData.workspace = workspaceBaseComponent->getWorkspace();
				workspaceData.isDummy = false;
				// If its the main camera, or an active one, set as primary
				unsigned int countCameras = AppStateManager::getSingletonPtr()->getCameraManager()->getCountCameras();
				if (1 == countCameras && AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera() == camera)
				{
					workspaceData.isPrimary = true;
				}
				else if (workspaceBaseComponent->getOwner()->getId() == GameObjectController::MAIN_CAMERA_ID)
				{
					workspaceData.isPrimary = true;
				}

				this->workspaceMap.emplace(camera, workspaceData);
			}
		}
	}

	void WorkspaceModule::setPrimaryWorkspace2(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::CompositorWorkspace* workspace)
	{
		if (nullptr == workspace)
		{
			return;
		}

		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			// If there is a dummy workspace, it must be first removed
			if (true == found->second.isDummy)
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::setPrimaryWorkspace2", _1(found),
				{
					// cannot be used anymore, still necessary?
					// found->second.workspace->setListener(nullptr);
					// found->second.workspace->removeListener()
					this->compositorManager->removeWorkspace(found->second.workspace);
					found->second.workspace = nullptr;
					found->second.workspaceBaseComponent->setWorkspace(nullptr);
				});
			}

			found->second.workspaceBaseComponent = nullptr;
			found->second.workspace = workspace;
			found->second.isDummy = true;
		}
		else
		{
			// Remove an existing current workspace before another one is added, because only one workspace can be running actively!
			for (auto& it = this->workspaceMap.cbegin(); it != this->workspaceMap.cend(); ++it)
			{
				if (false == it->second.isDummy)
				{
					it->second.workspaceBaseComponent->removeWorkspace();
				}
				else
				{
					ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::setPrimaryWorkspace2 removeWorkspace", _2(it, found),
					{
						this->compositorManager->removeWorkspace(it->second.workspace);
						found->second.workspace = nullptr;
						// it->second.workspaceBaseComponent->setWorkspace(nullptr);
					});
				}
			}

			WorkspaceData workspaceData;
			workspaceData.workspaceBaseComponent = nullptr;
			workspaceData.workspace = workspace;
			workspaceData.isDummy = true;

			this->workspaceMap.emplace(camera, workspaceData);
		}
	}

	void WorkspaceModule::addNthWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, WorkspaceBaseComponent* workspaceBaseComponent)
	{
		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			// If there is no external workspace, create a dummy one
			if (nullptr == workspaceBaseComponent)
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::createDummyWorkspace", _3(found, sceneManager, camera),
				{
					found->second.workspace = this->createDummyWorkspace(sceneManager, camera);
					found->second.workspaceBaseComponent = nullptr;
					// Its a dummy workspace
					found->second.isDummy = true;
				});
			}
			else
			{
				found->second.workspaceBaseComponent = workspaceBaseComponent;
				found->second.workspace = workspaceBaseComponent->getWorkspace();

				// No dummy workspace
				found->second.isDummy = false;
			}
		}
		else
		{
			// If there is no external workspace, create a dummy one
			if (nullptr == workspaceBaseComponent)
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::createDummyWorkspace", _2(sceneManager, camera),
				{
					WorkspaceData workspaceData;
					workspaceData.workspace = this->createDummyWorkspace(sceneManager, camera);
					workspaceData.isDummy = true;

					this->workspaceMap.emplace(camera, workspaceData);
				});
			}
			else
			{
				WorkspaceData workspaceData;
				workspaceData.workspaceBaseComponent = workspaceBaseComponent;
				workspaceData.workspace = workspaceBaseComponent->getWorkspace();
				workspaceData.isDummy = false;

				this->workspaceMap.emplace(camera, workspaceData);
			}
		}
	}

	Ogre::CompositorWorkspace* WorkspaceModule::getPrimaryWorkspace(Ogre::Camera* camera)
	{
		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			if (true == found->second.isPrimary)
			{
				return found->second.workspace;
			}
		}
		return nullptr;
	}

	WorkspaceBaseComponent* WorkspaceModule::getPrimaryWorkspaceComponent(void)
	{
		Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			if (true == found->second.isPrimary)
			{
				return found->second.workspaceBaseComponent;
			}
		}
		
		/*Ogre::String message = "[WorkspaceModule] Error: Could not get primary workspace component, because the camera: '" 
			+ camera->getName() + "' does not belong to the workspace map. Thus further rendering is not possible!\n"
			"Please make sure when calling 'setPrimaryWorkspace', that the correct camera is set!";
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
		throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, message + "\n", "NOWA");*/
		return nullptr;
	}

	void WorkspaceModule::setCameraPrimaryWorkspaceActive(void)
	{
		Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			if (true == found->second.isPrimary)
			{
				AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(camera, true);
			}
			else
			{
				AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(camera, false);
			}
		}
	}

	Ogre::CompositorWorkspace* WorkspaceModule::getWorkspace(Ogre::Camera* camera)
	{
		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			return found->second.workspace;
		}
		return nullptr;
	}

	WorkspaceBaseComponent* WorkspaceModule::getWorkspaceComponent(void)
	{
		Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			return found->second.workspaceBaseComponent;
		}

		/*Ogre::String message = "[WorkspaceModule] Error: Could not get primary workspace component, because the camera: '"
			+ camera->getName() + "' does not belong to the workspace map. Thus further rendering is not possible!\n"
			"Please make sure when calling 'setPrimaryWorkspace', that the correct camera is set!";
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
		throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, message + "\n", "NOWA");*/
		return nullptr;
	}

	CameraComponent* WorkspaceModule::getPrimaryCameraComponent(void) const
	{
		auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(CameraComponent::getStaticClassName());

		for (auto& it = this->workspaceMap.cbegin(); it != this->workspaceMap.cend(); ++it)
		{
			// Main camera should be the first one
			for (size_t i = 0; i < gameObjects.size(); i++)
			{
				const auto& gameObjectPtr = gameObjects[i];
				auto cameraCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<CameraComponent>());
				if (nullptr != cameraCompPtr)
				{
					if (cameraCompPtr->getCamera() == it->first && true == it->second.isPrimary)
					{
						return cameraCompPtr.get();
					}
				}
			}
		}
		return nullptr;
	}

	void WorkspaceModule::removeWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera)
	{
		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			// If there is a dummy workspace, it must be first remove
			if (true == found->second.isDummy)
			{
				// cannot be used anymore, still necessary?
				// found->second.workspace->setListener(nullptr);
				// found->second.workspace->removeListener()
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::removeWorkspace", _1(found),
				{
					this->compositorManager->removeWorkspace(found->second.workspace);
					found->second.workspace = nullptr;
				});
			}

			if (this->workspaceMap.size() == 1)
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::createDummyWorkspace", _3(found, sceneManager, camera),
				{
					found->second.workspace = this->createDummyWorkspace(sceneManager, camera);
					found->second.workspaceBaseComponent = nullptr;
					// Its a dummy workspace
					found->second.isDummy = true;
				});
			}

			this->workspaceMap.erase(camera);
		}
	}

	void WorkspaceModule::removeCamera(Ogre::Camera* camera)
	{
		auto found = this->workspaceMap.find(camera);
		if (found != this->workspaceMap.cend())
		{
			// If its a dummy workspace delete the workspace here
			if (true == found->second.isDummy)
			{
				// cannot be used anymore, still necessary?
				// found->second.workspace->setListener(nullptr);
				// found->second.workspace->removeListener()
				if (nullptr != found->second.workspace)
				{
					ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceModule::createDummyWorkspace", _1(found),
					{
						this->compositorManager->removeWorkspace(found->second.workspace);
						found->second.workspace = nullptr;
					});
				}
			}

			this->workspaceMap.erase(camera);
		}
	}

	bool WorkspaceModule::hasAnyWorkspace(void) const
	{
		return !this->workspaceMap.empty();
	}

	bool WorkspaceModule::hasMoreThanOneWorkspace(void) const
	{
		return this->workspaceMap.size() > 1;
	}

	Ogre::uint8 WorkspaceModule::getCountCameras(void)
	{
		Ogre::uint8 count = 0;
		auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(CameraComponent::getStaticClassName());

		for (size_t i = 0; i < gameObjects.size(); i++)
		{
			const auto& gameObjectPtr = gameObjects[i];
			auto cameraCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<CameraComponent>());
			if (nullptr != cameraCompPtr)
			{
				count++;
			}
		}
		return count;
	}

	void WorkspaceModule::setSplitScreenScenarioActive(bool splitScreenScenarioActive)
	{
		this->splitScreenScenarioActive = splitScreenScenarioActive;
	}

	bool WorkspaceModule::getSplitScreenScenarioActive(void) const
	{
		return this->splitScreenScenarioActive;
	}

	Ogre::CompositorWorkspace* WorkspaceModule::createDummyWorkspace(Ogre::SceneManager* sceneManager, Ogre::Camera* camera)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceModule] Creating dummy workspace");

		for (auto& it = this->workspaceMap.begin(); it != this->workspaceMap.end(); ++it)
		{
			if (false == it->second.isDummy)
			{
				it->second.workspaceBaseComponent->removeWorkspace();
			}
			else
			{
				if (nullptr != it->second.workspace)
				{
					this->compositorManager->removeWorkspace(it->second.workspace);
					it->second.workspace = nullptr;
				}
				
				// it->second.workspaceBaseComponent->setWorkspace(nullptr);
			}
		}

		// Remove an existing current workspace before another one is added, because only one workspace can be running actively!
		/*if (nullptr != this->currentlyActiveWorkspace)
		{
			this->compositorManager->removeWorkspace(this->currentlyActiveWorkspace);
		}*/

		return this->compositorManager->addWorkspace(sceneManager, Core::getSingletonPtr()->getOgreRenderWindow()->getTexture(), camera, "NOWAPbsWorkspace", true);
	}

} // namespace end