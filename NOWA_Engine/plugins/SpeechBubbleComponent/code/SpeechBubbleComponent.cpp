#include "NOWAPrecompiled.h"
#include "SpeechBubbleComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/GameObjectTitleComponent.h"
#include "gameobject/SimpleSoundComponent.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "RenderQueueEnums.h"

#include "OgreAbiUtils.h"
#include "OgreSimpleSpline.h"

namespace
{
    Ogre::String replaceAll(Ogre::String str, const Ogre::String& from, const Ogre::String& to)
    {
        size_t startPos = 0;
        while ((startPos = str.find(from, startPos)) != Ogre::String::npos)
        {
            str.replace(startPos, from.length(), to);
            startPos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    }

    Ogre::String mid(const Ogre::String& str, unsigned short pos1, unsigned short pos2)
    {
        unsigned short i;
        Ogre::String temp;
        for (i = pos1; i < pos2; i++)
        {
            temp += str[i];
        }

        return temp;
    }

    // Simple greedy word-wrap: breaks the caption into lines of at most
    // maxCharsPerLine characters, breaking on the last space before the limit
    // (never mid-word) so the bubble geometry (sized from the wrapped text's
    // AABB) and the visible text actually match up.
    Ogre::String wrapCaptionText(const Ogre::String& text, size_t maxCharsPerLine)
    {
        Ogre::String result;
        size_t lineStart = 0;
        size_t lastSpace = Ogre::String::npos;
        size_t lineLen = 0;

        for (size_t i = 0; i < text.size(); ++i)
        {
            char c = text[i];
            if (c == ' ')
            {
                lastSpace = i;
            }

            result += c;
            ++lineLen;

            if (lineLen >= maxCharsPerLine && lastSpace != Ogre::String::npos && lastSpace >= lineStart)
            {
                // Replace the space we broke on with a newline
                result[lastSpace] = '\n';
                lineLen = i - lastSpace;
                lineStart = lastSpace + 1;
                lastSpace = Ogre::String::npos;
            }
        }
        return result;
    }
}

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    SpeechBubbleComponent::SpeechBubbleComponent() :
        GameObjectComponent(),
        name("SpeechBubbleComponent"),
        lineNode(nullptr),
        manualObject(nullptr),
        gameObjectTitleComponent(nullptr),
        simpleSoundComponent(nullptr),
        indices(0),
        currentCaptionWidth(0.0f),
        currentCaptionHeight(0.0f),
        currentCharIndex(0),
        timeSinceLastRun(0.0f),
        couldDraw(false),
        speechDone(false),
        activated(new Variant(SpeechBubbleComponent::AttrActivated(), true, this->attributes)),
        caption(new Variant(SpeechBubbleComponent::AttrCaption(), "MyCaption", this->attributes)),
        runSpeech(new Variant(SpeechBubbleComponent::AttrRunSpeech(), false, this->attributes)),
        speechDuration(new Variant(SpeechBubbleComponent::AttrSpeechDuration(), 10.0f, this->attributes)),
        runSpeechSound(new Variant(SpeechBubbleComponent::AttrRunSpeechSound(), false, this->attributes)),
        keepCaption(new Variant(SpeechBubbleComponent::AttrKeepCaption(), false, this->attributes)),
        xOffsetStart(new Variant(SpeechBubbleComponent::AttrXOffsetStart(), -0.5f, this->attributes))
    {
        this->runSpeech->setDescription("Sets whether the caption should remain after the speech run.");
        this->speechDuration->setDescription("Sets the speed duration. That is how long the bubble shall remain in seconds.");
        this->runSpeechSound->setDescription("Sets whether to use a sound if the speech is running char by char.");
        this->keepCaption->setDescription("Sets whether to use a sound if the speech is running char by char.");
        this->xOffsetStart->setDescription("Horizontal offset of the bubble body (ellipse and tail) relative to the text anchor, "
                                            "in the local +X direction. Only the bubble shape shifts - the text stays centered "
                                            "on the anchor. Useful to keep the bubble from covering the character while the "
                                            "tail still points at its mouth.");
    }

