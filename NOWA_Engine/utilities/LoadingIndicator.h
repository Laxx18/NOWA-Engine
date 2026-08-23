#ifndef LOADING_INDICATOR_H
#define LOADING_INDICATOR_H

#include "defines.h"

#include "MyGUI.h"

namespace NOWA
{
    /**
     * @class   ILoadingIndicator
     * @brief   Something that visibly moves while a scene is loading.
     *
     * Attention: ALL three functions are called ON THE RENDER THREAD. onShow() and onHide() run
     * from the loading guard, onUpdate() runs from the throttled loading frame in the command pump
     * loop. Do not enqueue render commands from inside them - you are already there, and an
     * enqueueAndWait would deadlock against yourself.
     *
     * Attention: An implementation must be cheap. onUpdate() runs at the throttled loading frame
     * rate (about 15 Hz), and everything it does happens while the logic thread is trying to load.
     */
    class EXPORTED ILoadingIndicator
    {
    public:
        virtual ~ILoadingIndicator() = default;

        /**
         * @brief   Creates the widgets. Called once when loading starts.
         */
        virtual void onShow(void) = 0;

        /**
         * @brief   Advances the animation.
         * @param[in]   dt  Seconds since the previous onUpdate call.
         */
        virtual void onUpdate(Ogre::Real dt) = 0;

        /**
         * @brief   Destroys the widgets. Called once when loading finishes.
         */
        virtual void onHide(void) = 0;
    };

    /**
     * @class   DotsTextLoadingIndicator
     * @brief   Shows "Loading", "Loading.", "Loading..", "Loading..." in a loop.
     *
     * The cheapest possible indicator: one EditBox, no textures, no skin beyond the default. Use
     * this when you only want to prove that the application is alive.
     */
    class EXPORTED DotsTextLoadingIndicator : public ILoadingIndicator
    {
    public:
        /**
         * @param[in]   captionKey      MyGUI language key or plain text, without the dots.
         * @param[in]   secondsPerDot   How long one dot stays before the next is added.
         */
        DotsTextLoadingIndicator(const Ogre::String& captionKey = "#{Loading}", Ogre::Real secondsPerDot = 0.35f);

        virtual ~DotsTextLoadingIndicator();

        virtual void onShow(void) override;
        virtual void onUpdate(Ogre::Real dt) override;
        virtual void onHide(void) override;

    private:
        Ogre::String captionKey;
        Ogre::Real secondsPerDot;
        Ogre::Real elapsedSeconds;
        int dotCount;
        MyGUI::EditBox* label;
    };

    /**
     * @class   RotatingImageLoadingIndicator
     * @brief   Rotates an image, same approach as EngineResourceSceneListener.
     *
     * Attention: Needs the "RotatingSkin" skin and the given texture to be available. If the skin
     * is missing, this logs once and then does nothing rather than crashing - see onShow().
     */
    class EXPORTED RotatingImageLoadingIndicator : public ILoadingIndicator
    {
    public:
        /**
         * @param[in]   textureName         Texture to rotate.
         * @param[in]   revolutionsPerSecond Rotation speed.
         * @param[in]   sizeInPixels        Edge length of the (square) image.
         */
        RotatingImageLoadingIndicator(const Ogre::String& textureName = "NOWA_Logo3.png", Ogre::Real revolutionsPerSecond = 0.5f, int sizeInPixels = 130);

        virtual ~RotatingImageLoadingIndicator();

        virtual void onShow(void) override;
        virtual void onUpdate(Ogre::Real dt) override;
        virtual void onHide(void) override;

    private:
        Ogre::String textureName;
        Ogre::Real revolutionsPerSecond;
        int sizeInPixels;
        Ogre::Real currentAngle;
        MyGUI::ImageBox* image;
        MyGUI::RotatingSkin* rotatingSkin;
    };

}; // namespace end

#endif