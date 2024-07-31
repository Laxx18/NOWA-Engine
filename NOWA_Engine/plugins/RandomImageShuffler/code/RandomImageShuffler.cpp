#include "NOWAPrecompiled.h"
#include "RandomImageShuffler.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	RandomImageShuffler::RandomImageShuffler()
		: GameObjectComponent(),
		name("RandomImageShuffler"),
		imageWidget(nullptr),
		maskImageWidget(nullptr),
		startShuffle(false),
		stopShuffle(false),
		timeSinceLastImageShown(0.0f),
		gradientDelay(0.0f),
		animationTime(0.0f),
		animating(false),
		growing(false),
		initialSize(0, 0),
		activated(new Variant(RandomImageShuffler::AttrActivated(), true, this->attributes)),
		geometry(new Variant(RandomImageShuffler::AttrGeometry(), Ogre::Vector4(0.8f, 0.0f, 0.2f, 0.2f), this->attributes)),
		maskImage(new Variant(RandomImageShuffler::AttrMaskImage(), "", this->attributes)),
		displayTime(new Variant(RandomImageShuffler::AttrDisplayTime(), 100.0f, this->attributes)),
		useStopDelay(new Variant(RandomImageShuffler::AttrUseStopDelay(), false, this->attributes)),
		useStopEffect(new Variant(RandomImageShuffler::AttrUseStopEffect(), false, this->attributes)),
		onImageChosenFunctionName(new Variant(RandomImageShuffler::AttrOnImageChosenFunctionName(), "", this->attributes)),
		imageCount(new Variant(RandomImageShuffler::AttrImageCount(), 0, this->attributes))
	{
		this->activated->setDescription("If active the shuffle can be started via @start method or stopped via @stop method.");
		this->geometry->setDescription("Sets the geometry of the relative to the window screen in the format Vector4(pos.x, pos.y, width, height).");
		this->maskImage->addUserData(GameObject::AttrActionFileOpenDialog(), "MyGuiImages");
		this->maskImage->setDescription("Sets a image mask, which can be created in gimp and added to the Minimap.xml in the MyGui/Minimap folder.");
		this->displayTime->setDescription("Sets the image display time in milleseconds.");
		this->useStopDelay->setDescription("Sets whether to use stop delay. That is, a gradual slowdown before stopping at an image.");
		this->useStopEffect->setDescription("Sets whether to use stop effect (grow and reduce) final image which has been chosen.");
		this->onImageChosenFunctionName->setDescription("Sets the lua function name, to react when an image has been chosen after shuffeling. E.g. onImageChosen(imageName, imageIndex).");
		this->onImageChosenFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onImageChosenFunctionName->getString() + "(imageName, imageIndex)");
		this->imageCount->addUserData(GameObject::AttrActionNeedRefresh());
	}

	RandomImageShuffler::~RandomImageShuffler(void)
	{

	}

	const Ogre::String& RandomImageShuffler::getName() const
	{
		return this->name;
	}

	void RandomImageShuffler::install(const Ogre::NameValuePairList* options)
	{
		Ogre::ResourceGroupManager::getSingletonPtr()->addResourceLocation("../../media/MyGUI_Media/images", "FileSystem", "MyGuiImages");

		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<RandomImageShuffler>(RandomImageShuffler::getStaticClassId(), RandomImageShuffler::getStaticClassName());
	}

	void RandomImageShuffler::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool RandomImageShuffler::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Geometry")
		{
			this->geometry->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaskImage")
		{
			this->maskImage->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DisplayTime")
		{
			this->displayTime->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseStopDelay")
		{
			this->useStopDelay->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseStopEffect")
		{
			this->useStopEffect->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnImageChosenFunctionName")
		{
			this->onImageChosenFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ImageCount")
		{
			this->imageCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 0));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (this->images.size() < this->imageCount->getUInt())
		{
			this->images.resize(this->imageCount->getUInt());
		}
		for (size_t i = 0; i < this->images.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ImageName" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->images[i])
				{
					this->images[i] = new Variant(RandomImageShuffler::AttrImageName() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->images[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->images[i]->addUserData(GameObject::AttrActionSeparator());
				this->images[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "MyGuiImages");
			}
		}

		return true;
	}

	GameObjectCompPtr RandomImageShuffler::clone(GameObjectPtr clonedGameObjectPtr)
	{
		RandomImageShufflerPtr clonedCompPtr(boost::make_shared<RandomImageShuffler>());

		clonedCompPtr->setGeometry(this->geometry->getVector4());
		clonedCompPtr->setMaskImage(this->maskImage->getString());
		clonedCompPtr->setDisplayTime(this->displayTime->getReal());
		clonedCompPtr->setUseStopDelay(this->useStopDelay->getBool());
		clonedCompPtr->setUseStopEffect(this->useStopEffect->getBool());
		clonedCompPtr->setOnImageChosenFunctionName(this->onImageChosenFunctionName->getString());
		clonedCompPtr->setImageCount(this->imageCount->getUInt());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->images.size()); i++)
		{
			clonedCompPtr->setImageName(i, this->images[i]->getString());
		}

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool RandomImageShuffler::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RandomImageShuffler] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool RandomImageShuffler::connect(void)
	{
		this->setActivated(this->activated->getBool());

		return true;
	}

	bool RandomImageShuffler::disconnect(void)
	{
		this->stopShuffle = true;
		this->startShuffle = false;

		if (nullptr != this->maskImageWidget)
		{
			// Must be done, because mygui also holds a reference to the Ogre::TextureGpu texture.
			auto myGuiTexture = MyGUI::RenderManager::getInstancePtr()->getTexture(this->maskImage->getString());
			if (nullptr != myGuiTexture)
			{
				static_cast<MyGUI::Ogre2RenderManager*>(MyGUI::RenderManager::getInstancePtr())->removeTexture(myGuiTexture);
			}
			this->maskImageWidget->setImageTexture("");
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->maskImageWidget);
			this->maskImageWidget = nullptr;
		}
		if (nullptr != this->imageWidget)
		{
			this->imageWidget->setImageTexture("");
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->imageWidget);
			this->imageWidget = nullptr;
		}
		return true;
	}

	bool RandomImageShuffler::onCloned(void)
	{

		return true;
	}

	void RandomImageShuffler::onRemoveComponent(void)
	{

	}

	void RandomImageShuffler::onOtherComponentRemoved(unsigned int index)
	{

	}

	void RandomImageShuffler::onOtherComponentAdded(unsigned int index)
	{

	}

	void RandomImageShuffler::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (true == this->startShuffle && false == this->stopShuffle)
			{
				this->timeSinceLastImageShown += dt;
				if (this->timeSinceLastImageShown >= this->displayTime->getReal() * 0.001f)
				{
					this->switchImage();
					this->timeSinceLastImageShown = 0.0f;
				}
			}
			else if (true == this->stopShuffle && true == this->startShuffle && true == this->useStopDelay->getBool())
			{
				this->timeSinceLastImageShown += dt;
				if (this->timeSinceLastImageShown >= this->gradientDelay * 0.001f)
				{
					this->switchImage();
					this->timeSinceLastImageShown = 0.0f;
					if (this->gradientDelay < 1000.0f)
					{
						// Increment delay to slow down gradually
						this->gradientDelay += 100.0f;
					}
					else
					{
						this->startShuffle = false;
						this->stopShuffle = true;
						if (true == this->useStopEffect->getBool())
						{
							this->startAnimation();
						}
						if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->onImageChosenFunctionName->getString().empty())
						{
							this->gameObjectPtr->getLuaScript()->callTableFunction(this->onImageChosenFunctionName->getString(), this->imageWidget->_getTextureName(), this->imageWidget->getUserString("ImageIndex"));
						}
					}
				}
			}

			if (true == this->animating)
			{
				this->animateImage(dt);
			}
		}
	}

	void RandomImageShuffler::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (RandomImageShuffler::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (RandomImageShuffler::AttrGeometry() == attribute->getName())
		{
			this->setGeometry(attribute->getVector4());
		}
		else if (RandomImageShuffler::AttrMaskImage() == attribute->getName())
		{
			this->setMaskImage(attribute->getString());
		}
		else if (RandomImageShuffler::AttrDisplayTime() == attribute->getName())
		{
			this->setDisplayTime(attribute->getReal());
		}
		else if (RandomImageShuffler::AttrUseStopDelay() == attribute->getName())
		{
			this->setUseStopDelay(attribute->getBool());
		}
		else if (RandomImageShuffler::AttrUseStopEffect() == attribute->getName())
		{
			this->setUseStopEffect(attribute->getBool());
		}
		else if (RandomImageShuffler::AttrImageCount() == attribute->getName())
		{
			this->setImageCount(attribute->getUInt());
		}
		else if (RandomImageShuffler::AttrOnImageChosenFunctionName() == attribute->getName())
		{
			this->setOnImageChosenFunctionName(attribute->getString());
		}
		else
		{
			for (size_t i = 0; i < this->images.size(); i++)
			{
				if (RandomImageShuffler::AttrImageName() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->images[i]->setValue(attribute->getString());
				}
			}
		}
	}

	void RandomImageShuffler::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Geometry"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->geometry->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaskImage"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maskImage->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DisplayTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->displayTime->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseStopDelay"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useStopDelay->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseStopEffect"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useStopEffect->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OnImageChosenFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onImageChosenFunctionName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ImageCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->imageCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->images.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "ImageName" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->images[i]->getString())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String RandomImageShuffler::getClassName(void) const
	{
		return "RandomImageShuffler";
	}

	Ogre::String RandomImageShuffler::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void RandomImageShuffler::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (true == activated)
		{
			this->timeSinceLastImageShown = 0.0f;
			this->gradientDelay = this->displayTime->getReal();
			this->animationTime = 0.0f;
			this->animating = false;

			this->imageWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(this->geometry->getVector4().x, this->geometry->getVector4().y, this->geometry->getVector4().z, this->geometry->getVector4().w), MyGUI::Align::HCenter, "Overlapped");

			if (false == this->maskImage->getString().empty())
			{
				this->maskImageWidget = this->imageWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, this->imageWidget->getWidth(), this->imageWidget->getHeight()), MyGUI::Align::Stretch);
				this->maskImageWidget->setImageTexture(this->maskImage->getString());
			}
		}
	}

	bool RandomImageShuffler::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void RandomImageShuffler::setGeometry(const Ogre::Vector4& geometry)
	{
		this->geometry->setValue(geometry);
	}

	Ogre::Vector4 RandomImageShuffler::getGeometry(void) const
	{
		return this->geometry->getVector4();
	}

	void RandomImageShuffler::setMaskImage(const Ogre::String& maskImage)
	{
		this->maskImage->setValue(maskImage);
	}

	Ogre::String RandomImageShuffler::getMaskImage(void) const
	{
		return this->maskImage->getString();
	}

	void RandomImageShuffler::setDisplayTime(Ogre::Real displayTime)
	{
		this->displayTime->setValue(displayTime);
	}

	Ogre::Real RandomImageShuffler::getDisplayTime(void) const
	{
		return this->displayTime->getUInt();
	}

	void RandomImageShuffler::setUseStopDelay(bool useStopDelay)
	{
		this->useStopDelay->setValue(useStopDelay);
	}

	bool RandomImageShuffler::getUseStopDelay(void) const
	{
		return this->useStopDelay->getBool();
	}

	void RandomImageShuffler::setUseStopEffect(bool useStopEffect)
	{
		this->useStopEffect->setValue(useStopEffect);
	}

	bool RandomImageShuffler::getUseStopEffect(void) const
	{
		return this->useStopEffect->getBool();
	}

	void RandomImageShuffler::setImageCount(unsigned int imageCount)
	{
		this->imageCount->setValue(imageCount);

		size_t oldSize = this->images.size();

		if (imageCount > oldSize)
		{
			// Resize the waypoints array for count
			this->images.resize(imageCount);

			for (size_t i = oldSize; i < this->images.size(); i++)
			{
				this->images[i] = new Variant(RandomImageShuffler::AttrImageName() + Ogre::StringConverter::toString(i), "", this->attributes);
				this->images[i]->addUserData(GameObject::AttrActionSeparator());
				this->images[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "MyGuiImages");
			}
		}
		else if (imageCount < oldSize)
		{
			this->eraseVariants(this->images, imageCount);
		}
	}

	unsigned int RandomImageShuffler::getImageCount(void) const
	{
		return this->imageCount->getUInt();
	}

	void RandomImageShuffler::setImageName(unsigned int index, const Ogre::String& imageName)
	{
		if (index >= this->images.size())
			return;
		this->images[index]->setValue(imageName);
	}

	Ogre::String RandomImageShuffler::getImageName(unsigned int index)
	{
		if (index > this->images.size())
			return "";
		return this->images[index]->getString();
	}

	void RandomImageShuffler::start(void)
	{
		if (false == this->startShuffle)
		{
			this->timeSinceLastImageShown = 0.0f;
			this->gradientDelay = 0.0f;
			this->animationTime = 0.0f;
			this->animating = false;
			this->growing = true;
			this->stopShuffle = false;
			this->startShuffle = true;
		}
	}

	void RandomImageShuffler::stop(void)
	{
		if (false == this->stopShuffle)
		{
			if (true == this->useStopDelay->getBool())
			{
				this->stopShuffle = true;
			}
			else
			{
				this->stopShuffle = true;
				this->startShuffle = false;
				// Immediately displays the current image
				this->switchImage();

				if (true == this->useStopEffect->getBool())
				{
					this->startAnimation();
				}

				if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->onImageChosenFunctionName->getString().empty())
				{
					this->gameObjectPtr->getLuaScript()->callTableFunction(this->onImageChosenFunctionName->getString(), this->imageWidget->_getTextureName(), this->imageWidget->getUserString("ImageIndex"));
				}
			}
		}
	}

	void RandomImageShuffler::setOnImageChosenFunctionName(const Ogre::String& onImageChosenFunctionName)
	{
		this->onImageChosenFunctionName->setValue(onImageChosenFunctionName);
		if (false == onImageChosenFunctionName.empty())
		{
			this->onImageChosenFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onImageChosenFunctionName + "(imageName, imageIndex)");
		}
	}

	void RandomImageShuffler::switchImage()
	{
		static std::mt19937 rng(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
		std::uniform_int_distribution<int> dist(0, static_cast<int>(this->images.size() - 1));

		int index = dist(rng);
		this->imageWidget->setImageTexture(this->images[index]->getString());
		this->imageWidget->setUserString("ImageIndex", Ogre::StringConverter::toString(index));
	}

	void RandomImageShuffler::startAnimation(void)
	{
		this->animating = true;
		this->animationTime = 0.0f;
		this->initialSize = this->imageWidget->getSize();
	}

#if 0
	void RandomImageShuffler::animateImage(Ogre::Real dt)
	{
		this->animationTime += dt;
		Ogre::Real duration = 1.0f; // Duration of the animation in seconds
		Ogre::Real t = this->animationTime / duration;

		if (t >= 1.0f)
		{
			t = 1.0f;
			this->animationTime = 0;

			if (true == this->growing)
			{
				this->growing = false;
				// this->initialSize = this->targetSize;
				this->targetSize = MyGUI::IntSize(this->initialSize.width / 1.25, this->initialSize.height / 1.25); // Reduce to original size
			}
			else
			{
				this->animating = false;
				// Resets for the next animation
				this->growing = true;
			}
		}

		// Interpolate size
		int width = static_cast<int>(this->initialSize.width + t * (this->targetSize.width - this->initialSize.width));
		int height = static_cast<int>(this->initialSize.height + t * (this->targetSize.height - this->initialSize.height));

		this->imageWidget->setSize(width, height);

		if (nullptr != this->maskImageWidget)
		{
			this->maskImageWidget->setSize(width, height);
		}
	}
#else
	void RandomImageShuffler::animateImage(Ogre::Real dt)
	{
		this->animationTime += dt;
		Ogre::Real duration = 1.0f; // Duration of the animation in seconds
		// Calculate new size based on sine wave for a single grow/shrink cycle
		Ogre::Real t = this->animationTime / duration;

		// Once the animation is complete
		if (t > 1.0f) 
		{
			this->animating = false; // Stop the animation
			this->imageWidget->setSize(this->initialSize); // Set back to original size

			if (nullptr != this->maskImageWidget)
			{
				this->maskImageWidget->setSize(this->initialSize); // Set back to original size
			}
			return; // Exit here
		}

		// Calculate the sine value for smooth animation
		Ogre::Real sineValue = Ogre::Math::Sin(t * Ogre::Math::PI); // Go from 0 to 1
		int width = static_cast<int>(this->initialSize.width + sineValue * 80); // 300 is the base width
		int height = static_cast<int>(this->initialSize.height + sineValue * 80); // 300 is the base height

		this->imageWidget->setSize(width, height);

		if (nullptr != this->maskImageWidget)
		{
			this->maskImageWidget->setSize(width, height);
		}
	}
#endif

	// Lua registration part

	RandomImageShuffler* getRandomImageShuffler(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<RandomImageShuffler>(gameObject->getComponentWithOccurrence<RandomImageShuffler>(occurrenceIndex)).get();
	}

	RandomImageShuffler* getRandomImageShuffler(GameObject* gameObject)
	{
		return makeStrongPtr<RandomImageShuffler>(gameObject->getComponent<RandomImageShuffler>()).get();
	}

	RandomImageShuffler* getRandomImageShufflerFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<RandomImageShuffler>(gameObject->getComponentFromName<RandomImageShuffler>(name)).get();
	}

	void RandomImageShuffler::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<RandomImageShuffler, GameObjectComponent>("RandomImageShuffler")
			.def("setActivated", &RandomImageShuffler::setActivated)
			.def("isActivated", &RandomImageShuffler::isActivated)
			.def("start", &RandomImageShuffler::start)
			.def("stop", &RandomImageShuffler::stop)
		];

		LuaScriptApi::getInstance()->addClassToCollection("RandomImageShuffler", "class inherits GameObjectComponent", RandomImageShuffler::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("RandomImageShuffler", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("RandomImageShuffler", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getRandomImageShufflerFromName", &getRandomImageShufflerFromName);
		gameObjectClass.def("getRandomImageShuffler", (RandomImageShuffler * (*)(GameObject*)) & getRandomImageShuffler);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getRandomImageShuffler2", (RandomImageShuffler * (*)(GameObject*, unsigned int)) & getRandomImageShuffler);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "RandomImageShuffler getRandomImageShuffler2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "RandomImageShuffler getRandomImageShuffler()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "RandomImageShuffler getRandomImageShufflerFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castRandomImageShuffler", &GameObjectController::cast<RandomImageShuffler>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "RandomImageShuffler castRandomImageShuffler(RandomImageShuffler other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool RandomImageShuffler::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end