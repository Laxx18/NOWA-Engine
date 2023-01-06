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

#include "OgreALException.h"
#include "OgreException.h"
#include "OgreLogManager.h"
#include <sstream>

namespace OgreAL {
	void check(bool condition, const int errorNumber, const Ogre::String& description, const Ogre::String& source)
	{
		if (!condition)
		{
			Ogre::Exception exception(errorNumber, description, source);
			Ogre::LogManager::getSingleton().logMessage(exception.getFullDescription());
			throw exception;
		}
	}

	void check(const Error error, const Ogre::String& description, const Ogre::String& source)
	{
		if (error != AL_NO_ERROR)
		{
			std::stringstream ss;
			ss << description << ": OpenAL Error: " << getErrorDescription(error);
			Ogre::Exception exception(error, ss.str(), source);
			Ogre::LogManager::getSingleton().logMessage(exception.getFullDescription());
			// Deactivated because, the listener will not hear, that buffers could not be upated and throwing here, will interrupt the whole application!
			// throw exception;
		}
	}

	Ogre::String getErrorDescription(const Error error)
	{
		switch(error)
		{
		case AL_INVALID_VALUE:
			return Ogre::String("The value pointer given is not valid");
			break;
		case AL_INVALID_ENUM:
			return Ogre::String("The specified parameter is not valid");
			break;
		case AL_INVALID_NAME:
			return Ogre::String("The specified source name is not valid");
			break;
		case AL_INVALID_OPERATION:
			return Ogre::String("There is no current context");
			break;
		case AL_OUT_OF_MEMORY:
			return Ogre::String("The requested operation resulted in OpenAL running out of memory");
			break;
/*		case OV_EREAD:
			return Ogre::String("Read from media.");
			break;
		case OV_ENOTVORBIS:
			return Ogre::String("Not Vorbis data.");
			break;
		case OV_EVERSION:
			return Ogre::String("Vorbis version mismatch.");
			break;
		case OV_EBADHEADER:
			return Ogre::String("Invalid Vorbis header.");
			break;
		case OV_EFAULT:
			return Ogre::String("Internal logic fault (bug or heap/stack corruption.");
			break;
*/		default:
			return Ogre::String("Unknown Error");
			break;
		}
	}
} // Namespace
