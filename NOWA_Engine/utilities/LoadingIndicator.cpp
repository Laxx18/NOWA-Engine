#include "NOWAPrecompiled.h"
#include "LoadingIndicator.h"
#include "main/Core.h"
#include "utilities/MyGUIUtilities.h"

namespace NOWA
{
    // =============================================================================================
    // DotsTextLoadingIndicator
    // =============================================================================================

    DotsTextLoadingIndicator::DotsTextLoadingIndicator(const Ogre::String& captionKey, Ogre::Real secondsPerDot) : captionKey(captionKey), secondsPerDot(secondsPerDot), elapsedSeconds(0.0f), dotCount(0), label(nullptr)
    {
        if (this->secondsPerDot < 0.05f)
        {
            this->secondsPerDot = 0.05f;
        }
    }

    DotsTextLoadingIndicator::~DotsTextLoadingIndicator()
    {
        // Attention: no widget destruction here. The destructor may run on any thread, and MyGUI
        // must only be touched on the render thread. onHide() is the place for that, and the owner
        // is responsible for calling it.
        if (nullptr != this->label)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotsTextLoadingIndicator] Warning: destroyed while its widget still exists. onHide() was not called.");
        }
    }

    void DotsTextLoadingIndicator::onShow(void)
    {
        if (nullptr != this->label)
        {
            return;
        }

        this->elapsedSeconds = 0.0f;
        this->dotCount = 0;

        this->label = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBoxEmpty", 0.42f, 0.90f, 0.30f, 0.06f, MyGUI::Align::Default, "Overlapped", "LoadingIndicator_DotsLabel");

        if (nullptr == this->label)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotsTextLoadingIndicator] Could not create the label widget. Skin 'EditBoxEmpty' missing?");
            return;
        }

        this->label->setEditStatic(true);
        MyGUIUtilities::getInstance()->setFontSize(this->label, 18);
        this->label->setCaptionWithReplacing("#0044FF" + this->captionKey);
        this->label->setVisible(true);
    }

    void DotsTextLoadingIndicator::onUpdate(Ogre::Real dt)
    {
        if (nullptr == this->label)
        {
            return;
        }

        this->elapsedSeconds += dt;

        if (this->elapsedSeconds < this->secondsPerDot)
        {
            return;
        }

        this->elapsedSeconds = 0.0f;
        this->dotCount = (this->dotCount + 1) % 4;

        Ogre::String caption = "#0044FF" + this->captionKey;
        for (int i = 0; i < this->dotCount; i++)
        {
            caption += ".";
        }

        this->label->setCaptionWithReplacing(caption);
    }

    void DotsTextLoadingIndicator::onHide(void)
    {
        if (nullptr == this->label)
        {
            return;
        }

        MyGUI::Gui::getInstancePtr()->destroyWidget(this->label);
        this->label = nullptr;
    }

    // =============================================================================================
    // RotatingImageLoadingIndicator
    // =============================================================================================

    RotatingImageLoadingIndicator::RotatingImageLoadingIndicator(const Ogre::String& textureName, Ogre::Real revolutionsPerSecond, int sizeInPixels) :
        textureName(textureName),
        revolutionsPerSecond(revolutionsPerSecond),
        sizeInPixels(sizeInPixels),
        currentAngle(0.0f),
        image(nullptr),
        rotatingSkin(nullptr)
    {
    }

    RotatingImageLoadingIndicator::~RotatingImageLoadingIndicator()
    {
        if (nullptr != this->image)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RotatingImageLoadingIndicator] Warning: destroyed while its widget still exists. onHide() was not called.");
        }
    }

    void RotatingImageLoadingIndicator::onShow(void)
    {
        if (nullptr != this->image)
        {
            return;
        }

        this->currentAngle = 0.0f;

        Ogre::Window* renderWindow = Core::getSingletonPtr()->getOgreRenderWindow();
        if (nullptr == renderWindow)
        {
            return;
        }

        const int positionX = static_cast<int>(renderWindow->getWidth()) / 2 - this->sizeInPixels / 2;
        const int positionY = static_cast<int>(renderWindow->getHeight()) / 2 - this->sizeInPixels / 2;

        this->image = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin", MyGUI::IntCoord(positionX, positionY, this->sizeInPixels, this->sizeInPixels), MyGUI::Align::Default, "Overlapped", "LoadingIndicator_RotatingImage");

        if (nullptr == this->image)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RotatingImageLoadingIndicator] Could not create the image widget. Skin 'RotatingSkin' missing?");
            return;
        }

        this->image->setImageTexture(this->textureName);
        this->image->setVisible(true);

        MyGUI::ISubWidget* main = this->image->getSubWidgetMain();
        if (nullptr != main)
        {
            this->rotatingSkin = main->castType<MyGUI::RotatingSkin>();
            this->rotatingSkin->setCenter(MyGUI::IntPoint(this->image->getWidth() / 2, this->image->getHeight() / 2));
        }
        else
        {
            // Attention: the widget stays, it just will not rotate. Logged once, not per frame.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RotatingImageLoadingIndicator] Could not get RotatingSkin. Check resources.cfg for MyGUI_Media.");
        }
    }

    void RotatingImageLoadingIndicator::onUpdate(Ogre::Real dt)
    {
        if (nullptr == this->rotatingSkin)
        {
            return;
        }

        this->currentAngle += this->revolutionsPerSecond * Ogre::Math::TWO_PI * dt;

        if (this->currentAngle > Ogre::Math::TWO_PI)
        {
            this->currentAngle -= Ogre::Math::TWO_PI;
        }

        this->rotatingSkin->setAngle(this->currentAngle);
    }

    void RotatingImageLoadingIndicator::onHide(void)
    {
        if (nullptr == this->image)
        {
            return;
        }

        this->rotatingSkin = nullptr;
        MyGUI::Gui::getInstancePtr()->destroyWidget(this->image);
        this->image = nullptr;
    }

}; // namespace end