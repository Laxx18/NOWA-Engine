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
		animationOppositeDir(1.0f),
		animationRound(0),
		animating(false),
		growing(false),
		initialGeometry(0, 0, 0, 0),
		easeFunctions(Interpolator::EaseInSine),
		currentImageIndex(0),
		slideSpeed(2000.0f),
		scrollPositionX(0.0f),
		activated(new Variant(RandomImageShuffler::AttrActivated(), true, this->attributes)),
		geometry(new Variant(RandomImageShuffler::AttrGeometry(), Ogre::Vector4(0.8f, 0.0f, 0.2f, 0.2f), this->attributes)),
		maskImage(new Variant(RandomImageShuffler::AttrMaskImage(), "", this->attributes)),
		displayTime(new Variant(RandomImageShuffler::AttrDisplayTime(), 100.0f, this->attributes)),
		useSlideImage(new Variant(RandomImageShuffler::AttrUseSlideImage(), false, this->attributes)),
		useStopDelay(new Variant(RandomImageShuffler::AttrUseStopDelay(), false, this->attributes)),
		useStopEffect(new Variant(RandomImageShuffler::AttrUseStopEffect(), false, this->attributes)),
		easeFunction(new Variant(RandomImageShuffler::AttrEaseFunction(), Interpolator::getInstance()->getAllEaseFunctionNames(), this->attributes)),
		onImageChosenFunctionName(new Variant(RandomImageShuffler::AttrOnImageChosenFunctionName(), "", this->attributes)),
		imageCount(new Variant(RandomImageShuffler::AttrImageCount(), 0, this->attributes))
	{
		this->activated->setDescription("If active the shuffle can be started via @start method or stopped via @stop method.");
		this->geometry->setDescription("Sets the geometry of the relative to the window screen in the format Vector4(pos.x, pos.y, width, height).");
		this->maskImage->addUserData(GameObject::AttrActionFileOpenDialog(), "MyGuiImages");
		this->maskImage->setDescription("Sets a image mask, which can be created in gimp and added to the Minimap.xml in the MyGui/Minimap folder.");
		this->displayTime->setDescription("Sets the image display time in milleseconds.");
		this->useSlideImage->setDescription("Sets whether images shall slide during shuffle instead of randomly shown for a specific amount of time.");
		this->useStopDelay->setDescription("Sets whether to use stop delay. That is, a gradual slowdown before stopping at an image.");
		this->useStopEffect->setDescription("Sets whether to use stop effect (grow and reduce) final image which has been chosen.");
		this->useStopEffect->addUserData(GameObject::AttrActionNeedRefresh());
		this->easeFunction->setDescription("If use stop effect is switched on, sets the to be used ease function for showing the chosen image.");
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

	bool RandomImageShuffler::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseSlideImage")
		{
			this->useSlideImage->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
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
			this->easeFunction->setVisible(useStopEffect);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EaseFunction")
		{
			this->setEaseFunction(XMLConverter::getAttrib(propertyElement, "data"));
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
		clonedCompPtr->setUseSlideImage(this->useSlideImage->getBool());
		clonedCompPtr->setUseStopDelay(this->useStopDelay->getBool());
		clonedCompPtr->setUseStopEffect(this->useStopEffect->getBool());
		clonedCompPtr->setEaseFunction(this->easeFunction->getListSelectedValue());
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

		if (true == this->activated->getBool())
		{
			ENQUEUE_RENDER_COMMAND("RandomImageShuffler::connect",
			{
				this->imageWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(this->geometry->getVector4().x, this->geometry->getVector4().y, this->geometry->getVector4().z, this->geometry->getVector4().w), MyGUI::Align::HCenter, "Overlapped");

				this->initialGeometry = this->imageWidget->getAbsoluteRect();

				if (false == this->maskImage->getString().empty())
				{
					this->maskImageWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(this->geometry->getVector4().x, this->geometry->getVector4().y, this->geometry->getVector4().z, this->geometry->getVector4().w), MyGUI::Align::Stretch, "Overlapped");
					this->maskImageWidget->setImageTexture(this->maskImage->getString());
				}

				if (true == this->useSlideImage->getBool() && false == this->images.empty())
				{
					// Start off-screen
					// this->imageWidget->setRealPosition(MyGUI::FloatPoint(this->geometry->getVector4().x - this->geometry->getVector4().z, this->geometry->getVector4().y));
				}
			});
		}

		return true;
	}

	bool RandomImageShuffler::disconnect(void)
	{
		this->stopShuffle = true;
		this->startShuffle = false;

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		ENQUEUE_RENDER_COMMAND("RandomImageShuffler::disconnect",
		{
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
		});
		return true;
	}

	bool RandomImageShuffler::onCloned(void)
	{

		return true;
	}

	void RandomImageShuffler::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
	}

	void RandomImageShuffler::onOtherComponentRemoved(unsigned int index)
	{

	}

	void RandomImageShuffler::onOtherComponentAdded(unsigned int index)
	{

	}

	void RandomImageShuffler::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			auto closureFunction = [this, dt](Ogre::Real weight)
			{

				// If shuffle has been started
				if (true == this->startShuffle && false == this->stopShuffle)
				{
					// Checks if switch image, or use slide image

					this->timeSinceLastImageShown += dt;
					if (this->timeSinceLastImageShown >= this->displayTime->getReal() * 0.001f)
					{
						if (false == this->useSlideImage->getBool())
						{
							this->switchImage();
						}
						this->timeSinceLastImageShown = 0.0f;
					}

					if (true == this->useSlideImage->getBool())
					{
						this->slideImage(dt);
					}
				}
				else if (true == this->stopShuffle && true == this->startShuffle && true == this->useStopDelay->getBool())
				{
					this->timeSinceLastImageShown += dt;

					// If stop has been triggered but use stop delay is on, do not stop immediately, but gradually decrease speed, until it comes to an end

					if (true == this->useSlideImage->getBool())
					{
						this->slideImage(dt);
					}
					else
					{
						if (this->timeSinceLastImageShown >= this->gradientDelay * 0.001f)
						{
							if (false == this->useSlideImage->getBool())
							{
								this->switchImage();
							}
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
									NOWA::AppStateManager::LogicCommand logicCommand = [this]()
									{
										this->gameObjectPtr->getLuaScript()->callTableFunction(this->onImageChosenFunctionName->getString(), this->imageWidget->_getTextureName(), this->imageWidget->getUserString("ImageIndex"));
									};
									NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
								}
							}
						}
					}
				}

				if (true == this->animating)
				{
					this->animateImage(dt);
				}
			};
			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
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
		else if (RandomImageShuffler::AttrUseSlideImage() == attribute->getName())
		{
			this->setUseSlideImage(attribute->getBool());
		}
		else if (RandomImageShuffler::AttrUseStopDelay() == attribute->getName())
		{
			this->setUseStopDelay(attribute->getBool());
		}
		else if (RandomImageShuffler::AttrUseStopEffect() == attribute->getName())
		{
			this->setUseStopEffect(attribute->getBool());
		}
		else if (RandomImageShuffler::AttrEaseFunction() == attribute->getName())
		{
			this->setEaseFunction(attribute->getListSelectedValue());
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

	void RandomImageShuffler::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

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
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseSlideImage"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useSlideImage->getBool())));
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "EaseFunction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->easeFunction->getListSelectedValue())));
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
			this->animationOppositeDir = 1.0f;
			this->animationRound = 0;
			this->currentImageIndex = 0;
			this->slideSpeed = 2000.0f;
			this->scrollPositionX = 0.0f;
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

	void RandomImageShuffler::setUseSlideImage(bool useSlideImage)
	{
		this->useSlideImage->setValue(useSlideImage);
	}

	bool RandomImageShuffler::getUseSlideImage(void) const
	{
		return this->useSlideImage->getBool();
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
		this->easeFunction->setVisible(useStopEffect);
	}

	bool RandomImageShuffler::getUseStopEffect(void) const
	{
		return this->useStopEffect->getBool();
	}

	void RandomImageShuffler::setEaseFunction(const Ogre::String& easeFunction)
	{
		this->easeFunction->setListSelectedValue(easeFunction);
		this->easeFunctions = Interpolator::getInstance()->mapStringToEaseFunctions(easeFunction);
	}

	Ogre::String RandomImageShuffler::getEaseFunction(void) const
	{
		return this->easeFunction->getListSelectedValue();
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
			this->animationOppositeDir = 1.0f;
			this->animationRound = 0;
			this->currentImageIndex = 0;
			this->slideSpeed = 2000.0f;
			this->scrollPositionX = 0.0f;
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
				// this->switchImage();

				if (true == this->useStopEffect->getBool())
				{
					this->startAnimation();
				}

				if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->onImageChosenFunctionName->getString().empty())
				{
					NOWA::AppStateManager::LogicCommand logicCommand = [this]()
					{
						this->gameObjectPtr->getLuaScript()->callTableFunction(this->onImageChosenFunctionName->getString(), this->imageWidget->_getTextureName(), this->imageWidget->getUserString("ImageIndex"));
					};
					NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
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

	void RandomImageShuffler::switchImage(void)
	{
		static std::mt19937 rng(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
		std::uniform_int_distribution<int> dist(0, static_cast<int>(this->images.size() - 1));

		int index = dist(rng);
		this->imageWidget->setImageTexture(this->images[index]->getString());
		this->imageWidget->setUserString("ImageIndex", Ogre::StringConverter::toString(index));
	}

	void RandomImageShuffler::slideImage(Ogre::Real dt)
	{
		// Move image box from left to right
		this->scrollPositionX += this->slideSpeed * dt;

		if (this->scrollPositionX >= this->imageWidget->getImageSize().width * 2.0f)
		{
			// Once the image box is fully visible, switch to the next image
			this->currentImageIndex = (this->currentImageIndex + 1) % this->images.size();
			this->imageWidget->setImageTexture(this->images[this->currentImageIndex]->getString());
			this->imageWidget->setUserString("ImageIndex", Ogre::StringConverter::toString(this->currentImageIndex));

			this->scrollPositionX = 0.0f;
		}

		// Update image coordinate to scroll horizontally
		this->imageWidget->setImageRect(MyGUI::IntRect(this->imageWidget->getImageSize().width - static_cast<int>(this->scrollPositionX), 0, this->imageWidget->getImageSize().width * 2, this->imageWidget->getImageSize().height));

		// Calculate remaining distance to the next image boundary
		int distanceToNextImage = this->imageWidget->getImageSize().width - this->scrollPositionX;


		// Check if we need to stop
		if (true == this->stopShuffle)
		{
			// Gradually decrease the slide speed to simulate stopping
			this->slideSpeed -= 250.0f * dt; // Decrease speed by 250 units per second
			
			
			if (this->slideSpeed < 1000.0f)
			{
				this->slideSpeed = 1000.0f;
			}

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "--->dist " + Ogre::StringConverter::toString(distanceToNextImage) + " this->scrollPositionX: " + Ogre::StringConverter::toString(this->scrollPositionX) + " width: " + Ogre::StringConverter::toString(this->imageWidget->getImageSize().width));

			// Ensure that the image stops at the next boundary
			if (this->slideSpeed <= 1000.0f && distanceToNextImage <= 0)
			{
				this->imageWidget->setImageRect(MyGUI::IntRect(0, 0, this->imageWidget->getImageSize().width, this->imageWidget->getImageSize().height));
				this->slideSpeed = 0;
				this->startShuffle = false;

				if (true == this->useStopEffect->getBool())
				{
					this->startAnimation();
				}
				if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->onImageChosenFunctionName->getString().empty())
				{
					NOWA::AppStateManager::LogicCommand logicCommand = [this]()
					{
						this->gameObjectPtr->getLuaScript()->callTableFunction(this->onImageChosenFunctionName->getString(), this->imageWidget->_getTextureName(), this->imageWidget->getUserString("ImageIndex"));
					};
					NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
				}
			}
		}
	}

	void RandomImageShuffler::startAnimation(void)
	{
		this->animating = true;
		this->animationTime = 0.0f;
		this->animationOppositeDir = 1.0f;
		this->animationRound = 0;
	}

	void RandomImageShuffler::animateImage(Ogre::Real dt)
	{
		const Ogre::Real duration = 1.0f;

		if (Ogre::Math::RealEqual(this->animationOppositeDir, 1.0f))
		{
			this->animationTime += dt;
			if (this->animationTime >= duration)
			{
				this->animationOppositeDir *= -1.0f;
				this->animationRound++;
			}
		}
		else
		{
			this->animationTime -= dt;
			if (this->animationTime <= 0.0f)
			{
				this->animationOppositeDir *= -1.0f;
				this->animationRound++;
			}
		}
		// if the translation took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
		// also take the progress into account, the translation started at zero and should stop at zero
		if (this->animationRound == 2 && this->animationTime >= 0.0f)
		{
			this->animating = false; // Stop the animation
			this->imageWidget->setSize(MyGUI::IntSize(this->initialGeometry.width(), this->initialGeometry.height())); // Set back to original size

			return; // Exit here
		}

		Ogre::Real t = this->animationTime / duration;
		Ogre::Real resultValue = Interpolator::getInstance()->applyEaseFunction(0.0f, static_cast<Ogre::Real>(this->initialGeometry.width()) * 0.25f, this->easeFunctions, t);

		int width = static_cast<int>(this->initialGeometry.width() + resultValue);
		int height = static_cast<int>(this->initialGeometry.height() + resultValue);
		this->imageWidget->setSize(width, height);
	}

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