#include "NOWAPrecompiled.h"
#include "ObjectTitle.h"
#include "main/Core.h"

namespace NOWA
{

	ObjectTitle::ObjectTitle(const Ogre::String& strName, Ogre::MovableObject* pObject, Ogre::Camera* camera,
		const Ogre::String& strFontName, const Ogre::ColourValue& color)
		: pObject(pObject),
		camera(camera)
	{
		this->pOverlay = Ogre::v1::OverlayManager::getSingleton().create(strName + "_TitleOverlay");
		this->pContainer = (Ogre::v1::OverlayContainer*)Ogre::v1::OverlayManager::getSingleton().createOverlayElement("Panel", strName + "_TitleContainer");
		this->pOverlay->add2D(this->pContainer);

		this->pTextarea = Ogre::v1::OverlayManager::getSingleton().createOverlayElement("TextArea", strName + "_TitleTextArea");
		this->pTextarea->setDimensions(0.8f, 0.8f);
		this->pTextarea->setMetricsMode(Ogre::v1::GMM_PIXELS);
		this->pTextarea->setPosition(0.1f, 0.1f);

		Ogre::FontManager::getSingleton().load(strFontName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		this->pFont = (Ogre::Font*)Ogre::FontManager::getSingleton().getByName(strFontName).getPointer();
		this->pTextarea->setParameter("font_name", strFontName);
		this->pTextarea->setParameter("char_height", this->pFont->getParameter("size"));
		this->pTextarea->setParameter("horz_align", "left");
		this->pTextarea->setColour(color);

		this->pContainer->addChild(this->pTextarea);
		this->pContainer->show();

		//Titelposition einmal berechnen
		this->update();
	}

	ObjectTitle::~ObjectTitle()
	{
		Ogre::v1::OverlayManager *pOverlayManager = Ogre::v1::OverlayManager::getSingletonPtr();
		this->pTextarea->setCaption("");
		this->pTextarea->hide();
		this->pContainer->removeChild(this->pTextarea->getName());
		this->pContainer->hide();
		this->pOverlay->remove2D(this->pContainer);
		pOverlayManager->destroyOverlayElement(this->pTextarea);
		pOverlayManager->destroyOverlayElement(this->pContainer);
		this->pOverlay->hide();
		pOverlayManager->destroy(this->pOverlay);
	}

	void ObjectTitle::setTitle(const Ogre::String& strTitle)
	{
		this->pTextarea->setCaption(strTitle);
		textDim = getTextDimensions(strTitle);
		this->pContainer->setDimensions(textDim.x, textDim.y);
	}

	void ObjectTitle::setColor(const Ogre::ColourValue& color)
	{
		this->pTextarea->setColour(color);
	}

	void ObjectTitle::update()
	{
		if (nullptr == this->camera)
		{
			return;
		}

		if (!this->pObject->isVisible())
		{
			this->pOverlay->hide();
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
			/*if (cameraPlane.getSide(point) != Ogre::Plane::NEGATIVE_SIDE)
			{
				this->pOverlay->hide();
				return;
			}*/

			//2D Bildschirmkoordinaten fuer den Punkt erhalten
			point = this->camera->getProjectionMatrix() * (this->camera->getViewMatrix() * point);
			// point.y = point.y + 0.25f;

			//In Overlaykoordinatensystem kalibireren [-1, 1] bis [0, 1]
			Ogre::Real x = (point.x / 2) + 0.5f;
			Ogre::Real y = 1 - ((point.y / 2) + 0.5f);

			//Position aktualisieren und Text im Zentrum darstellen
			this->pContainer->setPosition(x - (textDim.x / 2), y);
			this->pOverlay->show();
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