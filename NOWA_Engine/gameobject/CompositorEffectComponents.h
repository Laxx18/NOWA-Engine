#ifndef COMPOSITOR_EFFECT_COMPONENTS_H
#define COMPOSITOR_EFFECT_COMPONENTS_H

#include "GameObjectComponent.h"
#include <array>
#include "main/Events.h"

namespace NOWA
{
	class EXPORTED CompositorEffectBaseComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectBaseComponent> CompositorEffectBaseCompPtr;
	public:
	
		friend class WorkspaceBaseComponent;

		CompositorEffectBaseComponent();

		virtual ~CompositorEffectBaseComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) = 0;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectBaseComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectBaseComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) { }

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: This component must be placed somewhere under a CameraComponent.";
		}

		/**
		 * @brief	Activates this compositor, so that it will be played during simulation.
		 * @param[in] activated Whether to activate this compositor effect or not
		 */
		virtual void setActivated(bool activated) override;

		virtual bool isActivated(void) const override;

		/**
		 * @brief Sets workspace game object id in order play the effect for that workspace.
		 * @param[in] workspaceGameObjectId The workspaceGameObjectId to set
		 */
		void setWorkspaceGameObjectId(unsigned long workspaceGameObjectId);

		/**
		 * @brief Gets the workspace game object id in order play the effect for that workspace.
		 * @return workspaceGameObjectId The workspaceGameObjectId to get
		 */
		unsigned long getWorkspaceGameObjectId(void) const;

	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrWorkspaceGameObjectId(void) { return "Workspace GameObject Id"; }
	protected:
		/**
		 * @brief	Shows and adds the desired effect at real time.
		 * @param[in] effectName	The effect name to show
		 * @note When the function is called with the same effect again, it will be disabled. Possible effects:
		 * - "Bloom"
		 * - "Glass"
		 * - "Old TV"
		 * - "B&W"
		 * - "Embossed"
		 * - "Sharpen Edges"
		 * - "Invert"
		 * - "Posterize"
		 * - "Laplace"
		 * - "Tiling"
		 * - "Old Movie"
		 * - "Radial Blur"
		 * - "ASCII"
		 * - "Halftone"
		 * - "Night Vision"
		 * - "Dither"
		 */
		void enableEffect(const Ogre::String& effectName, bool enabled);
	private:
			void handleWorkspaceComponentDeleted(NOWA::EventDataPtr eventData);
	protected:
		Variant* activated;
		Variant* workspaceGameObjectId;
		Ogre::String effectName;
		WorkspaceBaseComponent* workspaceBaseComponent;
        Ogre::Camera* camera;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectBloomComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectBloomComponent> CompositorEffectBloomCompPtr;
	public:

		CompositorEffectBloomComponent();

		virtual ~CompositorEffectBloomComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectBloomComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectBloomComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: This component must be placed somewhere under a CameraComponent.";
		}

		void setImageWeight(Ogre::Real imageWeight);

		Ogre::Real getImageWeight(void) const;

		void setBlurWeight(Ogre::Real blurWeight);

		Ogre::Real getBlurWeight(void) const;

	public:
		static const Ogre::String AttrImageWeight(void) { return "Image Weight"; }
		static const Ogre::String AttrBlurWeight(void) { return "Blur Weight"; }
	private:
		Variant* imageWeight;
		Variant* blurWeight;
		std::array<Ogre::MaterialPtr, 4> materials;
		std::array<Ogre::Pass*, 4> passes;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectGlassComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectGlassComponent> CompositorEffectGlassCompPtr;
	public:

		CompositorEffectGlassComponent();

		virtual ~CompositorEffectGlassComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectGlassComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectGlassComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: This component must be placed somewhere under a CameraComponent.";
		}

		void setGlassWeight(Ogre::Real imageWeight);

		Ogre::Real getGlassWeight(void) const;

		// void setBlurWeight(Ogre::Real blurWeight);

		// Ogre::Real getBlurWeight(void) const;

	public:
		static const Ogre::String AttrGlassWeight(void) { return "Glass Weight"; }
		// static const Ogre::String AttrBlurWeight(void) { return "Blur Weight"; }
	private:
		Variant* glassWeight;
		// Variant* blurWeight;
		Ogre::MaterialPtr material;
		Ogre::Pass* pass;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectOldTvComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectOldTvComponent> CompositorEffectOldTvCompPtr;
	public:

		CompositorEffectOldTvComponent();

		virtual ~CompositorEffectOldTvComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectOldTvComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectOldTvComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

		void setDistortionFrequency(Ogre::Real distortionFrequency);

		Ogre::Real getDistortionFrequency(void) const;

		void setDistortionScale(Ogre::Real distortionScale);

		Ogre::Real getDistortionScale(void) const;

		void setDistortionRoll(Ogre::Real distortionRoll);

		Ogre::Real getDistortionRoll(void) const;

		void setInterference(Ogre::Real interference);

		Ogre::Real getInterference(void) const;

		void setFrameLimit(Ogre::Real frameLimit);

		Ogre::Real getFrameLimit(void) const;

		void setFrameShape(Ogre::Real frameShape);

		Ogre::Real getFrameShape(void) const;

		void setFrameSharpness(Ogre::Real frameSharpness);

		Ogre::Real getFrameSharpness(void) const;

		void setTime(unsigned int time);

		unsigned int getTime(void) const;

		void setSinusTime(unsigned int sinusTime);

		unsigned int getSinusTime(void) const;

	public:
		static const Ogre::String AttrDistortionFrequency(void) { return "Distortion Frequency"; }
		static const Ogre::String AttrDistortionScale(void) { return "Distortion Scale"; }
		static const Ogre::String AttrDistortionRoll(void) { return "Distortion Roll"; }
		static const Ogre::String AttrInterference(void) { return "Interference"; }
		static const Ogre::String AttrFrameLimit(void) { return "Frame Limit"; }
		static const Ogre::String AttrFrameShape(void) { return "Frame Shape"; }
		static const Ogre::String AttrFrameSharpness(void) { return "Frame Sharpness"; }
		static const Ogre::String AttrTime(void) { return "Time"; }
		static const Ogre::String AttrSinusTime(void) { return "Sinus Time"; }
	private:
		Variant* distortionFrequency;
		Variant* distortionScale;
		Variant* distortionRoll;
		Variant* interference;
		Variant* frameLimit;
		Variant* frameShape;
		Variant* frameSharpness;
		Variant* time;
		Variant* sinusTime;

		Ogre::MaterialPtr material;
		Ogre::Pass*pass;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectBlackAndWhiteComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectBlackAndWhiteComponent> CompositorEffectBlackAndWhiteCompPtr;
	public:

		CompositorEffectBlackAndWhiteComponent();

		virtual ~CompositorEffectBlackAndWhiteComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectBlackAndWhiteComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectBlackAndWhiteComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

		void setColor(const Ogre::Vector3& color);

		Ogre::Vector3 getColor(void) const;

	public:
		static const Ogre::String AttrColor(void) { return "Color"; }
	private:
		Variant* color;
		Ogre::MaterialPtr material;
		Ogre::Pass*pass;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectColorComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectColorComponent> CompositorEffectColorCompPtr;
	public:

		CompositorEffectColorComponent();

		virtual ~CompositorEffectColorComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectColorComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectColorComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

		void setColor(const Ogre::Vector3& color);

		Ogre::Vector3 getColor(void) const;

	public:
		static const Ogre::String AttrColor(void) { return "Color"; }
	private:
		Variant* color;
		Ogre::MaterialPtr material;
		Ogre::Pass*pass;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectEmbossedComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectEmbossedComponent> CompositorEffectEmbossedCompPtr;
	public:

		CompositorEffectEmbossedComponent();

		virtual ~CompositorEffectEmbossedComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectEmbossedComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectEmbossedComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

		void setWeight(const Ogre::Real& weight);

		Ogre::Real getWeight(void) const;

	public:
		static const Ogre::String AttrWeight(void) { return "Weight"; }
	private:
		Variant* weight;
		Ogre::MaterialPtr material;
		Ogre::Pass*pass;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectSharpenEdgesComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectSharpenEdgesComponent> CompositorEffectSharpenEdgesCompPtr;
	public:

		CompositorEffectSharpenEdgesComponent();

		virtual ~CompositorEffectSharpenEdgesComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectSharpenEdgesComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectSharpenEdgesComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

		void setWeight(const Ogre::Real& weight);

		Ogre::Real getWeight(void) const;

	public:
		static const Ogre::String AttrWeight(void) { return "Weight"; }
	private:
		Variant* weight;
		Ogre::MaterialPtr material;
		Ogre::Pass*pass;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectDepthOfFieldComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectDepthOfFieldComponent> CompositorEffectDepthOfFieldCompPtr;
	public:

		CompositorEffectDepthOfFieldComponent();

		virtual ~CompositorEffectDepthOfFieldComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectDepthOfFieldComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectDepthOfFieldComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) {}

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

	public:
		static const Ogre::String AttrWeight(void) { return "Weight"; }
	private:
		Ogre::MaterialPtr material;
		Ogre::Pass* pass;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectHeightFogComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectHeightFogComponent> CompositorEffectHeightFogCompPtr;
	public:

		CompositorEffectHeightFogComponent();

		virtual ~CompositorEffectHeightFogComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectHeightFogComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectHeightFogComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) {}

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

	public:
		static const Ogre::String AttrWeight(void) { return "Weight"; }
	private:
		Ogre::MaterialPtr material;
		Ogre::Pass* pass;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CompositorEffectFogComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectFogComponent> CompositorEffectFogCompPtr;
	public:

		CompositorEffectFogComponent();

		virtual ~CompositorEffectFogComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectFogComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectFogComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) {}

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

	public:
		static const Ogre::String AttrWeight(void) { return "Weight"; }
	private:
		Ogre::MaterialPtr material;
		Ogre::Pass* pass;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	class EXPORTED CompositorEffectLightShaftsComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectLightShaftsComponent> CompositorEffectLightShaftsCompPtr;
	public:

		CompositorEffectLightShaftsComponent();

		virtual ~CompositorEffectLightShaftsComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("CompositorEffectLightShaftsComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectLightShaftsComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass) {}

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

	public:
		static const Ogre::String AttrWeight(void) { return "Weight"; }
	private:
		Ogre::MaterialPtr material;
		Ogre::Pass* pass;
	};

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
        * @brief   Two-pass cartoon / cel-shading post-process compositor effect component.
        *
        * Pass 1 (Postprocess/CartoonEdge):
        *   Sobel edge detection on the scene colour buffer → grayscale edge mask.
        *
        * Pass 2 (Postprocess/CartoonColor):
        *   Colour quantisation into discrete cel bands, saturation boost, and
        *   edge darkening using the mask from pass 1.
        *
        * Requirements: A camera component must exist.
        */
    class EXPORTED CompositorEffectCartoonComponent : public CompositorEffectBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::CompositorEffectCartoonComponent> CompositorEffectCartoonCompPtr;

    public:
        CompositorEffectCartoonComponent();

        virtual ~CompositorEffectCartoonComponent();

        /**
            * @see     GameObjectComponent::init
            */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
            * @see     GameObjectComponent::postInit
            */
        virtual bool postInit(void) override;

        /**
            * @see     GameObjectComponent::getClassName
            */
        virtual Ogre::String getClassName(void) const override;

        /**
            * @see     GameObjectComponent::getParentClassName
            */
        virtual Ogre::String getParentClassName(void) const override;

        /**
            * @see     GameObjectComponent::clone
            */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("CompositorEffectCartoonComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "CompositorEffectCartoonComponent";
        }

        /**
            * @see  GameObjectComponent::createStaticApiForLua
            */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
            luabind::module(lua)[luabind::class_<CompositorEffectCartoonComponent, GameObjectComponent>("CompositorEffectCartoonComponent")
                    .def("setEdgeThreshold", &CompositorEffectCartoonComponent::setEdgeThreshold)
                    .def("getEdgeThreshold", &CompositorEffectCartoonComponent::getEdgeThreshold)
                    .def("setEdgeStrength", &CompositorEffectCartoonComponent::setEdgeStrength)
                    .def("getEdgeStrength", &CompositorEffectCartoonComponent::getEdgeStrength)
                    .def("setNumBands", &CompositorEffectCartoonComponent::setNumBands)
                    .def("getNumBands", &CompositorEffectCartoonComponent::getNumBands)
                    .def("setSaturation", &CompositorEffectCartoonComponent::setSaturation)
                    .def("getSaturation", &CompositorEffectCartoonComponent::getSaturation)
                    .def("setEdgeDarkness", &CompositorEffectCartoonComponent::setEdgeDarkness)
                    .def("getEdgeDarkness", &CompositorEffectCartoonComponent::getEdgeDarkness)];
        }

        /**
            * @see     GameObjectComponent::actualizeValue
            */
        virtual void actualizeValue(Variant* attribute) override;

        /**
            * @see     GameObjectComponent::writeXML
            */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
            * @see     GameObjectComponent::getStaticInfoText
            */
        static Ogre::String getStaticInfoText(void)
        {
            return "Requirements: A camera component must exist. "
                    "Two-pass cartoon effect: Sobel edge detection + cel-shade colour quantisation.";
        }

        // -----------------------------------------------------------------------
        // CartoonEdge pass parameters
        // -----------------------------------------------------------------------

        /**
            * @brief   Minimum Sobel gradient magnitude to register as an edge.
            *          Lower values detect more (thinner/fainter) edges. Range: [0..1], default 0.1.
            */
        void setEdgeThreshold(Ogre::Real edgeThreshold);

        Ogre::Real getEdgeThreshold(void) const;

        /**
            * @brief   Intensity multiplier applied to detected edges before writing the mask.
            *          Values > 1 make edges more prominent. Default 1.0.
            */
        void setEdgeStrength(Ogre::Real edgeStrength);

        Ogre::Real getEdgeStrength(void) const;

        // -----------------------------------------------------------------------
        // CartoonColor pass parameters
        // -----------------------------------------------------------------------

        /**
            * @brief   Number of discrete colour levels per channel (cel bands).
            *          Fewer bands = harder, more stylised look. Default 4.0.
            */
        void setNumBands(Ogre::Real numBands);

        Ogre::Real getNumBands(void) const;

        /**
            * @brief   Colour saturation multiplier. Values > 1 produce more vivid cartoon colours.
            *          Default 1.4.
            */
        void setSaturation(Ogre::Real saturation);

        Ogre::Real getSaturation(void) const;

        /**
            * @brief   Brightness of outline pixels. 0 = pure black outlines, 1 = no darkening.
            *          Default 0.0.
            */
        void setEdgeDarkness(Ogre::Real edgeDarkness);

        Ogre::Real getEdgeDarkness(void) const;

    public:
        // Attribute name constants
        static const Ogre::String AttrEdgeThreshold(void)
        {
            return "Edge Threshold";
        }
        static const Ogre::String AttrEdgeStrength(void)
        {
            return "Edge Strength";
        }
        static const Ogre::String AttrNumBands(void)
        {
            return "Num Bands";
        }
        static const Ogre::String AttrSaturation(void)
        {
            return "Saturation";
        }
        static const Ogre::String AttrEdgeDarkness(void)
        {
            return "Edge Darkness";
        }

    private:
        // Pass 1: Postprocess/CartoonEdge
        Ogre::MaterialPtr edgeMaterial;
        Ogre::Pass* edgePass;

        // Pass 2: Postprocess/CartoonColor
        Ogre::MaterialPtr colorMaterial;
        Ogre::Pass* colorPass;

        // Variants (editor / serialisation)
        Variant* edgeThreshold;
        Variant* edgeStrength;
        Variant* numBands;
        Variant* saturation;
        Variant* edgeDarkness;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
     * @brief   Screen-space volumetric light / god-ray post-process compositor effect.
     *
     * Uses the classic Crytek / Kawase three-pass pipeline:
     *
     *   Pass 1  Sun Mask  — extracts bright pixels near the sun's screen position.
     *   Pass 2  Radial Blur — walks each pixel toward the sun accumulating with decay.
     *   Pass 3  Blend     — additively composites the rays onto the scene with a tint.
     *
     * The sun's screen-space position is derived each frame by projecting the
     * linked directional light's derived world direction through the camera's
     * view-projection matrix.  No depth texture is required — zero workspace
     * wiring changes are needed beyond the existing Ogre/Postprocess mechanism.
     *
     * Requirements: A camera component and at least one LightDirectionalComponent
     *               must exist in the scene.  If no light ID is set the component
     *               falls back to GameObjectController::MAIN_LIGHT_ID.
     */
    class EXPORTED CompositorEffectVolumetricLightComponent : public CompositorEffectBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::CompositorEffectVolumetricLightComponent> CompositorEffectVolumetricLightCompPtr;

    public:
        CompositorEffectVolumetricLightComponent();
        virtual ~CompositorEffectVolumetricLightComponent();

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual void onRemoveComponent(void) override;

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("CompositorEffectVolumetricLightComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "CompositorEffectVolumetricLightComponent";
        }

        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @brief   Per-frame update: projects the sun direction to screen space and
         *          pushes it to both the sun-mask and radial-blur shader passes.
         *          This runs every frame regardless of the notSimulating flag so that
         *          god rays follow the sun even in the editor.
         */
        virtual void update(Ogre::Real dt, bool notSimulating) override;

        static Ogre::String getStaticInfoText(void)
        {
            return "Requirements: A camera component and a LightDirectionalComponent must exist. "
                   "Screen-space god rays using the Crytek/Kawase three-pass technique. "
                   "No depth texture required.";
        }

		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        // -----------------------------------------------------------------------
        // Light source
        // -----------------------------------------------------------------------
        /**
         * @brief   Sets the game object ID of the directional light that acts as the sun.
         *          Pass 0 to fall back to GameObjectController::MAIN_LIGHT_ID.
         */
        void setLightId(unsigned long lightId);
        unsigned long getLightId(void) const;

        // -----------------------------------------------------------------------
        // Sun Mask pass parameters
        // -----------------------------------------------------------------------
        /** Luminance threshold: only pixels above this value are treated as sun. [0..1] default 0.8 */
        void setSunThreshold(Ogre::Real sunThreshold);
        Ogre::Real getSunThreshold(void) const;

        /** Screen-space radius around the sun position within which the mask is active. [0.1..1.5] default 0.4 */
        void setSunRadius(Ogre::Real sunRadius);
        Ogre::Real getSunRadius(void) const;

        // -----------------------------------------------------------------------
        // Radial Blur pass parameters
        // -----------------------------------------------------------------------
        /** Per-step luminance decay along the ray. [0.9..1.0] default 0.97 */
        void setDecay(Ogre::Real decay);
        Ogre::Real getDecay(void) const;

        /** Fraction of the pixel→sun path sampled. [0.1..1.0] default 0.8 */
        void setDensity(Ogre::Real density);
        Ogre::Real getDensity(void) const;

        /** Final exposure multiplier on the accumulated ray colour. [0.01..2.0] default 0.25 */
        void setExposure(Ogre::Real exposure);
        Ogre::Real getExposure(void) const;

        // -----------------------------------------------------------------------
        // Blend pass parameters
        // -----------------------------------------------------------------------
        /** Final strength of the additive blend onto the scene. [0..3] default 1.0 */
        void setGodRayStrength(Ogre::Real godRayStrength);
        Ogre::Real getGodRayStrength(void) const;

        /** RGB colour tint of the light shafts. Default: warm sunlight (1, 0.9, 0.7) */
        void setTint(const Ogre::Vector3& tint);
        Ogre::Vector3 getTint(void) const;

    public:
        static const Ogre::String AttrLightId(void)
        {
            return "Light Id";
        }
        static const Ogre::String AttrGodRayStrength(void)
        {
            return "God Ray Strength";
        }
        static const Ogre::String AttrSunThreshold(void)
        {
            return "Sun Threshold";
        }
        static const Ogre::String AttrSunRadius(void)
        {
            return "Sun Radius";
        }
        static const Ogre::String AttrDecay(void)
        {
            return "Decay";
        }
        static const Ogre::String AttrDensity(void)
        {
            return "Density";
        }
        static const Ogre::String AttrExposure(void)
        {
            return "Exposure";
        }
        static const Ogre::String AttrTint(void)
        {
            return "Tint";
        }

    private:
        // -----------------------------------------------------------------------
        // Internal helpers
        // -----------------------------------------------------------------------
        /**
         * @brief   Resolves the Ogre::Light* from the stored lightId Variant.
         *          Falls back to MAIN_LIGHT_ID when lightId == 0.
         *          Called from postInit and setLightId.
         */
        void resolveLight(void);

        // -----------------------------------------------------------------------
        // GPU pass pointers  (three materials, three passes)
        // -----------------------------------------------------------------------
        Ogre::MaterialPtr sunMaskMaterial;
        Ogre::Pass* sunMaskPass; ///< Postprocess/VolumetricLightSunMask

        Ogre::MaterialPtr radialBlurMaterial;
        Ogre::Pass* radialBlurPass; ///< Postprocess/VolumetricLightRadialBlur

        Ogre::MaterialPtr blendMaterial;
        Ogre::Pass* blendPass; ///< Postprocess/VolumetricLightBlend

        // -----------------------------------------------------------------------
        // Light reference
        // -----------------------------------------------------------------------
        Ogre::Light* sunLight; ///< Resolved directional light — not owned by this component

        // -----------------------------------------------------------------------
        // Variants
        // -----------------------------------------------------------------------
        Variant* lightId;
        Variant* godRayStrength;
        Variant* sunThreshold;
        Variant* sunRadius;
        Variant* decay;
        Variant* density;
        Variant* exposure;
        Variant* tint;
    };

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}; //namespace end

#endif