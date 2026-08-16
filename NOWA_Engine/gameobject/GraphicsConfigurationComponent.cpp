#include "NOWAPrecompiled.h"
#include "GraphicsConfigurationComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/MyGUIComponents.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "modules/WorkspaceModule.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    GraphicsConfigurationComponent::GraphicsConfigurationComponent() :
        GameObjectComponent(),
        name("GraphicsConfigurationComponent"),
        widget(nullptr),
        hasParent(false),
        isSimulating(false),
        eventsWired(false),
        resolutionCombo(nullptr),
        fullscreenCheck(nullptr),
        vsyncCheck(nullptr),
        vsyncIntervalCombo(nullptr),
        fsaaCombo(nullptr),
        shadowQualityCombo(nullptr),
        restartRequiredLabel(nullptr),
        applyButton(nullptr),
        okButton(nullptr),
        cancelButton(nullptr),
        restartRequired(false),
        activated(new Variant(GraphicsConfigurationComponent::AttrActivated(), false, this->attributes)),
        relativePosition(new Variant(GraphicsConfigurationComponent::AttrRelativePosition(), Ogre::Vector2(0.3f, 0.2f), this->attributes)),
        relativeSize(new Variant(GraphicsConfigurationComponent::AttrRelativeSize(), Ogre::Vector2(0.4f, 0.5f), this->attributes)),
        skin(new Variant(GraphicsConfigurationComponent::AttrSkin(), {"WoodWindow", "WoodPanel", "Window", "WindowCSX", "WindowC", "WindowCX", "WindowCS", "PanelSkin"}, this->attributes)),
        layer(new Variant(GraphicsConfigurationComponent::AttrLayer(), {"Overlapped", "Main", "Modal", "Middle", "Back", "DragAndDrop", "Wallpaper", "Info", "FadeMiddle", "FadeBusy", "Pointer", "Fade", "Statistic"}, this->attributes)),
        parentId(new Variant(GraphicsConfigurationComponent::AttrParentId(), static_cast<unsigned long>(0), this->attributes, true)),
        showAdvancedOptions(new Variant(GraphicsConfigurationComponent::AttrShowAdvancedOptions(), true, this->attributes)),
        okClickEventName(new Variant(GraphicsConfigurationComponent::AttrOkClickEventName(), Ogre::String(""), this->attributes)),
        applyClickEventName(new Variant(GraphicsConfigurationComponent::AttrApplyClickEventName(), Ogre::String(""), this->attributes)),
        cancelClickEventName(new Variant(GraphicsConfigurationComponent::AttrCancelClickEventName(), Ogre::String(""), this->attributes))
    {
        this->activated->setDescription("Shows the graphics configuration menu if activated.");
        this->relativeSize->setDescription("Sets the relative size of the configuration window. Increase it, if the rows do overlap.");
        this->skin->setDescription("Sets the skin of the configuration window.");
        this->layer->setDescription("Sets the MyGUI layer. Must be a layer that receives mouse picking (e.g. 'Overlapped'), else the window cannot be interacted with at all, not even natively by MyGUI (no dropdown, no checkbox, no click).");
        this->showAdvancedOptions->setDescription("If true, the shadow quality option is shown besides the display options.");
    }

    GraphicsConfigurationComponent::~GraphicsConfigurationComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GraphicsConfigurationComponent] Destructor graphics configuration component for game object: " + this->gameObjectPtr->getName());

        this->destroyMyGUIWidgets();
    }

    bool GraphicsConfigurationComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RelativePosition")
        {
            this->relativePosition->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RelativeSize")
        {
            this->relativeSize->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Skin")
        {
            this->skin->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Layer")
        {
            this->layer->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ParentId")
        {
            this->parentId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowAdvancedOptions")
        {
            this->showAdvancedOptions->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OkClickEventName")
        {
            this->setOkClickEventName(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ApplyClickEventName")
        {
            this->setApplyClickEventName(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CancelClickEventName")
        {
            this->setCancelClickEventName(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr GraphicsConfigurationComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // Cloning does not make sense, because there may only be one graphics configuration menu
        return nullptr;
    }

    bool GraphicsConfigurationComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GraphicsConfigurationComponent] Init graphics configuration component for game object: " + this->gameObjectPtr->getName());

        this->setActivated(this->activated->getBool());

        return true;
    }

    bool GraphicsConfigurationComponent::connect(void)
    {
        GameObjectComponent::connect();

        this->isSimulating = true;

        // If the widget already exists (activated=true from the scene), wire the events now.
        // If it does not exist yet, wireEvents() is called from createMyGuiWidgets() instead,
        // once the player activates the menu at runtime via Lua.
        this->wireEvents();

        return true;
    }

    bool GraphicsConfigurationComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        this->isSimulating = false;

        this->unwireEvents();

        return true;
    }

    bool GraphicsConfigurationComponent::onCloned(void)
    {
        return true;
    }

    void GraphicsConfigurationComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        this->destroyMyGUIWidgets();
    }

    void GraphicsConfigurationComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    void GraphicsConfigurationComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (GraphicsConfigurationComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (GraphicsConfigurationComponent::AttrRelativePosition() == attribute->getName())
        {
            this->setRelativePosition(attribute->getVector2());
        }
        else if (GraphicsConfigurationComponent::AttrRelativeSize() == attribute->getName())
        {
            this->setRelativeSize(attribute->getVector2());
        }
        else if (GraphicsConfigurationComponent::AttrSkin() == attribute->getName())
        {
            this->setSkin(attribute->getListSelectedValue());
        }
        else if (GraphicsConfigurationComponent::AttrLayer() == attribute->getName())
        {
            this->setLayer(attribute->getListSelectedValue());
        }
        else if (GraphicsConfigurationComponent::AttrParentId() == attribute->getName())
        {
            this->setParentId(attribute->getULong());
        }
        else if (GraphicsConfigurationComponent::AttrShowAdvancedOptions() == attribute->getName())
        {
            this->setShowAdvancedOptions(attribute->getBool());
        }
        else if (GraphicsConfigurationComponent::AttrOkClickEventName() == attribute->getName())
        {
            this->setOkClickEventName(attribute->getString());
        }
        else if (GraphicsConfigurationComponent::AttrApplyClickEventName() == attribute->getName())
        {
            this->setApplyClickEventName(attribute->getString());
        }
        else if (GraphicsConfigurationComponent::AttrCancelClickEventName() == attribute->getName())
        {
            this->setCancelClickEventName(attribute->getString());
        }
    }

    void GraphicsConfigurationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RelativePosition"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->relativePosition->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RelativeSize"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->relativeSize->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Skin"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->skin->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Layer"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->layer->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ParentId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->parentId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShowAdvancedOptions"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showAdvancedOptions->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OkClickEventName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->okClickEventName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ApplyClickEventName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->applyClickEventName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CancelClickEventName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cancelClickEventName->getString())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String GraphicsConfigurationComponent::getClassName(void) const
    {
        return "GraphicsConfigurationComponent";
    }

    Ogre::String GraphicsConfigurationComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void GraphicsConfigurationComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (true == activated)
        {
            this->createMyGuiWidgets();
        }
        else
        {
            this->destroyMyGUIWidgets();
        }
    }

    bool GraphicsConfigurationComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void GraphicsConfigurationComponent::setRelativePosition(const Ogre::Vector2& relativePosition)
    {
        this->relativePosition->setValue(relativePosition);

        if (nullptr != this->widget)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, relativePosition]()
            {
                this->widget->setRealPosition(relativePosition.x, relativePosition.y);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::setRelativePosition");
        }
    }

    Ogre::Vector2 GraphicsConfigurationComponent::getRelativePosition(void) const
    {
        return this->relativePosition->getVector2();
    }

    void GraphicsConfigurationComponent::setRelativeSize(const Ogre::Vector2& relativeSize)
    {
        this->relativeSize->setValue(relativeSize);

        if (nullptr != this->widget)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, relativeSize]()
            {
                this->widget->setRealSize(relativeSize.x, relativeSize.y);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::setRelativeSize");
        }
    }

    Ogre::Vector2 GraphicsConfigurationComponent::getRelativeSize(void) const
    {
        return this->relativeSize->getVector2();
    }

    void GraphicsConfigurationComponent::setSkin(const Ogre::String& skin)
    {
        this->skin->setListSelectedValue(skin);

        // The skin can only be exchanged by rebuilding the whole window, because the
        // child widgets are attached to it. Only do so, if the window does already exist.
        if (nullptr != this->widget)
        {
            const bool wasSimulating = this->isSimulating;
            this->destroyMyGUIWidgets();
            this->createMyGuiWidgets();
            if (true == wasSimulating)
            {
                this->wireEvents();
            }
        }
    }

    Ogre::String GraphicsConfigurationComponent::getSkin(void) const
    {
        return this->skin->getListSelectedValue();
    }

    void GraphicsConfigurationComponent::setLayer(const Ogre::String& layer)
    {
        this->layer->setListSelectedValue(layer);

        // Same as skin: the layer can only be changed by moving the widget, which MyGUI
        // supports directly via attachToLayerNode, no rebuild necessary.
        if (nullptr != this->widget)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, layer]()
            {
                MyGUI::LayerManager::getInstance().attachToLayerNode(layer, this->widget);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::setLayer");
        }
    }

    Ogre::String GraphicsConfigurationComponent::getLayer(void) const
    {
        return this->layer->getListSelectedValue();
    }

    void GraphicsConfigurationComponent::setParentId(unsigned long parentId)
    {
        this->parentId->setValue(parentId);

        if (nullptr == this->gameObjectPtr || nullptr == this->widget)
        {
            return;
        }

        for (unsigned int i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
        {
            auto gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(i));
            if (nullptr != gameObjectCompPtr)
            {
                auto myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
                if (nullptr != myGuiCompPtr && this->parentId->getULong() == myGuiCompPtr->getId())
                {
                    if (nullptr != myGuiCompPtr->getWidget())
                    {
                        MyGUI::Widget* parentWidget = myGuiCompPtr->getWidget();

                        NOWA::GraphicsModule::RenderCommand renderCommand = [this, parentWidget]()
                        {
                            this->widget->attachToWidget(parentWidget);
                        };
                        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::setParentId attach");

                        this->hasParent = true;
                        this->setRelativePosition(this->relativePosition->getVector2());
                        return;
                    }
                }
            }
        }

        // No parent found, so detach if it was attached before
        if (true == this->hasParent)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->widget->detachFromWidget();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::setParentId detach");

            this->hasParent = false;
        }
    }

    unsigned long GraphicsConfigurationComponent::getParentId(void) const
    {
        return this->parentId->getULong();
    }

    void GraphicsConfigurationComponent::setShowAdvancedOptions(bool showAdvancedOptions)
    {
        this->showAdvancedOptions->setValue(showAdvancedOptions);

        // Rebuild, because the window height and the row layout depend on this flag
        if (nullptr != this->widget)
        {
            const bool wasSimulating = this->isSimulating;
            this->destroyMyGUIWidgets();
            this->createMyGuiWidgets();
            if (true == wasSimulating)
            {
                this->wireEvents();
            }
        }
    }

    bool GraphicsConfigurationComponent::getShowAdvancedOptions(void) const
    {
        return this->showAdvancedOptions->getBool();
    }

    void GraphicsConfigurationComponent::setOkClickEventName(const Ogre::String& okClickEventName)
    {
        this->okClickEventName->setValue(okClickEventName);
        this->okClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), okClickEventName + "(thisComponent)=" + this->getClassName());
    }

    Ogre::String GraphicsConfigurationComponent::getOkClickEventName(void) const
    {
        return this->okClickEventName->getString();
    }

    void GraphicsConfigurationComponent::setApplyClickEventName(const Ogre::String& applyClickEventName)
    {
        this->applyClickEventName->setValue(applyClickEventName);
        this->applyClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), applyClickEventName + "(thisComponent)=" + this->getClassName());
    }

    Ogre::String GraphicsConfigurationComponent::getApplyClickEventName(void) const
    {
        return this->applyClickEventName->getString();
    }

    void GraphicsConfigurationComponent::setCancelClickEventName(const Ogre::String& cancelClickEventName)
    {
        this->cancelClickEventName->setValue(cancelClickEventName);
        this->cancelClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), cancelClickEventName + "(thisComponent)=" + this->getClassName());
    }

    Ogre::String GraphicsConfigurationComponent::getCancelClickEventName(void) const
    {
        return this->cancelClickEventName->getString();
    }

    MyGUI::Window* GraphicsConfigurationComponent::getWindow(void) const
    {
        return this->widget;
    }

    bool GraphicsConfigurationComponent::parseResolution(const Ogre::String& videoMode, unsigned int& outWidth, unsigned int& outHeight) const
    {
        // Format is: "1920 x 1080 @ 32-bit colour"
        Ogre::String::size_type separatorPosition = videoMode.find('x');
        if (Ogre::String::npos == separatorPosition)
        {
            return false;
        }

        outWidth = static_cast<unsigned int>(Ogre::StringConverter::parseInt(videoMode.substr(0, separatorPosition)));
        outHeight = static_cast<unsigned int>(Ogre::StringConverter::parseInt(videoMode.substr(separatorPosition + 1)));

        if (0 == outWidth || 0 == outHeight)
        {
            return false;
        }

        return true;
    }

    MyGUI::EditBox* GraphicsConfigurationComponent::createLabel(const Ogre::String& caption, Ogre::Real posY)
    {
        MyGUI::EditBox* label = this->widget->createWidgetReal<MyGUI::EditBox>("TextBox", 0.04f, posY, 0.42f, 0.055f, MyGUI::Align::Left | MyGUI::Align::Top);
        label->setCaptionWithReplacing(caption);
        label->setEditReadOnly(true);
        label->setEditStatic(true);
        this->createdWidgets.emplace_back(label);
        return label;
    }

    void GraphicsConfigurationComponent::createMyGuiWidgets(void)
    {
        if (nullptr != this->widget)
        {
            return;
        }

        const bool advanced = this->showAdvancedOptions->getBool();

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, advanced]()
        {
            // The layer MUST be one that receives mouse picking (e.g. 'Overlapped'). Layers
            // reserved for passive overlays never route mouse focus to their widgets at all,
            // so not even MyGUI's own native behaviour (dropdown open, checkbox toggle) works.
            this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Window>(this->skin->getListSelectedValue(), this->relativePosition->getVector2().x, this->relativePosition->getVector2().y, this->relativeSize->getVector2().x,
                this->relativeSize->getVector2().y, MyGUI::Align::Left | MyGUI::Align::Top, this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));

            this->widget->setCaptionWithReplacing("#{Graphics_Configuration}");
            this->widget->setMovable(true);

            const Ogre::Real rowHeight = 0.10f;
            const Ogre::Real controlX = 0.50f;
            const Ogre::Real controlWidth = 0.44f;
            const Ogre::Real controlHeight = 0.07f;
            Ogre::Real posY = 0.08f;

            MyGUI::LanguageManager* languageManager = MyGUI::LanguageManager::getInstancePtr();

            // ── Resolution ────────────────────────────────────────────────────
            this->createLabel("#{Resolution}:", posY);
            this->resolutionCombo = this->widget->createWidgetReal<MyGUI::ComboBox>("ComboBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
            this->resolutionCombo->setComboModeDrop(true);
            this->resolutionCombo->setEditReadOnly(true);
            this->createdWidgets.emplace_back(this->resolutionCombo);
            posY += rowHeight;

            // ── Fullscreen ────────────────────────────────────────────────────
            this->createLabel("#{Fullscreen}:", posY);
            this->fullscreenCheck = this->widget->createWidgetReal<MyGUI::Button>("CheckBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
            this->createdWidgets.emplace_back(this->fullscreenCheck);
            posY += rowHeight;

            // ── VSync ─────────────────────────────────────────────────────────
            this->createLabel("#{VSync}:", posY);
            this->vsyncCheck = this->widget->createWidgetReal<MyGUI::Button>("CheckBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
            this->createdWidgets.emplace_back(this->vsyncCheck);
            posY += rowHeight;

            // ── VSync interval ────────────────────────────────────────────────
            this->createLabel("#{VSync_Interval}:", posY);
            this->vsyncIntervalCombo = this->widget->createWidgetReal<MyGUI::ComboBox>("ComboBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
            this->vsyncIntervalCombo->setComboModeDrop(true);
            this->vsyncIntervalCombo->setEditReadOnly(true);
            this->vsyncIntervalCombo->addItem("1");
            this->vsyncIntervalCombo->addItem("2");
            this->vsyncIntervalCombo->addItem("3");
            this->createdWidgets.emplace_back(this->vsyncIntervalCombo);
            posY += rowHeight;

            // ── Anti aliasing (restart required) ───────────────────────────────
            this->createLabel("#{Anti_Aliasing}:", posY);
            this->fsaaCombo = this->widget->createWidgetReal<MyGUI::ComboBox>("ComboBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
            this->fsaaCombo->setComboModeDrop(true);
            this->fsaaCombo->setEditReadOnly(true);
            this->createdWidgets.emplace_back(this->fsaaCombo);
            posY += rowHeight;

            if (true == advanced)
            {
                // ── Shadow quality ────────────────────────────────────────────
                this->createLabel("#{Shadow_Quality}:", posY);
                this->shadowQualityCombo = this->widget->createWidgetReal<MyGUI::ComboBox>("ComboBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
                this->shadowQualityCombo->setComboModeDrop(true);
                this->shadowQualityCombo->setEditReadOnly(true);
                // Index 0 maps to -1 (scene default), index 1..4 map to shadow filter 0..3.
                // ComboBox::addItem does NOT run caption replacing, so #{} tags must be
                // resolved manually here, else MyGUI misinterprets the leading '#' as an
                // inline colour code and renders broken, partially coloured text.
                this->shadowQualityCombo->addItem(languageManager->replaceTags("#{Scene_Default}"));
                this->shadowQualityCombo->addItem(languageManager->replaceTags("#{Low}"));
                this->shadowQualityCombo->addItem(languageManager->replaceTags("#{Medium}"));
                this->shadowQualityCombo->addItem(languageManager->replaceTags("#{High}"));
                this->shadowQualityCombo->addItem(languageManager->replaceTags("#{Ultra}"));
                this->createdWidgets.emplace_back(this->shadowQualityCombo);
                posY += rowHeight;
            }
            else
            {
                this->shadowQualityCombo = nullptr;
            }

            // ── Restart required hint ─────────────────────────────────────────
            this->restartRequiredLabel = this->widget->createWidgetReal<MyGUI::EditBox>("TextBox", 0.04f, posY, 0.90f, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
            this->restartRequiredLabel->setCaptionWithReplacing("#{Restart_Required}");
            this->restartRequiredLabel->setTextColour(MyGUI::Colour::Red);
            this->restartRequiredLabel->setEditReadOnly(true);
            this->restartRequiredLabel->setEditStatic(true);
            this->restartRequiredLabel->setVisible(false);
            this->createdWidgets.emplace_back(this->restartRequiredLabel);
            posY += rowHeight;

            // ── Buttons — name is required here so buttonHit can dispatch by name ──
            this->applyButton = this->widget->createWidgetReal<MyGUI::Button>("Button", 0.04f, 0.90f, 0.26f, 0.07f, MyGUI::Align::Left | MyGUI::Align::Bottom, "graphicsApplyButton");
            this->applyButton->setCaptionWithReplacing("#{Apply}");
            this->createdWidgets.emplace_back(this->applyButton);

            this->okButton = this->widget->createWidgetReal<MyGUI::Button>("Button", 0.36f, 0.90f, 0.26f, 0.07f, MyGUI::Align::Left | MyGUI::Align::Bottom, "graphicsOkButton");
            this->okButton->setCaptionWithReplacing("#{Ok}");
            this->createdWidgets.emplace_back(this->okButton);

            this->cancelButton = this->widget->createWidgetReal<MyGUI::Button>("Button", 0.68f, 0.90f, 0.26f, 0.07f, MyGUI::Align::Left | MyGUI::Align::Bottom, "graphicsCancelButton");
            this->cancelButton->setCaptionWithReplacing("#{Cancel}");
            this->createdWidgets.emplace_back(this->cancelButton);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::createMyGuiWidgets");

        // Snapshot the state as it is right now, so that the cancel button can restore exactly this state
        this->captureCurrentSettings();
        this->populateWidgetsFromSettings();

        // Attach maybe the window to a parent MyGUI window
        this->setParentId(this->parentId->getULong());

        // If simulation is already running (menu opened at runtime, e.g. from a pause menu),
        // wire the events immediately, else connect() will do it once simulation starts.
        if (true == this->isSimulating)
        {
            this->wireEvents();
        }
    }

    void GraphicsConfigurationComponent::destroyMyGUIWidgets(void)
    {
        if (nullptr == this->widget)
        {
            return;
        }

        this->unwireEvents();

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (true == this->hasParent)
            {
                this->widget->detachFromWidget();
                this->hasParent = false;
            }

            // Destroying the window destroys all its children too, so the child pointers must only be nulled
            MyGUI::Gui::getInstancePtr()->destroyWidget(this->widget);
            this->widget = nullptr;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::destroyMyGUIWidgets");

        this->createdWidgets.clear();

        this->resolutionCombo = nullptr;
        this->fullscreenCheck = nullptr;
        this->vsyncCheck = nullptr;
        this->vsyncIntervalCombo = nullptr;
        this->fsaaCombo = nullptr;
        this->shadowQualityCombo = nullptr;
        this->restartRequiredLabel = nullptr;
        this->applyButton = nullptr;
        this->okButton = nullptr;
        this->cancelButton = nullptr;
        this->restartRequired = false;
    }

    void GraphicsConfigurationComponent::wireEvents(void)
    {
        if (nullptr == this->widget || true == this->eventsWired)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GraphicsConfigurationComponent::buttonHit);
            this->okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GraphicsConfigurationComponent::buttonHit);
            this->cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GraphicsConfigurationComponent::buttonHit);

            // MyGUI::Button (used as CheckBox skin) does not invert its own check state on
            // click by itself — that must be wired explicitly, else the checkbox never toggles.
            this->fullscreenCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyCheckBoxClick);
            this->vsyncCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyCheckBoxClick);

            this->resolutionCombo->eventComboAccept += MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyComboAccept);
            this->vsyncIntervalCombo->eventComboAccept += MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyComboAccept);
            this->fsaaCombo->eventComboAccept += MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyComboAccept);

            if (nullptr != this->shadowQualityCombo)
            {
                this->shadowQualityCombo->eventComboAccept += MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyComboAccept);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::wireEvents");

        this->eventsWired = true;
    }

    void GraphicsConfigurationComponent::unwireEvents(void)
    {
        if (nullptr == this->widget || false == this->eventsWired)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr != this->applyButton)
            {
                this->applyButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &GraphicsConfigurationComponent::buttonHit);
            }
            if (nullptr != this->okButton)
            {
                this->okButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &GraphicsConfigurationComponent::buttonHit);
            }
            if (nullptr != this->cancelButton)
            {
                this->cancelButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &GraphicsConfigurationComponent::buttonHit);
            }
            if (nullptr != this->fullscreenCheck)
            {
                this->fullscreenCheck->eventMouseButtonClick -= MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyCheckBoxClick);
            }
            if (nullptr != this->vsyncCheck)
            {
                this->vsyncCheck->eventMouseButtonClick -= MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyCheckBoxClick);
            }
            if (nullptr != this->resolutionCombo)
            {
                this->resolutionCombo->eventComboAccept -= MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyComboAccept);
            }
            if (nullptr != this->vsyncIntervalCombo)
            {
                this->vsyncIntervalCombo->eventComboAccept -= MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyComboAccept);
            }
            if (nullptr != this->fsaaCombo)
            {
                this->fsaaCombo->eventComboAccept -= MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyComboAccept);
            }
            if (nullptr != this->shadowQualityCombo)
            {
                this->shadowQualityCombo->eventComboAccept -= MyGUI::newDelegate(this, &GraphicsConfigurationComponent::notifyComboAccept);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::unwireEvents");

        this->eventsWired = false;
    }

    void GraphicsConfigurationComponent::captureCurrentSettings(void)
    {
        Core* core = Core::getSingletonPtr();

        std::pair<unsigned int, unsigned int> resolution = core->getCurrentVideoModeResolution();

        this->initialSettings.width = resolution.first;
        this->initialSettings.height = resolution.second;
        this->initialSettings.fullscreen = core->getIsFullscreen();
        this->initialSettings.vsync = core->getIsVSync();
        this->initialSettings.vsyncInterval = 1;
        this->initialSettings.fsaa = core->getCurrentFSAA();
        this->initialSettings.shadowQuality = core->getShadowQuality();

        this->currentSettings = this->initialSettings;
        this->restartRequired = false;
    }

    void GraphicsConfigurationComponent::populateWidgetsFromSettings(void)
    {
        if (nullptr == this->widget)
        {
            return;
        }

        Core* core = Core::getSingletonPtr();

        std::vector<Ogre::String> videoModes = core->getAvailableVideoModes();
        std::vector<Ogre::String> fsaaModes = core->getAvailableFSAAModes();
        Ogre::String currentVideoMode = core->getCurrentVideoMode();

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, videoModes, fsaaModes, currentVideoMode]()
        {
            // ── Resolution ────────────────────────────────────────────────────
            this->resolutionCombo->removeAllItems();
            size_t selectedResolutionIndex = 0;
            for (size_t i = 0; i < videoModes.size(); i++)
            {
                this->resolutionCombo->addItem(videoModes[i]);
                if (videoModes[i] == currentVideoMode)
                {
                    selectedResolutionIndex = i;
                }
            }
            if (this->resolutionCombo->getItemCount() > 0)
            {
                this->resolutionCombo->setIndexSelected(selectedResolutionIndex);
            }

            // ── Fullscreen / VSync ────────────────────────────────────────────
            this->fullscreenCheck->setStateCheck(this->currentSettings.fullscreen);
            this->vsyncCheck->setStateCheck(this->currentSettings.vsync);

            // ── VSync interval ────────────────────────────────────────────────
            size_t vsyncIntervalIndex = 0;
            if (this->currentSettings.vsyncInterval >= 1 && this->currentSettings.vsyncInterval <= 3)
            {
                vsyncIntervalIndex = static_cast<size_t>(this->currentSettings.vsyncInterval - 1);
            }
            this->vsyncIntervalCombo->setIndexSelected(vsyncIntervalIndex);

            // ── Anti aliasing ─────────────────────────────────────────────────
            this->fsaaCombo->removeAllItems();
            size_t selectedFsaaIndex = 0;
            for (size_t i = 0; i < fsaaModes.size(); i++)
            {
                this->fsaaCombo->addItem(fsaaModes[i]);
                if (fsaaModes[i] == this->currentSettings.fsaa)
                {
                    selectedFsaaIndex = i;
                }
            }
            if (this->fsaaCombo->getItemCount() > 0)
            {
                this->fsaaCombo->setIndexSelected(selectedFsaaIndex);
            }

            // ── Shadow quality ────────────────────────────────────────────────
            if (nullptr != this->shadowQualityCombo)
            {
                // Index 0 is the scene default (-1), index 1..4 map to 0..3
                this->shadowQualityCombo->setIndexSelected(static_cast<size_t>(this->currentSettings.shadowQuality + 1));
            }

            this->restartRequiredLabel->setVisible(this->restartRequired);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GraphicsConfigurationComponent::populateWidgetsFromSettings");
    }

    void GraphicsConfigurationComponent::readSettingsFromWidgets(void)
    {
        if (nullptr == this->widget)
        {
            return;
        }

        // Attention: This is called from the render thread inside a MyGUI event handler,
        // so no enqueueAndWait may be used here, else it would deadlock.
        if (MyGUI::ITEM_NONE != this->resolutionCombo->getIndexSelected())
        {
            Ogre::String selectedVideoMode = this->resolutionCombo->getItemNameAt(this->resolutionCombo->getIndexSelected());
            unsigned int width = 0;
            unsigned int height = 0;
            if (true == this->parseResolution(selectedVideoMode, width, height))
            {
                this->currentSettings.width = width;
                this->currentSettings.height = height;
            }
        }

        this->currentSettings.fullscreen = this->fullscreenCheck->getStateCheck();
        this->currentSettings.vsync = this->vsyncCheck->getStateCheck();

        if (MyGUI::ITEM_NONE != this->vsyncIntervalCombo->getIndexSelected())
        {
            this->currentSettings.vsyncInterval = static_cast<unsigned int>(this->vsyncIntervalCombo->getIndexSelected()) + 1;
        }

        if (MyGUI::ITEM_NONE != this->fsaaCombo->getIndexSelected())
        {
            this->currentSettings.fsaa = this->fsaaCombo->getItemNameAt(this->fsaaCombo->getIndexSelected());
        }

        if (nullptr != this->shadowQualityCombo)
        {
            if (MyGUI::ITEM_NONE != this->shadowQualityCombo->getIndexSelected())
            {
                this->currentSettings.shadowQuality = static_cast<short>(this->shadowQualityCombo->getIndexSelected()) - 1;
            }
        }
    }

    void GraphicsConfigurationComponent::applySettings(const GraphicsSettings& settings)
    {
        // Attention: This must run on the logic thread, because Core::setVideoMode and friends
        // use enqueueAndWait internally. Calling it from a MyGUI event handler (render thread)
        // would deadlock.
        Core* core = Core::getSingletonPtr();

        // ── Immediately applicable options ────────────────────────────────────
        std::pair<unsigned int, unsigned int> currentResolution = core->getCurrentVideoModeResolution();
        if (currentResolution.first != settings.width || currentResolution.second != settings.height)
        {
            core->setVideoMode(settings.width, settings.height);
        }

        if (core->getIsFullscreen() != settings.fullscreen)
        {
            core->setFullscreen(settings.fullscreen, 0);
        }

        core->setVSync(settings.vsync, settings.vsyncInterval);

        // ── Restart required options ──────────────────────────────────────────
        if (core->getCurrentFSAA() != settings.fsaa)
        {
            core->setFSAA(settings.fsaa);
        }

        // ── Engine quality options ────────────────────────────────────────────
        if (core->getShadowQuality() != settings.shadowQuality)
        {
            core->setShadowQuality(settings.shadowQuality);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GraphicsConfigurationComponent] Applied graphics settings.");
    }

    void GraphicsConfigurationComponent::saveSettings(void)
    {
        Core::getSingletonPtr()->saveGraphicsConfig();
        Core::getSingletonPtr()->saveCustomConfiguration();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GraphicsConfigurationComponent] Saved graphics settings.");
    }

    void GraphicsConfigurationComponent::notifyCheckBoxClick(MyGUI::Widget* sender)
    {
        // MyGUI::Button (used as CheckBox skin) does not invert its own check state on
        // click, that must be done explicitly, same pattern as PropertiesPanelComponent::buttonHit.
        MyGUI::Button* button = sender->castType<MyGUI::Button>(false);
        if (nullptr != button)
        {
            button->setStateCheck(!button->getStateCheck());
        }
    }

    void GraphicsConfigurationComponent::notifyComboAccept(MyGUI::ComboBox* sender, size_t index)
    {
        // Runs on the render thread, only read the widget values, do not apply anything here
        this->readSettingsFromWidgets();

        // The anti aliasing value can only take effect after a restart, so tell the player at once
        if (sender == this->fsaaCombo)
        {
            if (this->currentSettings.fsaa != this->initialSettings.fsaa)
            {
                this->restartRequired = true;
                this->restartRequiredLabel->setVisible(true);
            }
            else
            {
                this->restartRequired = false;
                this->restartRequiredLabel->setVisible(false);
            }
        }
    }

    void GraphicsConfigurationComponent::buttonHit(MyGUI::Widget* sender)
    {
        // Attention: This runs on the render thread. Everything that touches Core::setVideoMode
        // and friends must be deferred to the logic thread, because those use enqueueAndWait
        // internally, which would deadlock when called from the render thread.

        if ("graphicsApplyButton" == sender->getName())
        {
            this->readSettingsFromWidgets();

            const GraphicsSettings settingsToApply = this->currentSettings;

            NOWA::AppStateManager::LogicCommand logicCommand = [this, settingsToApply]()
            {
                this->applySettings(settingsToApply);

                if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->applyClickEventName->getString().empty())
                {
                    this->gameObjectPtr->getLuaScript()->callTableFunction(this->applyClickEventName->getString(), this);
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
        }
        else if ("graphicsOkButton" == sender->getName())
        {
            this->readSettingsFromWidgets();

            const GraphicsSettings settingsToApply = this->currentSettings;

            NOWA::AppStateManager::LogicCommand logicCommand = [this, settingsToApply]()
            {
                this->applySettings(settingsToApply);
                this->saveSettings();

                // Closes the window
                this->setActivated(false);

                if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->okClickEventName->getString().empty())
                {
                    this->gameObjectPtr->getLuaScript()->callTableFunction(this->okClickEventName->getString(), this);
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
        }
        else if ("graphicsCancelButton" == sender->getName())
        {
            const GraphicsSettings settingsToRestore = this->initialSettings;

            NOWA::AppStateManager::LogicCommand logicCommand = [this, settingsToRestore]()
            {
                // Restores exactly the state the window has been opened with
                this->applySettings(settingsToRestore);
                this->currentSettings = settingsToRestore;

                // Closes the window
                this->setActivated(false);

                if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->cancelClickEventName->getString().empty())
                {
                    this->gameObjectPtr->getLuaScript()->callTableFunction(this->cancelClickEventName->getString(), this);
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
        }
    }

    bool GraphicsConfigurationComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Only one graphics configuration menu does make sense per game object
        if (gameObject->getComponentCount<GraphicsConfigurationComponent>() < 1)
        {
            return true;
        }
        return false;
    }

    // Lua registration part

    GraphicsConfigurationComponent* getGraphicsConfigurationComponent(GameObject* gameObject)
    {
        return makeStrongPtr<GraphicsConfigurationComponent>(gameObject->getComponent<GraphicsConfigurationComponent>()).get();
    }

    GraphicsConfigurationComponent* getGraphicsConfigurationComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<GraphicsConfigurationComponent>(gameObject->getComponentFromName<GraphicsConfigurationComponent>(name)).get();
    }

    void GraphicsConfigurationComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<GraphicsConfigurationComponent, GameObjectComponent>("GraphicsConfigurationComponent")
                .def("setActivated", &GraphicsConfigurationComponent::setActivated)
                .def("isActivated", &GraphicsConfigurationComponent::isActivated)
                .def("setRelativePosition", &GraphicsConfigurationComponent::setRelativePosition)
                .def("getRelativePosition", &GraphicsConfigurationComponent::getRelativePosition)
                .def("setRelativeSize", &GraphicsConfigurationComponent::setRelativeSize)
                .def("getRelativeSize", &GraphicsConfigurationComponent::getRelativeSize)
                .def("setSkin", &GraphicsConfigurationComponent::setSkin)
                .def("getSkin", &GraphicsConfigurationComponent::getSkin)
                .def("setLayer", &GraphicsConfigurationComponent::setLayer)
                .def("getLayer", &GraphicsConfigurationComponent::getLayer)
                .def("setShowAdvancedOptions", &GraphicsConfigurationComponent::setShowAdvancedOptions)
                .def("getShowAdvancedOptions", &GraphicsConfigurationComponent::getShowAdvancedOptions)];

        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "class inherits GameObjectComponent", GraphicsConfigurationComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not. If activated, the graphics configuration menu is shown.");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "bool isActivated()", "Gets whether this component is activated.");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "void setRelativePosition(Vector2 relativePosition)", "Sets the relative position of the configuration window (0 = top/left, 1 = bottom/right).");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "Vector2 getRelativePosition()", "Gets the relative position of the configuration window.");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "void setRelativeSize(Vector2 relativeSize)", "Sets the relative size of the configuration window.");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "Vector2 getRelativeSize()", "Gets the relative size of the configuration window.");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "void setSkin(String skin)", "Sets the skin of the configuration window, e.g. 'WoodWindow'.");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "String getSkin()", "Gets the skin of the configuration window.");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "void setLayer(String layer)", "Sets the MyGUI layer. Must be a layer that receives mouse picking (e.g. 'Overlapped').");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "String getLayer()", "Gets the MyGUI layer.");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "void setShowAdvancedOptions(bool showAdvancedOptions)", "Sets whether the shadow quality option is shown.");
        LuaScriptApi::getInstance()->addClassToCollection("GraphicsConfigurationComponent", "bool getShowAdvancedOptions()", "Gets whether the shadow quality option is shown.");

        gameObjectClass.def("getGraphicsConfigurationComponentFromName", &getGraphicsConfigurationComponentFromName);
        gameObjectClass.def("getGraphicsConfigurationComponent", (GraphicsConfigurationComponent * (*)(GameObject*)) & getGraphicsConfigurationComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "GraphicsConfigurationComponent getGraphicsConfigurationComponent()", "Gets the component. This can be used if the game object has this component just once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "GraphicsConfigurationComponent getGraphicsConfigurationComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castGraphicsConfigurationComponent", &GameObjectController::cast<GraphicsConfigurationComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "GraphicsConfigurationComponent castGraphicsConfigurationComponent(GraphicsConfigurationComponent other)",
            "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end