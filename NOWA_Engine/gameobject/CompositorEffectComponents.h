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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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
		 * - "Keyhole"
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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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
		Ogre::Pass*pass;
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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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

	class EXPORTED CompositorEffectKeyholeComponent : public CompositorEffectBaseComponent
	{
	public:
		typedef boost::shared_ptr<NOWA::CompositorEffectKeyholeComponent> CompositorEffectKeyholeCompPtr;
	public:

		CompositorEffectKeyholeComponent();

		virtual ~CompositorEffectKeyholeComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
			return NOWA::getIdFromName("CompositorEffectKeyholeComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "CompositorEffectKeyholeComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Requirements: A camera component must exist.";
		}

		void setFrameShape(Ogre::Real frameShape);

		Ogre::Real getFrameShape(void) const;

	public:
		static const Ogre::String AttrFrameShape(void) { return "Frame Shape"; }
	private:
		Variant* frameShape;
		Ogre::MaterialPtr material;
		Ogre::Pass* pass;
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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) {}

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) {}

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

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

}; //namespace end

#endif