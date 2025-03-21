#ifndef SHADER_GENERATOR_TECHNIQUE_RESOLVER_LISTENER_H_
#define SHADER_GENERATOR_TECHNIQUE_RESOLVER_LISTENER_H_

#include "defines.h"
#include <Ogre.h>
#include <OgreRTShaderSystem.h>

namespace OBA
{

/** This class demonstrates basic usage of the RTShader system.
It sub class the material manager listener class and when a target scheme callback
is invoked with the shader generator scheme it tries to create an equivalent shader
based technique based on the default technique of the given material.
*/

class EXPORTED ShaderGeneratorTechniqueResolverListener : public Ogre::MaterialManager::Listener
{
public:
	ShaderGeneratorTechniqueResolverListener(Ogre::RTShader::ShaderGenerator *pShaderGenerator);
	~ShaderGeneratorTechniqueResolverListener();
	/** This is the hook point where shader based technique will be created.
	It will be called whenever the material manager won't find appropriate technique
	that satisfy the target scheme name. If the scheme name is out target RT Shader System
	scheme name we will try to create shader generated technique for it. 
	*/
	virtual Ogre::Technique* handleSchemeNotFound(unsigned short schemeIndex, 
		const Ogre::String& schemeName, Ogre::Material* originalMaterial, unsigned short lodIndex, 
		const Ogre::Renderable* rend);
protected:
	Ogre::RTShader::ShaderGenerator *pShaderGenerator;
};

}; //Ende Namensraum

#endif /* SHADERGENERATORTECHNIQUERESOLVERLISTENER_H_ */
