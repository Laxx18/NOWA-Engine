/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef RANDOMIMAGESHUFFLER_H
#define RANDOMIMAGESHUFFLER_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

#include "utilities/Interpolator.h"

namespace NOWA
{

	/**
	  * @brief		The random image shuffler shuffles through images randomly and allows the user to stop and display the current image.
	  *				The start and stop function shall be called in lua script or C++.
	  */
	class EXPORTED RandomImageShuffler : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<RandomImageShuffler> RandomImageShufflerPtr;
	public:

		RandomImageShuffler();

		virtual ~RandomImageShuffler();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		* @note		Do nothing here, because its called far to early and nothing is there of NOWA-Engine yet!
		*/
		virtual void initialise() override {};

		/**
		* @see		Ogre::Plugin::shutdown
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void shutdown() override {};

		/**
		* @see		Ogre::Plugin::uninstall
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void uninstall() override {};

		/**
		* @see		Ogre::Plugin::getName
		*/
		virtual const Ogre::String& getName() const override;
		
		/**
		* @see		Ogre::Plugin::getAbiCookie
		*/
		virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

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
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		void setGeometry(const Ogre::Vector4& geometry);

		Ogre::Vector4 getGeometry(void) const;

		void setMaskImage(const Ogre::String& maskImage);

		Ogre::String getMaskImage(void) const;

		void setDisplayTime(Ogre::Real displayTime);

		Ogre::Real getDisplayTime(void) const;

		void setUseSlideImage(bool useSlideImage);

		bool getUseSlideImage(void) const;

		void setUseStopDelay(bool useStopDelay);

		bool getUseStopDelay(void) const;

		void setUseStopEffect(bool useStopEffect);

		bool getUseStopEffect(void) const;

		void setEaseFunction(const Ogre::String& easeFunction);

		Ogre::String getEaseFunction(void) const;

		void setImageCount(unsigned int imageCount);

		unsigned int getImageCount(void) const;

		void setImageName(unsigned int index, const Ogre::String& imageName);

		Ogre::String getImageName(unsigned int index);

		void start(void);

		void stop(void);

		/**
		 * @brief Sets the lua function name, to react when an image has been chosen after shuffeling.
		 * @param[in]	onImageChosenFunctionName		The function name to set
		 */
		void setOnImageChosenFunctionName(const Ogre::String& onImageChosenFunctionName);
	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("RandomImageShuffler");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "RandomImageShuffler";
		}
	
		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage:The random image shuffler shuffles through images randomly and allows the user to stop and display the current image. "
				"The start and stop function shall be called in lua script or C++.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrGeometry(void) { return "Geometry"; }
		static const Ogre::String AttrMaskImage(void) { return "Mask Image"; }
		static const Ogre::String AttrDisplayTime(void) { return "Display Time MS"; }
		static const Ogre::String AttrUseSlideImage(void) { return "Use Slide Image"; }
		static const Ogre::String AttrUseStopDelay(void) { return "Use Stop Delay"; }
		static const Ogre::String AttrUseStopEffect(void) { return "Use Stop Effect"; }
		static const Ogre::String AttrEaseFunction(void) { return "Ease Function"; }
		static const Ogre::String AttrOnImageChosenFunctionName(void) { return "On Image Chosen Function Name"; }
		static const Ogre::String AttrImageCount(void) { return "Image Count"; }
		static const Ogre::String AttrImageName(void) { return "Image Name "; }
	private:
		void switchImage(void);

		void slideImage(Ogre::Real dt);

		void startAnimation(void);

		void animateImage(Ogre::Real dt);

	private:
		Ogre::String name;
		MyGUI::ImageBox* imageWidget;
		MyGUI::ImageBox* maskImageWidget;
		bool startShuffle;
		bool stopShuffle;
		Ogre::Real timeSinceLastImageShown;
		Ogre::Real gradientDelay;
		Ogre::Real animationTime;
		Ogre::Real animationOppositeDir;
		short animationRound;
		bool animating;
		bool growing;
		MyGUI::IntRect initialGeometry;
		Interpolator::EaseFunctions easeFunctions;
		size_t currentImageIndex;
		Ogre::Real slideSpeed;
		Ogre::Real scrollPositionX;

		Variant* activated;
		Variant* geometry;
		Variant* maskImage;
		Variant* displayTime;
		Variant* useSlideImage;
		Variant* useStopDelay;
		Variant* useStopEffect;
		Variant* easeFunction;
		Variant* onImageChosenFunctionName;
		Variant* imageCount;
		std::vector<Variant*> images;
	};

}; // namespace end

#endif

