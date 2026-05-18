#ifndef MYGUI_SPRITE_COMPONENT_BASE_H
#define MYGUI_SPRITE_COMPONENT_BASE_H

#include "MyGUIComponents.h"

namespace NOWA
{
    class EXPORTED MyGUISpriteComponentBase : public MyGUIComponent
    {
    public:
        // Lets MyGUIItemBoxComponent find ANY sprite via getComponentWithOccurrence<MyGUISpriteComponentBase>()
        // which matches against PARENT_CLASS_ID in the tuple.
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("MyGUISpriteComponentBase");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "MyGUISpriteComponentBase";
        }

        virtual void setRepeat(bool repeat) = 0;

        virtual void setActivated(bool activated) = 0;

        /**
         * @brief   Repositions and resizes the sprite widget to overlay a specific inventory cell.
         *          Must be called from the render thread.
         */
        virtual void positionOverCell(const MyGUI::IntCoord& cellAbsoluteCoord) = 0;
    };

}; // namespace end

#endif