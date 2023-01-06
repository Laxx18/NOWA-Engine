/*---------------------------------------------------------------------------*\
** This source file is part of OgreAL                                        **
** an OpenAL plugin for the Ogre Rendering Engine.                           **
**                                                                           **
** Copyright 2006 Casey Borders                                              **
**                                                                           **
** OgreAL is free software; you can redistribute it and/or modify it under   **
** the terms of the GNU Lesser General Public License as published by the    **
** Free Software Foundation; either version 2, or (at your option) any later **
** version.                                                                  **
**                                                                           **
** The developer really likes screenshots and while he recognises that the   **
** fact that this is an AUDIO plugin means that the fruits of his labor will **
** never been seen in these images he would like to kindly ask that you send **
** screenshots of your application using his library to                      **
** screenshots@mooproductions.org                                            **
**                                                                           **
** Please bear in mind that the sending of these screenshots means that you  **
** are agreeing to allow the developer to display them in the media of his   **
** choice.  They will, however, be fully credited to the person sending the  **
** email or, if you wish them to be credited differently, please state that  **
** in the body of the email.                                                 **
**                                                                           **
** OgreAL is distributed in the hope that it will be useful, but WITHOUT     **
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     **
** FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for **
** more details.                                                             **
**                                                                           **
** You should have received a copy of the GNU General Public License along   **
** with OgreAL; see the file LICENSE.  If not, write to the                  **
** Free Software Foundation, Inc.,                                           **
** 59 Temple Place - Suite 330,                                              **
** Boston, MA 02111-1307, USA.                                               **
\*---------------------------------------------------------------------------*/

#ifndef _OGREAL_LISTENER_H_
#define _OGREAL_LISTENER_H_

#include "OgreALPrereqs.h"

namespace OgreAL {
	/**
	 * Listener.
	 * @remarks
	 * There is only ever one listener in the scene and it is created
	 * when the SoundManager is initialized.  To get the reference to
	 * the listener use SoundManager::getListener.  The listener can 
	 * be attached to an Ogre::SceneNode or placed at some point in space.
	 *
	 * @see Ogre::MovableObject
	 * @see Ogre::Singleton
	 */
	class OgreAL_Export Listener : public Ogre::MovableObject, public Ogre::Singleton<Listener>
	{
	protected:
		/*
		** Constructors are protected to enforce the use of the 
		** factory via SoundManager::createListener
		*/
		/** Default Constructor. */
		Listener(Ogre::IdType id, Ogre::ObjectMemoryManager *objectMemoryManager, Ogre::SceneManager *manager);
		/** Normal Constructor. Should not be called directly! Use SoundManager::getListener */
		Listener(const Ogre::String& name, Ogre::IdType id, Ogre::ObjectMemoryManager *objectMemoryManager, Ogre::SceneManager *manager);

	public:
		/** Standard Destructor */
		virtual ~Listener();

		/** Returns the Listener singleton object */
		static Listener& getSingleton();
		/** Returns a pointer to the Listener singleton object */
		static Listener* getSingletonPtr();

		/** 
		 * Sets the gain. 
		 * @param gain The gain where 1.0 is full volume and 0.0 is off
		 * @note Gain should be positive
		 */
		void setGain(Ogre::Real gain);
		/** Returns the gain. */
		Ogre::Real getGain() const {return mGain;}
		/**
		 * Sets the position of the listener.
		 * @param x The x part of the position
		 * @param y The y part of the position
		 * @param z The z part of the position
		 * @note The position will be overridden if the listener is attached to a SceneNode
		 */
		void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z);
		/**
		 * Sets the position of the listener.
		 * @param vec The new postion for the listener.
		 * @note The position will be overridden if the listener is attached to a SceneNode
		 */
		void setPosition(const Ogre::Vector3& vec);
		/** Returns the position of the sound. */
		const Ogre::Vector3& getPosition() const {return mPosition;}
		/**
		 * Sets the direction of the listener.
		 * @param x The x part of the direction vector
		 * @param y The y part of the direction vector
		 * @param z The z part of the direction vector
		 * @note The direction will be overridden if the listener is attached to a SceneNode
		 */
		void setDirection(Ogre::Real x, Ogre::Real y, Ogre::Real z);
		/**
		 * Sets the direction of the listener.
		 * @param vec The direction vector.
		 * @note The direction will be overridden if the listener is attached to a SceneNode
		 */
		void setDirection(const Ogre::Vector3& vec);
		/** Returns the direction of the sound. */
		const Ogre::Vector3& getDirection() const {return mDirection;}
		/**
		 * Sets the velocity of the listener.
		 * @param x The x part of the velocity vector
		 * @param y The y part of the velocity vector
		 * @param z The z part of the velocity vector
		 */
		void setVelocity(Ogre::Real x, Ogre::Real y, Ogre::Real z);
		/**
		 * Sets the velocity of the listener.
		 * @param vec The velocity vector.
		 */
		void setVelocity(const Ogre::Vector3& vec);
		/** Returns the velocity of the sound. */
		const Ogre::Vector3& getVelocity() const {return mVelocity;}
		/** Returns the position of the sound including any transform from nodes it is attached to. */
		const Ogre::Vector3& getDerivedPosition() const;
		/** Returns the direction of the sound including any transform from nodes it is attached to. */
		const Ogre::Vector3& getDerivedDirection() const;

		/** Overridden from MovableObject */
		const Ogre::String& getMovableType() const;
		/** Overridden from MovableObject */
		const Ogre::AxisAlignedBox& getBoundingBox() const;
		/** Overridden from MovableObject */
		Ogre::Real getBoundingRadius() const {return 0; /* Not Visible */} 
		/** Overridden from MovableObject */
		void _updateRenderQueue(Ogre::RenderQueue* queue);
		/** Overridden from MovableObject */
		void _notifyAttached(Ogre::Node* parent, bool isTagPoint = false);
	#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR >5
		/** Overridden from MovableObject */
		virtual void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables = false){}
	#endif

	protected:
		/// Internal method for synchronising with parent node (if any)
		virtual void update() const;
		/// This is called each frame to update the position, direction, etc
		void updateListener();
		/// Convienance method to reset the sound state
		void initListener();

		/// Postion taking into account the parent node
		mutable Ogre::Vector3 mDerivedPosition;
		/// Direction taking into account the parent node
		mutable Ogre::Vector3 mDerivedDirection;
		/// Stored versions of parent orientation
		mutable Ogre::Quaternion mLastParentOrientation;
		/// Stored versions of parent position
		mutable Ogre::Vector3 mLastParentPosition;

		Ogre::Real mGain;
		Ogre::Vector3 mPosition;
		Ogre::Vector3 mDirection;
		Ogre::Vector3 mVelocity;
		Ogre::Vector3 mUp;
		ALfloat mOrientation[6];
		/// Is the local transform dirty?
		mutable bool mLocalTransformDirty;


		friend class SoundManager;
		friend class ListenerFactory;
	};

	/** Factory object for creating the listener */
	class OgreAL_Export ListenerFactory : public Ogre::MovableObjectFactory
	{
	public:
		ListenerFactory() {}
		~ListenerFactory() {}

		static Ogre::String FACTORY_TYPE_NAME;

		const Ogre::String& getType() const;
		void destroyInstance(Ogre::MovableObject* obj);

	protected:
		Ogre::MovableObject* createInstanceImpl( 
            Ogre::IdType id, Ogre::ObjectMemoryManager *objectMemoryManager, Ogre::SceneManager *manager,
			const Ogre::NameValuePairList* params = 0);
	};
} // Namespace
#endif
