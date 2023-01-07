/*
 * ShadowcasterHandler.h
 *
 *  Created on: 07.07.2014
 *      Author: marvin
 */

#ifndef SHADOWCASTERHANDLER_H_
#define SHADOWCASTERHANDLER_H_

class Shadow_casterHandler : public Ogre::MaterialManager::Listener
{
public:
	Shadow_casterHandler();
	virtual ~Shadow_casterHandler();
    /** @copydoc MaterialManager::Listener::handleSchemeNotFound */
    virtual Ogre::Technique* handleSchemeNotFound(unsigned short schemeIndex, const Ogre::String& schemeName, Ogre::Material* originalMaterial, 
		unsigned short lodIndex, const Ogre::Renderable* rend);
};

#endif /* SHADOWCASTERHANDLER_H_ */
