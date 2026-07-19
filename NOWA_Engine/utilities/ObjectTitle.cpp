#include "NOWAPrecompiled.h"
#include "ObjectTitle.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"

// Adjust this include if your project uses individual MyGUI headers instead
// of the umbrella header (e.g. MyGUI_Gui.h, MyGUI_TextBox.h).
#include <MyGUI.h>

namespace NOWA
{
    namespace
    {
        // ASSUMPTION — unverified, same as the Fade/Console layer guesses:
        // pick a layer that renders where a label should sit relative to
        // OTHER MyGUI widgets (menus etc.) — every MyGUI layer already
        // renders above the whole 3D scene (including gizmos), so "never
        // occluded" holds regardless of which one is picked here. Test and
        // swap if it ends up under/over the wrong things.
        const Ogre::String OBJECT_TITLE_LAYER_NAME = "Info";
    }

    ObjectTitle::ObjectTitle(const Ogre::String& strName, Ogre::MovableObject* pObject, Ogre::Camera* camera, const Ogre::String& strFontName, const Ogre::ColourValue& color) : pObject(pObject), camera(camera), pTextWidget(nullptr)
    {
        this->uniqueId = NOWA::makeUniqueID();

        auto widgetName = strName + "_TitleText";
        auto fontName = strFontName;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, widgetName, fontName, color]()
        {
            // Size (0,0) initially — setTitle() below sizes it to fit once
            // there is actual text to measure.
            this->pTextWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::TextBox>("TextBox", 0.0f, 0.0f, 0.0f, 0.0f, MyGUI::Align::Default, OBJECT_TITLE_LAYER_NAME, widgetName);

            // No confirmed MyGUI font resource matching strFontName — falls
            // back to the skin's default font if this name doesn't resolve.
            this->pTextWidget->setFontName(fontName);
            this->pTextWidget->setTextColour(MyGUI::Colour(color.r, color.g, color.b, color.a));
            this->pTextWidget->setTextAlign(MyGUI::Align::Center);
            this->pTextWidget->setNeedMouseFocus(false);
            this->pTextWidget->setNeedKeyFocus(false);
            this->pTextWidget->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ObjectTitle constructor render init");

        this->update();
    }

    ObjectTitle::~ObjectTitle()
    {
        auto widget = this->pTextWidget;

        NOWA::GraphicsModule::RenderCommand renderCommand = [widget]()
        {
            if (nullptr != widget)
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(widget);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "Destroy ObjectTitle widget");

        this->pTextWidget = nullptr;
    }

    void ObjectTitle::setTitle(const Ogre::String& strTitle)
    {
        auto widget = this->pTextWidget;

        NOWA::GraphicsModule::RenderCommand renderCommand = [widget, strTitle]()
        {
            if (nullptr == widget)
            {
                return;
            }

            widget->setCaption(strTitle);

            // Size the widget to exactly wrap the measured text. update()
            // re-reads this size fresh every frame to center it — no stale
            // cross-thread "text width" copy needed, unlike the original.
            MyGUI::IntSize textSize = widget->getTextSize();
            widget->setSize(textSize);
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ObjectTitle::setTitle");
    }

    void ObjectTitle::setColor(const Ogre::ColourValue& color)
    {
        auto widget = this->pTextWidget;

        NOWA::GraphicsModule::RenderCommand renderCommand = [widget, color]()
        {
            if (nullptr != widget)
            {
                widget->setTextColour(MyGUI::Colour(color.r, color.g, color.b, color.a));
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ObjectTitle::setColor");
    }

    void ObjectTitle::update()
    {
        if (nullptr == this->camera)
        {
            return;
        }

        if (!this->pObject->isVisible())
        {
            auto widget = this->pTextWidget;
            NOWA::GraphicsModule::RenderCommand renderCommand = [widget]()
            {
                if (nullptr != widget)
                {
                    widget->setVisible(false);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ObjectTitle::update_hide");
            return;
        }

        if (nullptr != this->pObject)
        {
            // Same projection math as the original — unrelated to the
            // Overlay->MyGUI change, only the destination widget differs.
            const Ogre::Aabb& aaBB = this->pObject->getLocalAabb();

            Ogre::Vector3 minimum = aaBB.getMinimum();
            Ogre::Vector3 maximum = aaBB.getMaximum();
            Ogre::Vector3 corners[8];
            corners[0] = minimum;
            corners[1] = Ogre::Vector3(minimum.x, maximum.y, minimum.z);
            corners[2] = Ogre::Vector3(maximum.x, maximum.y, minimum.z);
            corners[3] = Ogre::Vector3(maximum.x, minimum.y, minimum.z);
            corners[4] = maximum;
            corners[5] = Ogre::Vector3(minimum.x, maximum.y, maximum.z);
            corners[6] = Ogre::Vector3(minimum.x, minimum.y, maximum.z);
            corners[7] = Ogre::Vector3(maximum.x, minimum.y, maximum.z);

            Ogre::Vector3 point = (corners[0] + corners[2] + corners[4] + corners[6]) / 4.0f;

            point = this->camera->getProjectionMatrix() * (this->camera->getViewMatrix() * point);

            Ogre::Real x = (point.x / 2.0f) + 0.5f;
            Ogre::Real y = 1.0f - ((point.y / 2.0f) + 0.5f);

            auto widget = this->pTextWidget;

            NOWA::GraphicsModule::RenderCommand renderCommand = [widget, x, y]()
            {
                if (nullptr == widget)
                {
                    return;
                }

                MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
                int pixelX = static_cast<int>(x * viewSize.width);
                int pixelY = static_cast<int>(y * viewSize.height);

                // Center horizontally on the projected point, matching the
                // original's container->setPosition(x - textWidth/2, y).
                MyGUI::IntSize size = widget->getSize();
                widget->setPosition(pixelX - size.width / 2, pixelY);
                widget->setVisible(true);
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ObjectTitle::update_position");
        }
    }

} // namespace end