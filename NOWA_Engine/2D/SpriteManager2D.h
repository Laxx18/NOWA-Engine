#ifndef SPRITE_MANAGER_2D_H
#define SPRITE_MANAGER_2D_H

// Ogre 2d: a small wrapper for 2d Graphics Programming in Ogre.
/*
Wrapper for 2d Graphics in the Ogre 3d engine.

Coded by H. Hernán Moraldo from Moraldo Games
www.hernan.moraldo.com.ar/pmenglish/field.php

Thanks for the Cegui team as their rendering code in Ogre gave me
fundamental insight on the management of hardware buffers in Ogre.

--------------------

Copyright (c) 2006 Horacio Hernan Moraldo

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held liable
for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.

*/

#include <list>

#include "defines.h"

namespace NOWA
{

	struct Ogre2dSprite
	{
		Ogre::Real x1, y1, x2, y2;// sprite coordinates
		Ogre::Real tx1, ty1, tx2, ty2;// texture coordinates
		Ogre::ResourceHandle texHandle;// texture handle
	};

	struct VertexChunk
	{
		Ogre::ResourceHandle texHandle;
		unsigned int vertexCount;
	};

	class EXPORTED SpriteManager2D : public Ogre::RenderQueueListener
	{
	public:
		/// Initializes this 2d Manager
		/** and registers it as the render queue listener.*/
		SpriteManager2D(Ogre::SceneManager* sceneManager, Ogre::uint8 targetQueue, bool afterQueue);
		virtual ~SpriteManager2D();

		/// Called by Ogre, for being a render queue listener
		virtual void renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& skipThisInvocation);
		/// Called by Ogre, for being a render queue listener
		virtual void renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& repeatThisInvocation);

		Ogre::TexturePtr addSprite(const std::string& textureName);

		/// Buffers a sprite to be sent to the screen at render time.
		/**
		Sprite coordinates are in screen space: top left pixel is (-1, 1), and bottom right
		is (1, -1). The texture space, instead, ranges from (0, 0) to (1, 1).

		/param textureName Name of the texture to use in this sprite (remember: texture
		name, not material name!). The texture has to be already loaded by Ogre for this
		to work.
		/param x1 x coordinate for the top left point in the sprite.
		/param y1 y coordinate for the top left point in the sprite.
		/param x2 x coordinate for the bottom right point in the sprite.
		/param y2 y coordinate for the bottom right point in the sprite.
		/param tx1 u coordinate for the texture, in the top left point of the sprite.
		/param ty1 v coordinate for the texture, in the top left point of the sprite.
		/param tx2 u coordinate for the texture, in the bottom right point of the sprite.
		/param ty2 u coordinate for the texture, in the bottom right point of the sprite.
		*/
		void spriteBltFull(Ogre::TexturePtr texturePtr, Ogre::Real x1, Ogre::Real y1, Ogre::Real x2, Ogre::Real y2, Ogre::Real tx1 = 0.0f, Ogre::Real ty1 = 0.0f,
			Ogre::Real tx2 = 1.0f, Ogre::Real ty2 = 1.0f);

		void scroll(Ogre::TexturePtr texturePtr, Ogre::Real xSpeed, Ogre::Real ySpeed, Ogre::Real dt);

		void backgroundRelativeToPositionX(Ogre::TexturePtr texturePtr, Ogre::Real absoluteXPos, Ogre::Real dt, Ogre::Real xSpeed = 1.0f);

		void backgroundRelativeToPosition(Ogre::TexturePtr texturePtr, Ogre::Vector2 absolutePos, Ogre::Real dt, Ogre::Real xSpeed = 1.0f, Ogre::Real ySpeed = 0.1f);

	private:
		/// Render all the 2d data stored in the hardware buffers.
		void renderBuffer();
		/// Create a new hardware buffer
		/**
		/param size Vertex count for the new hardware buffer.
		*/
		void createHardwareBuffer(unsigned int size);
		/// Destroy the hardware buffer
		void destroyHardwareBuffer();
		/// Set Ogre for rendering
		void prepareForRender();
	private:
		Ogre::SceneManager* sceneManager;

		Ogre::uint8 targetQueue;
		bool afterQueue;

		// ogre specifics
		Ogre::v1::RenderOperation renderOp;
		Ogre::v1::HardwareVertexBufferSharedPtr hardwareBuffer;

		// sprite buffer
		std::list<Ogre2dSprite> sprites;

		std::vector<Ogre::TexturePtr> texturePtrList;
		Ogre::Real xScroll;
		Ogre::Real yScroll;
		Ogre::Real lastPositionX;
		Ogre::Vector2 lastPosition;
		bool firstTimePositionSet;
	};

}; //namespace end

#endif