    SpeechBubbleComponent::~SpeechBubbleComponent(void)
    {
    }

    void SpeechBubbleComponent::initialise()
    {
    }

    const Ogre::String& SpeechBubbleComponent::getName() const
    {
        return this->name;
    }

    void SpeechBubbleComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<SpeechBubbleComponent>(SpeechBubbleComponent::getStaticClassId(), SpeechBubbleComponent::getStaticClassName());
    }

    void SpeechBubbleComponent::shutdown()
    {
    }

    void SpeechBubbleComponent::uninstall()
    {
    }

    void SpeechBubbleComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool SpeechBubbleComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Caption")
        {
            this->caption->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RunSpeech")
        {
            this->runSpeech->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpeechDuration")
        {
            this->speechDuration->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RunSpeechSound")
        {
            this->runSpeechSound->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "KeepCaption")
        {
            this->keepCaption->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "XOffsetStart")
        {
            this->xOffsetStart->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr SpeechBubbleComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        SpeechBubbleComponentPtr clonedCompPtr(boost::make_shared<SpeechBubbleComponent>());

        clonedCompPtr->setCaption(this->caption->getString());
        clonedCompPtr->setRunSpeech(this->runSpeech->getBool());
        clonedCompPtr->setSpeechDuration(this->speechDuration->getReal());
        clonedCompPtr->setRunSpeechSound(this->runSpeechSound->getBool());
        clonedCompPtr->setKeepCaption(this->keepCaption->getBool());
        clonedCompPtr->setXOffsetStart(this->xOffsetStart->getReal());

        clonedCompPtr->setActivated(this->activated->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool SpeechBubbleComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SpeechBubbleComponent] Init component for game object: " + this->gameObjectPtr->getName());

        auto gameObjectTitleCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<GameObjectTitleComponent>());
        if (nullptr != gameObjectTitleCompPtr)
        {
            this->gameObjectTitleComponent = gameObjectTitleCompPtr.get();
            auto captionAttribute = this->gameObjectTitleComponent->getAttribute(GameObjectTitleComponent::AttrCaption());
            if (nullptr != captionAttribute)
            {
                captionAttribute->setVisible(false);
            }

            auto alwaysPresent = this->gameObjectTitleComponent->getAttribute(GameObjectTitleComponent::AttrAlwaysPresent());
            if (nullptr != alwaysPresent)
            {
                alwaysPresent->setValue(true);
            }
        }

        return true;
    }

    bool SpeechBubbleComponent::connect(void)
    {
        GameObjectComponent::connect();

        auto gameObjectTitleCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<GameObjectTitleComponent>());
        if (nullptr != gameObjectTitleCompPtr)
        {
            this->gameObjectTitleComponent = gameObjectTitleCompPtr.get();
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]
            {
                this->gameObjectTitleComponent->getMovableText()->setVisibleRequested(true);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::connect movable text visible true");
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpeechBubbleComponent] This component will not work, as a prior GameObjectTitleComponent is missing for game object: " + this->gameObjectPtr->getName());
        }

        auto simpleSoundCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<SimpleSoundComponent>());
        if (nullptr != simpleSoundCompPtr)
        {
            this->simpleSoundComponent = simpleSoundCompPtr.get();
        }

        this->setCaption(this->caption->getString());

        this->createSpeechBubble();

        this->timeSinceLastChar = 0.0f;
        this->timeSinceLastRun = 0.0f;
        this->speechDone = false;

        return true;
    }

    bool SpeechBubbleComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->indices = 0;
        this->currentCaptionWidth = 0.0f;
        this->currentCaptionHeight = 0.0f;
        this->currentCharIndex = 0;
        this->timeSinceLastChar = 0.0f;
        this->timeSinceLastRun = 0.0f;
        this->speechDone = false;
        this->couldDraw = false;

        if (nullptr != this->simpleSoundComponent)
        {
            this->simpleSoundComponent->setActivated(false);
        }

        if (nullptr != this->gameObjectTitleComponent)
        {
            this->gameObjectTitleComponent = nullptr;
            this->simpleSoundComponent = nullptr;
        }
        this->destroySpeechBubble();

        return true;
    }

    void SpeechBubbleComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating && true == this->activated->getBool())
        {
            if (nullptr == this->manualObject)
            {
                return;
            }

            auto closureFunction = [this](Ogre::Real renderDt)
            {
                this->indices = 0;
                if (this->manualObject->getNumSections() > 0)
                {
                    // Ogre will crash or throw exceptions if empty manual object is processed
                    if (true == this->couldDraw)
                    {
                        this->manualObject->beginUpdate(0);
                    }
                }
                else
                {
                    this->manualObject->clear();
                    this->manualObject->begin("WhiteNoLightingBackground", Ogre::OT_TRIANGLE_LIST);
                }

                this->drawSpeechBubble(renderDt);

                // Ogre will crash or throw exceptions if empty manual object is processed
                if (true == this->couldDraw)
                {
                    // Realllllllyyyyy important! Else the rectangle is a whole mess!
                    this->manualObject->index(0);
                    this->manualObject->end();
                }
                else
                {
                    this->manualObject->clear();
                }
            };
            Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
            NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
        }
    }

    void SpeechBubbleComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (SpeechBubbleComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (SpeechBubbleComponent::AttrCaption() == attribute->getName())
        {
            this->setCaption(attribute->getString());
        }
        else if (SpeechBubbleComponent::AttrRunSpeech() == attribute->getName())
        {
            this->setRunSpeech(attribute->getBool());
        }
        else if (SpeechBubbleComponent::AttrSpeechDuration() == attribute->getName())
        {
            this->setSpeechDuration(attribute->getReal());
        }
        else if (SpeechBubbleComponent::AttrRunSpeechSound() == attribute->getName())
        {
            this->setRunSpeechSound(attribute->getBool());
        }
        else if (SpeechBubbleComponent::AttrKeepCaption() == attribute->getName())
        {
            this->setKeepCaption(attribute->getBool());
        }
        else if (SpeechBubbleComponent::AttrXOffsetStart() == attribute->getName())
        {
            this->setXOffsetStart(attribute->getReal());
        }
    }

    void SpeechBubbleComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Caption"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->caption->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RunSpeech"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->runSpeech->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SpeechDuration"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speechDuration->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RunSpeechSound"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->runSpeechSound->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "KeepCaption"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->keepCaption->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "XOffsetStart"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->xOffsetStart->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    void SpeechBubbleComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        if (nullptr != this->gameObjectTitleComponent)
        {
            auto captionAttribute = this->gameObjectTitleComponent->getAttribute(SpeechBubbleComponent::AttrCaption());
            if (nullptr != captionAttribute)
            {
                captionAttribute->setVisible(true);
            }
        }
    }

    Ogre::String SpeechBubbleComponent::getClassName(void) const
    {
        return "SpeechBubbleComponent";
    }

    Ogre::String SpeechBubbleComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void SpeechBubbleComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, activated]
        {
            if (true == this->bConnected)
            {
                if (false == activated)
                {

                    if (nullptr != this->gameObjectTitleComponent)
                    {
                        this->gameObjectTitleComponent->getMovableText()->setVisibleRequested(false);
                    }
                }
                else
                {
                    if (nullptr != this->gameObjectTitleComponent)
                    {
                        this->gameObjectTitleComponent->getMovableText()->setVisibleRequested(true);
                    }
                    this->createSpeechBubble();
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setActivated");
    }

    bool SpeechBubbleComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void SpeechBubbleComponent::setCaption(const Ogre::String& caption)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, caption]
        {
            auto gameObjectTitleCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<GameObjectTitleComponent>());
            if (nullptr != gameObjectTitleCompPtr)
            {
                this->gameObjectTitleComponent = gameObjectTitleCompPtr.get();
                this->gameObjectTitleComponent->getMovableText()->setVisibleRequested(true);
            }

            Ogre::String tempCaption = replaceAll(caption, "\\n", "\n");
            this->caption->setValue(tempCaption);

            // Word-wrap BEFORE anything measures width - both the bubble sizing
            // below (getLocalAabb() on the full wrapped text) and the typewriter
            // effect in drawSpeechBubble() must use this SAME wrapped string, or
            // the bubble and the visible text drift apart again exactly like this
            // bug did with the single unwrapped line.
            this->wrappedCaption = wrapCaptionText(tempCaption, 40); // tune 40 to taste

            if (nullptr != this->gameObjectTitleComponent)
            {
                this->gameObjectTitleComponent->setCaption(this->wrappedCaption);
                // Must be called, in order to calculate bounding box for currentCaptionWidth, currentCaptionHeight
                this->gameObjectTitleComponent->getMovableText()->forceUpdate();

                this->gameObjectTitleComponent->getMovableText()->setVisibleRequested(true);

                this->currentCharIndex = 0;
                this->timeSinceLastRun = 0.0f;

                // Calculate speech bubble size
                Ogre::Aabb textAabb = this->gameObjectTitleComponent->getMovableText()->getLocalAabb();

                // GameObjectTitleComponent's text is H_CENTER-aligned (each wrapped
                // line is centered on local x = 0 individually, see
                // MovableText::_setupGeometry()), so getMaximum().x is already the
                // half-width the widest line needs. Halving it again here shrank the
                // bubble to roughly a quarter of the real text width - the "text
                // pokes out both sides" bug. Take the larger of the two half-extents
                // so a wider second/third line (different length than the widest
                // positive side) is still fully enclosed.
                this->currentCaptionWidth = this->gameObjectTitleComponent->getMovableText()->getLocalAabb().getMaximum().x * 0.5f + 0.3f;
                this->currentCaptionHeight = (textAabb.getMaximum().y - textAabb.getMinimum().y) * 0.5f + 0.1f;

                if (true == this->runSpeech->getBool())
                {
                    this->gameObjectTitleComponent->setCaption("");
                }
            }

            if (nullptr != this->lineNode)
            {
                this->lineNode->setVisible(true);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setCaption");
    }

    Ogre::String SpeechBubbleComponent::getCaption(void) const
    {
        return this->caption->getString();
    }

    void SpeechBubbleComponent::setRunSpeech(bool runSpeech)
    {
        this->runSpeech->setValue(runSpeech);
        if (false == runSpeech)
        {
            this->setCaption(this->caption->getString());
        }
    }

    bool SpeechBubbleComponent::getRunSpeech(void) const
    {
        return this->runSpeech->getBool();
    }

    void SpeechBubbleComponent::setSpeechDuration(Ogre::Real speechDurationSec)
    {
        if (speechDurationSec < 0.0f)
        {
            speechDurationSec = 0.0f;
        }
        this->speechDuration->setValue(speechDurationSec);
    }

    Ogre::Real SpeechBubbleComponent::getSpeechDuration(void) const
    {
        return this->speechDuration->getReal();
    }

    void SpeechBubbleComponent::setRunSpeechSound(bool runSpeechSound)
    {
        if (true == runSpeechSound)
        {
            if (nullptr == this->simpleSoundComponent)
            {
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[SpeechBubbleComponent] Could not set run speech sound, because there is no simple sound component. Add one first! For game object: " + this->gameObjectPtr->getName());
                return;
            }
        }

        this->runSpeechSound->setValue(runSpeechSound);
    }

    bool SpeechBubbleComponent::getRunSpeechSound(void) const
    {
        return this->runSpeechSound->getBool();
    }

    void SpeechBubbleComponent::setXOffsetStart(Ogre::Real xOffsetStart)
    {
        this->xOffsetStart->setValue(xOffsetStart);
    }

    Ogre::Real SpeechBubbleComponent::getXOffsetStart(void) const
    {
        return this->xOffsetStart->getReal();
    }

    void SpeechBubbleComponent::setKeepCaption(bool keepCaption)
    {
        this->keepCaption->setValue(keepCaption);
    }

    bool SpeechBubbleComponent::getKeepCaption(void) const
    {
        return this->keepCaption->getBool();
    }

    void SpeechBubbleComponent::reactOnSpeechDone(luabind::object closureFunction)
    {
        this->closureFunction = closureFunction;
    }

    void SpeechBubbleComponent::drawSpeechBubble(Ogre::Real dt)
    {
        this->couldDraw = false;

        if (nullptr != this->gameObjectTitleComponent)
        {
            if (this->caption->getString().empty() || this->currentCaptionWidth == 0.0f)
            {
                return;
            }

            if (true == this->runSpeech->getBool())
            {
                size_t totalCharacters = this->wrappedCaption.length();
                if (totalCharacters > 0)
                {
                    Ogre::Real timePerCharacter = this->speechDuration->getReal() / static_cast<Ogre::Real>(totalCharacters);

                    if (this->timeSinceLastChar >= timePerCharacter && this->currentCharIndex < totalCharacters)
                    {
                        this->timeSinceLastChar = 0.0f;
                        this->currentCharIndex++;

                        Ogre::String captionSoFar = mid(this->wrappedCaption, 0, this->currentCharIndex);
                        this->gameObjectTitleComponent->setCaption(captionSoFar);

                        if (this->runSpeechSound->getBool() && this->simpleSoundComponent)
                        {
                            Ogre::Real rndPitch = static_cast<Ogre::Real>(MathHelper::getInstance()->getRandomNumber(3, 10)) * 0.1f;
                            this->simpleSoundComponent->setPitch(rndPitch);
                            this->simpleSoundComponent->setActivated(true);
                        }
                    }
                }

                if (this->gameObjectTitleComponent->getCaption() == this->wrappedCaption && this->timeSinceLastRun >= this->speechDuration->getReal() + 3.0f)
                {
                    this->speechDone = true;
                    if (this->simpleSoundComponent)
                    {
                        this->simpleSoundComponent->setActivated(false);
                    }
                }

                if (this->speechDone)
                {
                    this->timeSinceLastChar = 0.0f;
                    this->timeSinceLastRun = 0.0f;
                    if (false == this->keepCaption->getBool())
                    {
                        this->gameObjectTitleComponent->getMovableText()->setVisibleRequested(false);
                        this->lineNode->setVisible(false);
                    }
                    this->speechDone = false;

                    if (this->closureFunction.is_valid())
                    {
                        NOWA::AppStateManager::LogicCommand logicCommand = [this]()
                        {
                            try
                            {
                                luabind::call_function<void>(this->closureFunction);
                            }
                            catch (luabind::error& error)
                            {
                                luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                                std::stringstream msg;
                                msg << errorMsg;

                                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[SpeechBubbleComponent] Caught error in 'reactOnSpeechDone' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
                            }
                        };
                        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
                    }
                }

                this->timeSinceLastChar += dt;
                this->timeSinceLastRun += dt;
            }

            if (true == this->gameObjectTitleComponent->getCaption().empty())
            {
                return;
            }

            Ogre::Vector3 p = this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_getDerivedPosition();
            Ogre::Quaternion o = this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_getDerivedOrientation();

            Ogre::Vector3 sp = Ogre::Vector3::ZERO;
            Ogre::Quaternion so = Ogre::Quaternion::IDENTITY;

            Ogre::Real accuracy = 17.0f;

            // Read the text's ACTUAL local AABB every frame - makes the bubble grow
            // smoothly as characters are typed, and avoids hardcoding assumptions
            // about text alignment (debug logging previously confirmed the text sits
            // ABOVE the anchor, not below).
            Ogre::Aabb liveTextAabb = this->gameObjectTitleComponent->getMovableText()->getLocalAabb();

            Ogre::Real textHalfWidth = (liveTextAabb.getMaximum().x - liveTextAabb.getMinimum().x) * 0.5f;
            Ogre::Real textHalfHeight = (liveTextAabb.getMaximum().y - liveTextAabb.getMinimum().y) * 0.5f;
            Ogre::Real textCenterY = (liveTextAabb.getMaximum().y + liveTextAabb.getMinimum().y) * 0.5f;

                        // An ellipse with radii (w*k, h*k) only fully contains a rectangle of
            // half-extents (w, h) when 2/k^2 <= 1, i.e. k >= sqrt(2) ~ 1.4142. The
            // previous 1.3 / 1.4 were BOTH below that threshold, so the four corners
            // of the text block were mathematically guaranteed to stick out. The more
            // wrapped lines, the larger the corner area and the more visible the bug.
            const Ogre::Real cornerFit = Ogre::Math::Sqrt(2.0f);
            const Ogre::Real widthPadding = cornerFit * 1.06f; // sqrt(2) + 6% breathing room
            const Ogre::Real heightPadding = cornerFit * 1.06f;

            const Ogre::Real bubbleHorizontalOffset = this->xOffsetStart->getReal();

            // X was previously assumed to be centred on local 0, which only holds for
            // H_CENTER alignment. GameObjectTitleComponent exposes the alignment as an
            // editable property, so with H_LEFT the text sat entirely on one side while
            // the ellipse stayed centred - the bubble ended up in the wrong place.
            const Ogre::Real textCenterX = (liveTextAabb.getMaximum().x + liveTextAabb.getMinimum().x) * 0.5f;

            // The ellipse is shifted by bubbleHorizontalOffset while the text is not,
            // so the required radius has to cover the distance from the shifted centre
            // out to the far text edge, not just the text's own half width.
            const Ogre::Real farthestTextDx = Ogre::Math::Abs(textCenterX - (textCenterX + bubbleHorizontalOffset)) + textHalfWidth;

            const Ogre::Real ellipseRadiusX = std::max(farthestTextDx * widthPadding, 0.1f);
            const Ogre::Real ellipseRadiusY = std::max(textHalfHeight * heightPadding, 0.1f);
            const Ogre::Real ellipseCenterX = textCenterX + bubbleHorizontalOffset;
            const Ogre::Real ellipseCenterY = textCenterY;

            // Tail: tip sits exactly at the anchor (local origin = mouth position,
            // NOT shifted by bubbleHorizontalOffset - the tail must still point at
            // the character's mouth even while the bubble body is shifted sideways).
            // Base vertices sit exactly ON the (shifted) ellipse boundary, computed
            // with the SAME parametric formula as the fan loop below - guaranteed to
            // touch, regardless of text size or offset.
            const Ogre::Real tailAngle1 = 310.0f * Ogre::Math::PI / 180.0f;
            const Ogre::Real tailAngle2 = 350.0f * Ogre::Math::PI / 180.0f;

            Ogre::Vector3 tailBase1(ellipseCenterX + ellipseRadiusX * Ogre::Math::Cos(tailAngle1), ellipseCenterY + ellipseRadiusY * Ogre::Math::Sin(tailAngle1), 0.0f);
            Ogre::Vector3 tailBase2(ellipseCenterX + ellipseRadiusX * Ogre::Math::Cos(tailAngle2), ellipseCenterY + ellipseRadiusY * Ogre::Math::Sin(tailAngle2), 0.0f);
            Ogre::Vector3 tailTip = Ogre::Vector3::ZERO;

            this->manualObject->position(p + (o * (so * (sp + tailBase1))));
            this->manualObject->colour(Ogre::ColourValue::White);
            this->manualObject->index(this->indices + 0);

            this->manualObject->position(p + (o * (so * (sp + tailBase2))));
            this->manualObject->colour(Ogre::ColourValue::White);
            this->manualObject->index(this->indices + 1);

            this->manualObject->position(p + (o * (so * (sp + tailTip))));
            this->manualObject->colour(Ogre::ColourValue::White);
            this->manualObject->index(this->indices + 2);
            this->indices += 3;

            std::vector<Ogre::Vector3> points(3);
            for (Ogre::Real theta = 0; theta <= 2 * Ogre::Math::PI; theta += Ogre::Math::PI / accuracy)
            {
                points[0] = Ogre::Vector3(ellipseCenterX, ellipseCenterY, 0.0f);
                points[1] = Ogre::Vector3(ellipseCenterX + ellipseRadiusX * Ogre::Math::Cos(theta - Ogre::Math::PI / accuracy), ellipseCenterY + ellipseRadiusY * Ogre::Math::Sin(theta - Ogre::Math::PI / accuracy), 0.0f);
                points[2] = Ogre::Vector3(ellipseCenterX + ellipseRadiusX * Ogre::Math::Cos(theta), ellipseCenterY + ellipseRadiusY * Ogre::Math::Sin(theta), 0.0f);

                this->manualObject->position(p + (o * (so * (sp + points[0]))));
                this->manualObject->colour(Ogre::ColourValue::White);
                this->manualObject->index(this->indices + 0);

                this->manualObject->position(p + (o * (so * (sp + points[1]))));
                this->manualObject->colour(Ogre::ColourValue::White);
                this->manualObject->index(this->indices + 1);

                this->manualObject->position(p + (o * (so * (sp + points[2]))));
                this->manualObject->colour(Ogre::ColourValue::White);
                this->manualObject->index(this->indices + 2);
                this->indices += 3;
            }

            this->couldDraw = true;
        }
    }

    void SpeechBubbleComponent::createSpeechBubble(void)
    {
        if (nullptr == this->manualObject)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]
            {
                if (nullptr == this->lineNode)
                {
                    this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
                }
                this->manualObject = this->gameObjectPtr->getSceneManager()->createManualObject();
                this->manualObject->setRenderQueueGroup(212);
                // this->manualObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_TRANSPARENT);

                this->manualObject->setName("SpeechBubble_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(index));
                this->manualObject->setQueryFlags(0 << 0);
                this->lineNode->attachObject(this->manualObject);
                this->manualObject->setCastShadows(false);
                this->lineNode->setVisible(true);

                // Depth bias so the bubble sorts BEHIND the text in the depth buffer
                // WITHOUT any actual world-space offset - a world-space offset
                // previously introduced visible parallax at grazing camera angles.
                // Modifies the shared "WhiteNoLightingBackground" datablock directly -
                // fine as long as this material is only used for background/bubble-
                // style flat quads elsewhere too (both a small depth push and
                // disabled culling are harmless for those).
                Ogre::HlmsDatablock* bubbleBaseDatablock = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault("WhiteNoLightingBackground");
                if (nullptr != bubbleBaseDatablock)
                {
                    Ogre::HlmsMacroblock macroblock = *bubbleBaseDatablock->getMacroblock();
                    macroblock.mDepthBiasConstant = 1.0f;
                    macroblock.mDepthBiasSlopeScale = 1.0f;
                    // Render both sides - the bubble is a flat, non-billboarding quad
                    // that must stay readable when viewed from behind the character.
                    macroblock.mCullMode = Ogre::CULL_NONE;
                    bubbleBaseDatablock->setMacroblock(macroblock);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::createSpeechBubble");
        }
    }

    void SpeechBubbleComponent::destroySpeechBubble(void)
    {
        if (this->lineNode != nullptr)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]
            {
                this->lineNode->detachAllObjects();
                this->gameObjectPtr->getSceneManager()->destroyManualObject(this->manualObject);
                this->manualObject = nullptr;
                this->lineNode->getParentSceneNode()->removeAndDestroyChild(this->lineNode);
                this->lineNode = nullptr;
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::destroySpeechBubble");
        }
    }

    // Lua registration part

    SpeechBubbleComponent* getSpeechBubbleComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<SpeechBubbleComponent>(gameObject->getComponentWithOccurrence<SpeechBubbleComponent>(occurrenceIndex)).get();
    }

    SpeechBubbleComponent* getSpeechBubbleComponent(GameObject* gameObject)
    {
        return makeStrongPtr<SpeechBubbleComponent>(gameObject->getComponent<SpeechBubbleComponent>()).get();
    }

    SpeechBubbleComponent* getSpeechBubbleComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<SpeechBubbleComponent>(gameObject->getComponentFromName<SpeechBubbleComponent>(name)).get();
    }

    void SpeechBubbleComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<SpeechBubbleComponent, GameObjectComponent>("SpeechBubbleComponent")
                .def("setActivated", &SpeechBubbleComponent::setActivated)
                .def("isActivated", &SpeechBubbleComponent::isActivated)
                .def("setRunSpeech", &SpeechBubbleComponent::setRunSpeech)
                .def("getRunSpeech", &SpeechBubbleComponent::getRunSpeech)
                .def("setCaption", &SpeechBubbleComponent::setCaption)
                .def("getCaption", &SpeechBubbleComponent::getCaption)
                .def("setSpeechDuration", &SpeechBubbleComponent::setSpeechDuration)
                .def("getSpeechDuration", &SpeechBubbleComponent::getSpeechDuration)
                .def("setRunSpeechSound", &SpeechBubbleComponent::setRunSpeechSound)
                .def("getRunSpeechSound", &SpeechBubbleComponent::getRunSpeechSound)
                .def("setKeepCaption", &SpeechBubbleComponent::setKeepCaption)
                .def("getKeepCaption", &SpeechBubbleComponent::getKeepCaption)
                .def("setXOffsetStart", &SpeechBubbleComponent::setXOffsetStart)
                .def("getXOffsetStart", &SpeechBubbleComponent::getXOffsetStart)
                .def("reactOnSpeechDone", &SpeechBubbleComponent::reactOnSpeechDone)];

        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "class inherits GameObjectComponent", SpeechBubbleComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool isActivated()", "Gets whether this component is activated.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setTextColor(Vector3 color)", "Sets the color for the text.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "Vector3 getTextColor()", "Gets the text color.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setRunSpeech(bool runSpeech)", "Sets whether the speech text shall appear char by char running.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool getRunSpeech()", "Gets whether the speech text shall appear char by char running.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setCaption(string caption)", "Sets the caption text to be displayed.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "string getCaption()", "Gets the caption text.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setSpeechDuration(float speechDuration)", "Sets the speed duration. That is how long the bubble shall remain in seconds.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getSpeechDuration()", "Gets the speed duration. That is how long the bubble shall remain in seconds.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setKeepCaption(bool keepCaption)", "Sets to keep the caption after the run speech is done.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool getKeepCaption()", "Gets whether the caption after the run speech is done is kept.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setXOffsetStart(float xOffsetStart)", "Sets the horizontal offset of the bubble body relative to the text anchor.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getXOffsetStart()", "Gets the horizontal offset of the bubble body relative to the text anchor.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setRunSpeechSound(bool runSpeechSound)", "Sets whether the caption should remain after the speech run.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool getRunSpeechSound()", "Gets whether the caption is remained after the speech run.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void reactOnSpeechDone(func closureFunction)", "Sets whether to react if a speech is done.");

        gameObjectClass.def("getSpeechBubbleComponentFromName", &getSpeechBubbleComponentFromName);
        gameObjectClass.def("getSpeechBubbleComponent", (SpeechBubbleComponent * (*)(GameObject*)) & getSpeechBubbleComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SpeechBubbleComponent getSpeechBubbleComponent()", "Gets the component. This can be used if the game object this component just once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SpeechBubbleComponent getSpeechBubbleComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castSpeechBubbleComponent", &GameObjectController::cast<SpeechBubbleComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "SpeechBubbleComponent castSpeechBubbleComponent(SpeechBubbleComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

    bool SpeechBubbleComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Can only be added once
        auto gameObjectTitleCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<GameObjectTitleComponent>());
        if (nullptr == gameObjectTitleCompPtr)
        {
            return false;
        }

        if (gameObject->getComponentCount<SpeechBubbleComponent>() < 2)
        {
            return true;
        }

        return true;
    }

}; // namespace end