#ifndef XML_CONVERTER_H
#define XML_CONVERTER_H

#include "defines.h"
#include "rapidxml.hpp"
#include "OgreString.h"
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
		static Ogre::String getAttrib(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, const Ogre::String& defaultValue = "")
		{
			if(xmlNode->first_attribute(attrib.c_str()))
				return xmlNode->first_attribute(attrib.c_str())->value();
			else
				return defaultValue;
		}

		static int getAttribInt(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, int defaultValue = 0)
		{
			if (xmlNode->first_attribute(attrib.c_str()))
				return Ogre::StringConverter::parseInt(xmlNode->first_attribute(attrib.c_str())->value());
			else
				return defaultValue;
		}

		static unsigned int getAttribUnsignedInt(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, unsigned int defaultValue = 0)
		{
			if (xmlNode->first_attribute(attrib.c_str()))
				return Ogre::StringConverter::parseUnsignedInt(xmlNode->first_attribute(attrib.c_str())->value());
			else
				return defaultValue;
		}

		static unsigned long getAttribUnsignedLong(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, unsigned long defaultValue = 0)
		{
			if (xmlNode->first_attribute(attrib.c_str()))
				return Ogre::StringConverter::parseUnsignedLong(xmlNode->first_attribute(attrib.c_str())->value());
			else
				return defaultValue;
		}

		static Ogre::Real getAttribReal(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Real defaultValue = 0.0f)
		{
			if(xmlNode->first_attribute(attrib.c_str()))
				return Ogre::StringConverter::parseReal(xmlNode->first_attribute(attrib.c_str())->value());
			else
				return defaultValue;
		}

		static Ogre::Radian getAttribRadian(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Radian defaultValue = Ogre::Radian(0.0f))
		{
			if(xmlNode->first_attribute(attrib.c_str()))
				return Ogre::StringConverter::parseAngle(xmlNode->first_attribute(attrib.c_str())->value());
			else
				return defaultValue;
		}

		static Ogre::Vector2 getAttribVector2(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Vector2 defaultValue = Ogre::Vector2::ZERO)
		{
			if(xmlNode->first_attribute(attrib.c_str()))
				return Ogre::StringConverter::parseVector2(xmlNode->first_attribute(attrib.c_str())->value());
			else
				return defaultValue;
		}

		static Ogre::Vector3 getAttribVector3(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Vector3 defaultValue = Ogre::Vector3::ZERO)
		{
			if(xmlNode->first_attribute(attrib.c_str()))
				return Ogre::StringConverter::parseVector3(xmlNode->first_attribute(attrib.c_str())->value());
			else
				return defaultValue;
		}

		static Ogre::Vector4 getAttribVector4(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Vector4 defaultValue = Ogre::Vector4::ZERO)
		{
			if (xmlNode->first_attribute(attrib.c_str()))
				return Ogre::StringConverter::parseVector4(xmlNode->first_attribute(attrib.c_str())->value());
			else
				return defaultValue;
		}

		static bool getAttribBool(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, bool defaultValue = false)
		{
			if(!xmlNode->first_attribute(attrib.c_str()))
				return defaultValue;

			if(Ogre::String(xmlNode->first_attribute(attrib.c_str())->value()) == "true")
				return true;

			return false;
		}

		static Ogre::Vector3 parseVector3(rapidxml::xml_node<>* xmlNode)
		{
			return Ogre::Vector3(
				Ogre::StringConverter::parseReal(xmlNode->first_attribute("x")->value()),
				Ogre::StringConverter::parseReal(xmlNode->first_attribute("y")->value()),
				Ogre::StringConverter::parseReal(xmlNode->first_attribute("z")->value())
				);
		}

		static Ogre::Quaternion parseQuaternion(rapidxml::xml_node<>* xmlNode)
		{
			Ogre::Quaternion orientation;

			if(xmlNode->first_attribute("qx"))
			{
				orientation.x = Ogre::StringConverter::parseReal(xmlNode->first_attribute("qx")->value());
				orientation.y = Ogre::StringConverter::parseReal(xmlNode->first_attribute("qy")->value());
				orientation.z = Ogre::StringConverter::parseReal(xmlNode->first_attribute("qz")->value());
				orientation.w = Ogre::StringConverter::parseReal(xmlNode->first_attribute("qw")->value());
			}
			if(xmlNode->first_attribute("qw"))
			{
				orientation.w = Ogre::StringConverter::parseReal(xmlNode->first_attribute("qw")->value());
				orientation.x = Ogre::StringConverter::parseReal(xmlNode->first_attribute("qx")->value());
				orientation.y = Ogre::StringConverter::parseReal(xmlNode->first_attribute("qy")->value());
				orientation.z = Ogre::StringConverter::parseReal(xmlNode->first_attribute("qz")->value());
			}
			else if(xmlNode->first_attribute("axisX"))
			{
				Ogre::Vector3 axis;
				axis.x = Ogre::StringConverter::parseReal(xmlNode->first_attribute("axisX")->value());
				axis.y = Ogre::StringConverter::parseReal(xmlNode->first_attribute("axisY")->value());
				axis.z = Ogre::StringConverter::parseReal(xmlNode->first_attribute("axisZ")->value());
				Ogre::Real angle = Ogre::StringConverter::parseReal(xmlNode->first_attribute("angle")->value());;
				orientation.FromAngleAxis(Ogre::Angle(angle), axis);
			}
			else if(xmlNode->first_attribute("angleX"))
			{
				Ogre::Vector3 axis;
				axis.x = Ogre::StringConverter::parseReal(xmlNode->first_attribute("angleX")->value());
				axis.y = Ogre::StringConverter::parseReal(xmlNode->first_attribute("angleY")->value());
				axis.z = Ogre::StringConverter::parseReal(xmlNode->first_attribute("angleZ")->value());
				//orientation.FromAxes(&axis);
				//orientation.F
			}
			else if(xmlNode->first_attribute("x"))
			{
				orientation.x = Ogre::StringConverter::parseReal(xmlNode->first_attribute("x")->value());
				orientation.y = Ogre::StringConverter::parseReal(xmlNode->first_attribute("y")->value());
				orientation.z = Ogre::StringConverter::parseReal(xmlNode->first_attribute("z")->value());
				orientation.w = Ogre::StringConverter::parseReal(xmlNode->first_attribute("w")->value());
			}
			else if(xmlNode->first_attribute("w"))
			{
				orientation.w = Ogre::StringConverter::parseReal(xmlNode->first_attribute("w")->value());
				orientation.x = Ogre::StringConverter::parseReal(xmlNode->first_attribute("x")->value());
				orientation.y = Ogre::StringConverter::parseReal(xmlNode->first_attribute("y")->value());
				orientation.z = Ogre::StringConverter::parseReal(xmlNode->first_attribute("z")->value());
			}

			return orientation;
		}

		static Ogre::ColourValue parseColour(rapidxml::xml_node<>* xmlNode)
		{
			return Ogre::ColourValue(
				Ogre::StringConverter::parseReal(xmlNode->first_attribute("r")->value()),
				Ogre::StringConverter::parseReal(xmlNode->first_attribute("g")->value()),
				Ogre::StringConverter::parseReal(xmlNode->first_attribute("b")->value()),
				xmlNode->first_attribute("a") != nullptr ? Ogre::StringConverter::parseReal(xmlNode->first_attribute("a")->value()) : 1
				);
		}

		static void parseStringVector(Ogre::String& str, Ogre::StringVector& list)
		{
			list.clear();
			Ogre::StringUtil::trim(str, true, true);
			if (str == "")
				return;

			size_t pos = str.find(";");
			while (pos != Ogre::String::npos)
			{
				list.push_back(str.substr(0, pos));
				str.erase(0, pos + 1);
				pos = str.find(";");
			}

			if (str != "")
			{
				list.push_back(str);
			}
		}

		//!Note String must be duplicated, else there will be thrash when writing rapid XML
		static char* ConvertString(Ogre::Real value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(double value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(Ogre::Radian value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(Ogre::Degree value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(unsigned int value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(int value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(short value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		/*static char* ConvertString(size_t value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}*/

		static char* ConvertString(long value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(unsigned long value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(bool value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(const Ogre::Vector2& value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(const Ogre::Vector3& value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(const Ogre::Vector4& value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(const Ogre::Matrix3& value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(const Ogre::Matrix4& value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(const Ogre::Quaternion& value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(const Ogre::ColourValue& value)
		{
			char* strId = _strdup(Ogre::StringConverter::toString(value).c_str());
			return strId;
		}

		static char* ConvertString(const Ogre::String& value)
		{
			char* strId = _strdup(value.c_str());
			return strId;
		}

		static char* ConvertString(const Ogre::IdString& value)
		{
			char* strId = _strdup(value.getFriendlyText().c_str());
			return strId;
		}
	};

}; // namespace end

#endif