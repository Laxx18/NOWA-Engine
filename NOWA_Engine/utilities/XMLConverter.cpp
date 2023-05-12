#include "NOWAPrecompiled.h"
#include "XMLConverter.h"

using namespace NOWA;

Ogre::String XMLConverter::getAttrib(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, const Ogre::String& defaultValue)
{
	if(xmlNode->first_attribute(attrib.c_str()))
		return xmlNode->first_attribute(attrib.c_str())->value();
	else
		return defaultValue;
}

int XMLConverter::getAttribInt(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, int defaultValue)
{
	if (xmlNode->first_attribute(attrib.c_str()))
		return Ogre::StringConverter::parseInt(xmlNode->first_attribute(attrib.c_str())->value());
	else
		return defaultValue;
}

unsigned int XMLConverter::getAttribUnsignedInt(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, unsigned int defaultValue)
{
	if (xmlNode->first_attribute(attrib.c_str()))
		return Ogre::StringConverter::parseUnsignedInt(xmlNode->first_attribute(attrib.c_str())->value());
	else
		return defaultValue;
}

unsigned long XMLConverter::getAttribUnsignedLong(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, unsigned long defaultValue)
{
	if (xmlNode->first_attribute(attrib.c_str()))
		return Ogre::StringConverter::parseUnsignedLong(xmlNode->first_attribute(attrib.c_str())->value());
	else
		return defaultValue;
}

Ogre::Real XMLConverter::getAttribReal(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Real defaultValue)
{
	if(xmlNode->first_attribute(attrib.c_str()))
		return Ogre::StringConverter::parseReal(xmlNode->first_attribute(attrib.c_str())->value());
	else
		return defaultValue;
}

Ogre::Radian XMLConverter::getAttribRadian(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Radian defaultValue)
{
	if(xmlNode->first_attribute(attrib.c_str()))
		return Ogre::StringConverter::parseAngle(xmlNode->first_attribute(attrib.c_str())->value());
	else
		return defaultValue;
}

Ogre::Vector2 XMLConverter::getAttribVector2(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Vector2 defaultValue)
{
	if(xmlNode->first_attribute(attrib.c_str()))
		return Ogre::StringConverter::parseVector2(xmlNode->first_attribute(attrib.c_str())->value());
	else
		return defaultValue;
}

Ogre::Vector3 XMLConverter::getAttribVector3(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Vector3 defaultValue)
{
	if(xmlNode->first_attribute(attrib.c_str()))
		return Ogre::StringConverter::parseVector3(xmlNode->first_attribute(attrib.c_str())->value());
	else
		return defaultValue;
}

Ogre::Vector4 XMLConverter::getAttribVector4(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, Ogre::Vector4 defaultValue)
{
	if (xmlNode->first_attribute(attrib.c_str()))
		return Ogre::StringConverter::parseVector4(xmlNode->first_attribute(attrib.c_str())->value());
	else
		return defaultValue;
}

bool XMLConverter::getAttribBool(rapidxml::xml_node<>* xmlNode, const Ogre::String& attrib, bool defaultValue)
{
	if(!xmlNode->first_attribute(attrib.c_str()))
		return defaultValue;

	if(Ogre::String(xmlNode->first_attribute(attrib.c_str())->value()) == "true")
		return true;

	return false;
}

Ogre::Vector3 XMLConverter::parseVector3(rapidxml::xml_node<>* xmlNode)
{
	return Ogre::Vector3(
		Ogre::StringConverter::parseReal(xmlNode->first_attribute("x")->value()),
		Ogre::StringConverter::parseReal(xmlNode->first_attribute("y")->value()),
		Ogre::StringConverter::parseReal(xmlNode->first_attribute("z")->value())
		);
}

Ogre::Quaternion XMLConverter::parseQuaternion(rapidxml::xml_node<>* xmlNode)
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

Ogre::ColourValue XMLConverter::parseColour(rapidxml::xml_node<>* xmlNode)
{
	return Ogre::ColourValue(
		Ogre::StringConverter::parseReal(xmlNode->first_attribute("r")->value()),
		Ogre::StringConverter::parseReal(xmlNode->first_attribute("g")->value()),
		Ogre::StringConverter::parseReal(xmlNode->first_attribute("b")->value()),
		xmlNode->first_attribute("a") != nullptr ? Ogre::StringConverter::parseReal(xmlNode->first_attribute("a")->value()) : 1
		);
}

void XMLConverter::parseStringVector(Ogre::String& str, Ogre::StringVector& list)
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
char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, Ogre::Real value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, double value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, Ogre::Radian value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, Ogre::Degree value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, unsigned int value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, int value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, short value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

/*char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, size_t value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}*/

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, long value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, unsigned long value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, bool value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, const Ogre::Vector2& value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, const Ogre::Vector3& value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, const Ogre::Vector4& value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, const Ogre::Matrix3& value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, const Ogre::Matrix4& value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, const Ogre::Quaternion& value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, const Ogre::ColourValue& value)
{
	Ogre::String d = Ogre::StringConverter::toString(value);
	return doc.allocate_string(d.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, const Ogre::String& value)
{
	return doc.allocate_string(value.data());
}

char* XMLConverter::ConvertString(rapidxml::xml_document<>& doc, const Ogre::IdString& value)
{
	return doc.allocate_string(value.getFriendlyText().data());
}