#include "NOWAPrecompiled.h"
#include "MyGUIMiniMapComponent.h"
#include "LuaScriptComponent.h"
#include "MyGUI_LayerManager.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "modules/InputDeviceModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    MiniMapToolTip::MiniMapToolTip()
    {
        MyGUI::LayoutManager::getInstance().loadLayout("ToolTip2.layout");
        this->toolTip = MyGUI::Gui::getInstance().findWidget<MyGUI::Widget>("tooltipPanel");
        this->textDescription = MyGUI::Gui::getInstance().findWidget<MyGUI::EditBox>("text_Desc");
    }

    void MiniMapToolTip::show(const MyGUI::IntPoint& point, const Ogre::String& description)
    {
        GraphicsModule::RenderCommand renderCommand = [this, point, description]()
        {
            // First fetch the viewport size.  (Do not try to getParent()->getSize().
            // Top level widgets do not have parents, but getParentSize() returns something useful anyway.)
            const MyGUI::IntSize& viewSize = this->toolTip->getParentSize();
            // Then set our tooltip panel size to something excessive...
            this->toolTip->setSize(viewSize.width / 2, viewSize.height / 2);
            // ... update its caption to whatever the sender widget has for tooltip text
            // (You did use setUserString(), right?)...
            MyGUI::UString toolTipText = description;
            if (true == toolTipText.empty())
            {
                return;
            }
            this->textDescription->setCaption(toolTipText);
            // ... fetch the new text size from the tooltip's Edit control...
            const MyGUI::IntSize& textSize = this->textDescription->getTextSize();
            // ... and resize the tooltip panel to match it.  The Stretch property on the Edit
            // control will see to it that the Edit control resizes along with it.
            // The constants are padding to fit in the edges of the PanelSmall skin; adjust as
            // necessary for your theme.
            this->toolTip->setSize(textSize.width + 6, textSize.height + 6);
            // You can fade it in smooth if you like, but that gets obnoxious.
            this->toolTip->setVisible(true);

            boundedMove(this->toolTip, point);
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "MiniMapToolTip::show");
    }

    void MiniMapToolTip::hide()
    {
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->toolTip->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "MiniMapToolTip::hide");
    }

    void MiniMapToolTip::move(const MyGUI::IntPoint& point)
    {
        GraphicsModule::RenderCommand renderCommand = [this, point]()
        {
            this->boundedMove(this->toolTip, point);
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "MiniMapToolTip::move");
    }

    void MiniMapToolTip::boundedMove(MyGUI::Widget* moving, const MyGUI::IntPoint& point)
    {
        const MyGUI::IntPoint offset(16, 16); // typical mouse cursor size - offset out from under it

        MyGUI::IntPoint boundedpoint = point + offset;

        const MyGUI::IntSize& size = moving->getSize();
        const MyGUI::IntSize& viewSize = moving->getParentSize();

        if ((boundedpoint.left + size.width) > viewSize.width)
        {
            boundedpoint.left -= offset.left + offset.left + size.width;
        }
        if ((boundedpoint.top + size.height) > viewSize.height)
        {
            boundedpoint.top -= offset.top + offset.top + size.height;
        }

        moving->setPosition(boundedpoint);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    MyGUIMiniMapComponent::MyGUIMiniMapComponent() : MyGUIWindowComponent(), toolTip(nullptr), miniMapTilesCount(0), bShowMiniMap(false), timeSinceLastUpdate(0.2f)
    {
        this->startPosition = new Variant(MyGUIMiniMapComponent::AttrStartPosition(), Ogre::Vector2(0.5f, 0.5f), this->attributes);
        this->scaleFactor = new Variant(MyGUIMiniMapComponent::AttrScaleFactor(), 1.0f, this->attributes);
        this->axis = new Variant(MyGUIMiniMapComponent::AttrAxis(), std::vector<Ogre::String>{"X,Y", "X,Z"}, this->attributes);
        this->showNames = new Variant(MyGUIMiniMapComponent::AttrShowNames(), true, this->attributes);
        this->useToolTip = new Variant(MyGUIMiniMapComponent::AttrUseToolTip(), true, this->attributes);
        this->useVisitation = new Variant(MyGUIMiniMapComponent::AttrUseVisitation(), true, this->attributes);

        this->trackableCount = new Variant(MyGUIMiniMapComponent::AttrTrackableCount(), 0, this->attributes);
        this->trackableImageAnimationSpeed = new Variant(MyGUIMiniMapComponent::AttrTrackableImageAnimationSpeed(), 0.2f, this->attributes);

        this->trackableCount->addUserData(GameObject::AttrActionNeedRefresh());

        this->position->setValue(Ogre::Vector2(0.0f, 0.0f));
        this->size->setValue(Ogre::Vector2(1.0f, 1.0f));

        this->useVisitation->setDescription("If activated, only tiles of scenes the player has entered are visible, plus those revealed via their 'Scene Visited' attribute or @setSceneVisited(...).");
        this->startPosition->setDescription("Relative position in the mini map window, at which the center of the whole map is placed. '0.5 0.5' is centered.");
        this->scaleFactor->setDescription("1 fits the whole map into the window, bigger values zoom in (around the 'Start Position').");

        this->axis->setDescription("The axis for exit direction. For Jump'n'Run e.g. 'X,Y' is correct and for a casual 3D scene 'X,Z'.");
        this->trackableCount->setDescription("Sets the count of track able game objects. The track able id is used to specify the game object that should be tracked on minimap. "
                                             "If the track able id is in another scene, the scene name must be specified. For example 'scene3:2341435213'"
                                             "Will search in scene3 for the game object with the id 2341435213 and in conjunction with the image attribute, the image will be placed correctly on the minimap. "
                                             "If the scene name is missing, its assumed, that the id is an global one(like the player which is available for each scene) and has the same id for each scene.");
        this->trackableImageAnimationSpeed->setDescription("Sets the trackable image animation speed in seconds. E.g. 0.5 would update the image 2 times a second.");
    }

    MyGUIMiniMapComponent::~MyGUIMiniMapComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIMiniMapComponent] Destructor MyGUI mini map component for game object: " + this->gameObjectPtr->getName());

        this->destroyTrackables();
        this->destroyMiniMap();

        if (nullptr != this->toolTip)
        {
            GraphicsModule::RenderCommand renderCommand = [this]()
            {
                delete this->toolTip;
                this->toolTip = nullptr;
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::~MyGUIMiniMapComponent");
        }
    }

    bool MyGUIMiniMapComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = MyGUIWindowComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartPosition")
        {
            this->startPosition->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScaleFactor")
        {
            this->scaleFactor->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseToolTip")
        {
            this->useToolTip->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseVisitation")
        {
            this->useVisitation->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Axis")
        {
            this->axis->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowNames")
        {
            this->showNames->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MiniMapTilesCount")
        {
            this->miniMapTilesCount = XMLConverter::getAttribUnsignedInt(propertyElement, "data");
            propertyElement = propertyElement->next_sibling("property");
        }

        // Only create new variant, if fresh loading. If snapshot is done, no new variant
        // must be created! Because the algorithm is working changed flag of each existing variant!
        if (this->skinNames.size() < this->miniMapTilesCount)
        {
            this->skinNames.resize(this->miniMapTilesCount);
            this->miniMapTilesColors.resize(this->miniMapTilesCount);
            this->toolTipDescriptions.resize(this->miniMapTilesCount);
            this->visitedList.resize(this->miniMapTilesCount);
        }

        for (size_t i = 0; i < this->miniMapTilesCount; i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SkinName" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->skinNames[i])
                {
                    this->skinNames[i] = new Variant(MyGUIMiniMapComponent::AttrSkinName() + Ogre::StringConverter::toString(i), std::vector<Ogre::String>{"PanelSkin"}, this->attributes);
                    this->skinNames[i]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
                }
                else
                {
                    this->skinNames[i]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MiniMapTileColor" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->miniMapTilesColors[i])
                {
                    this->miniMapTilesColors[i] = new Variant(MyGUIMiniMapComponent::AttrMiniMapTileColor() + Ogre::StringConverter::toString(i), XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
                    this->miniMapTilesColors[i]->addUserData(GameObject::AttrActionColorDialog());
                }
                else
                {
                    this->miniMapTilesColors[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ToolTipDescription" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->toolTipDescriptions[i])
                {
                    this->toolTipDescriptions[i] = new Variant(MyGUIMiniMapComponent::AttrToolTipDescription() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
                }
                else
                {
                    this->toolTipDescriptions[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (nullptr != propertyElement)
            {
                Ogre::String name = XMLConverter::getAttrib(propertyElement, "name");
                if (Ogre::String::npos != name.find(" Scene Visited"))
                {
                    if (nullptr == this->visitedList[i])
                    {
                        this->visitedList[i] = new Variant(name, XMLConverter::getAttribBool(propertyElement, "data"), this->attributes);
                        this->visitedList[i]->addUserData(GameObject::AttrActionSeparator());
                    }
                    else
                    {
                        this->visitedList[i]->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
                        this->visitedList[i]->addUserData(GameObject::AttrActionSeparator());
                    }
                    // Attention: only advance, if this really was the 'Scene Visited' property. It used to advance in any case, so a missing
                    // 'Scene Visited' property swallowed the next tile's 'SkinName' property.
                    propertyElement = propertyElement->next_sibling("property");
                }
                else if (nullptr != this->toolTipDescriptions[i])
                {
                    this->toolTipDescriptions[i]->addUserData(GameObject::AttrActionSeparator());
                }
            }
        }

        bool allVisitedVariantEmpty = false;
        for (size_t i = 0; i < this->visitedList.size(); i++)
        {
            if (nullptr == this->visitedList[i])
            {
                allVisitedVariantEmpty = true;
                break;
            }
        }

        if (true == allVisitedVariantEmpty)
        {
            this->visitedList.clear();
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TrackableCount")
        {
            this->trackableCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        this->trackableIds.resize(this->trackableCount->getUInt());
        this->trackableImages.resize(this->trackableCount->getUInt());
        this->trackableImageTileSizes.resize(this->trackableCount->getUInt());
        this->spriteAnimationIndices.resize(this->trackableCount->getUInt(), -1);

        for (size_t i = 0; i < this->trackableCount->getUInt(); i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TrackableId" + Ogre::StringConverter::toString(i))
            {
                this->trackableIds[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableId() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TrackableImage" + Ogre::StringConverter::toString(i))
            {
                this->trackableImages[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableImage() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
                propertyElement = propertyElement->next_sibling("property");

                this->trackableImages[i]->addUserData(GameObject::AttrActionFileOpenDialog());
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TrackableImageTileSize" + Ogre::StringConverter::toString(i))
            {
                this->trackableImageTileSizes[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableImageTileSize() + Ogre::StringConverter::toString(i), XMLConverter::getAttribVector2(propertyElement, "data"), this->attributes);
                propertyElement = propertyElement->next_sibling("property");

                this->trackableImageTileSizes[i]
                    ->setDescription("Sets the tile size: e.g. Image may be of size: 32x64, but tile size 32x32, so that sprite animation is done automatically switching the image tiles from 0 to 32 and 32 to 64 automatically");
                this->trackableImageTileSizes[i]->addUserData(GameObject::AttrActionSeparator());
            }
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TrackableImageAnimationSpeed")
        {
            this->setTrackableImageAnimationSpeed(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    GameObjectCompPtr MyGUIMiniMapComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    bool MyGUIMiniMapComponent::postInit(void)
    {
        // Creates the main window for map
        bool success = MyGUIWindowComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIMiniMapComponent] Init MyGUI mini map component for game object: " + this->gameObjectPtr->getName());

        if (nullptr != this->widget)
        {
            GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->widget->setVisible(false);
                this->setLayer("Overlapped");
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::postInit");
        }

        this->setTrackableCount(this->trackableCount->getUInt());

        return success;
    }

    void MyGUIMiniMapComponent::destroyMiniMap(void)
    {
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            for (size_t i = 0; i < this->textBoxMapTiles.size(); i++)
            {
                // this->textBoxMapTiles[i]->detachFromWidget();
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->textBoxMapTiles[i]);
            }
            for (size_t i = 0; i < this->windowMapTiles.size(); i++)
            {
                if (true == this->useToolTip->getBool())
                {
                    this->windowMapTiles[i]->eventToolTip -= newDelegate(this, &MyGUIMiniMapComponent::notifyToolTip);
                }

                this->windowMapTiles[i]->detachFromWidget();
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->windowMapTiles[i]);
            }
            this->textBoxMapTiles.clear();
            this->windowMapTiles.clear();

            if (nullptr != this->widget)
            {
                MyGUI::Window* window = this->widget->castType<MyGUI::Window>(false);
                if (window != nullptr)
                {
                    // window->eventKeyButtonPressed += newDelegate(this, &MyGUIMiniMapComponent::notifyKeyButtonPressed);
                    window->eventWindowButtonPressed -= newDelegate(this, &MyGUIMiniMapComponent::notifyWindowButtonPressed);
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::destroyMiniMap");
    }

    void MyGUIMiniMapComponent::destroyTrackables(void)
    {
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            for (size_t i = 0; i < this->trackableImageBoxes.size(); i++)
            {
                // this->trackableImageBoxes[i]->detachFromWidget();
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->trackableImageBoxes[i]);
            }
            this->trackableImageBoxes.clear();
            this->spriteAnimationIndices.clear();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::destroyTrackables");
    }

    void MyGUIMiniMapComponent::generateMiniMap(void)
    {
        // Threadsafe from the outside

        this->destroyMiniMap();

        const Ogre::Vector2 widgetPixelSize(static_cast<Ogre::Real>(this->widget->getAbsoluteCoord().width), static_cast<Ogre::Real>(this->widget->getAbsoluteCoord().height));
        const bool xyAxis = "X,Y" == this->axis->getListSelectedValue();

        // Attention: used to start at the alphabetically first scene file of the project (e.g. 'EvilMagic'), no matter which level is loaded.
        // Now it starts at the current scene and follows its exits, see MiniMapModule.
        this->miniMapDataList = AppStateManager::getSingletonPtr()->getMiniMapModule()->parseMinimaps(Core::getSingletonPtr()->getSceneName(), xyAxis, widgetPixelSize, this->startPosition->getVector2(), this->scaleFactor->getReal());

        if (this->miniMapDataList.size() > this->skinNames.size())
        {
            // If there are more scenes than saved properties, fill up with default data
            for (size_t i = this->skinNames.size(); i < this->miniMapDataList.size(); i++)
            {
                this->skinNames.push_back(new Variant(MyGUIMiniMapComponent::AttrSkinName() + Ogre::StringConverter::toString(i), std::vector<Ogre::String>{"PanelSkin", "WoodPanel", "WoodWindow"}, this->attributes));

                this->miniMapTilesColors.push_back(new Variant(MyGUIMiniMapComponent::AttrMiniMapTileColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.2f, 0.2f, 0.2f), this->attributes));
                this->miniMapTilesColors[i]->addUserData(GameObject::AttrActionColorDialog());

                this->toolTipDescriptions.push_back(new Variant(MyGUIMiniMapComponent::AttrToolTipDescription() + Ogre::StringConverter::toString(i), "", this->attributes));
            }
        }
        else if (this->miniMapDataList.size() < this->skinNames.size())
        {
            this->eraseVariants(this->skinNames, this->miniMapDataList.size());
            this->eraseVariants(this->miniMapTilesColors, this->miniMapDataList.size());
            this->eraseVariants(this->toolTipDescriptions, this->miniMapDataList.size());
        }
        this->miniMapTilesCount = static_cast<unsigned int>(this->miniMapDataList.size());

        // The 'Scene Visited' attributes carry the scene name. If they do not match the current tiles exactly (another scene set, or a scene
        // saved with an old project, e.g. 'Level3 Scene Visited'), they are recreated - otherwise a flag would be applied to the wrong scene.
        bool visitedNamesMatch = this->visitedList.size() == this->miniMapDataList.size();
        for (size_t i = 0; i < this->visitedList.size() && true == visitedNamesMatch; i++)
        {
            if (nullptr == this->visitedList[i] || this->visitedList[i]->getName() != this->miniMapDataList[i].sceneName + " Scene Visited")
            {
                visitedNamesMatch = false;
            }
        }

        if (false == visitedNamesMatch)
        {
            this->eraseVariants(this->visitedList, 0);
            for (size_t i = 0; i < this->miniMapDataList.size(); i++)
            {
                Variant* visitedVariant = new Variant(this->miniMapDataList[i].sceneName + " Scene Visited", false, this->attributes);
                visitedVariant->setDescription("Reveals this scene on the mini map from the start, e.g. for a map item. Scenes the player has entered are revealed automatically.");
                visitedVariant->addUserData(GameObject::AttrActionSeparator());
                this->visitedList.push_back(visitedVariant);
            }
        }

        for (size_t i = 0; i < this->miniMapDataList.size(); i++)
        {
            const MiniMapModule::MiniMapData& miniMapData = this->miniMapDataList[i];

            // Note: Widgets are created with absolute coordinates
            MyGUI::Widget* mapTileWindow = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Widget>(this->skinNames[i]->getListSelectedValue(), miniMapData.position.x, miniMapData.position.y, miniMapData.size.x, miniMapData.size.y,
                this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue());

            Ogre::Vector3 color = this->miniMapTilesColors[i]->getVector3();
            mapTileWindow->setColour(MyGUI::Colour(color.x, color.y, color.z));
            mapTileWindow->setUserString("description", this->toolTipDescriptions[i]->getString());

            // Attach each map tile to the main mini map window
            mapTileWindow->attachToWidget(this->widget);
            mapTileWindow->setRealSize(miniMapData.size.x, miniMapData.size.y);
            mapTileWindow->setRealPosition(miniMapData.position.x, miniMapData.position.y);

            this->windowMapTiles.emplace_back(mapTileWindow);

            // The name is a child of its tile, so it moves and scales with it.
            MyGUI::TextBox* mapTileTextBox = mapTileWindow->createWidgetReal<MyGUI::TextBox>("TextBox", 0.02f, 0.02f, 0.96f, 0.96f, MyGUI::Align::Stretch);
            mapTileTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
            mapTileTextBox->setNeedMouseFocus(false);

            if (false == this->bShowDebugData)
            {
                mapTileTextBox->setCaption(miniMapData.sceneName);
            }
            else
            {
                mapTileTextBox->setCaption(miniMapData.sceneName + " s: " + Ogre::StringConverter::toString(miniMapData.size.x) + " x " + Ogre::StringConverter::toString(miniMapData.size.y) +
                                           " p: " + Ogre::StringConverter::toString(miniMapData.position.x) + " x " + Ogre::StringConverter::toString(miniMapData.position.y));
            }

            // Set label so NOWA-Design shows the scene name next to the skin dropdown
            this->skinNames[i]->addUserData(GameObject::AttrActionLabel());
            this->skinNames[i]->setDescription(miniMapData.sceneName);

            this->textBoxMapTiles.emplace_back(mapTileTextBox);
        }

        this->applyVisibility();

        // Add close handler if the outer widget is a Window
        MyGUI::Window* window = this->widget->castType<MyGUI::Window>(false);
        if (nullptr != window)
        {
            window->eventWindowButtonPressed += newDelegate(this, &MyGUIMiniMapComponent::notifyWindowButtonPressed);
        }
    }

    void MyGUIMiniMapComponent::generateTrackables(void)
    {
        // Threadsafe from the outside
        this->destroyTrackables();

        const Ogre::Vector2 widgetPixelSize(static_cast<Ogre::Real>(this->widget->getAbsoluteCoord().width), static_cast<Ogre::Real>(this->widget->getAbsoluteCoord().height));
        if (widgetPixelSize.x <= 0.0f || widgetPixelSize.y <= 0.0f)
        {
            return;
        }

        const bool xyAxis = "X,Y" == this->axis->getListSelectedValue();

        for (unsigned int i = 0; i < this->trackableCount->getUInt(); i++)
        {
            // 'Level4:2341435213' - a game object in another scene, or '2341435213' - a game object of the current scene (e.g. the global player).
            const Ogre::String sceneAndId = this->trackableIds[i]->getString();
            const size_t found = sceneAndId.find(":");

            // Attention: the scene name used to be read into a second, shadowing variable (and with the ':' at the wrong end), so a scene was
            // never passed on.
            Ogre::String sceneName;
            unsigned long id = 0;
            if (Ogre::String::npos != found)
            {
                sceneName = sceneAndId.substr(0, found);
                id = Ogre::StringConverter::parseUnsignedLong(sceneAndId.substr(found + 1));
            }
            else
            {
                id = Ogre::StringConverter::parseUnsignedLong(sceneAndId);
            }

            if (0 == id)
            {
                continue;
            }

            std::pair<bool, Ogre::Vector2> trackableMiniMapPosition = AppStateManager::getSingletonPtr()->getMiniMapModule()->parseGameObjectMinimapPosition(sceneName, id, xyAxis);
            if (false == trackableMiniMapPosition.first)
            {
                continue;
            }

            // Shown in the size of one image tile.
            Ogre::Vector2 imagePixelSize(16.0f, 16.0f);
            if (nullptr != this->trackableImageTileSizes[i])
            {
                imagePixelSize = this->trackableImageTileSizes[i]->getVector2();
            }
            const Ogre::Real width = imagePixelSize.x / widgetPixelSize.x;
            const Ogre::Real height = imagePixelSize.y / widgetPixelSize.y;

            // A child of the mini map window like the tiles, created after them, so it is drawn on top. Centered on the position.
            MyGUI::ImageBox* trackableImage = this->widget->createWidgetReal<MyGUI::ImageBox>("ImageBox", trackableMiniMapPosition.second.x - width * 0.5f, trackableMiniMapPosition.second.y - height * 0.5f, width, height, MyGUI::Align::Default);
            trackableImage->setNeedMouseFocus(false);

            trackableImage->setImageTexture(this->trackableImages[i]->getString());
            trackableImage->setImageRect(MyGUI::IntRect(0, 0, trackableImage->getImageSize().width, trackableImage->getImageSize().height));
            trackableImage->setVisible(this->bShowMiniMap);

            this->trackableImageBoxes.emplace_back(trackableImage);
            this->spriteAnimationIndices.push_back(-1);

            if (nullptr != this->trackableImageTileSizes[i])
            {
                this->setTrackableImageTileSize(i, this->trackableImageTileSizes[i]->getVector2());
            }
        }
    }

    bool MyGUIMiniMapComponent::isTileRevealed(size_t index) const
    {
        if (index >= this->miniMapDataList.size())
        {
            return false;
        }

        if (true == AppStateManager::getSingletonPtr()->getMiniMapModule()->getIsSceneVisited(this->miniMapDataList[index].sceneName))
        {
            return true;
        }

        return index < this->visitedList.size() && nullptr != this->visitedList[index] && true == this->visitedList[index]->getBool();
    }

    void MyGUIMiniMapComponent::applyVisibility(void)
    {
        if (nullptr != this->widget)
        {
            this->widget->setVisible(this->bShowMiniMap);
        }

        for (size_t i = 0; i < this->windowMapTiles.size(); i++)
        {
            bool tileVisible = this->bShowMiniMap;
            if (true == tileVisible && true == this->useVisitation->getBool())
            {
                tileVisible = this->isTileRevealed(i);
            }

            this->windowMapTiles[i]->setVisible(tileVisible);
            if (i < this->textBoxMapTiles.size())
            {
                this->textBoxMapTiles[i]->setVisible(tileVisible && true == this->showNames->getBool());
            }
        }

        // Trackables are always shown with the map: the player is in the current scene, which is always visited.
        for (size_t i = 0; i < this->trackableImageBoxes.size(); i++)
        {
            this->trackableImageBoxes[i]->setVisible(this->bShowMiniMap);
        }
    }

    void MyGUIMiniMapComponent::setSceneVisited(unsigned int index, bool visited)
    {
        if (index >= this->miniMapDataList.size())
        {
            return;
        }
        this->setSceneVisited(this->miniMapDataList[index].sceneName, visited);
    }

    bool MyGUIMiniMapComponent::getIsSceneVisited(unsigned int index)
    {
        return this->isTileRevealed(index);
    }

    void MyGUIMiniMapComponent::setSceneVisited(const Ogre::String& sceneName, bool visited)
    {
        // Kept in the MiniMapModule, so it survives scene changes - this component is re-created with every scene.
        AppStateManager::getSingletonPtr()->getMiniMapModule()->setSceneVisited(sceneName, visited);

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyVisibility();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::setSceneVisited");
    }

    bool MyGUIMiniMapComponent::getIsSceneVisited(const Ogre::String& sceneName)
    {
        for (size_t i = 0; i < this->miniMapDataList.size(); i++)
        {
            if (sceneName == this->miniMapDataList[i].sceneName)
            {
                return this->isTileRevealed(i);
            }
        }
        return AppStateManager::getSingletonPtr()->getMiniMapModule()->getIsSceneVisited(sceneName);
    }

    void MyGUIMiniMapComponent::notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch)
    {
        // Does not work
        // if (key == MyGUI::KeyCode::Escape)
        if (NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->isActionDown(NOWA_A_MAP))
        {
            this->showMiniMap(false);
        }
    }

    void MyGUIMiniMapComponent::notifyWindowButtonPressed(MyGUI::Window* sender, const std::string& button)
    {
        if (button == "close")
        {
            this->showMiniMap(false);
        }
    }

    void MyGUIMiniMapComponent::mouseButtonClick(MyGUI::Widget* sender)
    {
        if (false == this->isSimulating)
        {
            return;
        }

        MyGUI::Window* window = sender->castType<MyGUI::Window>();
        if (nullptr == window)
        {
            return;
        }

        // Which map tile was clicked. This used to be computed and then never used - the lua
        // callback was invoked with no arguments at all, so a script could not tell which tile
        // the click belonged to.
        unsigned int index = 0;
        bool foundTile = false;
        for (unsigned int i = 0; i < static_cast<unsigned int>(this->windowMapTiles.size()); i++)
        {
            if (sender == this->windowMapTiles[i])
            {
                index = i;
                foundTile = true;
                break;
            }
        }

        if (false == foundTile)
        {
            // Without this, an unknown sender reported index 0 - indistinguishable from a real
            // click on the first tile.
            return;
        }

        // Call also function in lua script, if it does exist in the lua script component
        if (nullptr == this->gameObjectPtr->getLuaScript() || false == this->enabled->getBool())
        {
            return;
        }

        // The closure list is copied HERE, on the calling thread, instead of capturing a
        // POINTER into this component and dereferencing it later. The old comment claimed the
        // deferred copy was safe - true as far as luabind goes, but the pointer target lives
        // inside this component and dies with it.
        auto closures = this->mouseButtonClickClosureFunctions;

        if (true == closures.empty())
        {
            return;
        }

        // This callback comes from MyGUI on the render thread while the command runs on the
        // logic thread, and the component may be destroyed in between - the command's first
        // line would then already touch freed memory through 'this'.
        boost::weak_ptr<GameObjectComponent> weakThis = this->shared_from_this();

        NOWA::AppStateManager::LogicCommand logicCommand = [this, weakThis, closures, index]()
        {
            boost::shared_ptr<GameObjectComponent> strongThis = weakThis.lock();
            if (nullptr == strongThis)
            {
                return;
            }

            if (false == this->isSimulating)
            {
                return;
            }

            for (const auto& closure : closures)
            {
                if (false == closure.is_valid())
                {
                    continue;
                }
                try
                {
                    luabind::call_function<void>(closure, index);
                }
                catch (luabind::error& error)
                {
                    luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                    std::stringstream msg;
                    msg << errorMsg;
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIMiniMapComponent] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
                }
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    void MyGUIMiniMapComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating && true == this->bShowMiniMap)
        {
            // Update sprite animation values
            if (this->timeSinceLastUpdate >= 0.0f)
            {
                this->timeSinceLastUpdate -= dt;
            }
            else
            {
                auto closureFunction = [this](Ogre::Real renderDt)
                {
                    for (size_t i = 0; i < this->spriteAnimationIndices.size(); i++)
                    {
                        if (this->spriteAnimationIndices[i] != -1)
                        {
                            /** Tiles in file start numbering from left to right and from top to bottom.
                                For example:
                                    +---+---+---+
                                    | 0 | 1 | 2 |
                                    +---+---+---+
                                    | 3 | 4 | 5 |
                                    +---+---+---+
                            */

                            const MyGUI::IntSize& imageSize = this->trackableImageBoxes[i]->getImageSize();
                            const Ogre::Vector2& tileSize = this->trackableImageTileSizes[i]->getVector2();

                            int indexBoundsHorizontal = imageSize.width / static_cast<int>(tileSize.x);
                            int indexBoundsVertical = imageSize.height / static_cast<int>(tileSize.y);

                            // If within bounds, increment index, else start from the beginning again
                            if (this->spriteAnimationIndices[i] < (indexBoundsHorizontal * indexBoundsVertical) - 1)
                            {
                                this->spriteAnimationIndices[i]++;
                            }
                            else
                            {
                                this->spriteAnimationIndices[i] = 0;
                            }
                            // Set the index
                            this->trackableImageBoxes[i]->setImageIndex(this->spriteAnimationIndices[i]);
                        }
                    }
                };
                Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
                NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);

                this->timeSinceLastUpdate = this->trackableImageAnimationSpeed->getReal();
            }
        }
    }

    void MyGUIMiniMapComponent::showDebugData(void)
    {
        GameObjectComponent::showDebugData();

        // Levels may have been edited meanwhile - read them again.
        AppStateManager::getSingletonPtr()->getMiniMapModule()->clearSceneCache();

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->generateMiniMap();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::showDebugData");
    }

    bool MyGUIMiniMapComponent::connect(void)
    {
        bool success = MyGUIWindowComponent::connect();

        this->setUseVisitation(this->useVisitation->getBool());

        MiniMapModule* miniMapModule = AppStateManager::getSingletonPtr()->getMiniMapModule();

        // In NOWA-Design levels may have been edited since the last simulation. In the game the scene files never change, so they are read
        // only once per app state.
        if (false == Core::getSingletonPtr()->getIsGame())
        {
            miniMapModule->clearSceneCache();
        }

        // The player is in this scene now.
        miniMapModule->setSceneVisited(Core::getSingletonPtr()->getSceneName(), true);

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->generateMiniMap();
            this->generateTrackables();
            this->setUseToolTip(this->useToolTip->getBool());
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::connect");

        return success;
    }

    bool MyGUIMiniMapComponent::disconnect(void)
    {
        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        return MyGUIWindowComponent::disconnect();
    }

    void MyGUIMiniMapComponent::actualizeValue(Variant* attribute)
    {
        MyGUIWindowComponent::actualizeValue(attribute);

        if (MyGUIMiniMapComponent::AttrStartPosition() == attribute->getName())
        {
            this->setStartPosition(attribute->getVector2());
        }
        else if (MyGUIMiniMapComponent::AttrScaleFactor() == attribute->getName())
        {
            this->setScaleFactor(attribute->getReal());
        }
        else if (MyGUIMiniMapComponent::AttrUseToolTip() == attribute->getName())
        {
            this->setUseToolTip(attribute->getBool());
        }
        else if (MyGUIMiniMapComponent::AttrUseVisitation() == attribute->getName())
        {
            this->setUseVisitation(attribute->getBool());
        }
        else if (MyGUIMiniMapComponent::AttrAxis() == attribute->getName())
        {
            this->setAxis(attribute->getListSelectedValue());
        }
        else if (MyGUIMiniMapComponent::AttrShowNames() == attribute->getName())
        {
            this->setShowNames(attribute->getBool());
        }
        else if (MyGUIMiniMapComponent::AttrTrackableCount() == attribute->getName())
        {
            this->setTrackableCount(attribute->getUInt());
        }
        else if (MyGUIMiniMapComponent::AttrTrackableImageAnimationSpeed() == attribute->getName())
        {
            this->setTrackableImageAnimationSpeed(attribute->getReal());
        }
        else
        {
            for (unsigned int i = 0; i < this->miniMapTilesCount; i++)
            {
                if (MyGUIMiniMapComponent::AttrSkinName() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setSkinName(i, attribute->getListSelectedValue());
                }
                else if (MyGUIMiniMapComponent::AttrMiniMapTileColor() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setMiniMapTileColor(i, attribute->getVector3());
                }
                else if (MyGUIMiniMapComponent::AttrToolTipDescription() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setToolTipDescription(i, attribute->getString());
                }
                else if (i < this->visitedList.size() && nullptr != this->visitedList[i] && this->visitedList[i]->getName() == attribute->getName())
                {
                    // The attribute only reveals the scene from the start. What the player has visited is kept in the MiniMapModule.
                    this->visitedList[i]->setValue(attribute->getBool());
                }
            }
            for (unsigned int i = 0; i < this->trackableCount->getUInt(); i++)
            {
                if (MyGUIMiniMapComponent::AttrTrackableId() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setTrackableId(i, attribute->getString());
                }
                else if (MyGUIMiniMapComponent::AttrTrackableImage() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setTrackableImage(i, attribute->getString());
                }
                else if (MyGUIMiniMapComponent::AttrTrackableImageTileSize() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setTrackableImageTileSize(i, attribute->getVector2());
                }
            }
        }
    }

    void MyGUIMiniMapComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        MyGUIWindowComponent::writeXML(propertiesXML, doc);

        // 2 = int
        // 6 = real
        // 7 = string
        // 8 = vector2
        // 9 = vector3
        // 10 = vector4 -> also quaternion
        // 12 = bool
        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "StartPosition"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startPosition->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ScaleFactor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scaleFactor->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "UseToolTip"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useToolTip->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "UseVisitation"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useVisitation->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Axis"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->axis->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShowNames"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showNames->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MiniMapTilesCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->miniMapTilesCount)));
        propertiesXML->append_node(propertyXML);

        for (size_t i = 0; i < this->miniMapTilesCount; i++)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "SkinName" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->skinNames[i]->getListSelectedValue())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "MiniMapTileColor" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->miniMapTilesColors[i]->getVector3())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "ToolTipDescription" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->toolTipDescriptions[i]->getString())));
            propertiesXML->append_node(propertyXML);

            if (false == this->visitedList.empty() && nullptr != this->visitedList[i])
            {
                propertyXML = doc.allocate_node(node_element, "property");
                propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
                propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, this->visitedList[i]->getName())));
                propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->visitedList[i]->getBool())));
                propertiesXML->append_node(propertyXML);
            }
        }

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TrackableCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->trackableCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        for (size_t i = 0; i < this->trackableCount->getUInt(); i++)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TrackableId" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->trackableIds[i]->getString())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TrackableImage" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->trackableImages[i]->getString())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TrackableImageTileSize" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->trackableImageTileSizes[i]->getVector2())));
            propertiesXML->append_node(propertyXML);
        }

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TrackableImageAnimationSpeed"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->trackableImageAnimationSpeed->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String MyGUIMiniMapComponent::getClassName(void) const
    {
        return "MyGUIMiniMapComponent";
    }

    Ogre::String MyGUIMiniMapComponent::getParentClassName(void) const
    {
        return "MyGUIWindowComponent";
    }

    void MyGUIMiniMapComponent::notifyToolTip(MyGUI::Widget* sender, const MyGUI::ToolTipInfo& info)
    {
        if (true == this->useToolTip->getBool())
        {
            if (info.type == MyGUI::ToolTipInfo::Show)
            {
                MyGUI::UString description = sender->getUserString("description");
                if (true == description.empty())
                {
                    return;
                }

                this->toolTip->show(info.point, description);
                this->toolTip->move(info.point);
            }
            else if (info.type == MyGUI::ToolTipInfo::Hide)
            {
                this->toolTip->hide();
            }
            else if (info.type == MyGUI::ToolTipInfo::Move)
            {
                this->toolTip->move(info.point);
            }
        }
    }

    void MyGUIMiniMapComponent::setStartPosition(const Ogre::Vector2& startPosition)
    {
        this->startPosition->setValue(startPosition);
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->generateMiniMap();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::setStartPosition");
    }

    Ogre::Vector2 MyGUIMiniMapComponent::getStartPosition(void) const
    {
        return this->startPosition->getVector2();
    }

    void MyGUIMiniMapComponent::setScaleFactor(Ogre::Real scaleFactor)
    {
        if (scaleFactor <= 0.01f)
        {
            scaleFactor = 1.0f;
        }
        this->scaleFactor->setValue(scaleFactor);

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->generateMiniMap();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::setScaleFactor");
    }

    Ogre::Real MyGUIMiniMapComponent::getScaleFactor(void) const
    {
        return this->scaleFactor->getReal();
    }

    void MyGUIMiniMapComponent::setUseToolTip(bool useToolTip)
    {
        // Threadsafe from the outside
        if (true == this->useToolTip->getBool())
        {
            if (nullptr == this->toolTip)
            {
                this->toolTip = new MiniMapToolTip();
                toolTip->hide();
            }
            // Attention: Necessary for all map windows?
            if (nullptr != this->widget)
            {
                // this->widget->eventToolTip += newDelegate(this, &MyGUIMiniMapComponent::notifyToolTip);
                for (size_t i = 0; i < this->windowMapTiles.size(); i++)
                {
                    this->windowMapTiles[i]->eventToolTip += newDelegate(this, &MyGUIMiniMapComponent::notifyToolTip);
                }
            }
        }
        else
        {
            if (nullptr != this->toolTip)
            {
                delete this->toolTip;
                this->toolTip = nullptr;
            }
        }
    }

    bool MyGUIMiniMapComponent::getUseToolTip(void) const
    {
        return this->useToolTip->getBool();
    }

    void MyGUIMiniMapComponent::setUseVisitation(bool useVisitation)
    {
        this->useVisitation->setValue(useVisitation);
    }

    bool MyGUIMiniMapComponent::getUseVisitation(void) const
    {
        return this->useVisitation->getBool();
    }

    void MyGUIMiniMapComponent::setSkinName(unsigned int index, const Ogre::String& skinName)
    {
        if (index >= this->skinNames.size())
        {
            index = static_cast<unsigned int>(this->skinNames.size()) - 1;
        }
        this->skinNames[index]->setListSelectedValue(skinName);

        GraphicsModule::RenderCommand renderCommand = [this, index, skinName]()
        {
            this->windowMapTiles[index]->changeWidgetSkin(skinName);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::setSkinName");
    }

    Ogre::String MyGUIMiniMapComponent::getSkinName(unsigned int index) const
    {
        if (index >= this->skinNames.size())
        {
            return "";
        }
        return this->skinNames[index]->getListSelectedValue();
    }

    void MyGUIMiniMapComponent::setAxis(const Ogre::String& axis)
    {
        this->axis->setListSelectedValue(axis);
    }

    unsigned int MyGUIMiniMapComponent::getMiniMapTilesCount(void) const
    {
        return this->miniMapTilesCount;
    }

    void MyGUIMiniMapComponent::setMiniMapTileColor(unsigned int index, const Ogre::Vector3& color)
    {
        if (index >= this->miniMapTilesColors.size())
        {
            index = static_cast<unsigned int>(this->miniMapTilesColors.size()) - 1;
        }
        this->miniMapTilesColors[index]->setValue(color);

        GraphicsModule::RenderCommand renderCommand = [this, index, color]()
        {
            this->windowMapTiles[index]->setColour(MyGUI::Colour(color.x, color.y, color.z));
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "MyGUIMiniMapComponent::setMiniMapTileColor");
    }

    Ogre::Vector3 MyGUIMiniMapComponent::getMiniMapTileColor(unsigned int index)
    {
        if (index >= this->miniMapTilesColors.size())
        {
            return Ogre::Vector3::ZERO;
        }
        return this->miniMapTilesColors[index]->getVector3();
    }

    void MyGUIMiniMapComponent::setToolTipDescription(unsigned int index, const Ogre::String& description)
    {
        if (index >= this->toolTipDescriptions.size())
        {
            index = static_cast<unsigned int>(this->toolTipDescriptions.size()) - 1;
        }
        this->toolTipDescriptions[index]->setValue(description);

        if (index < this->windowMapTiles.size())
        {
            this->windowMapTiles[index]->setUserString("description", description);
        }
    }

    Ogre::String MyGUIMiniMapComponent::getToolTipDescription(unsigned int index)
    {
        if (index >= this->toolTipDescriptions.size())
        {
            return "";
        }
        return this->toolTipDescriptions[index]->getString();
    }

    void MyGUIMiniMapComponent::setMiniMapTileVisible(unsigned int index, bool miniMapTileVisible)
    {
        if (index >= this->windowMapTiles.size())
        {
            index = static_cast<unsigned int>(this->windowMapTiles.size()) - 1;
        }
        if (index < this->windowMapTiles.size())
        {
            GraphicsModule::RenderCommand renderCommand = [this, index, miniMapTileVisible]()
            {
                this->windowMapTiles[index]->setVisible(miniMapTileVisible);
                this->textBoxMapTiles[index]->setVisible(miniMapTileVisible);
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "MyGUIMiniMapComponent::setMiniMapTileVisible");
        }
    }

    bool MyGUIMiniMapComponent::isMiniMapTileVisible(unsigned int index) const
    {
        if (index >= this->windowMapTiles.size())
        {
            return false;
        }
        return this->windowMapTiles[index]->isVisible();
    }

    void MyGUIMiniMapComponent::setTrackableCount(unsigned int trackableCount)
    {
        this->trackableCount->setValue(trackableCount);

        bool trackableCountChanged = trackableCount != this->trackableIds.size();

        size_t oldSize = this->trackableIds.size();

        if (trackableCount > oldSize)
        {
            // Resize the waypoints array for count
            this->trackableIds.resize(trackableCount);
            this->trackableImages.resize(trackableCount);
            this->trackableImageTileSizes.resize(trackableCount);

            for (size_t i = oldSize; i < this->trackableIds.size(); i++)
            {
                this->trackableIds[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableId() + Ogre::StringConverter::toString(i), "", this->attributes);
                this->trackableImages[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableImage() + Ogre::StringConverter::toString(i), "circleRed.png", this->attributes);

                this->trackableImages[i]->addUserData(GameObject::AttrActionFileOpenDialog());

                this->trackableImageTileSizes[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableImageTileSize() + Ogre::StringConverter::toString(i), Ogre::Vector2(32.0f, 32.0f), this->attributes);
                this->trackableImageTileSizes[i]
                    ->setDescription("Sets the tile size: e.g. Image may be of size: 32x64, but tile size 32x32, so that sprite animation is done automatically switching the image tiles from 0 to 32 and 32 to 64 automatically");
                this->trackableImageTileSizes[i]->addUserData(GameObject::AttrActionSeparator());
            }
        }
        else if (trackableCount < oldSize)
        {
            this->eraseVariants(this->trackableIds, trackableCount);
            this->eraseVariants(this->trackableImages, trackableCount);
            this->eraseVariants(this->trackableImageTileSizes, trackableCount);
        }

        if (true == trackableCountChanged)
        {
            GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->generateMiniMap();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::setTrackableCount");
        }
    }

    unsigned int MyGUIMiniMapComponent::getTrackableCount(void) const
    {
        return this->trackableCount->getUInt();
    }

    void MyGUIMiniMapComponent::setTrackableId(unsigned int index, const Ogre::String& id)
    {
        if (index >= this->trackableIds.size())
        {
            index = static_cast<unsigned int>(this->trackableIds.size()) - 1;
        }
        this->trackableIds[index]->setValue(id);

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->generateMiniMap();
            this->generateTrackables();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::setTrackableId");
    }

    Ogre::String MyGUIMiniMapComponent::getTrackableId(unsigned int index)
    {
        if (index >= this->trackableIds.size())
        {
            return "";
        }
        return this->trackableIds[index]->getString();
    }

    void MyGUIMiniMapComponent::setTrackableImage(unsigned int index, const Ogre::String& imageName)
    {
        if (index >= this->trackableImages.size())
        {
            index = static_cast<unsigned int>(this->trackableImages.size()) - 1;
        }
        this->trackableImages[index]->setValue(imageName);

        GraphicsModule::RenderCommand renderCommand = [this, index, imageName]()
        {
            if (this->trackableImageBoxes.size() == this->trackableImages.size())
            {
                this->trackableImageBoxes[index]->setImageTexture(imageName);
                this->trackableImageBoxes[index]->setImageRect(MyGUI::IntRect(0, 0, this->trackableImageBoxes[index]->getImageSize().width, this->trackableImageBoxes[index]->getImageSize().height));
            }
            else
            {
                this->generateTrackables();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::setTrackableId");
    }

    Ogre::String MyGUIMiniMapComponent::getTrackableImage(unsigned int index)
    {
        if (index >= this->trackableImages.size())
        {
            return "";
        }
        return this->trackableImages[index]->getString();
    }

    void MyGUIMiniMapComponent::setTrackableImageTileSize(unsigned int index, const Ogre::Vector2& imageTileSize)
    {
        if (index >= this->trackableImageTileSizes.size())
        {
            index = static_cast<unsigned int>(this->trackableImageTileSizes.size()) - 1;
        }
        this->trackableImageTileSizes[index]->setValue(imageTileSize);

        GraphicsModule::RenderCommand renderCommand = [this, index, imageTileSize]()
        {
            if (this->trackableImageBoxes.size() == this->trackableImages.size())
            {
                // Sets the tile size: e.g. Image may be of size: 32x64, but tile size 32x32, so that sprite animation is done automatically switching the image tiles from 0 to 32 and 32 to 64 automatically
                this->trackableImageBoxes[index]->setImageTile(MyGUI::IntSize(static_cast<int>(imageTileSize.x), static_cast<int>(imageTileSize.y)));
                // If the image size is bigger as the tile, enable sprite animation, by setting the corresponding spriteAnimationIndices from -1 to 0

                unsigned int indexBoundsHorizontal = this->trackableImageBoxes[index]->getImageSize().width / static_cast<int>(this->trackableImageTileSizes[index]->getVector2().x);
                unsigned int indexBoundsVertical = this->trackableImageBoxes[index]->getImageSize().height / static_cast<int>(this->trackableImageTileSizes[index]->getVector2().y);

                if (indexBoundsHorizontal > 1 || indexBoundsVertical > 1)
                {
                    this->spriteAnimationIndices[index] = 0;
                }
                else
                {
                    this->spriteAnimationIndices[index] = -1;
                }
            }
            else
            {
                this->generateTrackables();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "MyGUIMiniMapComponent::setTrackableImageTileSize");
    }

    Ogre::Vector2 MyGUIMiniMapComponent::getTrackableImageTileSize(unsigned int index)
    {
        if (index >= this->trackableImageTileSizes.size())
        {
            return Ogre::Vector2::ZERO;
        }
        return this->trackableImageTileSizes[index]->getVector2();
    }

    void MyGUIMiniMapComponent::setTrackableImageAnimationSpeed(Ogre::Real speed)
    {
        this->timeSinceLastUpdate = speed;
    }

    Ogre::Real MyGUIMiniMapComponent::getTrackableImageAnimationSpeed(void) const
    {
        return this->timeSinceLastUpdate;
    }

    void MyGUIMiniMapComponent::setShowNames(bool showNames)
    {
        this->showNames->setValue(showNames);
    }

    void MyGUIMiniMapComponent::showMiniMap(bool bShow)
    {
        GraphicsModule::RenderCommand renderCommand = [this, bShow]()
        {
            this->bShowMiniMap = bShow;
            // Trackables move (e.g. the player), so their positions are taken anew each time the map is shown.
            this->generateTrackables();
            this->applyVisibility();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIMiniMapComponent::showMiniMap");
    }

    bool MyGUIMiniMapComponent::isMiniMapShown(void) const
    {
        return this->bShowMiniMap;
    }

}; // namespace end