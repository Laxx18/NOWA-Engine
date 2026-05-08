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

    /**
        * @brief   Two-pass cartoon / cel-shading post-process compositor effect component.
        *
        * Pass 1 (Postprocess/CartoonEdge):
        *   Sobel edge detection on the scene colour buffer -> grayscale edge mask.
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

		/**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

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
                   "Screen-space god rays using the Crytek/Kawase three-pass technique. ";
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

	/**
     * @brief   Depth Fog + Height Fog post-process compositor effect.
     *
     * Uses the depth buffer (automatically wired
     * infrastructure in reconnectAllNodes) to compute per-pixel fog based on:
     *
     *   Depth fog   — exponential fog that increases with camera distance.
     *                 Good for atmospheric haze, distant mountain fade.
     *
     *   Height fog  — exponential fog based on world-space Y position.
     *                 Good for low-lying ground mist, valley fog.
     *
     * Both layers are independent and can be combined freely.
     * Setting either density to 0.0 disables that layer entirely.
     *
     * Per-frame work (camera matrix projection, invViewProj) is pushed to
     * the GPU via updateTrackedClosure — no game-thread stalls.
     *
     * Requirements: A camera component must exist.
     *               The workspace must have depthTexture as an output channel
     *               (true for all workspace components: Pbs, Sky, Background).
     */
    class EXPORTED CompositorEffectFogComponent : public CompositorEffectBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::CompositorEffectFogComponent> CompositorEffectFogCompPtr;

    public:
        CompositorEffectFogComponent();
        virtual ~CompositorEffectFogComponent();

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual void onRemoveComponent(void) override;

		/**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("CompositorEffectFogComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "CompositorEffectFogComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @brief   Per-frame update: pushes the camera's current inverse
         *          view-projection matrix and projection params to the shader.
         *          Runs every frame so height fog tracks camera movement.
         */
        virtual void update(Ogre::Real dt, bool notSimulating) override;

        static Ogre::String getStaticInfoText(void)
        {
            return "Requirements: A camera component must exist.\n"
                   "The workspace must provide a depthTexture output channel.\n"
                   "Uses the scene depth buffer to compute per pixel fog.\n"
                   "\n"
                   "Supports two independent fog layers:\n"
                   "\n"
                   "Depth fog:\n"
                   "Exponential fog based on camera distance.\n"
                   "Useful for atmospheric haze and distant object fading.\n"
                   "\n"
                   "Height fog:\n"
                   "Exponential fog based on world space height.\n"
                   "Useful for ground mist, valleys and low lying fog.\n"
                   "\n"
                   "Both fog layers can be combined freely.\n"
                   "Setting a fog density to 0 disables that layer.\n"
                   "\n"
                   "Camera matrices and projection data are updated per frame\n"
                   "through the tracked closure system without game thread stalls.";
        }

        // -----------------------------------------------------------------------
        // Depth fog
        // -----------------------------------------------------------------------
        /** Exponential fog density for depth fog. 0 = disabled. Default 0.008. */
        void setDepthFogDensity(Ogre::Real density);
        Ogre::Real getDepthFogDensity(void) const;

        /** Linear depth (world units) where depth fog starts. Default 20.0. */
        void setDepthFogStart(Ogre::Real start);
        Ogre::Real getDepthFogStart(void) const;

        // -----------------------------------------------------------------------
        // Height fog
        // -----------------------------------------------------------------------
        /** Exponential fog density for height fog. 0 = disabled. Default 0.06. */
        void setHeightFogDensity(Ogre::Real density);
        Ogre::Real getHeightFogDensity(void) const;

        /** World-space Y below which height fog is at maximum. Default 0.0. */
        void setHeightFogStart(Ogre::Real worldY);
        Ogre::Real getHeightFogStart(void) const;

        /** World-space Y above which height fog is zero. Default 40.0. */
        void setHeightFogEnd(Ogre::Real worldY);
        Ogre::Real getHeightFogEnd(void) const;

        // -----------------------------------------------------------------------
        // Shared
        // -----------------------------------------------------------------------
        /** RGB colour of the fog. Default: cool grey-blue (0.7, 0.75, 0.8). */
        void setFogColor(const Ogre::Vector3& color);
        Ogre::Vector3 getFogColor(void) const;

        /**
         * @brief   Controls how much the fog suppresses the skybox.
         *          0 = sky pixels get full fog suppression (sharp horizon),
         *          1 = sky pixels receive the same fog as everything else (bleed).
         *          Default 0.3.
         */
        void setFogSkyBlend(Ogre::Real blend);
        Ogre::Real getFogSkyBlend(void) const;

    public:
        static const Ogre::String AttrDepthFogDensity(void)
        {
            return "Depth Fog Density";
        }
        static const Ogre::String AttrDepthFogStart(void)
        {
            return "Depth Fog Start";
        }
        static const Ogre::String AttrHeightFogDensity(void)
        {
            return "Height Fog Density";
        }
        static const Ogre::String AttrHeightFogStart(void)
        {
            return "Height Fog Start";
        }
        static const Ogre::String AttrHeightFogEnd(void)
        {
            return "Height Fog End";
        }
        static const Ogre::String AttrFogColor(void)
        {
            return "Fog Color";
        }
        static const Ogre::String AttrFogSkyBlend(void)
        {
            return "Fog Sky Blend";
        }

    private:
        Ogre::MaterialPtr fogMaterial;
        Ogre::Pass* fogPass;

        Variant* depthFogDensity;
        Variant* depthFogStart;
        Variant* heightFogDensity;
        Variant* heightFogStart;
        Variant* heightFogEnd;
        Variant* fogColor;
        Variant* fogSkyBlend;
    };

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
     * @brief   Depth-occlusion light shafts (crepuscular rays) post-process effect.
     *
     * Three-pass pipeline:
     *
     *   Pass 1  Occlusion Mask — depth buffer gates emission:
     *             sky pixels (depth ≈ 1.0) that are bright near the sun → emitters
     *             geometry pixels (depth < occlusionDepthThreshold)     → hard black occluders
     *           This produces shaft silhouettes cut by mountains, trees, buildings.
     *
     *   Pass 2  Radial Blur — 48-sample march toward sun with per-step decay.
     *             shaftSharpness boosts edge contrast (1.0 = soft, 3.0 = crisp shafts).
     *
     *   Pass 3  Blend — additive composite with tint and strength.
     *
     * The depth texture is automatically wired to channel 2 by the Option B
     * infrastructure in reconnectAllNodes() — no workspace changes required.
     *
     * The sun screen position is derived per-frame from the linked directional
     * light's world direction, identical to CompositorEffectVolumetricLightComponent.
     *
     * Requirements: A camera component and a LightDirectionalComponent must exist.
     */
    class EXPORTED CompositorEffectLightShaftsComponent : public CompositorEffectBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::CompositorEffectLightShaftsComponent> CompositorEffectLightShaftsCompPtr;

    public:
        CompositorEffectLightShaftsComponent();
        virtual ~CompositorEffectLightShaftsComponent();

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual void onRemoveComponent(void) override;

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("CompositorEffectLightShaftsComponent");
        }
        static Ogre::String getStaticClassName(void)
        {
            return "CompositorEffectLightShaftsComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
        virtual void update(Ogre::Real dt, bool notSimulating) override;

        static Ogre::String getStaticInfoText(void)
        {
            return "Requirements: A camera component and a LightDirectionalComponent must exist. "
                   "Depth-occlusion light shafts: geometry casts hard shadows into the rays. "
                   "Use shaftSharpness to control edge crispness.";
        }

        // -----------------------------------------------------------------------
        // Light source
        // -----------------------------------------------------------------------
        void setLightId(unsigned long lightId);
        unsigned long getLightId(void) const;

        // -----------------------------------------------------------------------
        // Pass 1 — Occlusion Mask
        // -----------------------------------------------------------------------
        /** rawDepth below this = geometry occluder. Default 0.9999. Range [0.99..1.0]. */
        void setOcclusionDepthThreshold(Ogre::Real threshold);
        Ogre::Real getOcclusionDepthThreshold(void) const;

        /** Screen-space emission radius around sun. Default 0.5. Range [0.05..1.5]. */
        void setSunRadius(Ogre::Real radius);
        Ogre::Real getSunRadius(void) const;

        /** Minimum sky luminance to emit. Default 0.6. Range [0..1]. */
        void setBrightnessThreshold(Ogre::Real threshold);
        Ogre::Real getBrightnessThreshold(void) const;

        // -----------------------------------------------------------------------
        // Pass 2 — Radial Blur
        // -----------------------------------------------------------------------
        /** Per-step decay. Default 0.96. Range [0.9..1.0]. */
        void setDecay(Ogre::Real decay);
        Ogre::Real getDecay(void) const;

        /** Fraction of path sampled. Default 0.9. Range [0.1..1.0]. */
        void setDensity(Ogre::Real density);
        Ogre::Real getDensity(void) const;

        /** Final exposure scale. Default 0.3. Range [0.01..2.0]. */
        void setExposure(Ogre::Real exposure);
        Ogre::Real getExposure(void) const;

        /** Edge contrast: 1=soft glow, 3=crisp shafts. Default 1.5. Range [0.5..4.0]. */
        void setShaftSharpness(Ogre::Real sharpness);
        Ogre::Real getShaftSharpness(void) const;

        // -----------------------------------------------------------------------
        // Pass 3 — Blend
        // -----------------------------------------------------------------------
        /** Additive blend multiplier. Default 1.0. Range [0..3]. */
        void setShaftStrength(Ogre::Real strength);
        Ogre::Real getShaftStrength(void) const;

        /** RGB colour tint. Default warm sunlight (1.0, 0.92, 0.75). */
        void setTint(const Ogre::Vector3& tint);
        Ogre::Vector3 getTint(void) const;

    public:
        static const Ogre::String AttrLightId(void)
        {
            return "Light Id";
        }
        static const Ogre::String AttrOcclusionDepthThreshold(void)
        {
            return "Occlusion Depth Threshold";
        }
        static const Ogre::String AttrSunRadius(void)
        {
            return "Sun Radius";
        }
        static const Ogre::String AttrBrightnessThreshold(void)
        {
            return "Brightness Threshold";
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
        static const Ogre::String AttrShaftSharpness(void)
        {
            return "Shaft Sharpness";
        }
        static const Ogre::String AttrShaftStrength(void)
        {
            return "Shaft Strength";
        }
        static const Ogre::String AttrTint(void)
        {
            return "Tint";
        }

    private:
        void resolveLight(void);

        // Three passes
        Ogre::MaterialPtr occlusionMaskMaterial;
        Ogre::Pass* occlusionMaskPass;

        Ogre::MaterialPtr radialBlurMaterial;
        Ogre::Pass* radialBlurPass;

        Ogre::MaterialPtr blendMaterial;
        Ogre::Pass* blendPass;

        Ogre::Light* sunLight;

        Variant* lightId;
        Variant* occlusionDepthThreshold;
        Variant* sunRadius;
        Variant* brightnessThreshold;
        Variant* decay;
        Variant* density;
        Variant* exposure;
        Variant* shaftSharpness;
        Variant* shaftStrength;
        Variant* tint;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief   Depth of Field post-process compositor effect.
     *
     * Supports two algorithm variants selectable via setDofType():
     *
     *   Gaussian  (type 0) — separable H+V Gaussian blur weighted by CoC.
     *                        Produces smooth circular bokeh.  4 passes.
     *                        Best for: landscapes, wide establishing shots.
     *
     *   Hex Bokeh (type 1) — three directional blurs at 0°/60°/−60° combined
     *                        into a hexagonal kernel.  6 passes.
     *                        Best for: close-ups, night cityscapes, lens-flare scenes.
     *
     * CoC model: two independent near/far blur ranges around a focus distance.
     * Setting either range to a large value (e.g. 9999) disables that half.
     *
     * Focus tracking: when focusGameObjectId is set (non-zero), the component
     * measures the game object's distance from the camera each frame and uses
     * that as the focus distance — ideal for cinematic auto-focus.
     *
     * The depth texture is auto-wired via Option B in reconnectAllNodes().
     *
     * Requirements: A camera component must exist.
     */
    class EXPORTED CompositorEffectDepthOfFieldComponent : public CompositorEffectBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::CompositorEffectDepthOfFieldComponent> CompositorEffectDepthOfFieldCompPtr;

        enum DofType
        {
            Gaussian = 0,
            HexBokeh = 1
        };

    public:
        CompositorEffectDepthOfFieldComponent();
        virtual ~CompositorEffectDepthOfFieldComponent();

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual void onRemoveComponent(void) override;

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("CompositorEffectDepthOfFieldComponent");
        }
        static Ogre::String getStaticClassName(void)
        {
            return "CompositorEffectDepthOfFieldComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
        virtual void update(Ogre::Real dt, bool notSimulating) override;

        static Ogre::String getStaticInfoText(void)
        {
            return "Requirements: A camera component must exist. "
                   "Depth of Field: Gaussian (smooth bokeh) or Hex Bokeh (6-sided bokeh). "
                   "Set focusGameObjectId for auto-focus tracking.";
        }

        // -----------------------------------------------------------------------
        // Type switch — changing type re-resolves material passes
        // -----------------------------------------------------------------------
        /**
         * @brief   Switch between Gaussian (0) and Hex Bokeh (1).
         *          Also updates effectName so enableEffect() targets the right node.
         */
        void setDofType(const Ogre::String& dofType);
        Ogre::String getDofType(void) const;

        // -----------------------------------------------------------------------
        // Focus tracking
        // -----------------------------------------------------------------------
        /**
         * @brief   When non-zero, auto-focuses on this game object per-frame.
         *          Overrides focusDistance while active.
         */
        void setFocusGameObjectId(unsigned long id);
        unsigned long getFocusGameObjectId(void) const;

        // -----------------------------------------------------------------------
        // CoC parameters — shared by both types
        // -----------------------------------------------------------------------
        /** Distance to the in-focus plane (world units). Default 20.0. */
        void setFocusDistance(Ogre::Real dist);
        Ogre::Real getFocusDistance(void) const;

        /** Depth range for near blur transition. Default 5.0. */
        void setNearBlurRange(Ogre::Real range);
        Ogre::Real getNearBlurRange(void) const;

        /** Depth range for far blur transition. Default 30.0. */
        void setFarBlurRange(Ogre::Real range);
        Ogre::Real getFarBlurRange(void) const;

        // -----------------------------------------------------------------------
        // Blur parameters — shared by both types
        // -----------------------------------------------------------------------
        /** Maximum blur kernel radius (UV units). Default 0.012 (Gaussian) / 0.014 (Hex). */
        void setBlurRadius(Ogre::Real radius);
        Ogre::Real getBlurRadius(void) const;

        /** Final blend strength multiplier [0..1]. Default 1.0. */
        void setBlendStrength(Ogre::Real strength);
        Ogre::Real getBlendStrength(void) const;

    public:
        static const Ogre::String AttrDofType(void)
        {
            return "Dof Type";
        }
        static const Ogre::String AttrFocusGameObjectId(void)
        {
            return "Focus Game Object Id";
        }
        static const Ogre::String AttrFocusDistance(void)
        {
            return "Focus Distance";
        }
        static const Ogre::String AttrNearBlurRange(void)
        {
            return "Near Blur Range";
        }
        static const Ogre::String AttrFarBlurRange(void)
        {
            return "Far Blur Range";
        }
        static const Ogre::String AttrBlurRadius(void)
        {
            return "Blur Radius";
        }
        static const Ogre::String AttrBlendStrength(void)
        {
            return "Blend Strength";
        }

    private:
        /**
         * @brief   (Re-)resolves all material pass pointers for the active DofType.
         *          Called from postInit and setDofType.
         */
        bool resolvePasses(void);

        // -----------------------------------------------------------------------
        // CoC pass — one per type, same interface
        // -----------------------------------------------------------------------
        Ogre::MaterialPtr cocMaterial;
        Ogre::Pass* cocPass;

        // -----------------------------------------------------------------------
        // Blur passes — Gaussian: 2 passes  |  Hex: 3 direction passes
        // -----------------------------------------------------------------------
        Ogre::MaterialPtr blurMaterial0; // Gaussian BlurH  |  Hex Dir0
        Ogre::Pass* blurPass0;

        Ogre::MaterialPtr blurMaterial1; // Gaussian BlurV  |  Hex Dir1
        Ogre::Pass* blurPass1;

        Ogre::MaterialPtr blurMaterial2; // (unused Gaussian) | Hex Dir2
        Ogre::Pass* blurPass2;

        // -----------------------------------------------------------------------
        // Blend pass — one per type, same interface
        // -----------------------------------------------------------------------
        Ogre::MaterialPtr blendMaterial;
        Ogre::Pass* blendPass;

        // -----------------------------------------------------------------------
        // Variants
        // -----------------------------------------------------------------------
        Variant* dofType;
        Variant* focusGameObjectId;
        Variant* focusDistance;
        Variant* nearBlurRange;
        Variant* farBlurRange;
        Variant* blurRadius;
        Variant* blendStrength;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief   Depth-based outline / silhouette post-process effect.
     *
     * Runs a Sobel operator on the LINEARISED depth buffer to detect
     * geometry silhouette edges.  Because depth (not colour) drives the
     * edge detection, texture detail is completely ignored — only actual
     * geometry discontinuities produce outlines.
     *
     * This is the correct approach compared to colour-based Sobel (cartoon),
     * which picks up every texture edge on detailed surfaces like terrain.
     *
     * Can be combined with the Cartoon effect:
     *   Cartoon  — colour quantisation + saturation
     *   Outline  — clean geometry silhouettes on top
     *
     * Parameters:
     *   outlineColor      — RGB of the outline strokes. Default: black.
     *   outlineThickness  — Sobel kernel step size in pixels. 1=thin, 3=thick.
     *   depthThreshold    — Minimum depth gradient to count as an edge.
     *                       Lower = more edges (small features), higher = fewer (silhouettes only).
     *   outlineStrength   — Final opacity of the outline blend [0..1].
     *
     * The depth texture is auto-wired by the Option B infrastructure in
     * reconnectAllNodes(). projectionParams is pushed from C++ in postInit
     * and update() since it depends on camera clip planes.
     *
     * Requirements: A camera component must exist.
     */
    class EXPORTED CompositorEffectOutlineComponent : public CompositorEffectBaseComponent
    {
    public:
        typedef boost::shared_ptr<NOWA::CompositorEffectOutlineComponent> CompositorEffectOutlineCompPtr;

    public:
        CompositorEffectOutlineComponent();
        virtual ~CompositorEffectOutlineComponent();

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual void onRemoveComponent(void) override;

        /**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("CompositorEffectOutlineComponent");
        }
        static Ogre::String getStaticClassName(void)
        {
            return "CompositorEffectOutlineComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @brief   Pushes updated projectionParams each frame in case the
         *          camera's clip planes change at runtime.
         */
        virtual void update(Ogre::Real dt, bool notSimulating) override;

        static Ogre::String getStaticInfoText(void)
        {
            return "Runs a Sobel operator on the linearised depth buffer to detect\n"
                   "geometry silhouette edges.\n"
                   "\n"
                   "Because depth instead of colour drives the edge detection,\n"
                   "texture details are ignored completely.\n"
                   "Only actual geometry discontinuities produce outlines.\n"
                   "\n"
                   "This is the correct approach compared to colour based Sobel\n"
                   "cartoon edge detection, which detects every texture edge on\n"
                   "detailed surfaces like terrain.\n"
                   "\n"
                   "Can be combined with the CompositorEffectCartoonComponent:\n"
                   "Cartoon handles colour quantisation and saturation,\n"
                   "Outline adds clean geometry silhouettes on top.\n"
                   "\n"
                   "Parameters:\n"
                   "outlineColor:\n"
                   "RGB colour of the outline strokes.\n"
                   "\n"
                   "outlineThickness:\n"
                   "Sobel kernel step size in pixels.\n"
                   "1 creates thin outlines, 3 creates thick outlines.\n"
                   "\n"
                   "depthThreshold:\n"
                   "Minimum depth gradient required to detect an edge.\n"
                   "Lower values detect more small features.\n"
                   "Higher values focus on large silhouettes only.\n"
                   "\n"
                   "outlineStrength:\n"
                   "Final opacity and intensity of the outline effect.\n"
                   "\n"
                   "The depth texture is automatically wired through the\n"
                   "workspace compositor infrastructure.\n"
                   "Projection parameters are updated per frame from the camera.";
        }

        /**
         * @brief   RGB colour of the outline strokes. Default: black (0, 0, 0).
         *          Set to a bright colour for a comic-book style glow outline.
         */
        void setOutlineColor(const Ogre::Vector3& color);
        Ogre::Vector3 getOutlineColor(void) const;

        /**
         * @brief   Sobel kernel step size (pixels). 1 = 1-pixel thin lines,
         *          2-3 = bolder strokes. Default 1.0.
         */
        void setOutlineThickness(Ogre::Real thickness);
        Ogre::Real getOutlineThickness(void) const;

        /**
         * @brief   Minimum linearised depth gradient to register as an edge.
         *          Lower values detect finer geometry (more lines).
         *          Higher values show only coarse silhouettes (fewer lines).
         *          Default 0.001. Typical range [0.0001 .. 0.05].
         */
        void setDepthThreshold(Ogre::Real threshold);
        Ogre::Real getDepthThreshold(void) const;

        /**
         * @brief   Final opacity of the outline blend over the scene [0..1].
         *          Default 1.0.
         */
        void setOutlineStrength(Ogre::Real strength);
        Ogre::Real getOutlineStrength(void) const;

    public:
        static const Ogre::String AttrOutlineColor(void)
        {
            return "Outline Color";
        }
        static const Ogre::String AttrOutlineThickness(void)
        {
            return "Outline Thickness";
        }
        static const Ogre::String AttrDepthThreshold(void)
        {
            return "Depth Threshold";
        }
        static const Ogre::String AttrOutlineStrength(void)
        {
            return "Outline Strength";
        }

    private:
        Ogre::MaterialPtr outlineMaterial;
        Ogre::Pass* outlinePass;

        Variant* outlineColor;
        Variant* outlineThickness;
        Variant* depthThreshold;
        Variant* outlineStrength;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}; //namespace end

#endif