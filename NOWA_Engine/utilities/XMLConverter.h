#ifndef XML_CONVERTER_H
#define XML_CONVERTER_H

#include "defines.h"
#include "rapidxml.hpp"
#include "OgreString.h"
#include "OgreIdString.h"
#include "OgreVector3.h"
#include "OgreQuaternion.h"
#include "OgreColourValue.h"
#include "OgrePrerequisites.h"
#include "OgreStringConverter.h"

namespace NOWA
{
	class EXPORTED XMLConverter
	{
	public:
		static Ogre::String getAttrib(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, const Ogre::String& defaultValue = "");

		static int getAttribInt(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, int defaultValue = 0);

		static unsigned int getAttribUnsignedInt(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, unsigned int defaultValue = 0);

		static unsigned long getAttribUnsignedLong(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, unsigned long defaultValue = 0);

		static Ogre::Real getAttribReal(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Real defaultValue = 0.0f);

		static Ogre::Radian getAttribRadian(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Radian defaultValue = Ogre::Radian(0.0f));

		static Ogre::Vector2 getAttribVector2(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Vector2 defaultValue = Ogre::Vector2::ZERO);

		static Ogre::Vector3 getAttribVector3(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Vector3 defaultValue = Ogre::Vector3::ZERO);

		static Ogre::Vector4 getAttribVector4(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Vector4 defaultValue = Ogre::Vector4::ZERO);

		static bool getAttribBool(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, bool defaultValue = false);

		static Ogre::Vector3 parseVector3(rapidxml::xml_node<>* xmlNode);

		static Ogre::Quaternion parseQuaternion(rapidxml::xml_node<>* xmlNode);

		static Ogre::ColourValue parseColour(rapidxml::xml_node<>* xmlNode);

		static void parseStringVector(Ogre::String& str, Ogre::StringVector& list);

		//!Note String must be duplicated, else there will be thrash when writing rapid XML
		static char* ConvertString(rapidxml::xml_document<>& doc, Ogre::Real value);

		static char* ConvertString(rapidxml::xml_document<>& doc, double value);

		static char* ConvertString(rapidxml::xml_document<>& doc, Ogre::Radian value);

		static char* ConvertString(rapidxml::xml_document<>& doc, Ogre::Degree value);

		static char* ConvertString(rapidxml::xml_document<>& doc, unsigned int value);

		static char* ConvertString(rapidxml::xml_document<>& doc, int value);

		static char* ConvertString(rapidxml::xml_document<>& doc, short value);

		/*static char* ConvertString(rapidxml::xml_document<>& doc, size_t value) */

		static char* ConvertString(rapidxml::xml_document<>& doc, long value);

		static char* ConvertString(rapidxml::xml_document<>& doc, unsigned long value);

		static char* ConvertString(rapidxml::xml_document<>& doc, bool value);

		static char* ConvertString(rapidxml::xml_document<>& doc, const Ogre::Vector2& value);

		static char* ConvertString(rapidxml::xml_document<>& doc, const Ogre::Vector3& value);

		static char* ConvertString(rapidxml::xml_document<>& doc, const Ogre::Vector4& value);

		static char* ConvertString(rapidxml::xml_document<>& doc, const Ogre::Matrix3& value);

		static char* ConvertString(rapidxml::xml_document<>& doc, const Ogre::Matrix4& value);

		static char* ConvertString(rapidxml::xml_document<>& doc, const Ogre::Quaternion& value);

		static char* ConvertString(rapidxml::xml_document<>& doc, const Ogre::ColourValue& value);

		static char* ConvertString(rapidxml::xml_document<>& doc, const Ogre::String& value);

		static char* ConvertString(rapidxml::xml_document<>& doc, const Ogre::IdString& value);
	};

}; // namespace end

#endif