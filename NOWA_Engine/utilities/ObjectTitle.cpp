#include "NOWAPrecompiled.h"
#include "ObjectTitle.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"

namespace NOWA
{

	ObjectTitle::ObjectTitle(const Ogre::String& strName, Ogre::MovableObject* pObject, Ogre::Camera* camera,
		const Ogre::String& strFontName, const Ogre::ColourValue& color)
		: pObject(pObject),
		camera(camera)
	{
		this->uniqueId = NOWA::makeUniqueID();

		// Save strName etc. for later use inside lambda
		auto fontName = strFontName;
		auto overlayName = strName + "_TitleOverlay";
		auto containerName = strName + "_TitleContainer";
		auto textAreaName = strName + "_TitleTextArea";

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("ObjectTitle constructor render init", _6(this, fontName, overlayName, containerName, textAreaName, color),
		{
			Ogre::v1::OverlayManager& overlayMgr = Ogre::v1::OverlayManager::getSingleton();
			Ogre::FontManager& fontMgr = Ogre::FontManager::getSingleton();

			this->pOverlay = overlayMgr.create(overlayName);
			this->pContainer = static_cast<Ogre::v1::OverlayContainer*>(overlayMgr.createOverlayElement("Panel", containerName));
			this->pOverlay->add2D(this->pContainer);

			this->pTextarea = overlayMgr.createOverlayElement("TextArea", textAreaName);
			this->pTextarea->setDimensions(0.8f, 0.8f);
			this->pTextarea->setMetricsMode(Ogre::v1::GMM_PIXELS);
			this->pTextarea->setPosition(0.1f, 0.1f);

			// Load font and set parameters
			fontMgr.load(fontName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			Ogre::Font * font = static_cast<Ogre::Font*>(fontMgr.getByName(fontName).getPointer());
			this->pFont = font;

			this->pTextarea->setParameter("font_name", fontName);
			this->pTextarea->setParameter("char_height", font->getParameter("size"));
			this->pTextarea->setParameter("horz_align", "left");
			this->pTextarea->setColour(color);

			this->pContainer->addChild(this->pTextarea);
			this->pContainer->show();
		});

		this->update();
	}

	ObjectTitle::~ObjectTitle()
	{
		Ogre::v1::OverlayManager* pOverlayManager = Ogre::v1::OverlayManager::getSingletonPtr();

		// Capture everything needed for safe deletion on render thread
		auto overlay = this->pOverlay;
		auto container = this->pContainer;
		auto textarea = this->pTextarea;

		ENQUEUE_RENDER_COMMAND_MULTI("Destroy ObjectTitle overlay", _4(pOverlayManager, overlay, container, textarea),
		{
			if (textarea)
			{
				textarea->setCaption("");
				textarea->hide();
			}

			if (container && textarea)
				container->removeChild(textarea->getName());

			if (container)
				container->hide();

			if (overlay && container)
				overlay->remove2D(container);

			if (pOverlayManager)
			{
				if (textarea)
					pOverlayManager->destroyOverlayElement(textarea);
				if (container)
					pOverlayManager->destroyOverlayElement(container);
				if (overlay)
				{
					overlay->hide();
					pOverlayManager->destroy(overlay);
				}
			}
		});

		// Optional: clear local pointers
		this->pTextarea = nullptr;
		this->pContainer = nullptr;
		this->pOverlay = nullptr;
	}


	void ObjectTitle::setTitle(const Ogre::String& strTitle)
	{
		textDim = getTextDimensions(strTitle); // Do this on logic thread

		auto textarea = this->pTextarea;
		auto container = this->pContainer;
		auto dim = this->textDim; // Make a copy to capture by value

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("ObjectTitle::setTitle", _4(textarea, container, strTitle, dim),
		{
			if (textarea)
			{
				textarea->setCaption(strTitle);
			}
			if (container)
			{
				container->setDimensions(dim.x, dim.y);
			}
		});
	}


	void ObjectTitle::setColor(const Ogre::ColourValue& color)
	{
		auto textarea = this->pTextarea;

		ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("ObjectTitle::setColor", _2(textarea, color),
		{
			if (textarea)
			{
				textarea->setColour(color);
			}
		});
	}

	void ObjectTitle::update()
	{
		if (nullptr == this->camera)
		{
			return;
		}

		if (!this->pObject->isVisible())
		{
			// Hide overlay safely on render thread
			auto overlay = this->pOverlay;
			// auto closureFunction = [this, overlay](Ogre::Real weight)
			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("ObjectTitle::update", _1(overlay),
			{
				if (overlay)
				{
					overlay->hide();
				}
			});
			// Ogre::String id = "ObjectTitle::update1" + Ogre::StringConverter::toString(this->uniqueId);
			// NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
			return;
		}
		if (this->pObject)
		{
			// Get center of bounding box
			const Ogre::Aabb& aaBB = this->pObject->getLocalAabb();

			Ogre::Vector3 minimum = aaBB.getMinimum();
			Ogre::Vector3 maximum = aaBB.getMaximum();
			Ogre::Vector3 corners[8];
			//Ogre::Vector3 mMinimum(-1,-1,-1);
			//Ogre::Vector3 mMaximum(1,1,1);
			//mMinimum*=100.0f;
			//mMaximum*=100.0f;
			corners[0] = minimum;
			corners[1] = Ogre::Vector3(minimum.x, maximum.y, minimum.z);
			corners[2] = Ogre::Vector3(maximum.x, maximum.y, minimum.z);
			corners[3] = Ogre::Vector3(maximum.x, minimum.y, minimum.z);
			corners[4] = maximum;
			corners[5] = Ogre::Vector3(minimum.x, maximum.y, maximum.z);
			corners[6] = Ogre::Vector3(minimum.x, minimum.y, maximum.z);
			corners[7] = Ogre::Vector3(maximum.x, minimum.y, maximum.z);

			Ogre::Vector3 point = (corners[0] + corners[2] + corners[4] + corners[6]) / 4.0f;

			// Den Text nur anzeigen, wenn die Kamera drauf schaut
			Ogre::Plane cameraPlane = Ogre::Plane(Ogre::Vector3(this->camera->getDerivedOrientation().zAxis()), this->camera->getDerivedPosition());

			//2D Bildschirmkoordinaten fuer den Punkt erhalten
			point = this->camera->getProjectionMatrix() * (this->camera->getViewMatrix() * point);
			// point.y = point.y + 0.25f;

			//In Overlaykoordinatensystem kalibireren [-1, 1] bis [0, 1]
			Ogre::Real x = (point.x / 2) + 0.5f;
			Ogre::Real y = 1 - ((point.y / 2) + 0.5f);

			// Defer final write to render thread
			auto container = this->pContainer;
			auto overlay = this->pOverlay;
			auto textWidth = this->textDim.x;

			// auto closureFunction = [this, overlay, container, x, y, textWidth](Ogre::Real weight)
			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("ObjectTitle::update2", _5(overlay, container, x, y, textWidth),
			{
				if (container)
				{
					container->setPosition(x - (textWidth / 2), y);
				}
				if (overlay)
				{
					overlay->show();
				}
			});
			// Ogre::String id = "ObjectTitle::update2" + Ogre::StringConverter::toString(this->uniqueId);
			// NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
		}
	}

	Ogre::Vector2 ObjectTitle::getTextDimensions(Ogre::String strText)
	{
		//Schriftgroesse erhalten
		Ogre::Real charHeight = Ogre::StringConverter::parseReal(this->pFont->getParameter("size"));

		Ogre::Vector2 result(0, 0);

		for (Ogre::String::iterator i = strText.begin(); i < strText.end(); ++i)
		{
			//Schriftgroessenverhaeltnis berechnen (Abstand der Buchstaben zueinander
			if (*i == 0x0020)
				result.x += this->pFont->getGlyphAspectRatio(0x0030);
			else
				result.x += this->pFont->getGlyphAspectRatio(*i);
		}

		//Schrift skalieren
		result.x = (result.x * charHeight) / (float)Core::getSingletonPtr()->getOgreRenderWindow()->getWidth();
		result.y = charHeight / (float)Core::getSingletonPtr()->getOgreRenderWindow()->getHeight();

		return result;
	}

} // namespace end