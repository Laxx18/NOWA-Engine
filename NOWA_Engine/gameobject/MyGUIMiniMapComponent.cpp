#include "NOWAPrecompiled.h"
#include "MyGUIMiniMapComponent.h"
#include "LuaScriptComponent.h"
#include "modules/LuaScriptApi.h"
#include "modules/InputDeviceModule.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "MyGUI_LayerManager.h"

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
	}

	void MiniMapToolTip::hide()
	{
		this->toolTip->setVisible(false);
	}

	void MiniMapToolTip::move(const MyGUI::IntPoint& point)
	{
		this->boundedMove(this->toolTip, point);
	}

	void MiniMapToolTip::boundedMove(MyGUI::Widget* moving, const MyGUI::IntPoint& point)
	{
		const MyGUI::IntPoint offset(16, 16);  // typical mouse cursor size - offset out from under it

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

	MyGUIMiniMapComponent::MyGUIMiniMapComponent()
		: MyGUIWindowComponent(),
		toolTip(nullptr),
		miniMapTilesCount(0),
		bShowMiniMap(false),
		timeSinceLastUpdate(0.2f)
	{
		this->startPosition = new Variant(MyGUIMiniMapComponent::AttrStartPosition(), Ogre::Vector2(0.1f, 0.5f), this->attributes);
		this->scaleFactor = new Variant(MyGUIMiniMapComponent::AttrScaleFactor(), 1.0f, this->attributes);
		this->axis = new Variant(MyGUIMiniMapComponent::AttrAxis(), std::vector<Ogre::String>{ "X,Y", "X,Z" }, this->attributes);
		this->showNames = new Variant(MyGUIMiniMapComponent::AttrShowNames(), true, this->attributes);
		this->useToolTip = new Variant(MyGUIMiniMapComponent::AttrUseToolTip(), true, this->attributes);
		this->useVisitation = new Variant(MyGUIMiniMapComponent::AttrUseVisitation(), true, this->attributes);
		
		this->trackableCount = new Variant(MyGUIMiniMapComponent::AttrTrackableCount(), 0, this->attributes);
		this->trackableImageAnimationSpeed = new Variant(MyGUIMiniMapComponent::AttrTrackableImageAnimationSpeed(), 0.2f, this->attributes);

		this->trackableCount->addUserData(GameObject::AttrActionNeedRefresh());

		this->position->setValue(Ogre::Vector2(0.0f, 0.0f));
		this->size->setValue(Ogre::Vector2(1.0f, 1.0f));

		this->useVisitation->setDescription("If activated, only minimap tiles are visible, which are marked as visited, via @setVisited(...) functionality.");

		this->axis->setDescription("The axis for exit direction. For Jump'n'Run e.g. 'X,Y' is correct and for a casual 3D world 'X,Z'.");
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
			delete this->toolTip;
			this->toolTip = nullptr;
		}
	}

	bool MyGUIMiniMapComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIWindowComponent::init(propertyElement, filename);
		
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
					this->skinNames[i] = new Variant(MyGUIMiniMapComponent::AttrSkinName() + Ogre::StringConverter::toString(i),
						std::vector<Ogre::String>{ "PanelSkin" }, this->attributes);
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
					this->miniMapTilesColors[i] = new Variant(MyGUIMiniMapComponent::AttrMiniMapTileColor() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
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
					this->toolTipDescriptions[i] = new Variant(MyGUIMiniMapComponent::AttrToolTipDescription() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
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
				}
				else
				{
					this->toolTipDescriptions[i]->addUserData(GameObject::AttrActionSeparator());
				}
				propertyElement = propertyElement->next_sibling("property");
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
				this->trackableIds[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableId() + Ogre::StringConverter::toString(i),
					XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TrackableImage" + Ogre::StringConverter::toString(i))
			{
				this->trackableImages[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableImage() + Ogre::StringConverter::toString(i),
					XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				propertyElement = propertyElement->next_sibling("property");

				this->trackableImages[i]->addUserData(GameObject::AttrActionFileOpenDialog());
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TrackableImageTileSize" + Ogre::StringConverter::toString(i))
			{
				this->trackableImageTileSizes[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableImageTileSize() + Ogre::StringConverter::toString(i),
					XMLConverter::getAttribVector2(propertyElement, "data"), this->attributes);
				propertyElement = propertyElement->next_sibling("property");

				this->trackableImageTileSizes[i]->setDescription("Sets the tile size: e.g. Image may be of size: 32x64, but tile size 32x32, so that sprite animation is done automatically switching the image tiles from 0 to 32 and 32 to 64 automatically");
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
			this->widget->setVisible(false);
		}

		this->setTrackableCount(this->trackableCount->getUInt());

		return success;
	}

	void MyGUIMiniMapComponent::destroyMiniMap(void)
	{
		for (size_t i = 0; i < this->textBoxMapTiles.size(); i++)
		{
			// this->textBoxMapTiles[i]->detachFromWidget();
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->textBoxMapTiles[i]);
		}
		for (size_t i = 0; i < this->windowMapTiles.size(); i++)
		{
			if (true == this->useToolTip->getBool())
				this->windowMapTiles[i]->eventToolTip -= newDelegate(this, &MyGUIMiniMapComponent::notifyToolTip);

			this->windowMapTiles[i]->detachFromWidget();
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->windowMapTiles[i]);
		}
		this->textBoxMapTiles.clear();
		this->windowMapTiles.clear();

		MyGUI::Window* window = this->widget->castType<MyGUI::Window>(false);
		if (window != nullptr)
		{
			// window->eventKeyButtonPressed += newDelegate(this, &MyGUIMiniMapComponent::notifyKeyButtonPressed);
			window->eventWindowButtonPressed -= newDelegate(this, &MyGUIMiniMapComponent::notifyWindowButtonPressed);
		}
	}

	void MyGUIMiniMapComponent::destroyTrackables(void)
	{
		for (size_t i = 0; i < this->trackableImageBoxes.size(); i++)
		{
			// this->trackableImageBoxes[i]->detachFromWidget();
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->trackableImageBoxes[i]);
		}
		this->trackableImageBoxes.clear();
		this->spriteAnimationIndices.clear();
	}

	void MyGUIMiniMapComponent::generateMiniMap(void)
	{
		this->destroyMiniMap();

		// MyGUI::IntSize tempSize = MyGUI::RenderManager::getInstance().getViewSize();

		Ogre::Vector2 viewPortSize = Ogre::Vector2(static_cast<Ogre::Real>(this->widget->getAbsoluteCoord().width), static_cast<Ogre::Real>(this->widget->getAbsoluteCoord().height)) / this->scaleFactor->getReal();

		// Check how many mini maps can be created
		this->miniMapDataList = AppStateManager::getSingletonPtr()->getMiniMapModule()->parseMinimaps(this->gameObjectPtr->getSceneManager(), 
			this->axis->getListSelectedValue() == "X,Y" ? true : false, this->startPosition->getVector2(), viewPortSize);

		if (this->miniMapDataList.size() > this->skinNames.size())
		{
			miniMapTilesCount = static_cast<unsigned int>(this->miniMapDataList.size());
			// If there are more scene, then actually properties loaded sind the last save, fill up with default data
			for (size_t i = this->skinNames.size(); i < this->miniMapDataList.size(); i++)
			{
				this->skinNames.push_back(new Variant(MyGUIMiniMapComponent::AttrSkinName() + Ogre::StringConverter::toString(i), std::vector<Ogre::String>{ "PanelSkin" }, this->attributes));
				this->miniMapTilesColors.push_back(new Variant(MyGUIMiniMapComponent::AttrMiniMapTileColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.2f, 0.2f, 0.2f), this->attributes));
				this->miniMapTilesColors[i]->addUserData(GameObject::AttrActionColorDialog());
				this->toolTipDescriptions.push_back(new Variant(MyGUIMiniMapComponent::AttrToolTipDescription() + Ogre::StringConverter::toString(i), "", this->attributes));
				this->visitedList.push_back(new Variant(this->miniMapDataList[i].sceneName + " Scene Visited", true, this->attributes));
			}
		}
		else if (this->miniMapDataList.size() < this->skinNames.size())
		{
			miniMapTilesCount = static_cast<unsigned int>(this->miniMapDataList.size());

			this->eraseVariants(this->skinNames, miniMapTilesCount);
			this->eraseVariants(this->miniMapTilesColors, miniMapTilesCount);
			this->eraseVariants(this->toolTipDescriptions, miniMapTilesCount);
			this->eraseVariants(this->visitedList, this->miniMapDataList.size());
		}

		if (true == this->visitedList.empty())
		{
			for (size_t i = 0; i < this->miniMapDataList.size(); i++)
			{
				this->visitedList.push_back(new Variant(this->miniMapDataList[i].sceneName + " Scene Visited", true, this->attributes));
			}
		}
		else
		{
			for (size_t i = 0; i < this->miniMapDataList.size(); i++)
			{
				if (nullptr == this->visitedList[i])
				{
					this->visitedList[i] = new Variant(this->miniMapDataList[i].sceneName + " Scene Visited", true, this->attributes);
				}
			}
		}


		for (size_t i = 0; i < this->miniMapDataList.size(); i++)
		{
			const MiniMapModule::MiniMapData& miniMapData = this->miniMapDataList[i];

			// Note: Widgets are created with absolute coordinates!?
			MyGUI::Widget* mapTileWindow = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Widget>(this->skinNames[i]->getListSelectedValue(),
				miniMapData.position.x, miniMapData.position.y, miniMapData.size.x, miniMapData.size.y,
				this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue());

			Ogre::Vector3 color = this->miniMapTilesColors[i]->getVector3();
			mapTileWindow->setColour(MyGUI::Colour(color.x, color.y, color.z));
			mapTileWindow->setUserString("description", this->toolTipDescriptions[i]->getString());
			mapTileWindow->setVisible(this->bShowMiniMap);

			// Attach each map tile to the main mini map window
			mapTileWindow->attachToWidget(this->widget);
			mapTileWindow->setRealSize(miniMapData.size.x, miniMapData.size.y);
			mapTileWindow->setRealPosition(miniMapData.position.x, miniMapData.position.y);

			this->windowMapTiles.emplace_back(mapTileWindow);

			MyGUI::TextBox* mapTileTextBox = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::TextBox>(/*this->skin->getListSelectedValue()*/"TextBox",
				miniMapData.position.x, miniMapData.position.y, miniMapData.size.x, miniMapData.size.y,
				MyGUI::Align::Stretch, this->layer->getListSelectedValue());

			mapTileTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);

			if (false == this->bShowDebugData)
				mapTileTextBox->setCaption(miniMapData.sceneName);
			else
				mapTileTextBox->setCaption(miniMapData.sceneName + " s: " + Ogre::StringConverter::toString(miniMapData.size.x) + " x " + Ogre::StringConverter::toString(miniMapData.size.y)
					+ " p: " + Ogre::StringConverter::toString(miniMapData.position.x) + " x " + Ogre::StringConverter::toString(miniMapData.position.y));
			mapTileTextBox->setVisible(this->showNames->getBool() && this->bShowMiniMap);

			// Set map tile text box as child of map tile window
			// mapTileTextBox->attachToWidget(mapTileWindow);
			// mapTileTextBox->setRealSize(miniMapData.size.x, 0.05f);
			mapTileTextBox->setRealPosition(miniMapData.position.x + 0.01f, miniMapData.position.y + 0.015f);

			// Set label
			this->skinNames[i]->addUserData(GameObject::AttrActionLabel());
			this->skinNames[i]->setDescription(miniMapData.sceneName);

			this->textBoxMapTiles.emplace_back(mapTileTextBox);
		}

		this->widget->setVisible(this->bShowMiniMap);
		// 
		// Add event handler, if its a window, to close the mini map when x button is pressed
		MyGUI::Window* window = this->widget->castType<MyGUI::Window>(false);
		if (window != nullptr)
		{
			// window->eventKeyButtonPressed += newDelegate(this, &MyGUIMiniMapComponent::notifyKeyButtonPressed);
			window->eventWindowButtonPressed += newDelegate(this, &MyGUIMiniMapComponent::notifyWindowButtonPressed);
		}
	}

	void  MyGUIMiniMapComponent::generateTrackables(void)
	{
		this->destroyTrackables();

		Ogre::Vector2 viewPortSize = Ogre::Vector2(static_cast<Ogre::Real>(this->widget->getAbsoluteCoord().width), static_cast<Ogre::Real>(this->widget->getAbsoluteCoord().height)) / this->scaleFactor->getReal();

		for (unsigned int i = 0; i < this->trackableCount->getUInt(); i++)
		{
			Ogre::String sceneAndId = this->trackableIds[i]->getString();
			size_t found = sceneAndId.find(":");

			Ogre::String sceneName;
			unsigned long id = 0;
			if (Ogre::String::npos != found)
			{
				Ogre::String sceneName = sceneAndId.substr(found);
				Ogre::String strId = sceneAndId.substr(found + 1, sceneAndId.length() - found);
				id = Ogre::StringConverter::parseUnsignedLong(strId);
			}
			else
			{
				id = Ogre::StringConverter::parseUnsignedLong(sceneAndId);
			}
			
			if (id != 0)
			{
				std::pair<bool, Ogre::Vector2> trackableMiniMapPosition = 
					AppStateManager::getSingletonPtr()->getMiniMapModule()->parseGameObjectMinimapPosition(sceneName, id, this->axis->getListSelectedValue() == "X,Y" ? true : false, viewPortSize);
				if (true == trackableMiniMapPosition.first)
				{
					Ogre::Real width = 8.0f / viewPortSize.x;
					Ogre::Real height = 8.0f / viewPortSize.y;

					MyGUI::ImageBox* trackableImage = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox",
						trackableMiniMapPosition.second.x - width * 0.5f, trackableMiniMapPosition.second.y + height * 0.5f, width, height,
						MyGUI::Align::Center, this->layer->getListSelectedValue());

					trackableImage->setImageTexture(this->trackableImages[i]->getString());
					trackableImage->setImageRect(MyGUI::IntRect(0, 0, trackableImage->getImageSize().width, trackableImage->getImageSize().height));

					// trackableImage->setImageIndex(1);
					trackableImage->setVisible(this->bShowMiniMap);

					this->trackableImageBoxes.emplace_back(trackableImage);
					this->spriteAnimationIndices.push_back(-1);

					if (nullptr != this->trackableImageTileSizes[i])
						this->setTrackableImageTileSize(i, this->trackableImageTileSizes[i]->getVector2());
				}
			}
		}
	}

	void MyGUIMiniMapComponent::setSceneVisited(unsigned int index, bool visited)
	{
		if (index > this->visitedList.size())
			index = static_cast<unsigned int>(this->visitedList.size()) - 1;
		this->visitedList[index]->setValue(visited);
	}

	bool MyGUIMiniMapComponent::getIsSceneVisited(unsigned int index)
	{
		if (index > this->visitedList.size())
			return false;
		return this->visitedList[index]->getBool();
	}

	void MyGUIMiniMapComponent::setSceneVisited(const Ogre::String& sceneName, bool visited)
	{
		for (size_t i = 0; i < this->miniMapDataList.size(); i++)
		{
			if (sceneName == this->miniMapDataList[i].sceneName)
			{
				this->visitedList[i]->setValue(visited);
				break;
			}
		}
	}

	bool MyGUIMiniMapComponent::getIsSceneVisited(const Ogre::String& sceneName)
	{
		for (size_t i = 0; i < this->miniMapDataList.size(); i++)
		{
			if (sceneName == this->miniMapDataList[i].sceneName)
			{
				return this->visitedList[i]->getBool();
			}
		}
		return false;
	}

	void MyGUIMiniMapComponent::notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch)
	{
		// Does not work
		// if (key == MyGUI::KeyCode::Escape)
		if (NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->isActionDown(NOWA_A_MAP))
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
		if (true == this->isSimulating)
		{
			MyGUI::Window* window = sender->castType<MyGUI::Window>();
			unsigned int index = 0;
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->windowMapTiles.size()); i++)
			{
				if (sender == this->windowMapTiles[i])
				{
					index = i;
					break;
				}
			}
			if (nullptr != window)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction, index);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	void MyGUIMiniMapComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->bShowMiniMap)
		{
			// Update sprite animation values
			if (this->timeSinceLastUpdate >= 0.0f)
				this->timeSinceLastUpdate -= dt;
			else
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
							this->spriteAnimationIndices[i]++;
						else
							this->spriteAnimationIndices[i] = 0;
						// Set the index
						this->trackableImageBoxes[i]->setImageIndex(this->spriteAnimationIndices[i]);
					}
				}

				this->timeSinceLastUpdate = this->trackableImageAnimationSpeed->getReal();
			}
		}
	}

	void MyGUIMiniMapComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
		this->generateMiniMap();
	}

	bool MyGUIMiniMapComponent::connect(void)
	{
		bool success = MyGUIWindowComponent::connect();

		this->setUseVisitation(this->useVisitation->getBool());
		this->generateMiniMap();
		this->generateTrackables();
		this->setUseToolTip(this->useToolTip->getBool());

		return success;
	}

	bool MyGUIMiniMapComponent::disconnect(void)
	{
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
				else if (this->visitedList[i]->getName() == attribute->getName())
				{
					this->setSceneVisited(i, attribute->getBool());
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

	void MyGUIMiniMapComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIWindowComponent::writeXML(propertiesXML, doc, filePath);

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
		this->generateMiniMap();
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
		this->generateMiniMap();
	}

	Ogre::Real MyGUIMiniMapComponent::getScaleFactor(void) const
	{
		return this->scaleFactor->getReal();
	}

	void MyGUIMiniMapComponent::setUseToolTip(bool useToolTip)
	{
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
		if (index > this->skinNames.size())
			index = static_cast<unsigned int>(this->skinNames.size()) - 1;
		this->skinNames[index]->setListSelectedValue(skinName);

		this->windowMapTiles[index]->changeWidgetSkin(skinName);
	}

	Ogre::String MyGUIMiniMapComponent::getSkinName(unsigned int index) const
	{
		if (index > this->skinNames.size())
			return "";
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
		if (index > this->miniMapTilesColors.size())
			index = static_cast<unsigned int>(this->miniMapTilesColors.size()) - 1;
		this->miniMapTilesColors[index]->setValue(color);

		this->windowMapTiles[index]->setColour(MyGUI::Colour(color.x, color.y, color.z));
	}

	Ogre::Vector3 MyGUIMiniMapComponent::getMiniMapTileColor(unsigned int index)
	{
		if (index > this->miniMapTilesColors.size())
			return Ogre::Vector3::ZERO;
		return this->miniMapTilesColors[index]->getVector3();
	}

	void MyGUIMiniMapComponent::setToolTipDescription(unsigned int index, const Ogre::String& description)
	{
		if (index > this->toolTipDescriptions.size())
			index = static_cast<unsigned int>(this->toolTipDescriptions.size()) - 1;
		this->toolTipDescriptions[index]->setValue(description);

		if (index < this->windowMapTiles.size())
			this->windowMapTiles[index]->setUserString("description", description);
	}

	Ogre::String MyGUIMiniMapComponent::getToolTipDescription(unsigned int index)
	{
		if (index > this->toolTipDescriptions.size())
			return "";
		return this->toolTipDescriptions[index]->getString();
	}

	void MyGUIMiniMapComponent::setMiniMapTileVisible(unsigned int index, bool miniMapTileVisible)
	{
		if (index > this->windowMapTiles.size())
			index = static_cast<unsigned int>(this->windowMapTiles.size()) - 1;

		if (index < this->windowMapTiles.size())
		{
			this->windowMapTiles[index]->setVisible(miniMapTileVisible);
			this->textBoxMapTiles[index]->setVisible(miniMapTileVisible);
		}
	}

	bool MyGUIMiniMapComponent::isMiniMapTileVisible(unsigned int index) const
	{
		if (index > this->windowMapTiles.size())
			return false;
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

			for (size_t i = oldSize; i < this->trackableIds.size(); i++)
			{
				this->trackableIds[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableId() + Ogre::StringConverter::toString(i), "", this->attributes);
				this->trackableImages[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableImage() + Ogre::StringConverter::toString(i), "circleRed.png", this->attributes);
				
				this->trackableImages[i]->addUserData(GameObject::AttrActionFileOpenDialog());

				this->trackableImageTileSizes[i] = new Variant(MyGUIMiniMapComponent::AttrTrackableImageTileSize() + Ogre::StringConverter::toString(i),
					Ogre::Vector2(32.0f, 32.0f), this->attributes);
				this->trackableImageTileSizes[i]->setDescription("Sets the tile size: e.g. Image may be of size: 32x64, but tile size 32x32, so that sprite animation is done automatically switching the image tiles from 0 to 32 and 32 to 64 automatically");
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
			this->generateMiniMap();
		}
	}

	unsigned int MyGUIMiniMapComponent::getTrackableCount(void) const
	{
		return this->trackableCount->getUInt();
	}

	void MyGUIMiniMapComponent::setTrackableId(unsigned int index, const Ogre::String& id)
	{
		if (index > this->trackableIds.size())
			index = static_cast<unsigned int>(this->trackableIds.size()) - 1;
		this->trackableIds[index]->setValue(id);

		this->generateMiniMap();
		this->generateTrackables();
	}

	Ogre::String MyGUIMiniMapComponent::getTrackableId(unsigned int index)
	{
		if (index > this->trackableIds.size())
			return "";
		return this->trackableIds[index]->getString();

		this->generateMiniMap();
	}

	void MyGUIMiniMapComponent::setTrackableImage(unsigned int index, const Ogre::String& imageName)
	{
		if (index > this->trackableImages.size())
			index = static_cast<unsigned int>(this->trackableImages.size()) - 1;
		this->trackableImages[index]->setValue(imageName);
		if (this->trackableImageBoxes.size() == this->trackableImages.size())
		{
			this->trackableImageBoxes[index]->setImageTexture(imageName);
			this->trackableImageBoxes[index]->setImageRect(MyGUI::IntRect(0, 0, this->trackableImageBoxes[index]->getImageSize().width, 
				this->trackableImageBoxes[index]->getImageSize().height));
		}
		else
		{
			this->generateTrackables();
		}
	}

	Ogre::String MyGUIMiniMapComponent::getTrackableImage(unsigned int index)
	{
		if (index > this->trackableImages.size())
			return "";
		return this->trackableImages[index]->getString();
	}

	void MyGUIMiniMapComponent::setTrackableImageTileSize(unsigned int index, const Ogre::Vector2& imageTileSize)
	{
		if (index > this->trackableImageTileSizes.size())
			index = static_cast<unsigned int>(this->trackableImageTileSizes.size()) - 1;
		this->trackableImageTileSizes[index]->setValue(imageTileSize);
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
	}

	Ogre::Vector2 MyGUIMiniMapComponent::getTrackableImageTileSize(unsigned int index)
	{
		if (index > this->trackableImageTileSizes.size())
			return Ogre::Vector2::ZERO;
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
		this->bShowMiniMap = bShow;
		this->generateTrackables();
		if (nullptr != this->widget)
		{
			this->widget->setVisible(bShow);

			if (false == this->useVisitation->getBool())
			{
				for (size_t i = 0; i < this->textBoxMapTiles.size(); i++)
				{
					this->textBoxMapTiles[i]->setVisible(bShow);
				}
				for (size_t i = 0; i < this->windowMapTiles.size(); i++)
				{
					this->windowMapTiles[i]->setVisible(bShow);
				}
				for (size_t i = 0; i < this->trackableImageBoxes.size(); i++)
				{
					this->trackableImageBoxes[i]->setVisible(bShow);
				}
			}
			else
			{
				if (true == bShow)
				{
					// Only show tile if also is set on visited list!
					for (size_t i = 0; i < this->textBoxMapTiles.size(); i++)
					{
						this->textBoxMapTiles[i]->setVisible(this->visitedList[i]->getBool());
					}
					for (size_t i = 0; i < this->windowMapTiles.size(); i++)
					{
						this->windowMapTiles[i]->setVisible(this->visitedList[i]->getBool());
					}
					for (size_t i = 0; i < this->trackableImageBoxes.size(); i++)
					{
						this->trackableImageBoxes[i]->setVisible(this->visitedList[i]->getBool());
					}
				}
				else
				{
					for (size_t i = 0; i < this->textBoxMapTiles.size(); i++)
					{
						this->textBoxMapTiles[i]->setVisible(false);
					}
					for (size_t i = 0; i < this->windowMapTiles.size(); i++)
					{
						this->windowMapTiles[i]->setVisible(false);
					}
					for (size_t i = 0; i < this->trackableImageBoxes.size(); i++)
					{
						this->trackableImageBoxes[i]->setVisible(false);
					}
				}
			}
		}
	}

	bool MyGUIMiniMapComponent::isMiniMapShown(void) const
	{
		return this->bShowMiniMap;
	}
	
}; // namespace end