#include "NOWAPrecompiled.h"

#include "LuaScriptApi.h"

#include "NOWA.h"

#include <luabind/iterator_policy.hpp>
#include <luabind/out_value_policy.hpp>
#include <luabind/raw_policy.hpp>
#include <luabind/return_reference_to_policy.hpp>
#include <luabind/copy_policy.hpp>

// see http://www.rasterbar.com/products/luabind/docs.html

// Note: Determine which functions a designer should use and restrict the ones that are to heavy

// Prototype this before operator.hpp so it can be found for tostring() operator.
// std::ostream& operator<<( std::ostream& stream, const Ogre::v1::Entity ent );

#include "luabind/operator.hpp"

// #undef _FUNCTIONAL_
#include <boost/bind.hpp>
#include <boost/bind/placeholders.hpp>

#include <algorithm>

using namespace luabind;
using namespace Ogre;

// Some helpful macros for defining constants (sort of) in Lua.  Similar to this code:
// object g = globals(L);
// object table = g["class"];
// table["constant"] = class::constant;
// lua comes from the functions below and the macro is able to use it here!
#define LUA_CONST_START( class ) { object g = globals(lua); object table = g[#class];
#define LUA_CONST( class, name ) table[#name] = class::name
#define LUA_CONST_END }

// http://www.gamedev.net/topic/525692-luabind-ownership-and-destruction/

//#define LuaClassNonDeletable(in_class)
//namespace luabind
//{
//	namespace detail
//	{
//		template<> struct delete_s<in_class>
//		{
//			static void apply(void* ptr)
//			{
//			}
//		};
//		template<> struct destruct_only_s<in_class>
//		{
//			static void apply(void* ptr)
//			{
//			}
//		};
//	}
//}
//
//LuaClassNonDeletable(GameObject)
//LuaClassNonDeletable(GameObjectComponent)
//LuaClassNonDeletable(PhysicsComponent)

// ***POLICY SOURCE***
// Here are the contents of table_policy.hpp:

#include <luabind/config.hpp>

// #include <luabind/detail/call.hpp>
// #include <boost/preprocessor/iteration/detail/local.hpp>

// #define BOOST_PP_LOCAL_LIMITS (1, 2)

#include <luabind/detail/policy.hpp>

namespace
{

#define	AddClassToCollection(className, function, description) \
		LuaScriptApi::getInstance()->addClassToCollection(className, function, description)

#define	AddFunctionToCollection(function, description) \
		LuaScriptApi::getInstance()->addFunctionToCollection(function, description)


	std::vector<Ogre::String> split(const Ogre::String& content, char delimiter)
	{
		std::vector<std::string> splits;
		std::string split;
		std::istringstream ss(content);
		while (std::getline(ss, split, delimiter))
		{
			splits.push_back(split);
		}
		return splits;
	}

	Ogre::String replaceAll(Ogre::String str, const Ogre::String& from, const Ogre::String& to)
	{
		size_t startPos = 0;
		while ((startPos = str.find(from, startPos)) != Ogre::String::npos)
		{
			str.replace(startPos, from.length(), to);
			startPos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return str;
	}

	Ogre::String currentErrorGameObject;
	Ogre::String currentCalledFunction;

	std::set<unsigned int> errorMessages;
	std::set<Ogre::String> errorMessages2;
}


namespace NOWA
{
	// This types are to primitve and lua binding would cause a crash when closing lua!
	/*void bindOgreReal(lua_State* lua)
	{
		module(lua)
		[
			class_<Ogre::Real>("Real")
		];
	}*/

	void log(const Ogre::String& message)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LuaScriptApi]: " + message);
	}

	Ogre::String toString(const Ogre::Vector2& vec)
	{
		return Ogre::StringConverter::toString(vec);
	}

	Ogre::String toString(const Ogre::Vector3& vec)
	{
		return Ogre::StringConverter::toString(vec);
	}

	Ogre::String toString(const Ogre::Vector4& vec)
	{
		return Ogre::StringConverter::toString(vec);
	}

	Ogre::String toString(const Ogre::ColourValue& color)
	{
		return Ogre::StringConverter::toString(color);
	}

	Ogre::String toString(const Ogre::Quaternion& quat)
	{
		return Ogre::StringConverter::toString(quat);
	}

	Ogre::String toString(Ogre::Real val)
	{
		return Ogre::StringConverter::toString(val);
	}

	bool matches(const Ogre::String& name, const Ogre::String& pattern)
	{
		return Ogre::StringUtil::match(name, pattern, false);
	}

	void bindString(lua_State* lua)
	{
		module(lua)
			[
				class_<String>("String")
			];

		luabind::module(lua)
			[
				luabind::def("log", &log)
			];

		luabind::module(lua)
			[
				luabind::def("toString", (Ogre::String(*)(const Vector2&)) & toString)
			];

		luabind::module(lua)
			[
				luabind::def("toString", (Ogre::String(*)(const Vector3&)) & toString)
			];

		luabind::module(lua)
			[
				luabind::def("toString", (Ogre::String(*)(const Vector4&)) & toString)
			];

		luabind::module(lua)
			[
				luabind::def("toString", (Ogre::String(*)(const Quaternion&)) & toString)
			];

		luabind::module(lua)
			[
				luabind::def("toString", (Ogre::String(*)(Real)) & toString)
			];

		luabind::module(lua)
			[
				luabind::def("toString", (Ogre::String(*)(const ColourValue&)) & toString)
			];

		luabind::module(lua)
			[
				luabind::def("matches", &matches)
			];

		// Causes for lua json schmaddel
		// AddClassToCollection("function", "toString([Vector2, Vector3, Vector4, Quaternion, number, ColorValue])", "Converts any type to string.");
	}

	void bindMatrix3(lua_State* lua)
	{
		module(lua)
			[
				class_<Matrix3>("Matrix3")
			];
		AddClassToCollection("Matrix3", "class", "Matrix3 class.");
	}

	void bindRadian(lua_State* lua)
	{
		module(lua)
			[
				class_<Radian>("Radian")
				.def(constructor<>())
			.def(constructor<const Radian&>())
			.def(constructor<Real>())
			.def(constructor<const Degree&>())
			// Operators
			.def(self + other<Degree>())
			.def(self + other<Radian>())
			.def(self - other<Degree>())
			.def(self - other<Radian>())
			.def(self * other<Degree>())
			.def(self * other<Real>())
			.def(self / other<Real>())
			.def(tostring(self))
			// Methods
			.def("valueDegrees", &Radian::valueDegrees)
			.def("valueRadians", &Radian::valueRadians)
			.def("valueAngleUnits", &Radian::valueAngleUnits)
			];
		AddClassToCollection("Radian", "class", "Radian class.");
		AddClassToCollection("Radian", "float valueDegrees()", "Gets the degrees as float from radian.");
		AddClassToCollection("Radian", "float valueRadians()", "Gets the radians as float from radian.");
		AddClassToCollection("Radian", "float valueAngleUnits()", "Gets the angle units as float from radian.");
	}

	void bindDegree(lua_State* lua)
	{
		module(lua)
			[
				class_<Degree>("Degree")
				.def(constructor<>())
			.def(constructor<const Degree&>())
			.def(constructor<Real>())
			.def(constructor<const Radian&>())
			// Operators
			.def(self + other<Degree>())
			.def(self + other<Radian>())
			.def(self - other<Degree>())
			.def(self - other<Radian>())
			.def(self * other<Degree>())
			.def(self * other<Real>())
			.def(self / other<Real>())
			.def(tostring(self))
			// Methods
			.def("valueDegrees", &Degree::valueDegrees)
			.def("valueRadians", &Degree::valueRadians)
			.def("valueAngleUnits", &Degree::valueAngleUnits)
			];
		AddClassToCollection("Degree", "class", "Degree class.");
		AddClassToCollection("Degree", "float valueDegrees()", "Gets the degrees as float from degree.");
		AddClassToCollection("Degree", "float valueRadians()", "Gets the radians as float from degree.");
		AddClassToCollection("Degree", "float valueAngleUnits()", "Gets the angle units as float from degree.");
	}

	void bindVector2(lua_State* lua)
	{
		module(lua)
			[
				class_<Vector2>("Vector2")
				.def_readwrite("x", &Vector2::x)
			.def_readwrite("y", &Vector2::y)
			.def(constructor<>())
			.def(constructor<const Vector2&>())
			.def(constructor<Real, Real>())
			// Operators
			.def(self + other<Vector2>())
			.def(self - other<Vector2>())
			.def(self * other<Vector2>())
			.def(self * other<Real>())
			.def(self / other<Vector2>())
			.def(self / other<Real>())
			.def(-self)
			.def(self == other<Vector2>())
			.def(self < other<Vector2>())
			.def(tostring(self))
			// Methods
			.def("crossProduct", &Vector2::crossProduct)
			.def("distance", &Vector2::distance)
			.def("dotProduct", &Vector2::dotProduct)
			.def("isZeroLength", &Vector2::isZeroLength)
			.def("length", &Vector2::length)
			.def("makeCeil", &Vector2::makeCeil)
			.def("makeFloor", &Vector2::makeFloor)
			.def("midPoint", &Vector2::midPoint)
			.def("normalise", &Vector2::normalise)
			.def("normalisedCopy", &Vector2::normalisedCopy)
			.def("perpendicular", &Vector2::perpendicular)
			.def("randomDeviant", &Vector2::randomDeviant)
			.def("reflect", &Vector2::reflect)
			.def("squaredDistance", &Vector2::squaredDistance)
			.def("squaredLength", &Vector2::squaredLength)
			.def("isNaN", &Vector2::isNaN)
			.def("angleBetween", &Vector2::angleBetween)
			.def("angleTo", &Vector2::angleTo)
			];

		AddClassToCollection("Vector2", "class", "Vector2 class.");
		AddClassToCollection("Vector2", "Vector2(float x, float y)", "Constructor.");
		AddClassToCollection("Vector2", "x", "type");
		AddClassToCollection("Vector2", "y", "type");
		AddClassToCollection("Vector2", "Vector2 crossProduct(Vector2 other)", "Gets the cross product between two vectors.");
		AddClassToCollection("Vector2", "float distance(Vector2 other)", "Gets the distance between two vectors.");
		AddClassToCollection("Vector2", "float dotProduct(Vector2 other)", "Gets the dot product between two vectors.");
		AddClassToCollection("Vector2", "bool isZeroLength(Vector2 other)", "Checks whether the length to the other vector is zero.");
		AddClassToCollection("Vector2", "float length()", "Gets the length between of a vector.");
		AddClassToCollection("Vector2", "Vector2 makeCeil()", "Rounds up or down the vector.");
		AddClassToCollection("Vector2", "Vector2 makeFloor()", "Rounds up the vector.");
		AddClassToCollection("Vector2", "Vector2 midPoint(Vector2 other)", "Gets the mid point between two vectors.");
		AddClassToCollection("Vector2", "Vector2 normalise()", "Normalizes the vector.");
		AddClassToCollection("Vector2", "Vector2 normalisedCopy()", "Normalizes the vector and return as copy.");
		AddClassToCollection("Vector2", "bool perpendicular()", "Generates a vector perpendicular to this vector (eg an 'up' vector).");
		AddClassToCollection("Vector2", "Vector2 randomDeviant(Radian angle)", "Generates a new random vector which deviates from this vector by a given angle in a random direction.");
		AddClassToCollection("Vector2", "Vector2 reflect(Vector2 normal)", "Calculates a reflection vector to the plane with the given normal.");
		AddClassToCollection("Vector2", "float squaredDistance(Vector2 other)", "Returns the square of the distance to another vector.");
		AddClassToCollection("Vector2", "float squaredLength()", "Returns the square of the length(magnitude) of the vector.");
		AddClassToCollection("Vector2", "bool isNaN()", "Check whether this vector contains valid values.");
		AddClassToCollection("Vector2", "Radian angleBetween(Vector2 other)", "Gets the angle between 2 vectors.");
		AddClassToCollection("Vector2", "Radian angleTo(Vector2 other)", "Gets the oriented angle between 2 vectors. Note: Vectors do not have to be unit-length but must represent directions. The angle is comprised between 0 and 2 PI.");

		LUA_CONST_START(Vector2)
			LUA_CONST(Vector2, ZERO);
		LUA_CONST(Vector2, UNIT_X);
		LUA_CONST(Vector2, UNIT_Y);
		LUA_CONST(Vector2, NEGATIVE_UNIT_X);
		LUA_CONST(Vector2, NEGATIVE_UNIT_Y);
		LUA_CONST(Vector2, UNIT_SCALE);
		LUA_CONST_END;

		AddClassToCollection("Vector2", "ZERO", "Zero-Vector.");
		AddClassToCollection("Vector2", "UNIT_X", "Vector2(1, 0)");
		AddClassToCollection("Vector2", "UNIT_Y", "Vector2(0, 1)");
		AddClassToCollection("Vector2", "NEGATIVE_UNIT_X", "Vector2(-1, 0)");
		AddClassToCollection("Vector2", "NEGATIVE_UNIT_Y", "Vector2(0, -1)");
		AddClassToCollection("Vector2", "UNIT_SCALE", "Vector2(1, 1)");
	}

	void bindVector3(lua_State* lua)
	{
		module(lua)
			[
				class_<Vector3>("Vector3")
				.def_readwrite("x", &Vector3::x)
			.def_readwrite("y", &Vector3::y)
			.def_readwrite("z", &Vector3::z)
			.def(constructor<>())
			.def(constructor<const Vector3&>())
			.def(constructor<Real, Real, Real>())
			// Operators
			.def(self + other<Vector3>())
			.def(self - other<Vector3>())
			.def(self * other<Vector3>())
			.def(self * other<Real>())
			.def(self / other<Vector3>())
			.def(self / other<Real>())
			.def(-self)
			.def(self == other<Vector3>())
			.def(self < other<Vector3>())
			.def(tostring(self))
			// Methods
			.def("crossProduct", &Vector3::crossProduct)
			.def("distance", &Vector3::distance)
			.def("dotProduct", &Vector3::dotProduct)
			.def("isZeroLength", &Vector3::isZeroLength)
			.def("length", &Vector3::length)
			.def("makeCeil", &Vector3::makeCeil)
			.def("makeFloor", &Vector3::makeFloor)
			.def("midPoint", &Vector3::midPoint)
			.def("normalise", &Vector3::normalise)
			.def("normalisedCopy", &Vector3::normalisedCopy)
			.def("perpendicular", &Vector3::perpendicular)
			.def("randomDeviant", &Vector3::randomDeviant)
			.def("reflect", &Vector3::reflect)
			.def("squaredDistance", &Vector3::squaredDistance)
			.def("squaredLength", &Vector3::squaredLength)
			.def("isNaN", &Vector3::isNaN)
			.def("angleBetween", &Vector3::angleBetween)
			.def("getRotationTo", &Vector3::getRotationTo)
			.def("positionCloses", &Vector3::positionCloses)
			.def("positionEquals", &Vector3::positionEquals)
			.def("absDotProduct", &Vector3::absDotProduct)
			.def("makeAbs", &Vector3::makeAbs)
			.def("directionEquals", &Vector3::directionEquals)
			.def("primaryAxis", &Vector3::primaryAxis)
			];

		AddClassToCollection("Vector3", "class", "Vector3 class.");
		AddClassToCollection("Vector3", "Vector3(float x, float y, float z)", "Constructor.");
		AddClassToCollection("Vector3", "x", "type");
		AddClassToCollection("Vector3", "y", "type");
		AddClassToCollection("Vector3", "z", "type");
		AddClassToCollection("Vector3", "Vector3 crossProduct(Vector3 other)", "Gets the cross product between two vectors.");
		AddClassToCollection("Vector3", "float distance(Vector3 other)", "Gets the distance between two vectors.");
		AddClassToCollection("Vector3", "float dotProduct(Vector3 other)", "Gets the dot product between two vectors.");
		AddClassToCollection("Vector3", "bool isZeroLength(Vector3 other)", "Checks whether the length to the other vector is zero.");
		AddClassToCollection("Vector3", "float length()", "Gets the length between of a vector.");
		AddClassToCollection("Vector3", "Vector3 makeCeil()", "Rounds up or down the vector.");
		AddClassToCollection("Vector3", "Vector3 makeFloor()", "Rounds up the vector.");
		AddClassToCollection("Vector3", "Vector3 midPoint(Vector3 other)", "Gets the mid point between two vectors.");
		AddClassToCollection("Vector3", "Vector3 normalise()", "Normalizes the vector.");
		AddClassToCollection("Vector3", "Vector3 normalisedCopy()", "Normalizes the vector and return as copy.");
		AddClassToCollection("Vector3", "bool perpendicular()", "Generates a vector perpendicular to this vector (eg an 'up' vector).");
		AddClassToCollection("Vector3", "Vector3 randomDeviant(Radian angle)", "Generates a new random vector which deviates from this vector by a given angle in a random direction.");
		AddClassToCollection("Vector3", "Vector3 reflect(Vector3 normal)", "Calculates a reflection vector to the plane with the given normal.");
		AddClassToCollection("Vector3", "float squaredDistance(Vector3 other)", "Returns the square of the distance to another vector.");
		AddClassToCollection("Vector3", "float squaredLength()", "Returns the square of the length(magnitude) of the vector.");
		AddClassToCollection("Vector3", "bool isNaN()", "Check whether this vector contains valid values.");
		AddClassToCollection("Vector3", "Radian angleBetween(Vector3 other)", "Gets the angle between 2 vectors.");
		AddClassToCollection("Vector3", "Radian getRotationTo(Vector3 other, Vector3 fallbackAxis)", "Gets the shortest arc quaternion to rotate this vector to the destination vector."
			"Note: If you call this with a dest vector that is close to the inverse of this vector, we will rotate 180 degrees around the 'fallbackAxis' "
			"(if specified, default is ZERO, or a generated axis if not) since in this case ANY axis of rotation is valid.");
		AddClassToCollection("Vector3", "bool positionCloses(Vector3 other, float tolerance)", "Returns whether this vector is within a positional tolerance of another vector, also take scale of the vectors into account. Note: Default tolerance is 1e-03f");
		AddClassToCollection("Vector3", "bool positionEquals(Vector3 other, float tolerance)", "Returns whether this vector is within a positional tolerance of another vector. Note: Default tolerance is 1e-03f");
		AddClassToCollection("Vector3", "float absDotProduct(Vector3 other)", "Calculates the absolute dot (scalar) product of this vector with another.");
		AddClassToCollection("Vector3", "void makeAbs()", "Causes negative members to become positive.");
		AddClassToCollection("Vector3", "bool directionEquals(Vector3 other, const Radian tolerance)", "Returns whether this vector is within a directional tolerance of another vector. Note: Both vectors should be normalised.");
		AddClassToCollection("Vector3", "Vector3 primaryAxis()", "Extract the primary (dominant) axis from this direction vector.");

		LUA_CONST_START(Vector3)
			LUA_CONST(Vector3, ZERO);
		LUA_CONST(Vector3, UNIT_X);
		LUA_CONST(Vector3, UNIT_Y);
		LUA_CONST(Vector3, UNIT_Z);
		LUA_CONST(Vector3, NEGATIVE_UNIT_X);
		LUA_CONST(Vector3, NEGATIVE_UNIT_Y);
		LUA_CONST(Vector3, NEGATIVE_UNIT_Z);
		LUA_CONST(Vector3, UNIT_SCALE);
		LUA_CONST_END;

		AddClassToCollection("Vector3", "ZERO", "Zero-Vector.");
		AddClassToCollection("Vector3", "UNIT_X", "Vector3(1, 0, 0)");
		AddClassToCollection("Vector3", "UNIT_Y", "Vector3(0, 1, 0)");
		AddClassToCollection("Vector3", "UNIT_Z", "Vector3(0, 0, 1)");
		AddClassToCollection("Vector3", "NEGATIVE_UNIT_X", "Vector3(-1, 0, 0)");
		AddClassToCollection("Vector3", "NEGATIVE_UNIT_Y", "Vector3(0, -1, 0)");
		AddClassToCollection("Vector3", "NEGATIVE_UNIT_Z", "Vector3(0, 0, -1)");
		AddClassToCollection("Vector3", "UNIT_SCALE", "Vector3(1, 1, 1)");
	}

	void bindVector4(lua_State* lua)
	{
		module(lua)
			[
				class_<Vector4>("Vector4")
				.def_readwrite("x", &Vector4::x)
			.def_readwrite("y", &Vector4::y)
			.def_readwrite("z", &Vector4::z)
			.def_readwrite("z", &Vector4::w)
			.def(constructor<>())
			.def(constructor<const Vector4&>())
			.def(constructor<Real, Real, Real, Real>())
			// Operators
			.def(self + other<Vector4>())
			.def(self - other<Vector4>())
			.def(self * other<Vector4>())
			.def(self * other<Real>())
			.def(self / other<Vector4>())
			.def(self / other<Real>())
			.def(-self)
			.def(self == other<Vector4>())
			// .def(self < other<Vector4>())
			.def(tostring(self))
			// Methods
			.def("dotProduct", &Vector4::dotProduct)
			.def("isNaN", &Vector4::isNaN)
			];

		AddClassToCollection("Vector4", "class", "Vector4 class.");
		AddClassToCollection("Vector4", "Vector4(float x, float y, float z, float w)", "Constructor.");
		AddClassToCollection("Vector4", "x", "type");
		AddClassToCollection("Vector4", "y", "type");
		AddClassToCollection("Vector4", "z", "type");
		AddClassToCollection("Vector4", "w", "type");
		AddClassToCollection("Vector4", "float dotProduct(Vector4 other)", "Gets the dot product between two vectors.");
		AddClassToCollection("Vector4", "bool isNaN()", "Check whether this vector contains valid values.");

		LUA_CONST_START(Vector4)
			LUA_CONST(Vector4, ZERO);
		LUA_CONST_END;

		AddClassToCollection("Vector4", "ZERO", "Zero-Vector.");
	}

	Matrix3 toRotationMatrix(Ogre::Quaternion* instance)
	{
		Matrix3 matrix;
		instance->ToRotationMatrix(matrix);
		return matrix;
	}

	luabind::object toAngleAxisFromRadian(Ogre::Quaternion* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		Ogre::Radian rad;
		Ogre::Vector3 axis;

		instance->ToAngleAxis(rad, axis);
		unsigned int i = 0;
		obj[i++] = rad;
		obj[i++] = axis;
		return obj;
	}

	luabind::object toAngleAxisFromDegree(Ogre::Quaternion* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		Ogre::Degree deg;
		Ogre::Vector3 axis;

		instance->ToAngleAxis(deg, axis);
		unsigned int i = 0;
		obj[i++] = deg;
		obj[i++] = axis;
		return obj;
	}

	luabind::object intermediate(Ogre::Quaternion* instance, const Quaternion& rkQ0, const Quaternion& rkQ1, const Quaternion& rkQ2)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		Quaternion rka;
		Quaternion rkB;

		instance->Intermediate(rkQ0, rkQ1, rkQ2, rka, rkB);
		unsigned int i = 0;
		obj[i++] = rka;
		obj[i++] = rkB;
		return obj;
	}

	void bindQuaternion(lua_State* lua)
	{
		module(lua)
			[
				class_<Ogre::Quaternion>("Quaternion")
				.def_readwrite("x", &Quaternion::x)
			.def_readwrite("y", &Quaternion::y)
			.def_readwrite("z", &Quaternion::z)
			.def_readwrite("w", &Quaternion::w)
			.def(constructor<Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real>())
			.def(constructor<const Quaternion&>())
			.def(constructor<Matrix3>())
			.def(constructor<Ogre::Radian, Ogre::Vector3>())
			.def(constructor<Ogre::Degree, Ogre::Vector3>())
			.def(constructor<Ogre::Vector3, Ogre::Vector3, Ogre::Vector3>())
			// operators
			.def(self + other<Ogre::Quaternion>())
			.def(self - other<Ogre::Quaternion>())
			.def(self * other<Ogre::Quaternion>())
			.def(self * other<Ogre::Vector3>())
			.def(self * other<Ogre::Real>())
			.def(self == other<Ogre::Quaternion>())
			.def(-self)
			.def(tostring(self))
			// Methods
			.def("fromRotationMatrix", &Quaternion::FromRotationMatrix)
			.def("toRotationMatrix", &toRotationMatrix)
			.def("toAngleAxisFromRadian", toAngleAxisFromRadian)
			.def("toAngleAxisFromDegree", toAngleAxisFromDegree)
			.def("fromAngleAxis", &Quaternion::FromAngleAxis)
			.def("xAxis", &Quaternion::xAxis)
			.def("yAxis", &Quaternion::yAxis)
			.def("zAxis", &Quaternion::zAxis)
			.def("dot", &Quaternion::Dot)
			.def("norm", &Quaternion::Norm)
			.def("normalise", &Quaternion::normalise)
			.def("inverse", &Quaternion::Inverse)
			.def("unitInverse", &Quaternion::UnitInverse)
			.def("exp", &Quaternion::Exp)
			.def("log", &Quaternion::Log)
			.def("getRoll", &Quaternion::getRoll)
			.def("getPitch", &Quaternion::getPitch)
			.def("getYaw", &Quaternion::getYaw)
			.def("equals", &Quaternion::equals)
			.def("orientationEquals", &Quaternion::orientationEquals)
			.def("slerp", &Quaternion::Slerp)
			.def("slerpExtraSpins", &Quaternion::SlerpExtraSpins)
			.def("intermediate", &intermediate)
			.def("squad", &Quaternion::Squad)
			.def("nlerp", &Quaternion::nlerp)
			.def("isNaN", &Quaternion::isNaN)
			];

		AddClassToCollection("Quaternion", "class", "Quaternion class.");
		AddClassToCollection("Quaternion", "Quaternion(Degree angle, Vector3 axis)", "Constructor.");
		AddClassToCollection("Quaternion", "x", "type");
		AddClassToCollection("Quaternion", "y", "type");
		AddClassToCollection("Quaternion", "z", "type");
		AddClassToCollection("Quaternion", "w", "type");
		AddClassToCollection("Quaternion", "void fromRotationMatrix(Matrix3 matrix)", "Constructs a quaternion from rotation matrix3.");
		AddClassToCollection("Quaternion", "void ToRotationMatrix(Matrix3 matrix)", "Constructs a rotation matrix3 from quaternion.");
		AddClassToCollection("Quaternion", "Radian-Vector toAngleAxisFromRadian()", "Constructs angle (radian) and axis from quaternion.");
		AddClassToCollection("Quaternion", "Degree-Vector toAngleAxisFromDegree()", "Constructs angle (degree) and axis from quaternion.");
		AddClassToCollection("Quaternion", "void fromAngleAxis(Radian rfAngle, Vector3 rkAxis)", "Constructs a quaternion from angle axis.");
		AddClassToCollection("Quaternion", "Vector3 xAxis()", "Returns the X orthonormal axis defining the quaternion. Same as doing xAxis = Vector3::UNIT_X * this. Also called the local X-axis");
		AddClassToCollection("Quaternion", "Vector3 yAxis()", "Returns the Y orthonormal axis defining the quaternion. Same as doing yAxis = Vector3::UNIT_Y * this. Also called the local Y-axis");
		AddClassToCollection("Quaternion", "Vector3 zAxis()", "Returns the Z orthonormal axis defining the quaternion. Same as doing zAxis = Vector3::UNIT_Z * this. Also called the local Z-axis");

		AddClassToCollection("Quaternion", "Real dot(Quaternion other)", "Returns the dot product of the quaternion.");
		AddClassToCollection("Quaternion", "float norm()", "Returns the norm value of the quaternion. Note: internally w*w+x*x+y*y+z*z; is done.");
		AddClassToCollection("Quaternion", "float normalise()", "Normalises this quaternion, and returns the previous length.");
		AddClassToCollection("Quaternion", "Quaternion inverse()", "Returns the inverse quaternion for angle subtract operations. Note: Apply to non-zero quaternion.");
		AddClassToCollection("Quaternion", "Quaternion unitInverse()", "Returns the inverse quaternion for angle subtract operations. Note: Apply to unit-length quaternion. That means quaternion must be normalized! After that this operation is cheaper.");
		AddClassToCollection("Quaternion", "Quaternion exp()", "Returns the exponential value of the quaternion.");
		AddClassToCollection("Quaternion", "Quaternion log()", "Returns the logarithmic value of the quaternion.");

		AddClassToCollection("Quaternion", "Radian getRoll(bool reprojectAxis)", "Calculate the local roll element of this quaternion. Note: reprojectAxis By default the method returns the 'intuitive' result"
			" that is, if you projected the local Y of the quaternion onto the X and Y axes, the angle between them is returned. If set to false though, the"
			" result is the actual yaw that will be used to implement the quaternion, which is the shortest possible path to get to the same orientation and "
			" may involve less axial rotation. The co-domain of the returned value is  from -180 to 180 degrees.");

		AddClassToCollection("Quaternion", "Radian getPitch(bool reprojectAxis)", "Calculate the local pitch element of this quaternion. Note: reprojectAxis By default the method returns the 'intuitive' result"
			" that is, if you projected the local Z of the quaternion onto the X and Y axes, the angle between them is returned. If set to true though, the"
			" result is the actual yaw that will be used to implement the quaternion, which is the shortest possible path to get to the same orientation and"
			" may involve less axial rotation. The co-domain of the returned value is from -180 to 180 degrees.");

		AddClassToCollection("Quaternion", "Radian getYaw(bool reprojectAxis)", "Calculate the local yaw element of this quaternion. Note: reprojectAxis By default the method returns the 'intuitive' result"
			" that is, if you projected the local Y of the quaternion onto the X and Z axes, the angle between them is returned. If set to true though, the"
			" result is the actual yaw that will be used to implement the quaternion, which is the shortest possible path to get to the same orientation and"
			" may involve less axial rotation. The co-domain of the returned value is from -180 to 180 degrees.");

		AddClassToCollection("Quaternion", "bool equals(Quaternion other, Radian tolerance)", "Equality with tolerance (tolerance is max angle difference).");
		AddClassToCollection("Quaternion", "bool orientationEquals(Quaternion other, float tolerance)", "Compare two quaternions which are assumed to be used as orientations. Note:"
			"Both equals() and orientationEquals() measure the exact same thing. One measures the difference by angle, the other by a different, non-linear metric."
			"Attention: slerp (0.75f, A, B) != slerp (0.25f, B, A); therefore be careful if your code relies in the order of the operands. This is specially important in IK animation."
			"Note: Default tolerance is 1e-3");
		AddClassToCollection("Quaternion", "Quaternion slerp(float fT, Quaternion rkP, Quaternion rkQ, bool shortestPath", "Performs Spherical linear interpolation between two quaternions, and returns the result."
			"Slerp ( 0.0f, A, B ) = A; Slerp ( 1.0f, A, B ) = B"
			" Slerp has the proprieties of performing the interpolation at constant velocity, and being torque-minimal (unless shortestPath=false)."
			" However, it's NOT commutative, which means Slerp ( 0.75f, A, B ) != Slerp ( 0.25f, B, A ); therefore be careful if your code relies in the order of the operands. This is specially important in IK animation.");
		AddClassToCollection("Quaternion", "Quaternion slerpExtraSpins (float fT, Quaternion rkP, Quaternion rkQ, int iExtraSpins)", "@see Slerp. It adds extra 'spins' (i.e. rotates several times) specified by parameter 'iExtraSpins' while interpolating before arriving to the final values");
		AddClassToCollection("Quaternion", "Quaternion intermediate (Quaternion rkQ0, Quaternion rkQ1, Quaternion rkQ2)", "Setup for spherical quadratic interpolation.");
		AddClassToCollection("Quaternion", "Quaternion squad(float fT, Quaternion rkP, Quaternion rkA, Quaternion rkB, Quaternion rkQ, bool shortestPath)", "Spherical quadratic interpolation. Note: shortest path is by default false");
		AddClassToCollection("Quaternion", "Quaternion nlerp(float fT, Quaternion rkP,  Quaternion rkQ, bool shortestPath", "Performs Normalised linear interpolation between two quaternions, and returns the result."
			" nlerp (0.0f, A, B) = A; nlerp (1.0f, A, B) = B; Nlerp is faster than Slerp. Nlerp has the proprieties of being commutative (@see Slerp; commutativity is desired in certain places, like IK animation), and"
			" being torque-minimal (unless shortestPath=false). However, it's performing the interpolation at non-constant velocity; sometimes this is desired,"
			" sometimes it is not. Having a non-constant velocity can produce a more natural rotation feeling without the need of tweaking the weights; however"
			" if your scene relies on the timing of the rotation or assumes it will point at a specific angle at a specific weight value, Slerp is a better choice."
			" Note: shortest path is by default false");
		AddClassToCollection("Quaternion", "bool isNaN", "Check whether this quaternion contains valid values.");
		AddClassToCollection("Quaternion", "void fromRotationMatrix(Matrix3 matrix)", "Constructs a quaternion from rotation matrix3.");

		LUA_CONST_START(Quaternion)
			LUA_CONST(Quaternion, IDENTITY);
		LUA_CONST_END;

		AddClassToCollection("Quaternion", "IDENTITY", "Identity-Quaternion (w = 1, x = 0, y = 0, z = 0).");
	}

	void bindColorValue(lua_State* lua)
	{
		module(lua)
			[
				class_<ColourValue>("ColorValue")
				.def(constructor<>())
			.def(constructor<Real, Real, Real, Real>())
			.def(constructor<Real, Real, Real>())
			.def(tostring(self))
			.def_readwrite("r", &ColourValue::r)
			.def_readwrite("g", &ColourValue::g)
			.def_readwrite("b", &ColourValue::b)
			.def_readwrite("a", &ColourValue::a)
			.def("saturate", &ColourValue::saturate)

			// Operators
			.def(self + other<ColourValue>())
			.def(self - other<ColourValue>())
			.def(self * other<ColourValue>())
			.def(self * Real())
			.def(self / Real())
			];

		AddClassToCollection("ColorValue", "class", "ColorValue class.");
		AddClassToCollection("ColorValue", "ColorValue(float r, float g, float b, float a)", "Constructor.");
		AddClassToCollection("ColorValue", "r", "type");
		AddClassToCollection("ColorValue", "g", "type");
		AddClassToCollection("ColorValue", "b", "type");
		AddClassToCollection("ColorValue", "a", "type");
		AddClassToCollection("ColorValue", "void saturate()", "Clamps the value to the range [0, 1].");

		// Will not work because of ColorValue naming
		/*LUA_CONST_START(ColourValue)
			LUA_CONST(ColourValue, ZERO);
			LUA_CONST(ColourValue, Black);
			LUA_CONST(ColourValue, White);
			LUA_CONST(ColourValue, Red);
			LUA_CONST(ColourValue, Green);
			LUA_CONST(ColourValue, Blue);
		LUA_CONST_END;*/
	}

	//void bindMath(lua_State* lua)
	//{
	//	// Ogre::Math::
	//	module(lua)
	//	[
	//		class_<Math>("Math")
	//		// Methods
	//		.def("abs", (Real (Math::*)(Real)) &Math::Abs)
	//		.def("abs", (Degree(Math::*)(Degree)) &Math::Abs)
	//		.def("abs", (Radian(Math::*)(Radian)) &Math::Abs)
	//		.def("acos", &Math::ACos)
	//		.def("asin", &Math::ASin)
	//		.def("atan", &Math::ATan)
	//		.def("atan2", &Math::ATan2)
	//		.def("ceil", &Math::Ceil)
	//		.def("isNaN", &Math::isNaN)
	//		.def("cos", (Real(Math::*)(const Radian&, bool)) &Math::Cos)
	//		.def("cos", (Real(Math::*)(Real, bool)) &Math::Cos)
	//		.def("exp", &Math::Exp)
	//		.def("floor", &Math::Floor)
	//		.def("log", &Math::Log)
	//		.def("log2", &Math::Log2)
	//		.def("logN", &Math::LogN)
	//		.def("pow", &Math::Pow)
	//		.def("saturate", &Math::saturate)
	//		.def("sin", (Real(Math::*)(const Radian&, bool)) &Math::Sin)
	//		.def("sin", (Real(Math::*)(Real, bool)) &Math::Sin)
	//		.def("tan", (Real(Math::*)(const Radian&, bool)) &Math::Tan)
	//		.def("tan", (Real(Math::*)(Real, bool)) &Math::Tan)
	//		.def("perpendicular", &Math::perpendicular)
	//		.def("positionCloses", &Math::positionCloses)
	//		.def("positionEquals", &Math::positionEquals)
	//		.def("randomDeviant", &Math::randomDeviant)
	//		.def("reflect", &Math::reflect)
	//		.def("squaredDistance", &Math::squaredDistance)
	//		.def("squaredLength", &Math::squaredLength)
	//	];
	//}

	/*std::ostream& operator<<(std::ostream& stream, const Entity ent)
	{
		return stream << ent.getName();
	}*/

	//bool areButtonsDown(luabind::object buttons)
	//{
	//	short pressedButtonCount = -1;
	//	short buttonsCount = 0;

	//	// https://sourceforge.net/p/luabind/mailman/message/6187131/
	//	// Checks if the correct buttons are pressed, order is not relevant
	//	const auto& pressedButtons = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getPressedButtons();
	//	for (size_t i = 0; i < pressedButtons.size(); i++)
	//	{
	//		for (luabind::iterator it(buttons), end; it != end; ++it)
	//		{
	//			Ogre::String strKey = object_cast<Ogre::String>(it.key());

	//			luabind::object val = *it;

	//			if (luabind::type(val) == LUA_TNUMBER)
	//			{
	//				if (val == pressedButtons[i])
	//				{
	//					pressedButtonCount++;
	//				}
	//			}
	//			buttonsCount++;
	//		}
	//	}
	//	return pressedButtonCount == buttonsCount - 1;
	//}

	//bool areButtonsDown(luabind::argument const& buttons)
	//{
	//	short pressedButtonCount = -1;
	//	short buttonsCount = 0;

	//	// https://sourceforge.net/p/luabind/mailman/message/6187131/
	//	// Checks if the correct buttons are pressed, order is not relevant
	//	const auto& pressedButtons = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getPressedButtons();
	//	for (size_t i = 0; i < pressedButtons.size(); i++)
	//	{
	//		for (luabind::iterator it(buttons), end; it != end; ++it)
	//		{
	//			Ogre::String strKey = object_cast<Ogre::String>(it.key());

	//			luabind::object val = *it;

	//			if (luabind::type(val) == LUA_TNUMBER)
	//			{
	//				if (val == pressedButtons[i])
	//				{
	//					pressedButtonCount++;
	//				}
	//			}
	//			buttonsCount++;
	//		}
	//	}
	//	return pressedButtonCount == buttonsCount - 1;
	//}

	// Not used?
	bool isButtonDown(InputDeviceModule::JoyStickButton button)
	{
		short pressedButtonCount = -1;
		short buttonsCount = 0;

		// https://sourceforge.net/p/luabind/mailman/message/6187131/
		// Checks if the correct buttons are pressed, order is not relevant
		const auto& pressedButtons = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getPressedButtons();
		for (size_t i = 0; i < pressedButtons.size(); i++)
		{
			if (pressedButtons[i] == button)
			{
				pressedButtonCount++;
			}
			buttonsCount++;
		}
		return pressedButtonCount == buttonsCount - 1;
	}

	// Not used?
	bool areTwoButtonsDown(InputDeviceModule::JoyStickButton button1, InputDeviceModule::JoyStickButton button2)
	{
		short pressedButtonCount = -1;
		short buttonsCount = 0;

		// https://sourceforge.net/p/luabind/mailman/message/6187131/
		// Checks if the correct buttons are pressed, order is not relevant
		const auto& pressedButtons = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getPressedButtons();
		for (size_t i = 0; i < pressedButtons.size(); i++)
		{
			if (pressedButtons[i] == button1 || pressedButtons[i] == button2)
			{
				pressedButtonCount++;
			}
			buttonsCount++;
		}
		return pressedButtonCount == buttonsCount - 1;
	}

	luabind::object getPressedButtons(InputDeviceModule* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		const auto& pressedButtons = instance->getPressedButtons();

		unsigned int i = 0;
		for (auto& it = pressedButtons.cbegin(); it != pressedButtons.cend(); ++it)
		{
			obj[i++] = *it;
		}

		return obj;
	}

	// Not used?
	bool isButtonDown2(InputDeviceModule::JoyStickButton button)
	{
		short pressedButtonCount = -1;
		short buttonsCount = 0;

		// https://sourceforge.net/p/luabind/mailman/message/6187131/
		// Checks if the correct buttons are pressed, order is not relevant
		const auto& pressedButtons = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getPressedButtons();
		for (size_t i = 0; i < pressedButtons.size(); i++)
		{
			if (pressedButtons[i] == button)
			{
				pressedButtonCount++;
			}
			buttonsCount++;
		}
		return pressedButtonCount == buttonsCount - 1;
	}

	// Not used?
	bool areTwoButtonsDown2(InputDeviceModule::JoyStickButton button1, InputDeviceModule::JoyStickButton button2)
	{
		short pressedButtonCount = -1;
		short buttonsCount = 0;

		// https://sourceforge.net/p/luabind/mailman/message/6187131/
		// Checks if the correct buttons are pressed, order is not relevant
		const auto& pressedButtons = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getPressedButtons();
		for (size_t i = 0; i < pressedButtons.size(); i++)
		{
			if (pressedButtons[i] == button1 || pressedButtons[i] == button2)
			{
				pressedButtonCount++;
			}
			buttonsCount++;
		}
		return pressedButtonCount == buttonsCount - 1;
	}

	void bindInput(lua_State* lua)
	{
		/*
		OIS::Mouse* mouse;
		OIS::Keyboard* keyboard;
		*/
		module(lua)
			[
				class_<OIS::Mouse>("Mouse")
				.def("getMouseState", &OIS::Mouse::getMouseState)
			.enum_("MouseButtonID")
			[
				value("MB_LEFT", OIS::MB_Left),
				value("MB_RIGHT", OIS::MB_Right),
				value("MB_MIDDLE", OIS::MB_Middle),
				value("MB_BUTTON_3", OIS::MB_Button3),
				value("MB_BUTTON_4", OIS::MB_Button4),
				value("MB_BUTTON_5", OIS::MB_Button5),
				value("MB_BUTTON_6", OIS::MB_Button6),
				value("MB_BUTTON_7", OIS::MB_Button7)
			]
			];

		AddClassToCollection("Mouse", "class", "OIS mouse class.");
		AddClassToCollection("Mouse", "MouseState getMouseState()", "Gets the mouse button state.");
		AddClassToCollection("MouseButtonID", "MB_LEFT", "Left mouse button.");
		AddClassToCollection("MouseButtonID", "MB_MIDDLE", "Middle mouse button.");
		AddClassToCollection("MouseButtonID", "MB_BUTTON_3", "3 mouse button.");
		AddClassToCollection("MouseButtonID", "MB_BUTTON_4", "4 mouse button.");
		AddClassToCollection("MouseButtonID", "MB_BUTTON_5", "5 mouse button.");
		AddClassToCollection("MouseButtonID", "MB_BUTTON_6", "6 mouse button.");
		AddClassToCollection("MouseButtonID", "MB_BUTTON_7", "7 mouse button.");

		module(lua)
			[
				class_<OIS::Axis>("Axis")
				.def_readwrite("abs", &OIS::Axis::abs)
			.def_readwrite("rel", &OIS::Axis::rel)
			// .def_readwrite("absOnly", &OIS::Axis::absOnly)
			];
		AddClassToCollection("Axis", "class", "Axis of OIS mouse.");
		AddClassToCollection("Axis", "abs", "type");
		AddClassToCollection("Axis", "rel", "type");
		// AddClassToCollection("Axis", "absOnly()", "?");

		module(lua)
			[
				class_<OIS::MouseState>("MouseState")
				.def("buttonDown", &OIS::MouseState::buttonDown)
			.def_readwrite("width", &OIS::MouseState::width)
			.def_readwrite("height", &OIS::MouseState::height)
			.def_readwrite("X", &OIS::MouseState::X)
			.def_readwrite("Y", &OIS::MouseState::Y)
			.def_readwrite("Z", &OIS::MouseState::Z)
			];
		AddClassToCollection("MouseState", "class", "MouseState of OIS mouse.");
		AddClassToCollection("MouseState", "width", "type");
		AddClassToCollection("MouseState", "height", "type");
		AddClassToCollection("MouseState", "X", "type");
		AddClassToCollection("MouseState", "Y", "type");
		AddClassToCollection("MouseState", "Z", "type");

		module(lua)
			[
				class_<OIS::Keyboard>("Keyboard")
				.enum_("Modifier")
			[
				value("Alt", OIS::Keyboard::Alt),
				value("Shift", OIS::Keyboard::Shift),
				value("Ctrl", OIS::Keyboard::Ctrl),
				value("CapsLock", OIS::Keyboard::CapsLock),
				value("NumLock", OIS::Keyboard::NumLock)
			]
		.def("isKeyDown", &OIS::Keyboard::isKeyDown)
			.def("isModifierDown", &OIS::Keyboard::isModifierDown)
			];

		AddClassToCollection("Keyboard", "class", "OIS Keyboard class.");
		AddClassToCollection("Keyboard", "bool isKeyDown(KeyCode key)", "Gets whether a specifig key is down.");
		AddClassToCollection("Keyboard", "bool isModifierDown(Modifier mod)", "Gets whether a specifig modifier is down.");
		AddClassToCollection("Modifier", "Alt", "Alt modifier.");
		AddClassToCollection("Modifier", "Shift", "Shift modifier.");
		AddClassToCollection("Modifier", "Ctrl", "Ctrl modifier.");
		AddClassToCollection("Modifier", "CapsLock", "CapsLock modifier.");
		AddClassToCollection("Modifier", "NumLock", "NumLock modifier.");

		module(lua)
			[
				class_<OIS::KeyEvent>("KeyEvent")
				// .def_readwrite("key", &OIS::KeyEvent::key)
				// .def_readwrite("text", &OIS::KeyEvent::text)
			.enum_("KeyCode")
			[
				value("KC_UNASSIGNED", OIS::KeyCode::KC_UNASSIGNED),
				value("KC_ESCAPE", OIS::KeyCode::KC_ESCAPE),
				value("KC_1", OIS::KeyCode::KC_1),
				value("KC_2", OIS::KeyCode::KC_2),
				value("KC_3", OIS::KeyCode::KC_3),
				value("KC_4", OIS::KeyCode::KC_4),
				value("KC_5", OIS::KeyCode::KC_5),
				value("KC_6", OIS::KeyCode::KC_6),
				value("KC_7", OIS::KeyCode::KC_7),
				value("KC_8", OIS::KeyCode::KC_8),
				value("KC_9", OIS::KeyCode::KC_9),
				value("KC_0", OIS::KeyCode::KC_0),
				value("KC_MINUS", OIS::KeyCode::KC_MINUS),
				value("KC_EQUALS", OIS::KeyCode::KC_EQUALS),
				value("KC_BACK", OIS::KeyCode::KC_BACK),
				value("KC_TAB", OIS::KeyCode::KC_TAB),
				value("KC_Q", OIS::KeyCode::KC_Q),
				value("KC_W", OIS::KeyCode::KC_W),
				value("KC_E", OIS::KeyCode::KC_E),
				value("KC_R", OIS::KeyCode::KC_R),
				value("KC_T", OIS::KeyCode::KC_T),
				value("KC_Y", OIS::KeyCode::KC_Y),
				value("KC_U", OIS::KeyCode::KC_U),
				value("KC_I", OIS::KeyCode::KC_I),
				value("KC_O", OIS::KeyCode::KC_O),
				value("KC_P", OIS::KeyCode::KC_P),
				value("KC_LBRACKET", OIS::KeyCode::KC_LBRACKET),
				value("KC_RBRACKET", OIS::KeyCode::KC_RBRACKET),
				value("KC_RETURN", OIS::KeyCode::KC_RETURN),
				value("KC_LCONTROL", OIS::KeyCode::KC_LCONTROL),
				value("KC_A", OIS::KeyCode::KC_A),
				value("KC_S", OIS::KeyCode::KC_S),
				value("KC_D", OIS::KeyCode::KC_D),
				value("KC_F", OIS::KeyCode::KC_F),
				value("KC_G", OIS::KeyCode::KC_G),
				value("KC_H", OIS::KeyCode::KC_H),
				value("KC_J", OIS::KeyCode::KC_J),
				value("KC_K", OIS::KeyCode::KC_K),
				value("KC_L", OIS::KeyCode::KC_L),
				value("KC_SEMICOLON", OIS::KeyCode::KC_SEMICOLON),
				value("KC_APOSTROPHE", OIS::KeyCode::KC_APOSTROPHE),
				value("KC_GRAVE", OIS::KeyCode::KC_GRAVE),
				value("KC_LSHIFT", OIS::KeyCode::KC_LSHIFT),
				value("KC_BACKSLASH", OIS::KeyCode::KC_BACKSLASH),
				value("KC_Z", OIS::KeyCode::KC_Z),
				value("KC_X", OIS::KeyCode::KC_X),
				value("KC_C", OIS::KeyCode::KC_C),
				value("KC_V", OIS::KeyCode::KC_V),
				value("KC_B", OIS::KeyCode::KC_B),
				value("KC_N", OIS::KeyCode::KC_N),
				value("KC_M", OIS::KeyCode::KC_M),
				value("KC_COMMA", OIS::KeyCode::KC_COMMA),
				value("KC_PERIOD", OIS::KeyCode::KC_PERIOD),
				value("KC_SLASH", OIS::KeyCode::KC_SLASH),
				value("KC_RSHIFT", OIS::KeyCode::KC_RSHIFT),
				value("KC_MULTIPLY", OIS::KeyCode::KC_MULTIPLY),
				value("KC_LMENU", OIS::KeyCode::KC_LMENU),
				value("KC_SPACE", OIS::KeyCode::KC_SPACE),
				value("KC_CAPITAL", OIS::KeyCode::KC_CAPITAL),
				value("KC_F1", OIS::KeyCode::KC_F1),
				value("KC_F2", OIS::KeyCode::KC_F2),
				value("KC_F3", OIS::KeyCode::KC_F3),
				value("KC_F4", OIS::KeyCode::KC_F4),
				value("KC_F5", OIS::KeyCode::KC_F5),
				value("KC_F6", OIS::KeyCode::KC_F6),
				value("KC_F7", OIS::KeyCode::KC_F7),
				value("KC_F8", OIS::KeyCode::KC_F8),
				value("KC_F9", OIS::KeyCode::KC_F9),
				value("KC_F10", OIS::KeyCode::KC_F10),
				value("KC_NUMLOCK", OIS::KeyCode::KC_NUMLOCK),
				value("KC_SCROLL", OIS::KeyCode::KC_SCROLL),
				value("KC_NUMPAD7", OIS::KeyCode::KC_NUMPAD7),
				value("KC_NUMPAD8", OIS::KeyCode::KC_NUMPAD8),
				value("KC_NUMPAD9", OIS::KeyCode::KC_NUMPAD9),
				value("KC_SUBTRACT", OIS::KeyCode::KC_SUBTRACT),
				value("KC_NUMPAD4", OIS::KeyCode::KC_NUMPAD4),
				value("KC_NUMPAD5", OIS::KeyCode::KC_NUMPAD5),
				value("KC_NUMPAD6", OIS::KeyCode::KC_NUMPAD6),
				value("KC_ADD", OIS::KeyCode::KC_ADD),
				value("KC_NUMPAD1", OIS::KeyCode::KC_NUMPAD1),
				value("KC_NUMPAD2", OIS::KeyCode::KC_NUMPAD2),
				value("KC_NUMPAD3", OIS::KeyCode::KC_NUMPAD3),
				value("KC_NUMPAD0", OIS::KeyCode::KC_NUMPAD0),
				value("KC_DECIMAL", OIS::KeyCode::KC_DECIMAL),
				value("KC_OEM_102", OIS::KeyCode::KC_OEM_102),
				value("KC_F11", OIS::KeyCode::KC_F11),
				value("KC_F12", OIS::KeyCode::KC_F12),
				value("KC_F13", OIS::KeyCode::KC_F13),
				value("KC_F14", OIS::KeyCode::KC_F14),
				value("KC_F15", OIS::KeyCode::KC_F15),
				value("KC_KANA", OIS::KeyCode::KC_KANA),
				value("KC_ABNT_C1", OIS::KeyCode::KC_ABNT_C1),
				value("KC_CONVERT", OIS::KeyCode::KC_CONVERT),
				value("KC_NOCONVERT", OIS::KeyCode::KC_NOCONVERT),
				value("KC_YEN", OIS::KeyCode::KC_YEN),
				value("KC_ABNT_C2", OIS::KeyCode::KC_ABNT_C2),
				value("KC_NUMPADEQUALS", OIS::KeyCode::KC_NUMPADEQUALS),
				value("KC_PREVTRACK", OIS::KeyCode::KC_PREVTRACK),
				value("KC_AT", OIS::KeyCode::KC_AT),
				value("KC_COLON", OIS::KeyCode::KC_COLON),
				value("KC_UNDERLINE", OIS::KeyCode::KC_UNDERLINE),
				value("KC_KANJI", OIS::KeyCode::KC_KANJI),
				value("KC_STOP", OIS::KeyCode::KC_STOP),
				value("KC_AX", OIS::KeyCode::KC_AX),
				value("KC_UNLABELED", OIS::KeyCode::KC_UNLABELED),
				value("KC_NEXTTRACK", OIS::KeyCode::KC_NEXTTRACK),
				value("KC_NUMPADENTER", OIS::KeyCode::KC_NUMPADENTER),
				value("KC_RCONTROL", OIS::KeyCode::KC_RCONTROL),
				value("KC_MUTE", OIS::KeyCode::KC_MUTE),
				value("KC_CALCULATOR", OIS::KeyCode::KC_CALCULATOR),
				value("KC_PLAYPAUSE", OIS::KeyCode::KC_PLAYPAUSE),
				value("KC_MEDIASTOP", OIS::KeyCode::KC_MEDIASTOP),
				value("KC_VOLUMEDOWN", OIS::KeyCode::KC_VOLUMEDOWN),
				value("KC_VOLUMEUP", OIS::KeyCode::KC_VOLUMEUP),
				value("KC_WEBHOME", OIS::KeyCode::KC_WEBHOME),
				value("KC_NUMPADCOMMA", OIS::KeyCode::KC_NUMPADCOMMA),
				value("KC_DIVIDE", OIS::KeyCode::KC_DIVIDE),
				value("KC_SYSRQ", OIS::KeyCode::KC_SYSRQ),
				value("KC_RMENU", OIS::KeyCode::KC_RMENU),
				value("KC_PAUSE", OIS::KeyCode::KC_PAUSE),
				value("KC_HOME", OIS::KeyCode::KC_HOME),
				value("KC_UP", OIS::KeyCode::KC_UP),
				value("KC_PGUP", OIS::KeyCode::KC_PGUP),
				value("KC_LEFT", OIS::KeyCode::KC_LEFT),
				value("KC_RIGHT", OIS::KeyCode::KC_RIGHT),
				value("KC_END", OIS::KeyCode::KC_END),
				value("KC_DOWN", OIS::KeyCode::KC_DOWN),
				value("KC_PGDOWN", OIS::KeyCode::KC_PGDOWN),
				value("KC_INSERT", OIS::KeyCode::KC_INSERT),
				value("KC_DELETE", OIS::KeyCode::KC_DELETE),
				value("KC_LWIN", OIS::KeyCode::KC_LWIN),
				value("KC_RWIN", OIS::KeyCode::KC_RWIN),
				value("KC_APPS", OIS::KeyCode::KC_APPS),
				value("KC_POWER", OIS::KeyCode::KC_POWER),
				value("KC_SLEEP", OIS::KeyCode::KC_SLEEP),
				value("KC_WAKE", OIS::KeyCode::KC_WAKE),
				value("KC_WEBSEARCH", OIS::KeyCode::KC_WEBSEARCH),
				value("KC_WEBFAVORITES", OIS::KeyCode::KC_WEBFAVORITES),
				value("KC_WEBREFRESH", OIS::KeyCode::KC_WEBREFRESH),
				value("KC_WEBSTOP", OIS::KeyCode::KC_WEBSTOP),
				value("KC_WEBFORWARD", OIS::KeyCode::KC_WEBFORWARD),
				value("KC_WEBBACK", OIS::KeyCode::KC_WEBBACK),
				value("KC_MYCOMPUTER", OIS::KeyCode::KC_MYCOMPUTER),
				value("KC_MAIL", OIS::KeyCode::KC_MAIL),
				value("KC_MEDIASELECT", OIS::KeyCode::KC_MEDIASELECT)
			]
			];

		AddClassToCollection("KeyEvent", "class", "OIS Keyboard key event class.");
		AddClassToCollection("KeyEvent", "KC_1", "KC_1 key.");
		AddClassToCollection("KeyEvent", "KC_2", "KC_2 key.");
		AddClassToCollection("KeyEvent", "KC_3", "KC_3 key.");
		AddClassToCollection("KeyEvent", "KC_4", "KC_4 key.");
		AddClassToCollection("KeyEvent", "KC_5", "KC_5 key.");
		AddClassToCollection("KeyEvent", "KC_6", "KC_6 key.");
		AddClassToCollection("KeyEvent", "KC_7", "KC_7 key.");
		AddClassToCollection("KeyEvent", "KC_8", "KC_8 key.");
		AddClassToCollection("KeyEvent", "KC_9", "KC_9 key.");
		AddClassToCollection("KeyEvent", "KC_0", "KC_0 key.");
		AddClassToCollection("KeyEvent", "KC_MINUS", "KC_MINUS key.");
		AddClassToCollection("KeyEvent", "KC_EQUALS", "KC_EQUALS key.");
		AddClassToCollection("KeyEvent", "KC_BACK", "KC_BACK key.");
		AddClassToCollection("KeyEvent", "KC_TAB", "KC_TAB key.");
		AddClassToCollection("KeyEvent", "KC_Q", "KC_Q key.");
		AddClassToCollection("KeyEvent", "KC_W", "KC_W key.");
		AddClassToCollection("KeyEvent", "KC_E", "KC_E key.");
		AddClassToCollection("KeyEvent", "KC_R", "KC_R key.");
		AddClassToCollection("KeyEvent", "KC_T", "KC_T key.");
		AddClassToCollection("KeyEvent", "KC_Y", "KC_Y key.");
		AddClassToCollection("KeyEvent", "KC_U", "KC_U key.");
		AddClassToCollection("KeyEvent", "KC_I", "KC_I key.");
		AddClassToCollection("KeyEvent", "KC_O", "KC_O key.");
		AddClassToCollection("KeyEvent", "KC_P", "KC_P key.");
		AddClassToCollection("KeyEvent", "KC_LBRACKET", "KC_LBRACKET key.");
		AddClassToCollection("KeyEvent", "KC_RBRACKET", "KC_RBRACKET key.");
		AddClassToCollection("KeyEvent", "KC_RETURN", "KC_RETURN key.");
		AddClassToCollection("KeyEvent", "KC_LCONTROL", "KC_LCONTROL key.");
		AddClassToCollection("KeyEvent", "KC_A", "KC_A key.");
		AddClassToCollection("KeyEvent", "KC_S", "KC_S key.");
		AddClassToCollection("KeyEvent", "KC_D", "KC_D key.");
		AddClassToCollection("KeyEvent", "KC_F", "KC_F key.");
		AddClassToCollection("KeyEvent", "KC_G", "KC_G key.");
		AddClassToCollection("KeyEvent", "KC_H", "KC_H key.");
		AddClassToCollection("KeyEvent", "KC_J", "KC_J key.");
		AddClassToCollection("KeyEvent", "KC_K", "KC_K key.");
		AddClassToCollection("KeyEvent", "KC_L", "KC_L key.");
		AddClassToCollection("KeyEvent", "KC_SEMICOLON", "KC_SEMICOLON key.");
		AddClassToCollection("KeyEvent", "KC_APOSTROPHE", "KC_APOSTROPHE key.");
		AddClassToCollection("KeyEvent", "KC_GRAVE", "KC_GRAVE key.");
		AddClassToCollection("KeyEvent", "KC_LSHIFT", "KC_LSHIFT key.");
		AddClassToCollection("KeyEvent", "KC_BACKSLASH", "KC_BACKSLASH key.");
		AddClassToCollection("KeyEvent", "KC_Z", "KC_Z key.");
		AddClassToCollection("KeyEvent", "KC_X", "KC_X key.");
		AddClassToCollection("KeyEvent", "KC_C", "KC_C key.");
		AddClassToCollection("KeyEvent", "KC_V", "KC_V key.");
		AddClassToCollection("KeyEvent", "KC_B", "KC_B key.");
		AddClassToCollection("KeyEvent", "KC_N", "KC_N key.");
		AddClassToCollection("KeyEvent", "KC_M", "KC_M key.");
		AddClassToCollection("KeyEvent", "KC_COMMA", "KC_COMMA key.");
		AddClassToCollection("KeyEvent", "KC_PERIOD", "KC_PERIOD key.");
		AddClassToCollection("KeyEvent", "KC_SLASH", "KC_SLASH key.");
		AddClassToCollection("KeyEvent", "KC_RSHIFT", "KC_RSHIFT key.");
		AddClassToCollection("KeyEvent", "KC_MULTIPLY", "KC_MULTIPLY key.");
		AddClassToCollection("KeyEvent", "KC_LMENU", "KC_LMENU key.");
		AddClassToCollection("KeyEvent", "KC_SPACE", "KC_SPACE key.");
		AddClassToCollection("KeyEvent", "KC_CAPITAL", "KC_CAPITAL key.");
		AddClassToCollection("KeyEvent", "KC_F1", "KC_F1 key.");
		AddClassToCollection("KeyEvent", "KC_F2", "KC_F2 key.");
		AddClassToCollection("KeyEvent", "KC_F3", "KC_F3 key.");
		AddClassToCollection("KeyEvent", "KC_F4", "KC_F4 key.");
		AddClassToCollection("KeyEvent", "KC_F5", "KC_F5 key.");
		AddClassToCollection("KeyEvent", "KC_F6", "KC_F6 key.");
		AddClassToCollection("KeyEvent", "KC_F7", "KC_F7 key.");
		AddClassToCollection("KeyEvent", "KC_F8", "KC_F8 key.");
		AddClassToCollection("KeyEvent", "KC_F9", "KC_F9 key.");
		AddClassToCollection("KeyEvent", "KC_F10", "KC_F10 key.");
		AddClassToCollection("KeyEvent", "KC_NUMLOCK", "KC_NUMLOCK key.");
		AddClassToCollection("KeyEvent", "KC_SCROLL", "KC_SCROLL key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD7", "KC_NUMPAD7 key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD8", "KC_NUMPAD8 key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD9", "KC_NUMPAD9 key.");
		AddClassToCollection("KeyEvent", "KC_SUBTRACT", "KC_SUBTRACT key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD4", "KC_NUMPAD4 key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD5", "KC_NUMPAD5 key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD6", "KC_NUMPAD6 key.");
		AddClassToCollection("KeyEvent", "KC_ADD", "KC_ADD key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD1", "KC_NUMPAD1 key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD2", "KC_NUMPAD2 key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD3", "KC_NUMPAD3 key.");
		AddClassToCollection("KeyEvent", "KC_NUMPAD0", "KC_NUMPAD0 key.");
		AddClassToCollection("KeyEvent", "KC_DECIMAL", "KC_DECIMAL key.");
		AddClassToCollection("KeyEvent", "KC_OEM_102", "KC_OEM_102 key.");
		AddClassToCollection("KeyEvent", "KC_F11", "KC_F11 key.");
		AddClassToCollection("KeyEvent", "KC_F12", "KC_F12 key.");
		AddClassToCollection("KeyEvent", "KC_F13", "KC_F13 key.");
		AddClassToCollection("KeyEvent", "KC_F14", "KC_F14 key.");
		AddClassToCollection("KeyEvent", "KC_F15", "KC_F15 key.");
		AddClassToCollection("KeyEvent", "KC_KANA", "KC_KANA key.");
		AddClassToCollection("KeyEvent", "KC_ABNT_C1", "KC_ABNT_C1 key.");
		AddClassToCollection("KeyEvent", "KC_CONVERT", "KC_CONVERT key.");
		AddClassToCollection("KeyEvent", "KC_NOCONVERT", "KC_NOCONVERT key.");
		AddClassToCollection("KeyEvent", "KC_YEN", "KC_YEN key.");
		AddClassToCollection("KeyEvent", "KC_ABNT_C2", "KC_ABNT_C2 key.");
		AddClassToCollection("KeyEvent", "KC_NUMPADEQUALS", "KC_NUMPADEQUALS key.");
		AddClassToCollection("KeyEvent", "KC_PREVTRACK", "KC_PREVTRACK key.");
		AddClassToCollection("KeyEvent", "KC_AT", "KC_AT key.");
		AddClassToCollection("KeyEvent", "KC_COLON", "KC_COLON key.");
		AddClassToCollection("KeyEvent", "KC_UNDERLINE", "KC_UNDERLINE key.");
		AddClassToCollection("KeyEvent", "KC_KANJI", "KC_KANJI key.");
		AddClassToCollection("KeyEvent", "KC_STOP", "KC_STOP key.");
		AddClassToCollection("KeyEvent", "KC_AX", "KC_AX key.");
		AddClassToCollection("KeyEvent", "KC_UNLABELED", "KC_UNLABELED key.");
		AddClassToCollection("KeyEvent", "KC_NEXTTRACK", "KC_NEXTTRACK key.");
		AddClassToCollection("KeyEvent", "KC_NUMPADENTER", "KC_NUMPADENTER key.");
		AddClassToCollection("KeyEvent", "KC_RCONTROL", "KC_RCONTROL key.");
		AddClassToCollection("KeyEvent", "KC_MUTE", "KC_MUTE key.");
		AddClassToCollection("KeyEvent", "KC_CALCULATOR", "KC_CALCULATOR key.");
		AddClassToCollection("KeyEvent", "KC_PLAYPAUSE", "KC_PLAYPAUSE key.");
		AddClassToCollection("KeyEvent", "KC_MEDIASTOP", "KC_MEDIASTOP key.");
		AddClassToCollection("KeyEvent", "KC_VOLUMEDOWN", "KC_VOLUMEDOWN key.");
		AddClassToCollection("KeyEvent", "KC_VOLUMEUP", "KC_VOLUMEUP key.");
		AddClassToCollection("KeyEvent", "KC_WEBHOME", "KC_WEBHOME key.");
		AddClassToCollection("KeyEvent", "KC_NUMPADCOMMA", "KC_NUMPADCOMMA key.");
		AddClassToCollection("KeyEvent", "KC_DIVIDE", "KC_DIVIDE key.");
		AddClassToCollection("KeyEvent", "KC_SYSRQ", "KC_SYSRQ key.");
		AddClassToCollection("KeyEvent", "KC_RMENU", "KC_RMENU key.");
		AddClassToCollection("KeyEvent", "KC_PAUSE", "KC_PAUSE key.");
		AddClassToCollection("KeyEvent", "KC_HOME", "KC_HOME key.");
		AddClassToCollection("KeyEvent", "KC_UP", "KC_UP key.");
		AddClassToCollection("KeyEvent", "KC_PGUP", "KC_PGUP key.");
		AddClassToCollection("KeyEvent", "KC_LEFT", "KC_LEFT key.");
		AddClassToCollection("KeyEvent", "KC_RIGHT", "KC_RIGHT key.");
		AddClassToCollection("KeyEvent", "KC_END", "KC_END key.");
		AddClassToCollection("KeyEvent", "KC_DOWN", "KC_DOWN key.");
		AddClassToCollection("KeyEvent", "KC_PGDOWN", "KC_PGDOWN key.");
		AddClassToCollection("KeyEvent", "KC_INSERT", "KC_INSERT key.");
		AddClassToCollection("KeyEvent", "KC_DELETE", "KC_DELETE key.");
		AddClassToCollection("KeyEvent", "KC_LWIN", "KC_LWIN key.");
		AddClassToCollection("KeyEvent", "KC_RWIN", "KC_RWIN key.");
		AddClassToCollection("KeyEvent", "KC_APPS", "KC_APPS key.");
		AddClassToCollection("KeyEvent", "KC_POWER", "KC_POWER key.");
		AddClassToCollection("KeyEvent", "KC_SLEEP", "KC_SLEEP key.");
		AddClassToCollection("KeyEvent", "KC_WAKE", "KC_WAKE key.");
		AddClassToCollection("KeyEvent", "KC_WEBSEARCH", "KC_WEBSEARCH key.");
		AddClassToCollection("KeyEvent", "KC_WEBFAVORITES", "KC_WEBFAVORITES key.");
		AddClassToCollection("KeyEvent", "KC_WEBREFRESH", "KC_WEBREFRESH key.");
		AddClassToCollection("KeyEvent", "KC_WEBSTOP", "KC_WEBSTOP key.");
		AddClassToCollection("KeyEvent", "KC_WEBFORWARD", "KC_WEBFORWARD key.");
		AddClassToCollection("KeyEvent", "KC_WEBBACK", "KC_WEBBACK key.");
		AddClassToCollection("KeyEvent", "KC_MYCOMPUTER", "KC_MYCOMPUTER key.");
		AddClassToCollection("KeyEvent", "KC_MAIL", "KC_MAIL key.");
		AddClassToCollection("KeyEvent", "KC_MEDIASELECT", "KC_MEDIASELECT key.");

		module(lua)
		[
			class_<InputDeviceModule>("InputDeviceModule")
			.enum_("Action")
			[
				value("UP", InputDeviceModule::UP),
				value("DOWN", InputDeviceModule::DOWN),
				value("LEFT", InputDeviceModule::LEFT),
				value("RIGHT", InputDeviceModule::RIGHT),
				value("JUMP", InputDeviceModule::JUMP),
				value("RUN", InputDeviceModule::RUN),
				value("COWER", InputDeviceModule::COWER),
				value("DUCK", InputDeviceModule::DUCK),
				value("SNEAK", InputDeviceModule::SNEAK),
				value("ATTACK_1", InputDeviceModule::ATTACK_1),
				value("ATTACK_2", InputDeviceModule::ATTACK_2),
				value("ACTION", InputDeviceModule::ACTION),
				value("RELOAD", InputDeviceModule::RELOAD),
				value("INVENTORY", InputDeviceModule::INVENTORY),
				value("MAP", InputDeviceModule::MAP),
				value("PAUSE", InputDeviceModule::PAUSE),
				value("START", InputDeviceModule::START),
				value("SAVE", InputDeviceModule::SAVE),
				value("LOAD", InputDeviceModule::LOAD),
				value("CAMERA_FORWARD", InputDeviceModule::CAMERA_FORWARD),
				value("CAMERA_BACKWARD", InputDeviceModule::CAMERA_BACKWARD),
				value("CAMERA_LEFT", InputDeviceModule::CAMERA_LEFT),
				value("CAMERA_RIGHT", InputDeviceModule::CAMERA_RIGHT),
				value("CAMERA_UP", InputDeviceModule::CAMERA_UP),
				value("CAMERA_DOWN", InputDeviceModule::CAMERA_DOWN),
				value("CONSOLE", InputDeviceModule::CONSOLE),
				value("WEAPON_CHANGE_FORWARD", InputDeviceModule::WEAPON_CHANGE_FORWARD),
				value("WEAPON_CHANGE_BACKWARD", InputDeviceModule::WEAPON_CHANGE_BACKWARD),
				value("FLASH_LIGHT", InputDeviceModule::FLASH_LIGHT),
				value("SELECT", InputDeviceModule::SELECT),
				value("GRID", InputDeviceModule::GRID)
			]
		    .enum_("JoyStickButton")
			[
				value("BUTTON_A", InputDeviceModule::BUTTON_A),
				value("BUTTON_B", InputDeviceModule::BUTTON_B),
				value("BUTTON_X", InputDeviceModule::BUTTON_X),
				value("BUTTON_Y", InputDeviceModule::BUTTON_Y),
				value("BUTTON_LB", InputDeviceModule::BUTTON_LB),
				value("BUTTON_RB", InputDeviceModule::BUTTON_RB),
				value("BUTTON_LT", InputDeviceModule::BUTTON_LT),
				value("BUTTON_RT", InputDeviceModule::BUTTON_RT),
				value("BUTTON_SELECT", InputDeviceModule::BUTTON_SELECT),
			    value("BUTTON_START", InputDeviceModule::BUTTON_START),
				value("BUTTON_LEFT_STICK", InputDeviceModule::BUTTON_LEFT_STICK),
				value("BUTTON_RIGHT_STICK", InputDeviceModule::BUTTON_RIGHT_STICK),
			    value("BUTTON_UP", InputDeviceModule::BUTTON_LEFT_STICK_UP),
				value("BUTTON_DOWN", InputDeviceModule::BUTTON_LEFT_STICK_DOWN),
				value("BUTTON_LEFT", InputDeviceModule::BUTTON_LEFT_STICK_LEFT),
				value("BUTTON_RIGHT", InputDeviceModule::BUTTON_LEFT_STICK_RIGHT),
				value("BUTTON_RIGHT_STICK_UP", InputDeviceModule::BUTTON_RIGHT_STICK_UP),
				value("BUTTON_RIGHT_STICK_DOWN", InputDeviceModule::BUTTON_RIGHT_STICK_DOWN),
				value("BUTTON_RIGHT_STICK_LEFT", InputDeviceModule::BUTTON_RIGHT_STICK_LEFT),
				value("BUTTON_RIGHT_STICK_RIGHT", InputDeviceModule::BUTTON_RIGHT_STICK_RIGHT),
				value("BUTTON_NONE", InputDeviceModule::BUTTON_NONE)
			]
		    .def("getMappedKey", &InputDeviceModule::getMappedKey)
			.def("getStringFromMappedKey", &InputDeviceModule::getStringFromMappedKey)
			.def("getMappedKeyFromString", &InputDeviceModule::getMappedKeyFromString)
			.def("getMappedButton", &InputDeviceModule::getMappedButton)
			.def("getStringFromMappedButton", &InputDeviceModule::getStringFromMappedButton)
			.def("setJoyStickDeadZone", &InputDeviceModule::setJoyStickDeadZone)
			.def("hasActiveJoyStick", &InputDeviceModule::hasActiveJoyStick)
			.def("getLeftStickHorizontalMovingStrength", &InputDeviceModule::getLeftStickHorizontalMovingStrength)
			.def("getLeftStickVerticalMovingStrength", &InputDeviceModule::getLeftStickVerticalMovingStrength)
			.def("getRightStickHorizontalMovingStrength", &InputDeviceModule::getRightStickHorizontalMovingStrength)
			.def("getRightStickVerticalMovingStrength", &InputDeviceModule::getRightStickVerticalMovingStrength)
			.def("isKeyDown", &InputDeviceModule::isKeyDown)
			.def("isButtonDown", &InputDeviceModule::isButtonDown)
			.def("isActionDown", &InputDeviceModule::isActionDown)
			// https://sourceforge.net/p/luabind/mailman/message/21021347/
			// .def("areButtonsDown", &InputDeviceModule::areButtonsDown, luabind::copy_table(boost::arg<3>())) // _1 is for this, _2 is the actual in parameter // boost::arg<2>()
			// .def("areButtonsDown", &InputDeviceModule::areButtonsDown, raw(boost::bind(InputDeviceModule::areButtonsDown, _2)))
			// .def("areButtonsDown", &areButtonsDown, raw(boost::arg<3>()))
			.def("areButtonsDown2", &InputDeviceModule::areButtonsDown2)
			.def("areButtonsDown3", &InputDeviceModule::areButtonsDown3)
			.def("areButtonsDown4", &InputDeviceModule::areButtonsDown4)
			// .def("toAngleAxis", (void(Quaternion::*)(Radian&, Vector3&) const) &Quaternion::ToAngleAxis)
			.def("getPressedButton", &InputDeviceModule::getPressedButton)
			.def("getPressedButtons", &getPressedButtons)
		];

		object globalVars = globals(lua);
		globalVars["InputDeviceModule"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0);
		globalVars["InputDeviceModule2"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1);

		AddClassToCollection("InputDeviceModule", "class", "InputDeviceModule singleton for managing inputs like mouse, keyboard, joystick and mapping keys.");
		// AddClassToCollection("InputDeviceModule", "InputDeviceModule InputDeviceModule()", "Gets the input device instance.");
		AddClassToCollection("InputDeviceModule", "KeyCode getMappedKey(Action action)", "Gets the OIS key that is mapped as action.");
		AddClassToCollection("InputDeviceModule", "String getStringFromMappedKey(Action action)", "Gets the OIS key as string that is mapped as action.");
		AddClassToCollection("InputDeviceModule", "KeyCode getMappedKeyFromString(String key)", "Gets the OIS key from string key.");
		AddClassToCollection("InputDeviceModule", "JoyStickButton getMappedButton(Action action)", "Gets the OIS joystick button that is mapped as action.");
		AddClassToCollection("InputDeviceModule", "String getStringFromMappedButton(Action action)", "Gets the OIS joystick button as string that is mapped as action.");
		AddClassToCollection("InputDeviceModule", "void setJoyStickDeadZone(float deadZone)", "Sets the joystick dead zone.");
		AddClassToCollection("InputDeviceModule", "bool hasActiveJoyStick()", "Gets whether a joystick is plugged in and active.");
		AddClassToCollection("InputDeviceModule", "float getLeftStickHorizontalMovingStrength()", "Gets the strength of the left stick horizontal moving."
			" If 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].");
		AddClassToCollection("InputDeviceModule", "float getLeftStickVerticalMovingStrength()", "Gets the strength of the left stick vertical moving."
			" If 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].");
		AddClassToCollection("InputDeviceModule", "float getLeftStickHorizontalMovingStrength()", "Gets the strength of the right stick horizontal moving."
			" If 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].");
		AddClassToCollection("InputDeviceModule", "float getLeftStickVerticalMovingStrength()", "Gets the strength of the right stick vertical moving."
			" If 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].");
		AddClassToCollection("InputDeviceModule", "bool isKeyDown(KeyCode key)", "Gets whether a specifig key is down.");
		AddClassToCollection("InputDeviceModule", "bool isButtonDown(JoyStickButton button)", "Gets whether a specifig joystick button is down.");
		AddClassToCollection("InputDeviceModule", "bool isActionDown(Action action)", "Gets whether a specifig mapped action is down.");
		AddClassToCollection("InputDeviceModule", "bool areButtonsDown2(JoyStickButton button1, JoyStickButton button2)", "Gets whether two specific joystick buttons are down at the same time.");
		AddClassToCollection("InputDeviceModule", "bool areButtonsDown3(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3)", "Gets whether three specific joystick buttons are down at the same time.");
		AddClassToCollection("InputDeviceModule", "bool areButtonsDown3(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3, JoyStickButton button4)", "Gets whether four specific joystick buttons are down at the same time.");
		AddClassToCollection("InputDeviceModule", "JoyStickButton getPressedButton()", "Gets the currently pressed joystick button.");
		AddClassToCollection("InputDeviceModule", "Table[index][JoyStickButton] getPressedButtons()", "Gets the currently simultanously pressed joystick buttons.");

		// For keyboard
		globalVars["NOWA_K_UP"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::UP);
		globalVars["NOWA_K_DOWN"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::DOWN);
		globalVars["NOWA_K_LEFT"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::LEFT);
		globalVars["NOWA_K_RIGHT"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::RIGHT);
		globalVars["NOWA_K_JUMP"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::JUMP);
		globalVars["NOWA_K_RUN"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::RUN);
		globalVars["NOWA_K_COWER"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::COWER);
		globalVars["NOWA_K_DUCK"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::DUCK);
		globalVars["NOWA_K_SNEAK"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::SNEAK);
		globalVars["NOWA_K_ACTION"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::ACTION);
		globalVars["NOWA_K_ATTACK_1"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::ATTACK_1);
		globalVars["NOWA_K_ATTACK_2"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::ATTACK_2);
		globalVars["NOWA_K_RELOAD"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::RELOAD);
		globalVars["NOWA_K_INVENTORY"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::INVENTORY);
		globalVars["NOWA_K_SAVE"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::SAVE);
		globalVars["NOWA_K_LOAD"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::LOAD);
		globalVars["NOWA_K_PAUSE"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::PAUSE);
		globalVars["NOWA_K_START"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::START);
		globalVars["NOWA_K_MAP"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::MAP);
		globalVars["NOWA_K_CAMERA_FORWARD"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::CAMERA_FORWARD);
		globalVars["NOWA_K_CAMERA_BACKWARD"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::CAMERA_BACKWARD);
		globalVars["NOWA_K_CAMERA_RIGHT"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::CAMERA_RIGHT);
		globalVars["NOWA_K_CAMERA_UP"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::CAMERA_UP);
		globalVars["NOWA_K_CAMERA_DOWN"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::CAMERA_DOWN);
		globalVars["NOWA_K_CAMERA_LEFT"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::CAMERA_LEFT);
		globalVars["NOWA_K_CONSOLE"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::CONSOLE);
		globalVars["NOWA_K_WEAPON_CHANGE_FORWARD"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::WEAPON_CHANGE_FORWARD);
		globalVars["NOWA_K_WEAPON_CHANGE_BACKWARD"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::WEAPON_CHANGE_BACKWARD);
		globalVars["NOWA_K_FLASH_LIGHT"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::FLASH_LIGHT);
		globalVars["NOWA_K_SELECT"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(InputDeviceModule::SELECT);

		// For joystick
		globalVars["NOWA_B_JUMP"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::JUMP);
		globalVars["NOWA_B_RUN"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::RUN);
		globalVars["NOWA_B_ATTACK_1"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::ATTACK_1);
		globalVars["NOWA_B_ACTION"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::ACTION);
		globalVars["NOWA_B_RELOAD"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::RELOAD);
		globalVars["NOWA_B_INVENTORY"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::INVENTORY);
		globalVars["NOWA_B_MAP"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::MAP);
		globalVars["NOWA_B_PAUSE"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::PAUSE);
		globalVars["NOWA_B_START"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::START);
		globalVars["NOWA_B_FLASH_LIGHT"] = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(InputDeviceModule::FLASH_LIGHT);

		// For second joystick
		globalVars["NOWA_B_JUMP_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::JUMP);
		globalVars["NOWA_B_RUN_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::RUN);
		globalVars["NOWA_B_ATTACK_1_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::ATTACK_1);
		globalVars["NOWA_B_ACTION_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::ACTION);
		globalVars["NOWA_B_RELOAD_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::RELOAD);
		globalVars["NOWA_B_INVENTORY_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::INVENTORY);
		globalVars["NOWA_B_MAP_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::MAP);
		globalVars["NOWA_B_PAUSE_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::PAUSE);
		globalVars["NOWA_B_START_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::START);
		globalVars["NOWA_B_FLASH_LIGHT_2"] = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(1)->getMappedButton(InputDeviceModule::FLASH_LIGHT);

		// For both
		globalVars["NOWA_A_UP"] = InputDeviceModule::UP;
		globalVars["NOWA_A_DOWN"] = InputDeviceModule::DOWN;
		globalVars["NOWA_A_LEFT"] = InputDeviceModule::LEFT;
		globalVars["NOWA_A_RIGHT"] = InputDeviceModule::RIGHT;
		globalVars["NOWA_A_JUMP"] = InputDeviceModule::JUMP;
		globalVars["NOWA_A_RUN"] = InputDeviceModule::RUN;
		globalVars["NOWA_A_COWER"] = InputDeviceModule::COWER;
		globalVars["NOWA_A_DUCK"] = InputDeviceModule::DUCK;
		globalVars["NOWA_A_SNEAK"] = InputDeviceModule::SNEAK;
		globalVars["NOWA_A_ACTION"] = InputDeviceModule::ACTION;
		globalVars["NOWA_A_ATTACK_1"] = InputDeviceModule::ATTACK_1;
		globalVars["NOWA_A_ATTACK_2"] = InputDeviceModule::ATTACK_2;
		globalVars["NOWA_A_RELOAD"] = InputDeviceModule::RELOAD;
		globalVars["NOWA_A_INVENTORY"] = InputDeviceModule::INVENTORY;
		globalVars["NOWA_A_SAVE"] = InputDeviceModule::SAVE;
		globalVars["NOWA_A_LOAD"] = InputDeviceModule::LOAD;
		globalVars["NOWA_A_PAUSE"] = InputDeviceModule::PAUSE;
		globalVars["NOWA_A_START"] = InputDeviceModule::START;
		globalVars["NOWA_A_MAP"] = InputDeviceModule::MAP;
		globalVars["NOWA_A_CAMERA_FORWARD"] = InputDeviceModule::CAMERA_FORWARD;
		globalVars["NOWA_A_CAMERA_BACKWARD"] = InputDeviceModule::CAMERA_BACKWARD;
		globalVars["NOWA_A_CAMERA_RIGHT"] = InputDeviceModule::CAMERA_RIGHT;
		globalVars["NOWA_A_CAMERA_UP"] = InputDeviceModule::CAMERA_UP;
		globalVars["NOWA_A_CAMERA_DOWN"] = InputDeviceModule::CAMERA_DOWN;
		globalVars["NOWA_A_CAMERA_LEFT"] = InputDeviceModule::CAMERA_LEFT;
		globalVars["NOWA_A_CONSOLE"] = InputDeviceModule::CONSOLE;
		globalVars["NOWA_A_WEAPON_CHANGE_FORWARD"] = InputDeviceModule::WEAPON_CHANGE_FORWARD;
		globalVars["NOWA_A_WEAPON_CHANGE_BACKWARD"] = InputDeviceModule::WEAPON_CHANGE_BACKWARD;
		globalVars["NOWA_A_FLASH_LIGHT"] = InputDeviceModule::FLASH_LIGHT;
		globalVars["NOWA_A_SELECT"] = InputDeviceModule::SELECT;

		AddClassToCollection("InputMapping", "NOWA_K_UP", "Mapped up key.");
		AddClassToCollection("InputMapping", "NOWA_K_DOWN", "Mapped down key.");
		AddClassToCollection("InputMapping", "NOWA_K_LEFT", "Mapped left key.");
		AddClassToCollection("InputMapping", "NOWA_K_RIGHT", "Mapped right key.");
		AddClassToCollection("InputMapping", "NOWA_K_JUMP", "Mapped jump key.");
		AddClassToCollection("InputMapping", "NOWA_K_RUN", "Mapped run key.");
		AddClassToCollection("InputMapping", "NOWA_K_DUCK", "Mapped duck key.");
		AddClassToCollection("InputMapping", "NOWA_K_SNEAK", "Mapped sneak key.");
		AddClassToCollection("InputMapping", "NOWA_K_ACTION", "Mapped action like open door key.");
		AddClassToCollection("InputMapping", "NOWA_K_ATTACK_1", "Mapped attack 1 key.");
		AddClassToCollection("InputMapping", "NOWA_K_ATTACK_2", "Mapped attack 2 key.");
		AddClassToCollection("InputMapping", "NOWA_K_RELOAD", "Mapped reload key.");
		AddClassToCollection("InputMapping", "NOWA_K_INVENTORY", "Mapped inventory key.");
		AddClassToCollection("InputMapping", "NOWA_K_SAVE", "Mapped save key.");
		AddClassToCollection("InputMapping", "NOWA_K_LOAD", "Mapped load key.");
		AddClassToCollection("InputMapping", "NOWA_K_PAUSE", "Mapped pause key.");
		AddClassToCollection("InputMapping", "NOWA_K_START", "Mapped start key.");
		AddClassToCollection("InputMapping", "NOWA_K_MAP", "Mapped map key.");
		AddClassToCollection("InputMapping", "NOWA_K_CAMERA_FORWARD", "Mapped camera forward key.");
		AddClassToCollection("InputMapping", "NOWA_K_CAMERA_BACKWARD", "Mapped backward key.");
		AddClassToCollection("InputMapping", "NOWA_K_CAMERA_RIGHT", "Mapped camera right key.");
		AddClassToCollection("InputMapping", "NOWA_K_CAMERA_UP", "Mapped camera up key.");
		AddClassToCollection("InputMapping", "NOWA_K_CAMERA_DOWN", "Mapped camera down key.");
		AddClassToCollection("InputMapping", "NOWA_K_CAMERA_LEFT", "Mapped camera left key.");
		AddClassToCollection("InputMapping", "NOWA_K_CONSOLE", "Mapped console key.");
		AddClassToCollection("InputMapping", "NOWA_K_WEAPON_CHANGE_FORWARD", "Mapped weapon change forward key.");
		AddClassToCollection("InputMapping", "NOWA_K_WEAPON_CHANGE_BACKWARD", "Mapped weapon change backward key.");
		AddClassToCollection("InputMapping", "NOWA_K_FLASH_LIGHT", "Mapped flash light key.");
		AddClassToCollection("InputMapping", "NOWA_K_SELECT", "Mapped select key.");

		AddClassToCollection("InputMapping", "NOWA_B_JUMP", "Jump button.");
		AddClassToCollection("InputMapping", "NOWA_B_RUN", "Run button.");
		AddClassToCollection("InputMapping", "NOWA_B_ATTACK_1", "Attack 1 button.");
		AddClassToCollection("InputMapping", "NOWA_B_ACTION", "Action like door open button.");
		AddClassToCollection("InputMapping", "NOWA_B_RELOAD", "Reload button.");
		AddClassToCollection("InputMapping", "NOWA_B_INVENTORY", "Inventory button.");
		AddClassToCollection("InputMapping", "NOWA_B_MAP", "Map button.");
		AddClassToCollection("InputMapping", "NOWA_B_PAUSE", "Pause button.");
		AddClassToCollection("InputMapping", "NOWA_B_MENU", "Menu button.");
		AddClassToCollection("InputMapping", "NOWA_B_FLASH_LIGHT", "Flash light button.");

		AddClassToCollection("InputMapping", "NOWA_A_UP", "Mapped up action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_DOWN", "Mapped down action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_LEFT", "Mapped left action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_RIGHT", "Mapped right action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_JUMP", "Mapped jump action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_RUN", "Mapped run action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_COWER", "Mapped cower action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_DUCK", "Mapped duck action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_SNEAK", "Mapped sneak action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_ACTION", "Mapped action like door open (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_ATTACK_1", "Mapped attack 1 action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_ATTACK_2", "Mapped attack 2 action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_RELOAD", "Mapped reload action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_INVENTORY", "Mapped inventory action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_SAVE", "Mapped save action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_LOAD", "Mapped load action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_PAUSE", "Mapped pause action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_MENU", "Mapped menu action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_MAP", "Mapped map action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_CAMERA_FORWARD", "Mapped camera forward action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_CAMERA_BACKWARD", "Mapped camera backward action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_CAMERA_RIGHT", "Mapped camera right action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_CAMERA_UP", "Mapped camera up action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_CAMERA_DOWN", "Mapped camera down action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_CAMERA_LEFT", "Mapped camera left action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_CONSOLE", "Mapped console action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_WEAPON_CHANGE_FORWARD", "Mapped weapon change forward action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_WEAPON_CHANGE_BACKWARD", "Mapped weapon change backward action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_FLASH_LIGHT", "Mapped flash light action (does not matter if keyboard or joystick is used).");
		AddClassToCollection("InputMapping", "NOWA_A_SELECT", "Mapped select action (does not matter if keyboard or joystick is used).");
	}

	void bindMovableObject(lua_State* lua)
	{
		module(lua)
			[
				class_<MovableObject>("MovableObject")
				.def("getParentSceneNode", &MovableObject::getParentSceneNode)
			// .def("detachFromParent", &MovableObject::detachFromParent)
			.def("setVisible", &MovableObject::setVisible)
			.def("getVisible", &MovableObject::getVisible)
			// .def("getCastShadows", &MovableObject::getCastShadows)
			// .def("setCastShadows", &MovableObject::setCastShadows)
			];

		AddClassToCollection("MovableObject", "class", "Base class for Ogre objects.");
		AddClassToCollection("MovableObject", "SceneNode getParentSceneNode()", "Gets the parent scene node, at which this movable object is attached to.");
		AddClassToCollection("MovableObject", "void setVisible(bool visible)", "Switches the visibility of the movable object.");
		AddClassToCollection("MovableObject", "bool getVisible()", "Gets whether this movable object is visible.");
	}

	//bind Ogre::v1::Entity
	//NOTE: not all functions are binded
	void bindEntity(lua_State* lua)
	{
		module(lua)
			[
				class_<v1::Entity, MovableObject>("Entity")
				.def("setVisible", &v1::Entity::setVisible)
			.def("getVisible", &v1::Entity::getVisible)
			.def("getCastShadows", &v1::Entity::getCastShadows)
			// .def("setMaterialName", &v1::Entity::setMaterialName)		//HACK : LuaBind may not support the optional arg here
			.def("getName", &v1::Entity::getName)
			];
		AddClassToCollection("Entity", "class", "Base class for an Ogre mesh object.");
		AddClassToCollection("Entity", "void setVisible(bool visible)", "Sets the entity visible (rendered) or not.");
		AddClassToCollection("Entity", "bool getVisible()", "Gets whether the entity is visible or not.");
		AddClassToCollection("Entity", "bool getCastShadows()", "Gets whether shadows are casted or not.");
		AddClassToCollection("Entity", "string getName()", "Gets the name of the entity.");

		// has protected constructor and destructor so its not possible to create that here
		//module(lua)
		//[
		//	class_<SubEntity>("SubEntity")
		//	.def("getMaterialName", &SubEntity::getMaterialName)
		//	.def("setMaterialName", (void (SubEntity::*)(const Ogre::String&, const Ogre::String&)) &SubEntity::setMaterialName)
		//	.def("getParent", &SubEntity::getParent)
		//	// .def("getSubMesh", &SubEntity::getSubMesh)
		//	.def("getCastsShadows", &SubEntity::getCastsShadows)
		//];
	}

	// Fake member function for simplifing binding, as the real functions 
	// have optional aguments, which I don't want to use in the Lua script.
	// However luabind does not support optional arguments.
	// Think of "obj" as "this"
	SceneNode* createChildSceneNode(SceneNode* obj, const String name)
	{
		Ogre::SceneNode* childNode = obj->createChildSceneNode();
		childNode->setName(name);
		return childNode;
	}

	void setDirection(SceneNode* obj, const Ogre::Vector3& direction)
	{
		obj->setDirection(direction);
	}

	void bindSceneNode(lua_State* lua)
	{
		module(lua)
			[
				class_<Node>("Node")
				.enum_("TransformSpace")
			[
				value("TS_LOCAL", Ogre::Node::TS_LOCAL),
				value("TS_PARENT", Ogre::Node::TS_PARENT),
				value("TS_WORLD", Ogre::Node::TS_WORLD)
			]
			];

		module(lua)
			[
				class_<SceneNode, Node>("SceneNode")
				.def("createChildSceneNode", &createChildSceneNode)
			.def("attachObject", &SceneNode::attachObject)
			.def("setPosition", (void (SceneNode::*)(const Vector3&)) & SceneNode::setPosition)
			.def("setPosition2", (void (SceneNode::*)(Real, Real, Real)) & SceneNode::setPosition)
			.def("translate", (void (SceneNode::*)(const Vector3&, Ogre::Node::TransformSpace)) & SceneNode::translate)
			.def("translate", (void (SceneNode::*)(Real, Real, Real, Ogre::Node::TransformSpace)) & SceneNode::translate)
			.def("setDirection", (void (SceneNode::*)(const Vector3&, Ogre::Node::TransformSpace, const Vector3&)) & SceneNode::setDirection)
			.def("setDirection2", (void (SceneNode::*)(Real, Real, Real, Ogre::Node::TransformSpace, const Vector3&)) & SceneNode::setDirection)
			.def("setScale", (void (SceneNode::*)(const Vector3&)) & SceneNode::setScale)
			.def("setScale2", (void (SceneNode::*)(Real, Real, Real)) & SceneNode::setScale)
			.def("scale", (void (SceneNode::*)(const Vector3&)) & SceneNode::scale)
			.def("scale2", (void (SceneNode::*)(Real, Real, Real)) & SceneNode::scale)
			.def("getScale", &SceneNode::getScale)
			// .def("setDirection", (void (SceneNode::*)(SceneNode::*, const Vector3&)) &setDirection)
			.def("getPosition", &SceneNode::getPosition)
			.def("setOrientation", (void (SceneNode::*)(Quaternion)) & SceneNode::setOrientation)
			.def("setOrientation2", (void (SceneNode::*)(Real, Real, Real, Real)) & SceneNode::setOrientation)
			.def("rotate", (void (SceneNode::*)(const Vector3&, const Radian&, Ogre::Node::TransformSpace)) & SceneNode::rotate)
			.def("rotate2", (void (SceneNode::*)(const Quaternion&, Ogre::Node::TransformSpace)) & SceneNode::rotate)
			.def("roll", &SceneNode::roll)
			.def("pitch", &SceneNode::pitch)
			.def("yaw", &SceneNode::yaw)
			.def("getOrientation", &SceneNode::getOrientation)
			.def("getParentSceneNode", &SceneNode::getParentSceneNode)
			.def("convertLocalToWorldPosition", &SceneNode::convertLocalToWorldPosition)
			.def("convertLocalToWorldOrientation", &SceneNode::convertLocalToWorldOrientation)
			.def("convertWorldToLocalPosition", &SceneNode::convertWorldToLocalPosition)
			.def("convertWorldToLocalOrientation", &SceneNode::convertWorldToLocalOrientation)
			// .def("detachAllObjects", &SceneNode::detachAllObjects)
			// .def("detachObject", (void (SceneNode::*)(MovableObject* ))&SceneNode::detachObject)
			.def("lookAt", &SceneNode::lookAt)
			.def("setVisible", &SceneNode::setVisible)
			];
		AddClassToCollection("SceneNode", "class", "Class for managing scene graph hierarchies.");
		AddClassToCollection("SceneNode", "SceneNode createChildSceneNode()", "Creates a child scene node of this node.");
		AddClassToCollection("SceneNode", "void attachObject(MovableObject movableObject)", "Attaches a movable object to this node. Object will be rendered.");
		AddClassToCollection("SceneNode", "void setPosition(Vector3 vector)", "Sets the position of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void setPosition2(float x, float y, float z)", "Sets the position of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void translate(Vector3 vector)", "Translates the node by the given vector from its current position. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void translate2(float x, float y, float z)", "Translates the node by the given vector from its current position. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void setDirection(Vector3 vector, TransformSpace space)", "Sets the direction of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void setDirection2(float x, float y, float z, TransformSpace space)", "Sets the direction of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void setScale(Vector3 vector)", "Scales the scene node absolute and all its attached objects.");
		AddClassToCollection("SceneNode", "void setScale2(float x, float y, float z)", "Scales the scene node absolute and all its attached objects.");
		AddClassToCollection("SceneNode", "void scale(Vector3 vector)", "Scales the scene node relative and all its attached objects.");
		AddClassToCollection("SceneNode", "void scale2(float x, float y, float z)", "Scales the scene node relative and all its attached objects.");
		AddClassToCollection("SceneNode", "void setOrientation(Quaternion quaternion)", "Sets the absolute orientation of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void setOrientation2(float w, float x, float y, float z)", "Sets the absolute orientation of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void rotate(Quaternion quaternion)", "Rotates the node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void rotate2(float w, float x, float y, float z)", "Rotates the node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void roll(Vector vector)", "Rolls this node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void pitch(Vector vector)", "Pitches this node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "void yaw(Vector vector)", "Yaws this node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");
		AddClassToCollection("SceneNode", "Vector3 getPosition()", "Gets the position of this node.");
		AddClassToCollection("SceneNode", "Vector3 getScale()", "Gets the scale of this node.");
		AddClassToCollection("SceneNode", "Quaternion getOrientation()", "Gets the Orientation of this node.");
		AddClassToCollection("SceneNode", "SceneNode getParentSceneNode()", "Gets the parent scene node. If it does not exist, nil will be delivered.");
		AddClassToCollection("SceneNode", "Vector3 convertLocalToWorldPosition(Vector3 localPosition)", "Gets the world position of a point in the Node local space useful for simple transforms that don't require a child Node.");
		AddClassToCollection("SceneNode", "Quaternion convertLocalToWorldOrientation(Quaternion localOrientation)", "Gets the world orientation of an orientation in the Node local space useful for simple transforms that don't require a child Node.");
		AddClassToCollection("SceneNode", "Vector3 convertWorldToLocalPosition(Vector3 worldPosition)", "Gets the local position of a point in the Node world space.");
		AddClassToCollection("SceneNode", "Quaternion convertWorldToLocalOrientation(Quaternion worldOrientation)", "Gets the local orientation of a point in the Node world space.");
		AddClassToCollection("SceneNode", "void lookAt(Vector3 vector)", "Looks at the given vector. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.");


		object globalVars = globals(lua);
		globalVars["TS_LOCAL"] = Ogre::Node::TS_LOCAL;
		globalVars["TS_PARENT"] = Ogre::Node::TS_PARENT;
		globalVars["TS_WORLD"] = Ogre::Node::TS_WORLD;
		AddClassToCollection("Node", "class", "Base class for SceneNodes.");
		AddClassToCollection("TransformSpace", "TS_LOCAL", "Local transform space.");
		AddClassToCollection("TransformSpace", "TS_PARENT", "Parent transform space.");
		AddClassToCollection("TransformSpace", "TS_WORLD", "World transform space.");
	}

	//dummy function to wrap up Ogre::SceneManager::createEntity
	//think of obj as self
	Ogre::v1::Entity* createEntity(Ogre::SceneManager* sceneManager, const Ogre::String& entityName, const Ogre::String& meshName)
	{
		return sceneManager->createEntity(entityName, meshName);
	}

	void setOldNodeDirection(Ogre::v1::OldNode* instance, const Ogre::Vector3& direction, const Vector3& localDirectionVector)
	{
		// The direction we want the local direction point to
		Ogre::Vector3 targetDir = direction.normalisedCopy();
		if (nullptr != instance->getParent())
		{
			targetDir = instance->getParent()->_getDerivedOrientation() * targetDir;
		}
		const Ogre::Quaternion& currentOrient = instance->_getDerivedOrientation();

		Ogre::Quaternion targetOrientation;

		// Get current local direction relative to world space
		Ogre::Vector3 currentDir = currentOrient * localDirectionVector;

		if ((currentDir + targetDir).squaredLength() < 0.00005f)
		{
			// Oops, a 180 degree turn (infinite possible rotation axes)
			// Default to yaw i.e. use current UP
			targetOrientation = Ogre::Quaternion(-currentOrient.y, -currentOrient.z, currentOrient.w, currentOrient.x);
		}
		else
		{
			// Derive shortest arc to new direction
			Ogre::Quaternion rotQuat = currentDir.getRotationTo(targetDir);
			targetOrientation = rotQuat * currentOrient;
		}
		instance->setOrientation(targetOrientation);
	}

	void bindSceneManager(lua_State* lua)
	{
		module(lua)
			[
				class_<Ogre::SceneManager>("SceneManager")
				//destroy funcs
				// .def("clearScene", &SceneManager::clearScene)
				// .def("destroyAllCameras", &SceneManager::destroyAllCameras)
				// .def("destroyAllEntities", &SceneManager::destroyAllEntities)
				// .def("destroyCamera", (void(SceneManager::*)(Camera*))&SceneManager::destroyCamera)
				// .def("destroyCamera", (void(SceneManager::*)(const String&))&SceneManager::destroyCamera)
				// .def("destroyEntity", (void(SceneManager::*)(const String&)) &SceneManager::destroyEntity)
				// .def("destroyEntity", (void(Ogre::SceneManager::*)(Ogre::v1::Entity*))&Ogre::SceneManager::destroyEntity)
				//creators & getters


				// .def("getSceneNode", (Ogre::SceneNode*(SceneManager::*)(IdType)) &SceneManager::getSceneNode)
				// .def("getRootSceneNode", &SceneManager::getRootSceneNode)
				// .def("createCamera", &SceneManager::createCamera)
				// .def("getCamera", &SceneManager::getCamera)
				// .def("createEntity", &createEntity)
				// .def("getEntity", &SceneManager::getEntity)
			];

		AddClassToCollection("SceneManager", "class", "Main class for all Ogre specifig functions.");

		module(lua)
			[
				class_<Ogre::v1::OldNode>("OldNode")
				.enum_("TransformSpace")
			[
				value("TS_LOCAL", Ogre::v1::OldNode::TS_LOCAL),
				value("TS_PARENT", Ogre::v1::OldNode::TS_PARENT),
				value("TS_WORLD", Ogre::v1::OldNode::TS_WORLD)
			]
		.def("getPosition", &Ogre::v1::OldNode::getPosition)
			.def("getOrientation", &Ogre::v1::OldNode::getOrientation)
			.def("setDirection", &setOldNodeDirection)
			.def("getDerivedPosition", &Ogre::v1::OldNode::_getDerivedPosition)
			.def("getDerivedOrientation", &Ogre::v1::OldNode::_getDerivedOrientation)
			.def("setPosition", (void (Ogre::v1::OldNode::*)(const Vector3&)) & Ogre::v1::OldNode::setPosition)
			.def("setPosition2", (void (Ogre::v1::OldNode::*)(Real, Real, Real)) & Ogre::v1::OldNode::setPosition)
			.def("setDerivedPosition", &Ogre::v1::OldNode::_setDerivedPosition)
			.def("setDerivedOrientation", &Ogre::v1::OldNode::_setDerivedOrientation)
			.def("setOrientation", (void (Ogre::v1::OldNode::*)(const Quaternion&)) & Ogre::v1::OldBone::setOrientation)
			.def("setOrientation2", (void (Ogre::v1::OldNode::*)(Real, Real, Real, Real)) & Ogre::v1::OldBone::setOrientation)
			.def("roll", &Ogre::v1::OldNode::roll)
			.def("pitch", &Ogre::v1::OldNode::pitch)
			.def("yaw", &Ogre::v1::OldNode::yaw)
			.def("translate", (void (Ogre::v1::OldNode::*)(const Vector3&, Ogre::v1::OldNode::TransformSpace)) & Ogre::v1::OldNode::translate)
			.def("translate2", (void (Ogre::v1::OldNode::*)(Real, Real, Real, Ogre::v1::OldNode::TransformSpace)) & Ogre::v1::OldNode::translate)
			.def("rotate", (void (Ogre::v1::OldNode::*)(const Vector3&, const Radian&, Ogre::v1::OldNode::TransformSpace)) & Ogre::v1::OldNode::rotate)
			.def("rotate2", (void (Ogre::v1::OldNode::*)(const Quaternion&, Ogre::v1::OldNode::TransformSpace)) & Ogre::v1::OldNode::rotate)
			];

		AddClassToCollection("OldNode", "class", "Class for managing a v1 skeleton node.");
		AddClassToCollection("OldNode", "Vector3 getPosition()", "Gets the position of this v1 skeleton node.");
		AddClassToCollection("OldNode", "Quaternion getOrientation()", "Gets the Orientation of this v1 skeleton node.");
		AddClassToCollection("OldNode", "void setDirection(Vector3 vector, Vector3 localDirectionVector)", "Sets the direction of this v1 skeleton node.");
		AddClassToCollection("OldNode", "Vector3 getPositionDerived()", "Gets the derived position of this v1 skeleton node.");
		AddClassToCollection("OldNode", "Quaternion getOrientationDerived()", "Gets the derived Orientation of this v1 skeleton node.");
		AddClassToCollection("OldNode", "void setPosition(Vector3 vector)", "Sets the position of this v1 skeleton node.");
		AddClassToCollection("OldNode", "void setPosition2(float x, float y, float z)", "Sets the position of this v1 skeleton node.");
		AddClassToCollection("OldNode", "void setDerivedPosition(Vector3 vector)", "Sets the derived position of this v1 skeleton node.");
		AddClassToCollection("OldNode", "void setDerivedOrientation(Quaternion quaternion)", "Sets the derived orientation of this v1 skeleton node.");
		AddClassToCollection("OldNode", "void setOrientation(Quaternion quaternion)", "Sets the absolute orientation of this v1 skeleton node.");
		AddClassToCollection("OldNode", "void setOrientation(float w, float x, float y, float z)", "Sets the absolute orientation of this v1 skeleton node.");
		AddClassToCollection("OldNode", "void roll(Vector vector)", "Rolls this v1 skeleton node relative.");
		AddClassToCollection("OldNode", "void pitch(Vector vector)", "Pitches this v1 skeleton node relative.");
		AddClassToCollection("OldNode", "void yaw(Vector vector)", "Yaws this v1 skeleton node relative.");
		AddClassToCollection("OldNode", "void translate(Vector3 vector)", "Translates the v1 skeleton node by the given vector from its current position.");
		AddClassToCollection("OldNode", "void translate2(float x, float y, float z)", "Translates the v1 skeleton node by the given vector from its current position.");
		AddClassToCollection("OldNode", "void rotate(Quaternion quaternion)", "Rotates the v1 skeleton node relative.");
		AddClassToCollection("OldNode", "void rotate2(float w, float x, float y, float z)", "Rotates the v1 skeleton node relative.");

		module(lua)
			[
				class_<Ogre::v1::Skeleton>("Skeleton")
				.def("setBindingPose", &Ogre::v1::Skeleton::setBindingPose)
			.def("reset", &Ogre::v1::Skeleton::reset)
			];
		// ??

		module(lua)
			[
				class_<Ogre::v1::OldBone, Ogre::v1::OldNode>("OldBone")
				.def("setManuallyControlled", &Ogre::v1::OldBone::setManuallyControlled)
			.def("setInheritOrientation", &Ogre::v1::OldBone::setInheritOrientation)
			.def("reset", &Ogre::v1::OldBone::reset)
			// .def("resetToInitialState", &Ogre::v1::OldBone::resetToInitialState)
			// .def("setInitialState", &Ogre::v1::OldBone::setInitialState)
			.def("convertLocalToWorldPosition", &Ogre::v1::OldBone::convertLocalToWorldPosition)
			.def("convertWorldToLocalPosition", &Ogre::v1::OldBone::convertWorldToLocalPosition)
			.def("convertLocalToWorldOrientation", &Ogre::v1::OldBone::convertLocalToWorldOrientation)
			.def("convertWorldToLocalOrientation", &Ogre::v1::OldBone::convertWorldToLocalOrientation)
			.def("getParent", &Ogre::v1::OldBone::getParent)
			// .def("getBindingPoseInversePosition", &Ogre::v1::OldBone::_getBindingPoseInversePosition)
			// .def("getBindingPoseInverseOrientation", &Ogre::v1::OldBone::_getBindingPoseInverseOrientation)
			// .def("getBindingPoseInverseScale", &Ogre::v1::OldBone::_getBindingPoseInverseScale)
			];

		AddClassToCollection("OldBone", "class", "Class for managing a v1 skeleton bone.");
		AddClassToCollection("OldBone", "void setManuallyControlled(bool manuallyControlled)", "Sets whether this v1 old bone should be controlled manually. Note: Animation will no longer transform this v1 old bone.");
		AddClassToCollection("OldBone", "void setInheritOrientation(bool inherit)", "Tells the OldNode whether it should inherit orientation from it's parent OldNode.");
		AddClassToCollection("OldBone", "Vector3 convertLocalToWorldPosition(Vector3 worldPosition)", "Gets the world position of a point in the OldNode local space useful for simple transforms that don't require a child OldNode.");
		AddClassToCollection("OldBone", "Vector3 convertLocalToWorldPosition(Vector3 localPosition)", "Gets the world position of a point in the OldNode local space useful for simple transforms that don't require a child OldNode.");
		AddClassToCollection("OldBone", "Quaternion convertWorldToLocalOrientation(Quaternion worldOrientation)", "Gets the local orientation, relative to this OldNode, of the given world-space orientation.");
		AddClassToCollection("OldBone", "Quaternion convertLocalToWorldOrientation(Quaternion localOrientation)", "Gets the world orientation of an orientation in the OldNode local space useful for simple transforms that don't require a child OldNode.");
		AddClassToCollection("OldBone", "OldNode getParent()", "Gets this OldNode's parent (nil if this is the root).");

	}

	// TODO: Fix the case in the names of Application's members.
	//void bindApplication( lua_State* L )
	//{
	//	module(L)
	//	[
	//		class_<Application>("Application")
	//		.def( "setBackgroundColour", Application::SetBackgroundColour )
	//		.def( "createEntity", Application::CreateEntity )
	//		.def( "getRootNode", Application::GetRootNode )
	//		.def( "getCamera", Application::GetCamera )
	//	];
	//
	//	module(L)
	//	[
	//		def( "getApplication", getApplication )
	//	];
	//}

	void bindCamera(lua_State* lua)
	{
		module(lua)
			[
				class_<Camera>("Camera")
				.def("setPosition", (void (Camera::*)(const Vector3&)) & Camera::setPosition)
			.def("setPosition", (void (Camera::*)(Real, Real, Real)) & Camera::setPosition)
			.def("lookAt", (void (Camera::*)(const Vector3&)) & Camera::lookAt)
			.def("lookAt2", (void (Camera::*)(Real, Real, Real)) & Camera::lookAt)
			.def("setNearClipDistance", &Camera::setNearClipDistance)
			.def("setFarClipDistance", &Camera::setFarClipDistance)
			.def("move", &Camera::move)
			.def("moveRelative", &Camera::moveRelative)
			.def("setDirection2", (void (Camera::*)(const Vector3&)) & Camera::setDirection)
			.def("setDirection", (void (Camera::*)(Real, Real, Real)) & Camera::setDirection)
			.def("getDirection", &Camera::getDirection)
			.def("rotate", (void (SceneNode::*)(const Vector3&, const Radian&)) & Camera::rotate)
			.def("rotate2", (void (SceneNode::*)(const Quaternion&)) & Camera::rotate)
			.def("setOrientation", &Camera::setOrientation)
			.def("getOrientation", &Camera::getOrientation)
			];

		AddClassToCollection("Camera", "class", "A viewpoint from which the scene will be rendered.");
		AddClassToCollection("Camera", "void setPosition(Vector3 vector)", "Sets the position of the camera.");
		AddClassToCollection("Camera", "void setPosition(float x, float y, float z)", "Sets the position of the camera.");
		AddClassToCollection("Camera", "void lookAt(Vector3 vector)", "Looks at the given vector.");
		AddClassToCollection("Camera", "void lookAt2(float x, float y, float z)", "Looks at the given vector.");
		AddClassToCollection("Camera", "void setNearClipDistance(float distance)", "Sets the near clip distance at which the scene is being rendered.");
		AddClassToCollection("Camera", "void setFarClipDistance(float distance)", "Sets the far clip distance until which the scene is being rendered.");
		AddClassToCollection("Camera", "void move(Vector3 vector)", "?");
		AddClassToCollection("Camera", "void moveRelative(Vector3 vector)", "?");
		AddClassToCollection("Camera", "void setDirection(Vector3 vector)", "Sets at which direction vector the camera should be rotated.");
		AddClassToCollection("Camera", "void setDirection2(float x, float y, float z)", "Sets at which direction vector the camera should be rotated.");
		AddClassToCollection("Camera", "Vector3 getDirection()", "Gets the direction of the camera.");
		AddClassToCollection("Camera", "void rotate(Vector3 vector, Radian radian)", "Rotates the camera relative by the given radian and the given vector axis.");
		AddClassToCollection("Camera", "void rotate2(Quaternion quaternion)", "Rotates the camera relative by the given quaternion.");
		AddClassToCollection("Camera", "void setOrientation(Quaternion quaternion)", "Sets the absolute orientation of the camera.");
		AddClassToCollection("Camera", "Quaternion getOrientation()", "Gets the absolute orientation of the camera.");
	}

	// Note: its not possible to get a shared_ptr to bind for lua
	// Note: Really important, no shared ptr to lua, else game objects/components are deleted when lua script module is destroyed, which is really late and game object controller is long gone, which causes ugly crash
	GameObjectComponent* getGameObjectComponentByIndex(GameObject* gameObject, unsigned int index)
	{
		return makeStrongPtr<GameObjectComponent>(gameObject->getComponentByIndex(index)).get();
	}

	AnimationComponent* getAnimationComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<AnimationComponent>(gameObject->getComponentWithOccurrence<AnimationComponent>(occurrenceIndex)).get();
	}

	AnimationComponent* getAnimationComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AnimationComponent>(gameObject->getComponent<AnimationComponent>()).get();
	}

	AttributesComponent* getAttributesComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AttributesComponent>(gameObject->getComponent<AttributesComponent>()).get();
	}

	AttributeEffectComponent* getAttributeEffectComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<AttributeEffectComponent>(gameObject->getComponentWithOccurrence<AttributeEffectComponent>(occurrenceIndex)).get();
	}

	AttributeEffectComponent* getAttributeEffectComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AttributeEffectComponent>(gameObject->getComponent<AttributeEffectComponent>()).get();
	}

	PhysicsBuoyancyComponent* getPhysicsBuoyancyComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsBuoyancyComponent>(gameObject->getComponent<PhysicsBuoyancyComponent>()).get();
	}

	PhysicsTriggerComponent* getPhysicsTriggerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsTriggerComponent>(gameObject->getComponent<PhysicsTriggerComponent>()).get();
	}

	DistributedComponent* getDistributedComponent(GameObject* gameObject)
	{
		return makeStrongPtr<DistributedComponent>(gameObject->getComponent<DistributedComponent>()).get();
	}

	ExitComponent* getExitComponent(GameObject* gameObject)
	{
		return makeStrongPtr<ExitComponent>(gameObject->getComponent<ExitComponent>()).get();
	}

	GameObjectTitleComponent* getGameObjectTitleComponent(GameObject* gameObject)
	{
		return makeStrongPtr<GameObjectTitleComponent>(gameObject->getComponent<GameObjectTitleComponent>()).get();
	}

	NavMeshComponent* getNavMeshComponent(GameObject* gameObject)
	{
		return makeStrongPtr<NavMeshComponent>(gameObject->getComponent<NavMeshComponent>()).get();
	}

	ParticleUniverseComponent* getParticleUniverseComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<ParticleUniverseComponent>(gameObject->getComponentWithOccurrence<ParticleUniverseComponent>(occurrenceIndex)).get();
	}

	ParticleUniverseComponent* getParticleUniverseComponent(GameObject* gameObject)
	{
		return makeStrongPtr<ParticleUniverseComponent>(gameObject->getComponent<ParticleUniverseComponent>()).get();
	}

	PlayerControllerJumpNRunComponent* getPlayerControllerJumpNRunComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PlayerControllerJumpNRunComponent>(gameObject->getComponent<PlayerControllerJumpNRunComponent>()).get();
	}

	PlayerControllerJumpNRunLuaComponent* getPlayerControllerJumpNRunLuaComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PlayerControllerJumpNRunLuaComponent>(gameObject->getComponent<PlayerControllerJumpNRunLuaComponent>()).get();
	}

	PlayerControllerClickToPointComponent* getPlayerControllerClickToPointComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PlayerControllerClickToPointComponent>(gameObject->getComponent<PlayerControllerClickToPointComponent>()).get();
	}

	PhysicsActiveComponent* getPhysicsActiveComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsActiveComponent>(gameObject->getComponent<PhysicsActiveComponent>()).get();
	}

	PhysicsComponent* getPhysicsComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsComponent>(gameObject->getComponent<PhysicsComponent>()).get();
	}

	PhysicsActiveCompoundComponent* getPhysicsActiveCompoundComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsActiveCompoundComponent>(gameObject->getComponent<PhysicsActiveCompoundComponent>()).get();
	}

	PhysicsActiveDestructableComponent* getPhysicsActiveDestructableComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsActiveDestructableComponent>(gameObject->getComponent<PhysicsActiveDestructableComponent>()).get();
	}

	PhysicsActiveKinematicComponent* getPhysicsActiveKinematicComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsActiveKinematicComponent>(gameObject->getComponent<PhysicsActiveKinematicComponent>()).get();
	}

	PhysicsArtifactComponent* getPhysicsArtifactComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsArtifactComponent>(gameObject->getComponent<PhysicsArtifactComponent>()).get();
	}

	PhysicsRagDollComponent* getPhysicsRagDollComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsRagDollComponent>(gameObject->getComponent<PhysicsRagDollComponent>()).get();
	}

	PhysicsCompoundConnectionComponent* getPhysicsCompoundConnectionComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsCompoundConnectionComponent>(gameObject->getComponent<PhysicsCompoundConnectionComponent>()).get();
	}

	PhysicsMaterialComponent* getPhysicsMaterialComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<PhysicsMaterialComponent>(gameObject->getComponentWithOccurrence<PhysicsMaterialComponent>(occurrenceIndex)).get();
	}

	PhysicsMaterialComponent* getPhysicsMaterialComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsMaterialComponent>(gameObject->getComponent<PhysicsMaterialComponent>()).get();
	}

	PhysicsPlayerControllerComponent* getPhysicsPlayerControllerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsPlayerControllerComponent>(gameObject->getComponent<PhysicsPlayerControllerComponent>()).get();
	}

	PlaneComponent* getPlaneComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PlaneComponent>(gameObject->getComponent<PlaneComponent>()).get();
	}

	/*PhysicsMathSliderComponent* getPhysicsMathSliderComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsMathSliderComponent>(gameObject->getComponent<PhysicsMathSliderComponent>()).get();
	}*/

	/*PhysicsTerrainComponent* getPhysicsTerrainComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsTerrainComponent>(gameObject->getComponent<PhysicsTerrainComponent>()).get();
	}
*/
	SimpleSoundComponent* getSimpleSoundComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<SimpleSoundComponent>(gameObject->getComponentWithOccurrence<SimpleSoundComponent>(occurrenceIndex)).get();
	}

	SimpleSoundComponent* getSimpleSoundComponent(GameObject* gameObject)
	{
		return makeStrongPtr<SimpleSoundComponent>(gameObject->getComponent<SimpleSoundComponent>()).get();
	}

	SoundComponent* getSoundComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<SoundComponent>(gameObject->getComponentWithOccurrence<SoundComponent>(occurrenceIndex)).get();
	}

	SoundComponent* getSoundComponent(GameObject* gameObject)
	{
		return makeStrongPtr<SoundComponent>(gameObject->getComponent<SoundComponent>()).get();
	}

	SpawnComponent* getSpawnComponent(GameObject* gameObject)
	{
		return makeStrongPtr<SpawnComponent>(gameObject->getComponent<SpawnComponent>()).get();
	}

	VehicleComponent* getVehicleComponent(GameObject* gameObject)
	{
		return makeStrongPtr<VehicleComponent>(gameObject->getComponent<VehicleComponent>()).get();
	}

	AiLuaComponent* getAiLuaComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiLuaComponent>(gameObject->getComponent<AiLuaComponent>()).get();
	}

	PhysicsExplosionComponent* getPhysicsExplosionComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsExplosionComponent>(gameObject->getComponent<PhysicsExplosionComponent>()).get();
	}

	CameraComponent* getCameraComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CameraComponent>(gameObject->getComponent<CameraComponent>()).get();
	}

	CameraBehaviorBaseComponent* getCameraBehaviorBaseComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CameraBehaviorBaseComponent>(gameObject->getComponent<CameraBehaviorBaseComponent>()).get();
	}

	CameraBehaviorFirstPersonComponent* getCameraBehaviorFirstPersonComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CameraBehaviorFirstPersonComponent>(gameObject->getComponent<CameraBehaviorFirstPersonComponent>()).get();
	}

	CameraBehaviorThirdPersonComponent* getCameraBehaviorThirdPersonComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CameraBehaviorThirdPersonComponent>(gameObject->getComponent<CameraBehaviorThirdPersonComponent>()).get();
	}

	CameraBehaviorFollow2DComponent* getCameraBehaviorFollow2DComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CameraBehaviorFollow2DComponent>(gameObject->getComponent<CameraBehaviorFollow2DComponent>()).get();
	}

	CameraBehaviorZoomComponent* getCameraBehaviorZoomComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CameraBehaviorZoomComponent>(gameObject->getComponent<CameraBehaviorZoomComponent>()).get();
	}

	CompositorEffectBloomComponent* getCompositorEffectBloomComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CompositorEffectBloomComponent>(gameObject->getComponent<CompositorEffectBloomComponent>()).get();
	}

	CompositorEffectGlassComponent* getCompositorEffectGlassComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CompositorEffectGlassComponent>(gameObject->getComponent<CompositorEffectGlassComponent>()).get();
	}

	CompositorEffectOldTvComponent* getCompositorEffectOldTvComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CompositorEffectOldTvComponent>(gameObject->getComponent<CompositorEffectOldTvComponent>()).get();
	}

	CompositorEffectKeyholeComponent* getCompositorEffectKeyholeComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CompositorEffectKeyholeComponent>(gameObject->getComponent<CompositorEffectKeyholeComponent>()).get();
	}

	CompositorEffectBlackAndWhiteComponent* getCompositorEffectBlackAndWhiteComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CompositorEffectBlackAndWhiteComponent>(gameObject->getComponent<CompositorEffectBlackAndWhiteComponent>()).get();
	}

	CompositorEffectColorComponent* getCompositorEffectColorComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CompositorEffectColorComponent>(gameObject->getComponent<CompositorEffectColorComponent>()).get();
	}

	CompositorEffectEmbossedComponent* getCompositorEffectEmbossedComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CompositorEffectEmbossedComponent>(gameObject->getComponent<CompositorEffectEmbossedComponent>()).get();
	}

	CompositorEffectSharpenEdgesComponent* getCompositorEffectSharpenEdgesComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CompositorEffectSharpenEdgesComponent>(gameObject->getComponent<CompositorEffectSharpenEdgesComponent>()).get();
	}

	HdrEffectComponent* getHdrEffectComponent(GameObject* gameObject)
	{
		return makeStrongPtr<HdrEffectComponent>(gameObject->getComponent<HdrEffectComponent>()).get();
	}

	AiMoveComponent* getAiMoveComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiMoveComponent>(gameObject->getComponent<AiMoveComponent>()).get();
	}

	AiMoveRandomlyComponent* getAiMoveRandomlyComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiMoveRandomlyComponent>(gameObject->getComponent<AiMoveRandomlyComponent>()).get();
	}

	AiPathFollowComponent* getAiPathFollowComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiPathFollowComponent>(gameObject->getComponent<AiPathFollowComponent>()).get();
	}

	AiWanderComponent* getAiWanderComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiWanderComponent>(gameObject->getComponent<AiWanderComponent>()).get();
	}

	AiFlockingComponent* getAiFlockingComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiFlockingComponent>(gameObject->getComponent<AiFlockingComponent>()).get();
	}

	AiRecastPathNavigationComponent* getAiRecastPathNavigationComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiRecastPathNavigationComponent>(gameObject->getComponent<AiRecastPathNavigationComponent>()).get();
	}

	AiObstacleAvoidanceComponent* getAiObstacleAvoidanceComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiObstacleAvoidanceComponent>(gameObject->getComponent<AiObstacleAvoidanceComponent>()).get();
	}

	AiHideComponent* getAiHideComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiHideComponent>(gameObject->getComponent<AiHideComponent>()).get();
	}

	AiMoveComponent2D* getAiMoveComponent2D(GameObject* gameObject)
	{
		return makeStrongPtr<AiMoveComponent2D>(gameObject->getComponent<AiMoveComponent2D>()).get();
	}

	AiPathFollowComponent2D* getAiPathFollowComponent2D(GameObject* gameObject)
	{
		return makeStrongPtr<AiPathFollowComponent2D>(gameObject->getComponent<AiPathFollowComponent2D>()).get();
	}

	AiWanderComponent2D* getAiWanderComponent2D(GameObject* gameObject)
	{
		return makeStrongPtr<AiWanderComponent2D>(gameObject->getComponent<AiWanderComponent2D>()).get();
	}

	DatablockPbsComponent* getDatablockPbsComponent(GameObject* gameObject)
	{
		return makeStrongPtr<DatablockPbsComponent>(gameObject->getComponent<DatablockPbsComponent>()).get();
	}

	DatablockUnlitComponent* getDatablockUnlitComponent(GameObject* gameObject)
	{
		return makeStrongPtr<DatablockUnlitComponent>(gameObject->getComponent<DatablockUnlitComponent>()).get();
	}

	DatablockTerraComponent* getDatablockTerraComponent(GameObject* gameObject)
	{
		return makeStrongPtr<DatablockTerraComponent>(gameObject->getComponent<DatablockTerraComponent>()).get();
	}

	JointComponent* getJointComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointComponent>(gameObject->getComponent<JointComponent>()).get();
	}

	JointHingeComponent* getJointHingeComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointHingeComponent>(gameObject->getComponent<JointHingeComponent>()).get();
	}

	JointHingeActuatorComponent* getJointHingeActuatorComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointHingeActuatorComponent>(gameObject->getComponent<JointHingeActuatorComponent>()).get();
	}

	JointBallAndSocketComponent* getJointBallAndSocketComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointBallAndSocketComponent>(gameObject->getComponent<JointBallAndSocketComponent>()).get();
	}

	JointPointToPointComponent* getJointPointToPointComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointPointToPointComponent>(gameObject->getComponent<JointPointToPointComponent>()).get();
	}

	JointPinComponent* getJointPinComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointPinComponent>(gameObject->getComponent<JointPinComponent>()).get();
	}

	JointPlaneComponent* getJointPlaneComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointPlaneComponent>(gameObject->getComponent<JointPlaneComponent>()).get();
	}

	JointSpringComponent* getJointSpringComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointSpringComponent>(gameObject->getComponent<JointSpringComponent>()).get();
	}

	JointAttractorComponent* getJointAttractorComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointAttractorComponent>(gameObject->getComponent<JointAttractorComponent>()).get();
	}

	JointCorkScrewComponent* getJointCorkScrewComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointCorkScrewComponent>(gameObject->getComponent<JointCorkScrewComponent>()).get();
	}

	JointPassiveSliderComponent* getJointPassiveSliderComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointPassiveSliderComponent>(gameObject->getComponent<JointPassiveSliderComponent>()).get();
	}

	JointSliderActuatorComponent* getJointSliderActuatorComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointSliderActuatorComponent>(gameObject->getComponent<JointSliderActuatorComponent>()).get();
	}

	JointSlidingContactComponent* getJointSlidingContactComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointSlidingContactComponent>(gameObject->getComponent<JointSlidingContactComponent>()).get();
	}

	JointActiveSliderComponent* getJointActiveSliderComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointActiveSliderComponent>(gameObject->getComponent<JointActiveSliderComponent>()).get();
	}

	JointMathSliderComponent* getJointMathSliderComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointMathSliderComponent>(gameObject->getComponent<JointMathSliderComponent>()).get();
	}

	JointKinematicComponent* getJointKinematicComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointKinematicComponent>(gameObject->getComponent<JointKinematicComponent>()).get();
	}

	JointDryRollingFrictionComponent* getJointDryRollingFrictionComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointDryRollingFrictionComponent>(gameObject->getComponent<JointDryRollingFrictionComponent>()).get();
	}

	JointPathFollowComponent* getJointPathFollowComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointPathFollowComponent>(gameObject->getComponent<JointPathFollowComponent>()).get();
	}

	JointGearComponent* getJointGearComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointGearComponent>(gameObject->getComponent<JointGearComponent>()).get();
	}

	JointRackAndPinionComponent* getJointRackAndPinionComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointRackAndPinionComponent>(gameObject->getComponent<JointRackAndPinionComponent>()).get();
	}

	JointWormGearComponent* getJointWormGearComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointWormGearComponent>(gameObject->getComponent<JointWormGearComponent>()).get();
	}

	JointPulleyComponent* getJointPulleyComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointPulleyComponent>(gameObject->getComponent<JointPulleyComponent>()).get();
	}

	JointUniversalComponent* getJointUniversalComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointUniversalComponent>(gameObject->getComponent<JointUniversalComponent>()).get();
	}

	JointUniversalActuatorComponent* getJointUniversalActuatorComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointUniversalActuatorComponent>(gameObject->getComponent<JointUniversalActuatorComponent>()).get();
	}

	Joint6DofComponent* getJoint6DofComponent(GameObject* gameObject)
	{
		return makeStrongPtr<Joint6DofComponent>(gameObject->getComponent<Joint6DofComponent>()).get();
	}

	JointMotorComponent* getJointMotorComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointMotorComponent>(gameObject->getComponent<JointMotorComponent>()).get();
	}

	JointWheelComponent* getJointWheelComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointWheelComponent>(gameObject->getComponent<JointWheelComponent>()).get();
	}

	LightDirectionalComponent* getLightDirectionalComponent(GameObject* gameObject)
	{
		return makeStrongPtr<LightDirectionalComponent>(gameObject->getComponent<LightDirectionalComponent>()).get();
	}

	LightPointComponent* getLightPointComponent(GameObject* gameObject)
	{
		return makeStrongPtr<LightPointComponent>(gameObject->getComponent<LightPointComponent>()).get();
	}

	LightSpotComponent* getLightSpotComponent(GameObject* gameObject)
	{
		return makeStrongPtr<LightSpotComponent>(gameObject->getComponent<LightSpotComponent>()).get();
	}

	LightAreaComponent* getLightAreaComponent(GameObject* gameObject)
	{
		return makeStrongPtr<LightAreaComponent>(gameObject->getComponent<LightAreaComponent>()).get();
	}

	FadeComponent* getFadeComponent(GameObject* gameObject)
	{
		return makeStrongPtr<FadeComponent>(gameObject->getComponent<FadeComponent>()).get();
	}

	MeshDecalComponent* getMeshDecalComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MeshDecalComponent>(gameObject->getComponent<MeshDecalComponent>()).get();
	}

	TagPointComponent* getTagPointComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<TagPointComponent>(gameObject->getComponentWithOccurrence<TagPointComponent>(occurrenceIndex)).get();
	}

	TagPointComponent* getTagPointComponent(GameObject* gameObject)
	{
		return makeStrongPtr<TagPointComponent>(gameObject->getComponent<TagPointComponent>()).get();
	}

	TimeTriggerComponent* getTimeTriggerComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<TimeTriggerComponent>(gameObject->getComponentWithOccurrence<TimeTriggerComponent>(occurrenceIndex)).get();
	}

	TimeTriggerComponent* getTimeTriggerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<TimeTriggerComponent>(gameObject->getComponent<TimeTriggerComponent>()).get();
	}

	TimeLineComponent* getTimeLineComponent(GameObject* gameObject)
	{
		return makeStrongPtr<TimeLineComponent>(gameObject->getComponent<TimeLineComponent>()).get();
	}

	MoveMathFunctionComponent* getMoveMathFunctionComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MoveMathFunctionComponent>(gameObject->getComponent<MoveMathFunctionComponent>()).get();
	}

	TagChildNodeComponent* getTagChildNodeComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<TagChildNodeComponent>(gameObject->getComponentWithOccurrence<TagChildNodeComponent>(occurrenceIndex)).get();
	}

	TagChildNodeComponent* getTagChildNodeComponent(GameObject* gameObject)
	{
		return makeStrongPtr<TagChildNodeComponent>(gameObject->getComponent<TagChildNodeComponent>()).get();
	}

	NodeTrackComponent* getNodeTrackComponent(GameObject* gameObject)
	{
		return makeStrongPtr<NodeTrackComponent>(gameObject->getComponent<NodeTrackComponent>()).get();
	}

	LineComponent* getLineComponent(GameObject* gameObject)
	{
		return makeStrongPtr<LineComponent>(gameObject->getComponent<LineComponent>()).get();
	}

	LinesComponent* getLinesComponent(GameObject* gameObject)
	{
		return makeStrongPtr<LinesComponent>(gameObject->getComponent<LinesComponent>()).get();
	}

	NodeComponent* getNodeComponent(GameObject* gameObject)
	{
		return makeStrongPtr<NodeComponent>(gameObject->getComponent<NodeComponent>()).get();
	}

	ManualObjectComponent* getManualObjectComponent(GameObject* gameObject)
	{
		return makeStrongPtr<ManualObjectComponent>(gameObject->getComponent<ManualObjectComponent>()).get();
	}

	RectangleComponent* getRectangleComponent(GameObject* gameObject)
	{
		return makeStrongPtr<RectangleComponent>(gameObject->getComponent<RectangleComponent>()).get();
	}

	ValueBarComponent* getValueBarComponent(GameObject* gameObject)
	{
		return makeStrongPtr<ValueBarComponent>(gameObject->getComponent<ValueBarComponent>()).get();
	}

	BillboardComponent* getBillboardComponent(GameObject* gameObject)
	{
		return makeStrongPtr<BillboardComponent>(gameObject->getComponent<BillboardComponent>()).get();
	}

	RibbonTrailComponent* getRibbonTrailComponent(GameObject* gameObject)
	{
		return makeStrongPtr<RibbonTrailComponent>(gameObject->getComponent<RibbonTrailComponent>()).get();
	}

	MyGUIWindowComponent* getMyGUIWindowComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIWindowComponent>(gameObject->getComponentWithOccurrence<MyGUIWindowComponent>(occurrenceIndex)).get();
	}

	MyGUITextComponent* getMyGUITextComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUITextComponent>(gameObject->getComponentWithOccurrence<MyGUITextComponent>(occurrenceIndex)).get();
	}

	MyGUIButtonComponent* getMyGUIButtonComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIButtonComponent>(gameObject->getComponentWithOccurrence<MyGUIButtonComponent>(occurrenceIndex)).get();
	}

	MyGUICheckBoxComponent* getMyGUICheckBoxComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUICheckBoxComponent>(gameObject->getComponentWithOccurrence<MyGUICheckBoxComponent>(occurrenceIndex)).get();
	}

	MyGUIImageBoxComponent* getMyGUIImageBoxComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIImageBoxComponent>(gameObject->getComponentWithOccurrence<MyGUIImageBoxComponent>(occurrenceIndex)).get();
	}

	MyGUIProgressBarComponent* getMyGUIProgressBarComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIProgressBarComponent>(gameObject->getComponentWithOccurrence<MyGUIProgressBarComponent>(occurrenceIndex)).get();
	}

	MyGUIComboBoxComponent* getMyGUIComboBoxComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIComboBoxComponent>(gameObject->getComponentWithOccurrence<MyGUIComboBoxComponent>(occurrenceIndex)).get();
	}

	MyGUIListBoxComponent* getMyGUIListBoxComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIListBoxComponent>(gameObject->getComponentWithOccurrence<MyGUIListBoxComponent>(occurrenceIndex)).get();
	}

	MyGUIMessageBoxComponent* getMyGUIMessageBoxComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIMessageBoxComponent>(gameObject->getComponentWithOccurrence<MyGUIMessageBoxComponent>(occurrenceIndex)).get();
	}

	MyGUIPositionControllerComponent* getMyGUIPositionControllerComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIPositionControllerComponent>(gameObject->getComponentWithOccurrence<MyGUIPositionControllerComponent>(occurrenceIndex)).get();
	}

	MyGUIFadeAlphaControllerComponent* getMyGUIFadeAlphaControllerComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIFadeAlphaControllerComponent>(gameObject->getComponentWithOccurrence<MyGUIFadeAlphaControllerComponent>(occurrenceIndex)).get();
	}

	MyGUIScrollingMessageControllerComponent* getMyGUIScrollingMessageControllerComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIScrollingMessageControllerComponent>(gameObject->getComponentWithOccurrence<MyGUIScrollingMessageControllerComponent>(occurrenceIndex)).get();
	}

	MyGUIEdgeHideControllerComponent* getMyGUIEdgeHideControllerComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIEdgeHideControllerComponent>(gameObject->getComponentWithOccurrence<MyGUIEdgeHideControllerComponent>(occurrenceIndex)).get();
	}

	MyGUIRepeatClickControllerComponent* getMMyGUIRepeatClickControllerComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGUIRepeatClickControllerComponent>(gameObject->getComponentWithOccurrence<MyGUIRepeatClickControllerComponent>(occurrenceIndex)).get();
	}

	MyGUIWindowComponent* getMyGUIWindowComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIWindowComponent>(gameObject->getComponent<MyGUIWindowComponent>()).get();
	}

	MyGUIItemBoxComponent* getMyGUIItemBoxComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIItemBoxComponent>(gameObject->getComponent<MyGUIItemBoxComponent>()).get();
	}

	InventoryItemComponent* getInventoryItemComponent(GameObject* gameObject)
	{
		return makeStrongPtr<InventoryItemComponent>(gameObject->getComponent<InventoryItemComponent>()).get();
	}

	MyGUITextComponent* getMyGUITextComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUITextComponent>(gameObject->getComponent<MyGUITextComponent>()).get();
	}

	MyGUIButtonComponent* getMyGUIButtonComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIButtonComponent>(gameObject->getComponent<MyGUIButtonComponent>()).get();
	}

	MyGUICheckBoxComponent* getMyGUICheckBoxComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUICheckBoxComponent>(gameObject->getComponent<MyGUICheckBoxComponent>()).get();
	}

	MyGUIImageBoxComponent* getMyGUIImageBoxComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIImageBoxComponent>(gameObject->getComponent<MyGUIImageBoxComponent>()).get();
	}

	MyGUIProgressBarComponent* getMyGUIProgressBarComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIProgressBarComponent>(gameObject->getComponent<MyGUIProgressBarComponent>()).get();
	}

	MyGUIComboBoxComponent* getMyGUIComboBoxComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIComboBoxComponent>(gameObject->getComponent<MyGUIComboBoxComponent>()).get();
	}

	MyGUIListBoxComponent* getMyGUIListBoxComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIListBoxComponent>(gameObject->getComponent<MyGUIListBoxComponent>()).get();
	}

	MyGUIMessageBoxComponent* getMyGUIMessageBoxComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIMessageBoxComponent>(gameObject->getComponent<MyGUIMessageBoxComponent>()).get();
	}

	MyGUIPositionControllerComponent* getMyGUIPositionControllerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIPositionControllerComponent>(gameObject->getComponent<MyGUIPositionControllerComponent>()).get();
	}

	MyGUIFadeAlphaControllerComponent* getMyGUIFadeAlphaControllerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIFadeAlphaControllerComponent>(gameObject->getComponent<MyGUIFadeAlphaControllerComponent>()).get();
	}

	MyGUIScrollingMessageControllerComponent* getMyGUIScrollingMessageControllerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIScrollingMessageControllerComponent>(gameObject->getComponent<MyGUIScrollingMessageControllerComponent>()).get();
	}

	MyGUIEdgeHideControllerComponent* getMyGUIEdgeHideControllerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIEdgeHideControllerComponent>(gameObject->getComponent<MyGUIEdgeHideControllerComponent>()).get();
	}

	MyGUIRepeatClickControllerComponent* getMyGUIRepeatClickControllerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIRepeatClickControllerComponent>(gameObject->getComponent<MyGUIRepeatClickControllerComponent>()).get();
	}

	MyGUIMiniMapComponent* getMyGUIMiniMapComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGUIMiniMapComponent>(gameObject->getComponent<MyGUIMiniMapComponent>()).get();
	}

	LuaScriptComponent* getLuaScriptComponent(GameObject* gameObject)
	{
		return makeStrongPtr<LuaScriptComponent>(gameObject->getComponent<LuaScriptComponent>()).get();
	}

	AnimationComponent* getAnimationComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AnimationComponent>(gameObject->getComponentFromName<AnimationComponent>(name)).get();
	}

	AttributesComponent* getAttributesComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AttributesComponent>(gameObject->getComponentFromName<AttributesComponent>(name)).get();
	}

	AttributeEffectComponent* getAttributeEffectComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AttributeEffectComponent>(gameObject->getComponentFromName<AttributeEffectComponent>(name)).get();
	}

	PhysicsBuoyancyComponent* getPhysicsBuoyancyComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsBuoyancyComponent>(gameObject->getComponentFromName<PhysicsBuoyancyComponent>(name)).get();
	}

	PhysicsTriggerComponent* getPhysicsTriggerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsTriggerComponent>(gameObject->getComponentFromName<PhysicsTriggerComponent>(name)).get();
	}

	DistributedComponent* getDistributedComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<DistributedComponent>(gameObject->getComponentFromName<DistributedComponent>(name)).get();
	}

	ExitComponent* getExitComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<ExitComponent>(gameObject->getComponentFromName<ExitComponent>(name)).get();
	}

	GameObjectTitleComponent* getGameObjectTitleComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<GameObjectTitleComponent>(gameObject->getComponentFromName<GameObjectTitleComponent>(name)).get();
	}

	NavMeshComponent* getNavMeshComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<NavMeshComponent>(gameObject->getComponentFromName<NavMeshComponent>(name)).get();
	}

	ParticleUniverseComponent* getParticleUniverseComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<ParticleUniverseComponent>(gameObject->getComponentFromName<ParticleUniverseComponent>(name)).get();
	}

	PlayerControllerJumpNRunComponent* getPlayerControllerJumpNRunComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PlayerControllerJumpNRunComponent>(gameObject->getComponentFromName<PlayerControllerJumpNRunComponent>(name)).get();
	}

	PlayerControllerJumpNRunLuaComponent* getPlayerControllerJumpNRunLuaComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PlayerControllerJumpNRunLuaComponent>(gameObject->getComponentFromName<PlayerControllerJumpNRunLuaComponent>(name)).get();
	}

	PlayerControllerClickToPointComponent* getPlayerControllerClickToPointComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PlayerControllerClickToPointComponent>(gameObject->getComponentFromName<PlayerControllerClickToPointComponent>(name)).get();
	}

	PhysicsActiveComponent* getPhysicsActiveComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsActiveComponent>(gameObject->getComponentFromName<PhysicsActiveComponent>(name)).get();
	}

	PhysicsComponent* getPhysicsComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsComponent>(gameObject->getComponentFromName<PhysicsComponent>(name)).get();
	}

	PhysicsActiveCompoundComponent* getPhysicsActiveCompoundComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsActiveCompoundComponent>(gameObject->getComponentFromName<PhysicsActiveCompoundComponent>(name)).get();
	}

	PhysicsActiveDestructableComponent* getPhysicsActiveDestructableComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsActiveDestructableComponent>(gameObject->getComponentFromName<PhysicsActiveDestructableComponent>(name)).get();
	}

	PhysicsActiveKinematicComponent* getPhysicsActiveKinematicComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsActiveKinematicComponent>(gameObject->getComponentFromName<PhysicsActiveKinematicComponent>(name)).get();
	}

	PhysicsArtifactComponent* getPhysicsArtifactComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsArtifactComponent>(gameObject->getComponentFromName<PhysicsArtifactComponent>(name)).get();
	}

	PhysicsRagDollComponent* getPhysicsRagDollComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsRagDollComponent>(gameObject->getComponentFromName<PhysicsRagDollComponent>(name)).get();
	}

	PhysicsCompoundConnectionComponent* getPhysicsCompoundConnectionComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsCompoundConnectionComponent>(gameObject->getComponentFromName<PhysicsCompoundConnectionComponent>(name)).get();
	}

	PhysicsMaterialComponent* getPhysicsMaterialComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsMaterialComponent>(gameObject->getComponentFromName<PhysicsMaterialComponent>(name)).get();
	}

	PhysicsPlayerControllerComponent* getPhysicsPlayerControllerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsPlayerControllerComponent>(gameObject->getComponentFromName<PhysicsPlayerControllerComponent>(name)).get();
	}

	PlaneComponent* getPlaneComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PlaneComponent>(gameObject->getComponentFromName<PlaneComponent>(name)).get();
	}

	SimpleSoundComponent* getSimpleSoundComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<SimpleSoundComponent>(gameObject->getComponentFromName<SimpleSoundComponent>(name)).get();
	}

	SoundComponent* getSoundComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<SoundComponent>(gameObject->getComponentFromName<SoundComponent>(name)).get();
	}

	SpawnComponent* getSpawnComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<SpawnComponent>(gameObject->getComponentFromName<SpawnComponent>(name)).get();
	}

	VehicleComponent* getVehicleComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<VehicleComponent>(gameObject->getComponentFromName<VehicleComponent>(name)).get();
	}

	AiLuaComponent* getAiLuaComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiLuaComponent>(gameObject->getComponentFromName<AiLuaComponent>(name)).get();
	}

	PhysicsExplosionComponent* getPhysicsExplosionComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsExplosionComponent>(gameObject->getComponentFromName<PhysicsExplosionComponent>(name)).get();
	}

	CameraComponent* getCameraComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CameraComponent>(gameObject->getComponentFromName<CameraComponent>(name)).get();
	}

	CameraBehaviorBaseComponent* getCameraBehaviorBaseComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CameraBehaviorBaseComponent>(gameObject->getComponentFromName<CameraBehaviorBaseComponent>(name)).get();
	}

	CameraBehaviorFirstPersonComponent* getCameraBehaviorFirstPersonComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CameraBehaviorFirstPersonComponent>(gameObject->getComponentFromName<CameraBehaviorFirstPersonComponent>(name)).get();
	}

	CameraBehaviorThirdPersonComponent* getCameraBehaviorThirdPersonComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CameraBehaviorThirdPersonComponent>(gameObject->getComponentFromName<CameraBehaviorThirdPersonComponent>(name)).get();
	}

	CameraBehaviorFollow2DComponent* getCameraBehaviorFollow2DComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CameraBehaviorFollow2DComponent>(gameObject->getComponentFromName<CameraBehaviorFollow2DComponent>(name)).get();
	}

	CameraBehaviorZoomComponent* getCameraBehaviorZoomComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CameraBehaviorZoomComponent>(gameObject->getComponentFromName<CameraBehaviorZoomComponent>(name)).get();
	}
	
	CompositorEffectBloomComponent* getCompositorEffectBloomComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CompositorEffectBloomComponent>(gameObject->getComponentFromName<CompositorEffectBloomComponent>(name)).get();
	}

	CompositorEffectGlassComponent* getCompositorEffectGlassComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CompositorEffectGlassComponent>(gameObject->getComponentFromName<CompositorEffectGlassComponent>(name)).get();
	}

	CompositorEffectOldTvComponent* getCompositorEffectOldTvComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CompositorEffectOldTvComponent>(gameObject->getComponentFromName<CompositorEffectOldTvComponent>(name)).get();
	}

	CompositorEffectKeyholeComponent* getCompositorEffectKeyholeComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CompositorEffectKeyholeComponent>(gameObject->getComponentFromName<CompositorEffectKeyholeComponent>(name)).get();
	}

	CompositorEffectBlackAndWhiteComponent* getCompositorEffectBlackAndWhiteComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CompositorEffectBlackAndWhiteComponent>(gameObject->getComponentFromName<CompositorEffectBlackAndWhiteComponent>(name)).get();
	}

	CompositorEffectColorComponent* getCompositorEffectColorComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CompositorEffectColorComponent>(gameObject->getComponentFromName<CompositorEffectColorComponent>(name)).get();
	}

	CompositorEffectEmbossedComponent* getCompositorEffectEmbossedComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CompositorEffectEmbossedComponent>(gameObject->getComponentFromName<CompositorEffectEmbossedComponent>(name)).get();
	}

	CompositorEffectSharpenEdgesComponent* getCompositorEffectSharpenEdgesComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CompositorEffectSharpenEdgesComponent>(gameObject->getComponentFromName<CompositorEffectSharpenEdgesComponent>(name)).get();
	}

	HdrEffectComponent* getHdrEffectComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<HdrEffectComponent>(gameObject->getComponentFromName<HdrEffectComponent>(name)).get();
	}

	AiMoveComponent* getAiMoveComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiMoveComponent>(gameObject->getComponentFromName<AiMoveComponent>(name)).get();
	}

	AiMoveRandomlyComponent* getAiMoveRandomlyComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiMoveRandomlyComponent>(gameObject->getComponentFromName<AiMoveRandomlyComponent>(name)).get();
	}

	AiPathFollowComponent* getAiPathFollowComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiPathFollowComponent>(gameObject->getComponentFromName<AiPathFollowComponent>(name)).get();
	}

	AiWanderComponent* getAiWanderComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiWanderComponent>(gameObject->getComponentFromName<AiWanderComponent>(name)).get();
	}

	AiFlockingComponent* getAiFlockingComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiFlockingComponent>(gameObject->getComponentFromName<AiFlockingComponent>(name)).get();
	}

	AiRecastPathNavigationComponent* getAiRecastPathNavigationComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiRecastPathNavigationComponent>(gameObject->getComponentFromName<AiRecastPathNavigationComponent>(name)).get();
	}

	AiObstacleAvoidanceComponent* getAiObstacleAvoidanceComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiObstacleAvoidanceComponent>(gameObject->getComponentFromName<AiObstacleAvoidanceComponent>(name)).get();
	}

	AiHideComponent* getAiHideComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiHideComponent>(gameObject->getComponentFromName<AiHideComponent>(name)).get();
	}

	AiMoveComponent2D* getAiMoveComponent2DFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiMoveComponent2D>(gameObject->getComponentFromName<AiMoveComponent2D>(name)).get();
	}

	AiPathFollowComponent2D* getAiPathFollowComponent2DFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiPathFollowComponent2D>(gameObject->getComponentFromName<AiPathFollowComponent2D>(name)).get();
	}

	AiWanderComponent2D* getAiWanderComponent2DFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiWanderComponent2D>(gameObject->getComponentFromName<AiWanderComponent2D>(name)).get();
	}

	DatablockPbsComponent* getDatablockPbsComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<DatablockPbsComponent>(gameObject->getComponentFromName<DatablockPbsComponent>(name)).get();
	}

	DatablockUnlitComponent* getDatablockUnlitComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<DatablockUnlitComponent>(gameObject->getComponentFromName<DatablockUnlitComponent>(name)).get();
	}

	DatablockTerraComponent* getDatablockTerraComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<DatablockTerraComponent>(gameObject->getComponentFromName<DatablockTerraComponent>(name)).get();
	}

	JointComponent* getJointComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointComponent>(gameObject->getComponentFromName<JointComponent>(name)).get();
	}

	JointHingeComponent* getJointHingeComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointHingeComponent>(gameObject->getComponentFromName<JointHingeComponent>(name)).get();
	}

	JointHingeActuatorComponent* getJointHingeActuatorComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointHingeActuatorComponent>(gameObject->getComponentFromName<JointHingeActuatorComponent>(name)).get();
	}

	JointBallAndSocketComponent* getJointBallAndSocketComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointBallAndSocketComponent>(gameObject->getComponentFromName<JointBallAndSocketComponent>(name)).get();
	}

	JointPointToPointComponent* getJointPointToPointComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointPointToPointComponent>(gameObject->getComponentFromName<JointPointToPointComponent>(name)).get();
	}

	JointPinComponent* getJointPinComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointPinComponent>(gameObject->getComponentFromName<JointPinComponent>(name)).get();
	}

	JointPlaneComponent* getJointPlaneComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointPlaneComponent>(gameObject->getComponentFromName<JointPlaneComponent>(name)).get();
	}

	JointSpringComponent* getJointSpringComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointSpringComponent>(gameObject->getComponentFromName<JointSpringComponent>(name)).get();
	}

	JointAttractorComponent* getJointAttractorComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointAttractorComponent>(gameObject->getComponentFromName<JointAttractorComponent>(name)).get();
	}

	JointCorkScrewComponent* getJointCorkScrewComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointCorkScrewComponent>(gameObject->getComponentFromName<JointCorkScrewComponent>(name)).get();
	}

	JointPassiveSliderComponent* getJointPassiveSliderComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointPassiveSliderComponent>(gameObject->getComponentFromName<JointPassiveSliderComponent>(name)).get();
	}

	JointSliderActuatorComponent* getJointSliderActuatorComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointSliderActuatorComponent>(gameObject->getComponentFromName<JointSliderActuatorComponent>(name)).get();
	}

	JointSlidingContactComponent* getJointSlidingContactComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointSlidingContactComponent>(gameObject->getComponentFromName<JointSlidingContactComponent>(name)).get();
	}

	JointActiveSliderComponent* getJointActiveSliderComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointActiveSliderComponent>(gameObject->getComponentFromName<JointActiveSliderComponent>(name)).get();
	}

	JointMathSliderComponent* getJointMathSliderComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointMathSliderComponent>(gameObject->getComponentFromName<JointMathSliderComponent>(name)).get();
	}

	JointKinematicComponent* getJointKinematicComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointKinematicComponent>(gameObject->getComponentFromName<JointKinematicComponent>(name)).get();
	}

	JointDryRollingFrictionComponent* getJointDryRollingFrictionComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointDryRollingFrictionComponent>(gameObject->getComponentFromName<JointDryRollingFrictionComponent>(name)).get();
	}

	JointPathFollowComponent* getJointPathFollowComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointPathFollowComponent>(gameObject->getComponentFromName<JointPathFollowComponent>(name)).get();
	}

	JointGearComponent* getJointGearComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointGearComponent>(gameObject->getComponentFromName<JointGearComponent>(name)).get();
	}

	JointRackAndPinionComponent* getJointRackAndPinionComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointRackAndPinionComponent>(gameObject->getComponentFromName<JointRackAndPinionComponent>(name)).get();
	}

	JointWormGearComponent* getJointWormGearComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointWormGearComponent>(gameObject->getComponentFromName<JointWormGearComponent>(name)).get();
	}

	JointPulleyComponent* getJointPulleyComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointPulleyComponent>(gameObject->getComponentFromName<JointPulleyComponent>(name)).get();
	}

	JointUniversalComponent* getJointUniversalComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointUniversalComponent>(gameObject->getComponentFromName<JointUniversalComponent>(name)).get();
	}

	JointUniversalActuatorComponent* getJointUniversalActuatorComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointUniversalActuatorComponent>(gameObject->getComponentFromName<JointUniversalActuatorComponent>(name)).get();
	}

	Joint6DofComponent* getJoint6DofComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<Joint6DofComponent>(gameObject->getComponentFromName<Joint6DofComponent>(name)).get();
	}

	JointMotorComponent* getJointMotorComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointMotorComponent>(gameObject->getComponentFromName<JointMotorComponent>(name)).get();
	}

	JointWheelComponent* getJointWheelComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointWheelComponent>(gameObject->getComponentFromName<JointWheelComponent>(name)).get();
	}

	LightDirectionalComponent* getLightDirectionalComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<LightDirectionalComponent>(gameObject->getComponentFromName<LightDirectionalComponent>(name)).get();
	}

	LightPointComponent* getLightPointComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<LightPointComponent>(gameObject->getComponentFromName<LightPointComponent>(name)).get();
	}

	LightSpotComponent* getLightSpotComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<LightSpotComponent>(gameObject->getComponentFromName<LightSpotComponent>(name)).get();
	}

	LightAreaComponent* getLightAreaComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<LightAreaComponent>(gameObject->getComponentFromName<LightAreaComponent>(name)).get();
	}

	FadeComponent* getFadeComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<FadeComponent>(gameObject->getComponentFromName<FadeComponent>(name)).get();
	}

	MeshDecalComponent* getMeshDecalComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MeshDecalComponent>(gameObject->getComponentFromName<MeshDecalComponent>(name)).get();
	}

	TagPointComponent* getTagPointComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<TagPointComponent>(gameObject->getComponentFromName<TagPointComponent>(name)).get();
	}

	TimeTriggerComponent* getTimeTriggerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<TimeTriggerComponent>(gameObject->getComponentFromName<TimeTriggerComponent>(name)).get();
	}

	TimeLineComponent* getTimeLineComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<TimeLineComponent>(gameObject->getComponentFromName<TimeLineComponent>(name)).get();
	}

	MoveMathFunctionComponent* getMoveMathFunctionComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MoveMathFunctionComponent>(gameObject->getComponentFromName<MoveMathFunctionComponent>(name)).get();
	}

	TagChildNodeComponent* getTagChildNodeComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<TagChildNodeComponent>(gameObject->getComponentFromName<TagChildNodeComponent>(name)).get();
	}

	NodeTrackComponent* getNodeTrackComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<NodeTrackComponent>(gameObject->getComponentFromName<NodeTrackComponent>(name)).get();
	}

	LineComponent* getLineComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<LineComponent>(gameObject->getComponentFromName<LineComponent>(name)).get();
	}

	LinesComponent* getLinesComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<LinesComponent>(gameObject->getComponentFromName<LinesComponent>(name)).get();
	}

	NodeComponent* getNodeComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<NodeComponent>(gameObject->getComponentFromName<NodeComponent>(name)).get();
	}

	ManualObjectComponent* getManualObjectComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<ManualObjectComponent>(gameObject->getComponentFromName<ManualObjectComponent>(name)).get();
	}

	RectangleComponent* getRectangleComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<RectangleComponent>(gameObject->getComponentFromName<RectangleComponent>(name)).get();
	}

	ValueBarComponent* getValueBarComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<ValueBarComponent>(gameObject->getComponentFromName<ValueBarComponent>(name)).get();
	}

	BillboardComponent* getBillboardComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<BillboardComponent>(gameObject->getComponentFromName<BillboardComponent>(name)).get();
	}

	RibbonTrailComponent* getRibbonTrailComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<RibbonTrailComponent>(gameObject->getComponentFromName<RibbonTrailComponent>(name)).get();
	}

	MyGUIWindowComponent* getMyGUIWindowComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIWindowComponent>(gameObject->getComponentFromName<MyGUIWindowComponent>(name)).get();
	}

	MyGUIItemBoxComponent* getMyGUIItemBoxComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIItemBoxComponent>(gameObject->getComponentFromName<MyGUIItemBoxComponent>(name)).get();
	}

	InventoryItemComponent* getInventoryItemComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<InventoryItemComponent>(gameObject->getComponentFromName<InventoryItemComponent>(name)).get();
	}

	MyGUITextComponent* getMyGUITextComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUITextComponent>(gameObject->getComponentFromName<MyGUITextComponent>(name)).get();
	}

	MyGUIButtonComponent* getMyGUIButtonComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIButtonComponent>(gameObject->getComponentFromName<MyGUIButtonComponent>(name)).get();
	}

	MyGUICheckBoxComponent* getMyGUICheckBoxComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUICheckBoxComponent>(gameObject->getComponentFromName<MyGUICheckBoxComponent>(name)).get();
	}

	MyGUIImageBoxComponent* getMyGUIImageBoxComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIImageBoxComponent>(gameObject->getComponentFromName<MyGUIImageBoxComponent>(name)).get();
	}

	MyGUIProgressBarComponent* getMyGUIProgressBarComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIProgressBarComponent>(gameObject->getComponentFromName<MyGUIProgressBarComponent>(name)).get();
	}

	MyGUIComboBoxComponent* getMyGUIComboBoxComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIComboBoxComponent>(gameObject->getComponentFromName<MyGUIComboBoxComponent>(name)).get();
	}

	MyGUIListBoxComponent* getMyGUIListBoxComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIListBoxComponent>(gameObject->getComponentFromName<MyGUIListBoxComponent>(name)).get();
	}

	MyGUIMessageBoxComponent* getMyGUIMessageBoxComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIMessageBoxComponent>(gameObject->getComponentFromName<MyGUIMessageBoxComponent>(name)).get();
	}

	MyGUIPositionControllerComponent* getMyGUIPositionControllerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIPositionControllerComponent>(gameObject->getComponentFromName<MyGUIPositionControllerComponent>(name)).get();
	}

	MyGUIFadeAlphaControllerComponent* getMyGUIFadeAlphaControllerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIFadeAlphaControllerComponent>(gameObject->getComponentFromName<MyGUIFadeAlphaControllerComponent>(name)).get();
	}

	MyGUIScrollingMessageControllerComponent* getMyGUIScrollingMessageControllerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIScrollingMessageControllerComponent>(gameObject->getComponentFromName<MyGUIScrollingMessageControllerComponent>(name)).get();
	}

	MyGUIEdgeHideControllerComponent* getMyGUIEdgeHideControllerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIEdgeHideControllerComponent>(gameObject->getComponentFromName<MyGUIEdgeHideControllerComponent>(name)).get();
	}

	MyGUIRepeatClickControllerComponent* getMyGUIRepeatClickControllerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIRepeatClickControllerComponent>(gameObject->getComponentFromName<MyGUIRepeatClickControllerComponent>(name)).get();
	}

	MyGUIMiniMapComponent* getMyGUIMiniMapComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGUIMiniMapComponent>(gameObject->getComponentFromName<MyGUIMiniMapComponent>(name)).get();
	}

	LuaScriptComponent* getLuaScriptComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<LuaScriptComponent>(gameObject->getComponentFromName<LuaScriptComponent>(name)).get();
	}

	JointComponent* getRagJointComponent(PhysicsRagDollComponent::RagBone* ragBone)
	{
		return ragBone->getJointComponent().get();
	}

	JointHingeComponent* getRagJointHingeComponent(PhysicsRagDollComponent::RagBone* ragBone)
	{
		return boost::dynamic_pointer_cast<JointHingeComponent>(ragBone->getJointComponent()).get();
	}

	JointUniversalComponent* getRagJointUniversalComponent(PhysicsRagDollComponent::RagBone* ragBone)
	{
		return boost::dynamic_pointer_cast<JointUniversalComponent>(ragBone->getJointComponent()).get();
	}

	JointBallAndSocketComponent* getRagJointBallAndSocketComponent(PhysicsRagDollComponent::RagBone* ragBone)
	{
		return boost::dynamic_pointer_cast<JointBallAndSocketComponent>(ragBone->getJointComponent()).get();
	}

	JointHingeActuatorComponent* getRagJointHingeActuatorComponent(PhysicsRagDollComponent::RagBone* ragBone)
	{
		return boost::dynamic_pointer_cast<JointHingeActuatorComponent>(ragBone->getJointComponent()).get();
	}

	JointUniversalActuatorComponent* getRagJointUniversalActuatorComponent(PhysicsRagDollComponent::RagBone* ragBone)
	{
		return boost::dynamic_pointer_cast<JointUniversalActuatorComponent>(ragBone->getJointComponent()).get();
	}

	Ogre::v1::Entity* getEntity(GameObject* gameObject)
	{
		return gameObject->getMovableObject<Ogre::v1::Entity>();
	}

	int getRandomNumberInt(MathHelper* instance, int min, int max)
	{
		return instance->getRandomNumber<int>(min, max);
	}

	/*Ogre::Real getRandomNumberFloat(MathHelper* instance, Ogre::Real min, Ogre::Real max)
	{
		return instance->getRandomNumber<Ogre::Real>(min, max);
	}*/

	long getRandomNumberLong(MathHelper* instance, long min, long max)
	{
		return instance->getRandomNumber<long>(min, max);
	}

	GameObject* getConnectedGameObject(GameObject* gameObject)
	{
		auto gameObjectPtr = NOWA::makeStrongPtr(gameObject->getConnectedGameObjectPtr());
		if (nullptr != gameObjectPtr)
		{
			return gameObjectPtr.get();
		}

		currentErrorGameObject = gameObject->getName();
		currentCalledFunction = "getConnectedGameObject";

		return nullptr;
	}

	Ogre::String getId(GameObject* instance)
	{
		// Wtf: Lua messes up totally with numbers of 10 digits, that is:
		// 1146341527 will become:
		// 1.14634e+09 which is:
		// 1.146.340.000 -> catastophal fubbes!!!!
		// Hence work with string as ids in lua
		return Ogre::StringConverter::toString(instance->getId());
	}

	Ogre::String getCategoryId(GameObject* instance)
	{
		return Ogre::StringConverter::toString(instance->getCategoryId());
	}

	Ogre::String getGameObjectReferenceId(GameObject* instance)
	{
		return Ogre::StringConverter::toString(instance->getReferenceId());
	}

	// ...
	// more types to come

	void bindGameObject(lua_State* lua, class_<GameObject>& gameObject)
	{
		gameObject.def("getId", &getId);
		gameObject.def("getName", &GameObject::getName);
		gameObject.def("getUniqueName", &GameObject::getUniqueName);
		gameObject.def("getSceneManager", &GameObject::getSceneManager);
		gameObject.def("getSceneNode", &GameObject::getSceneNode);
		// Here later multiplicate out all types in anonym function
		gameObject.def("getEntity", &getEntity);
		// gameObject.def("getMovableObject", &getMovableObject);
		gameObject.def("getCategory", &GameObject::getCategory);
		// gameObject.def("getCategoryId", &GameObject::getCategoryId);
		gameObject.def("getCategoryId", &getCategoryId);
		gameObject.def("getTagName", &GameObject::getTagName);
		gameObject.def("changeCategory", (void (GameObject::*)(const Ogre::String&, const Ogre::String&)) & GameObject::changeCategory);
		gameObject.def("changeCategory2", (void (GameObject::*)(const Ogre::String&)) & GameObject::changeCategory);

		// gameObject.def("GameObjectComponents", &GameObject::getComponents);
		// gameObject.def("addComponent", &GameObject::addComponent);
		gameObject.def("setAttributePosition", &GameObject::setAttributePosition);
		gameObject.def("setAttributeScale", &GameObject::setAttributeScale);
		gameObject.def("setAttributeOrientation", &GameObject::setAttributeOrientation);
		gameObject.def("getPosition", &GameObject::getPosition);
		gameObject.def("getOrientation", &GameObject::getOrientation);
		gameObject.def("getScale", &GameObject::getScale);
		// gameObject.def("setControlledByClientID", (void (GameObject::*)(unsigned int)) &GameObject::setControlledByClientID);
		gameObject.def("getControlledByClientID", &GameObject::getControlledByClientID);
		gameObject.def("getSize", &GameObject::getSize);
		gameObject.def("getCenterOffset", &GameObject::getCenterOffset);
		gameObject.def("getBottomOffset", &GameObject::getBottomOffset);
		gameObject.def("getMiddle", &GameObject::getMiddle);
		gameObject.def("setVisible", &GameObject::setVisible);
		gameObject.def("isVisible", &GameObject::isVisible);
		gameObject.def("isDynamic", &GameObject::isDynamic);
		gameObject.def("setUseReflection", &GameObject::setUseReflection);
		gameObject.def("getUseReflection", &GameObject::getUseReflection);
		gameObject.def("getDefaultDirection", &GameObject::getDefaultDirection);
		gameObject.def("setClampY", &GameObject::setClampY);
		gameObject.def("getClampY", &GameObject::getClampY);
		gameObject.def("setReferenceId", &GameObject::setReferenceId);
		gameObject.def("getReferenceId", &getGameObjectReferenceId);
		gameObject.def("showBoundingBox", &GameObject::showBoundingBox);
		gameObject.def("showDebugData", &GameObject::showDebugData);
		gameObject.def("getConnectedGameObject", &getConnectedGameObject);

		// get access to all components and cast them to strong shared ptr internally here
		gameObject.def("getComponentByIndex", &getGameObjectComponentByIndex);
		gameObject.def("getIndexFromComponent", &GameObject::getIndexFromComponent);
		gameObject.def("getOccurrenceIndexFromComponent", &GameObject::getOccurrenceIndexFromComponent);
		gameObject.def("deleteComponent", (bool (GameObject::*)(const Ogre::String&, unsigned int)) & GameObject::deleteComponent);
		gameObject.def("deleteComponentByIndex", &GameObject::deleteComponentByIndex);

		gameObject.def("getAnimationComponent", (AnimationComponent * (*)(GameObject*)) & getAnimationComponent);
		gameObject.def("getAnimationComponent2", (AnimationComponent * (*)(GameObject*, unsigned int)) & getAnimationComponent);
		gameObject.def("getAttributesComponent", &getAttributesComponent);
		gameObject.def("getAttributeEffectComponent", (AttributeEffectComponent * (*)(GameObject*)) & getAttributeEffectComponent);
		gameObject.def("getAttributeEffectComponent2", (AttributeEffectComponent * (*)(GameObject*, unsigned int)) & getAttributeEffectComponent);

		gameObject.def("getPhysicsBuoyancyComponent", &getPhysicsBuoyancyComponent);
		gameObject.def("getPhysicsTriggerComponent", &getPhysicsTriggerComponent);
		gameObject.def("getDistributedComponent", &getDistributedComponent);
		gameObject.def("getExitComponent", &getExitComponent);
		gameObject.def("getGameObjectTitleComponent", &getGameObjectTitleComponent);
		gameObject.def("getAiMoveComponent", &getAiMoveComponent);
		gameObject.def("getAiMoveRandomlyComponent", &getAiMoveRandomlyComponent);
		gameObject.def("getAiPathFollowComponent", &getAiPathFollowComponent);
		gameObject.def("getAiWanderComponent", &getAiWanderComponent);
		gameObject.def("getAiFlockingComponent", &getAiFlockingComponent);
		gameObject.def("getAiRecastPathNavigationComponent", &getAiRecastPathNavigationComponent);
		gameObject.def("getAiObstacleAvoidanceComponent", &getAiObstacleAvoidanceComponent);
		gameObject.def("getAiHideComponent", &getAiHideComponent);
		gameObject.def("getAiMoveComponent2D", &getAiMoveComponent2D);
		gameObject.def("getAiPathFollowComponent2D", &getAiPathFollowComponent2D);
		gameObject.def("getAiWanderComponent2D", &getAiWanderComponent2D);
		gameObject.def("getCameraBehaviorBaseComponent", &getCameraBehaviorBaseComponent);
		gameObject.def("getCameraBehaviorFirstPersonComponent", &getCameraBehaviorFirstPersonComponent);
		gameObject.def("getCameraBehaviorThirdPersonComponent", &getCameraBehaviorThirdPersonComponent);
		gameObject.def("getCameraBehaviorFollow2DComponent", &getCameraBehaviorFollow2DComponent);
		gameObject.def("getCameraBehaviorZoomComponent", &getCameraBehaviorZoomComponent);
		gameObject.def("getCameraComponent", &getCameraComponent);
		gameObject.def("getCompositorEffectBloomComponent", &getCompositorEffectBloomComponent);
		gameObject.def("getCompositorEffectGlassComponent", &getCompositorEffectGlassComponent);
		gameObject.def("getCompositorEffectOldTvComponent", &getCompositorEffectOldTvComponent);
		gameObject.def("getCompositorEffectKeyholeComponent", &getCompositorEffectKeyholeComponent);
		gameObject.def("getCompositorEffectBlackAndWhiteComponent", &getCompositorEffectBlackAndWhiteComponent);
		gameObject.def("getCompositorEffectColorComponent", &getCompositorEffectColorComponent);
		gameObject.def("getCompositorEffectEmbossedComponent", &getCompositorEffectEmbossedComponent);
		gameObject.def("getCompositorEffectSharpenEdgesComponent", &getCompositorEffectSharpenEdgesComponent);

		gameObject.def("getHdrEffectComponent", &getHdrEffectComponent);

		gameObject.def("getDatablockPbsComponent", &getDatablockPbsComponent);
		gameObject.def("getDatablockUnlitComponent", &getDatablockUnlitComponent);
		gameObject.def("getDatablockTerraComponent", &getDatablockTerraComponent);
		gameObject.def("getJointComponent", &getJointComponent);
		gameObject.def("getJointHingeComponent", &getJointHingeComponent);
		gameObject.def("getJointHingeActuatorComponent", &getJointHingeActuatorComponent);
		gameObject.def("getJointBallAndSocketComponent", &getJointBallAndSocketComponent);
		gameObject.def("getJointPointToPointComponent", &getJointPointToPointComponent);
		gameObject.def("getJointPinComponent", &getJointPinComponent);
		gameObject.def("getJointPlaneComponent", &getJointPlaneComponent);
		gameObject.def("getJointSpringComponent", &getJointSpringComponent);
		gameObject.def("getJointAttractorComponent", &getJointAttractorComponent);
		gameObject.def("getJointCorkScrewComponent", &getJointCorkScrewComponent);
		gameObject.def("getJointPassiveSliderComponent", &getJointPassiveSliderComponent);
		gameObject.def("getJointSliderActuatorComponent", &getJointSliderActuatorComponent);
		gameObject.def("getJointSlidingContactComponent", &getJointSlidingContactComponent);
		gameObject.def("getJointActiveSliderComponent", &getJointActiveSliderComponent);
		gameObject.def("getJointMathSliderComponent", &getJointMathSliderComponent);
		gameObject.def("getJointKinematicComponent", &getJointKinematicComponent);
		gameObject.def("getJointDryRollingFrictionComponent", &getJointDryRollingFrictionComponent);
		gameObject.def("getJointPathFollowComponent", &getJointPathFollowComponent);
		gameObject.def("getJointGearComponent", &getJointGearComponent);
		gameObject.def("getJointRackAndPinionComponent", &getJointRackAndPinionComponent);
		gameObject.def("getJointWormGearComponent", &getJointWormGearComponent);
		gameObject.def("getJointPulleyComponent", &getJointPulleyComponent);
		gameObject.def("getJointUniversalComponent", &getJointUniversalComponent);
		gameObject.def("getJointUniversalActuatorComponent", &getJointUniversalActuatorComponent);
		gameObject.def("getJoint6DofComponent", &getJoint6DofComponent);
		gameObject.def("getJointMotorComponent", &getJointMotorComponent);
		gameObject.def("getJoinWheelComponent", &getJointWheelComponent);

		gameObject.def("getLightDirectionalComponent", &getLightDirectionalComponent);
		gameObject.def("getLightPointComponent", &getLightPointComponent);
		gameObject.def("getLightSpotComponent", &getLightSpotComponent);
		gameObject.def("getLightAreaComponent", &getLightAreaComponent);
		gameObject.def("getFadeComponent", &getFadeComponent);

		gameObject.def("getNavMeshComponent", &getNavMeshComponent);
		gameObject.def("getParticleUniverseComponent", (ParticleUniverseComponent * (*)(GameObject*)) & getParticleUniverseComponent);
		gameObject.def("getParticleUniverseComponent2", (ParticleUniverseComponent * (*)(GameObject*, unsigned int)) & getParticleUniverseComponent);

		gameObject.def("getPlayerControllerJumpNRunComponent", &getPlayerControllerJumpNRunComponent);
		gameObject.def("getPlayerControllerJumpNRunLuaComponent", &getPlayerControllerJumpNRunLuaComponent);

		gameObject.def("getPlayerControllerClickToPointComponent", &getPlayerControllerClickToPointComponent);

		gameObject.def("getPhysicsComponent", &getPhysicsComponent);
		gameObject.def("getPhysicsActiveComponent", &getPhysicsActiveComponent);
		gameObject.def("getPhysicsActiveCompoundComponent", &getPhysicsActiveCompoundComponent);
		gameObject.def("getPhysicsActiveDestructibleComponent", &getPhysicsActiveDestructableComponent);
		gameObject.def("getPhysicsActiveKinematicComponent", &getPhysicsActiveKinematicComponent);
		gameObject.def("getPhysicsArtifactComponent", &getPhysicsArtifactComponent);
		gameObject.def("getPhysicsRagDollComponent", &getPhysicsRagDollComponent);
		gameObject.def("getPhysicsCompoundConnectionComponent", &getPhysicsCompoundConnectionComponent);
		gameObject.def("getPhysicsMaterialComponent", (PhysicsMaterialComponent * (*)(GameObject*)) & getPhysicsMaterialComponent);
		gameObject.def("getPhysicsMaterialComponent2", (PhysicsMaterialComponent * (*)(GameObject*, unsigned int)) & getPhysicsMaterialComponent);

		gameObject.def("getPhysicsPlayerControllerComponent", &getPhysicsPlayerControllerComponent);
		gameObject.def("getPlaneComponent", &getPlaneComponent);
		gameObject.def("getSimpleSoundComponent", (SimpleSoundComponent * (*)(GameObject*)) & getSimpleSoundComponent);
		gameObject.def("getSimpleSoundComponent2", (SimpleSoundComponent * (*)(GameObject*, unsigned int)) & getSimpleSoundComponent);
		gameObject.def("getSoundComponent", (SoundComponent * (*)(GameObject*)) & getSoundComponent);
		gameObject.def("getSoundComponent2", (SoundComponent * (*)(GameObject*, unsigned int)) & getSoundComponent);

		gameObject.def("getSpawnComponent", (SpawnComponent* (*)(GameObject*))& getSpawnComponent);
		gameObject.def("getSpawnComponent", (SpawnComponent* (*)(GameObject*, unsigned int))& getSpawnComponent);
		gameObject.def("getVehicleComponent", &getVehicleComponent);
		gameObject.def("getAiLuaComponent", &getAiLuaComponent);
		gameObject.def("getPhysicsExplosionComponent", &getPhysicsExplosionComponent);
		gameObject.def("getMeshDecalComponent", &getMeshDecalComponent);
		gameObject.def("getTagPointComponent", (TagPointComponent * (*)(GameObject*)) & getTagPointComponent);
		gameObject.def("getTagPointComponent2", (TagPointComponent * (*)(GameObject*, unsigned int)) & getTagPointComponent);
		gameObject.def("getTimeTriggerComponent", (TimeTriggerComponent * (*)(GameObject*)) & getTimeTriggerComponent);
		gameObject.def("getTimeTriggerComponent2", (TimeTriggerComponent * (*)(GameObject*, unsigned int)) & getTimeTriggerComponent);
		gameObject.def("getTimeLineComponent", &getTimeLineComponent);
		gameObject.def("getMoveMathFunctionComponent", &getMoveMathFunctionComponent);

		gameObject.def("getTagChildNodeComponent", (TagChildNodeComponent * (*)(GameObject*)) & getTagChildNodeComponent);
		gameObject.def("getTagChildNodeComponent2", (TagChildNodeComponent * (*)(GameObject*, unsigned int)) & getTagChildNodeComponent);
		gameObject.def("getNodeTrackComponent", &getNodeTrackComponent);
		gameObject.def("getLineComponent", &getLineComponent);
		gameObject.def("getLinesComponent", &getLinesComponent);
		gameObject.def("getNodeComponent", &getNodeComponent);
		gameObject.def("getManualObjectComponent", &getManualObjectComponent);
		gameObject.def("getRectangleComponent", &getRectangleComponent);
		gameObject.def("getValueBarComponent", &getValueBarComponent);
		gameObject.def("getBillboardComponent", &getBillboardComponent);
		gameObject.def("getRibbonTrailComponent", &getRibbonTrailComponent);
		gameObject.def("getMyGUIWindowComponent2", (MyGUIWindowComponent * (*)(GameObject*, unsigned int)) & getMyGUIWindowComponent);
		gameObject.def("getMyGUITextComponent2", (MyGUITextComponent * (*)(GameObject*, unsigned int)) & getMyGUITextComponent);
		gameObject.def("getMyGUIButtonComponent2", (MyGUIButtonComponent * (*)(GameObject*, unsigned int)) & getMyGUIButtonComponent);
		gameObject.def("getMyGUICheckBoxComponent2", (MyGUICheckBoxComponent * (*)(GameObject*, unsigned int)) & getMyGUICheckBoxComponent);
		gameObject.def("getMyGUIImageBoxComponent2", (MyGUIImageBoxComponent * (*)(GameObject*, unsigned int)) & getMyGUIImageBoxComponent);
		gameObject.def("getMyGUIProgressBarComponent2", (MyGUIProgressBarComponent * (*)(GameObject*, unsigned int)) & getMyGUIProgressBarComponent);
		gameObject.def("getMyGUIComboBoxComponent2", (MyGUIComboBoxComponent * (*)(GameObject*, unsigned int)) & getMyGUIComboBoxComponent);
		gameObject.def("getMyGUIListBoxComponent2", (MyGUIListBoxComponent * (*)(GameObject*, unsigned int)) & getMyGUIListBoxComponent);
		gameObject.def("getMyGUIMessageBoxComponent2", (MyGUIMessageBoxComponent * (*)(GameObject*, unsigned int)) & getMyGUIMessageBoxComponent);
		gameObject.def("getMyGUIPositionControllerComponent2", (MyGUIPositionControllerComponent * (*)(GameObject*, unsigned int)) & getMyGUIPositionControllerComponent);
		gameObject.def("getMyGUIFadeAlphaControllerComponent2", (MyGUIFadeAlphaControllerComponent * (*)(GameObject*, unsigned int)) & getMyGUIFadeAlphaControllerComponent);
		gameObject.def("getMyGUIScrollingMessageControllerComponent2", (MyGUIScrollingMessageControllerComponent * (*)(GameObject*, unsigned int)) & getMyGUIScrollingMessageControllerComponent);
		gameObject.def("getMyGUIEdgeHideControllerComponent2", (MyGUIEdgeHideControllerComponent * (*)(GameObject*, unsigned int)) & getMyGUIEdgeHideControllerComponent);
		gameObject.def("getMyGUIRepeatClickControllerComponent2", (MyGUIRepeatClickControllerComponent * (*)(GameObject*, unsigned int)) & getMyGUIRepeatClickControllerComponent);
		gameObject.def("getMyGUIWindowComponent", (MyGUIWindowComponent * (*)(GameObject*)) & getMyGUIWindowComponent);
		gameObject.def("getMyGUIItemBoxComponent", &getMyGUIItemBoxComponent);
		gameObject.def("getInventoryItemComponent", &getInventoryItemComponent);
		gameObject.def("getMyGUITextComponent", (MyGUITextComponent * (*)(GameObject*)) & getMyGUITextComponent);
		gameObject.def("getMyGUIButtonComponent", (MyGUIButtonComponent * (*)(GameObject*)) & getMyGUIButtonComponent);
		gameObject.def("getMyGUICheckBoxComponent", (MyGUICheckBoxComponent * (*)(GameObject*)) & getMyGUICheckBoxComponent);
		gameObject.def("getMyGUIImageBoxComponent", (MyGUIImageBoxComponent * (*)(GameObject*)) & getMyGUIImageBoxComponent);
		gameObject.def("getMyGUIProgressBarComponent", (MyGUIProgressBarComponent * (*)(GameObject*)) & getMyGUIProgressBarComponent);
		gameObject.def("getMyGUIComboBoxComponent", (MyGUIComboBoxComponent * (*)(GameObject*)) & getMyGUIComboBoxComponent);
		gameObject.def("getMyGUIListBoxComponent", (MyGUIListBoxComponent * (*)(GameObject*)) & getMyGUIListBoxComponent);
		gameObject.def("getMyGUIMessageBoxComponent", (MyGUIMessageBoxComponent * (*)(GameObject*)) & getMyGUIMessageBoxComponent);
		gameObject.def("getMyGUIPositionControllerComponent", (MyGUIPositionControllerComponent * (*)(GameObject*)) & getMyGUIPositionControllerComponent);
		gameObject.def("getMyGUIFadeAlphaControllerComponent", (MyGUIFadeAlphaControllerComponent * (*)(GameObject*)) & getMyGUIFadeAlphaControllerComponent);
		gameObject.def("getMyGUIScrollingMessageControllerComponent", (MyGUIScrollingMessageControllerComponent * (*)(GameObject*)) & getMyGUIScrollingMessageControllerComponent);
		gameObject.def("getMyGUIEdgeHideControllerComponent", (MyGUIEdgeHideControllerComponent * (*)(GameObject*)) & getMyGUIEdgeHideControllerComponent);
		gameObject.def("getMyGUIRepeatClickControllerComponent", (MyGUIRepeatClickControllerComponent * (*)(GameObject*)) & getMyGUIRepeatClickControllerComponent);
		gameObject.def("getMyGUIMiniMapComponent", (MyGUIMiniMapComponent * (*)(GameObject*)) & getMyGUIMiniMapComponent);
		gameObject.def("getLuaScriptComponent", &getLuaScriptComponent);

		gameObject.def("getAnimationComponentFromName", &getAnimationComponentFromName);
		gameObject.def("getAttributesComponentFromName", &getAttributesComponentFromName);
		gameObject.def("getAttributeEffectComponentFromName", &getAttributeEffectComponentFromName);
		gameObject.def("getPhysicsBuoyancyComponentFromName", &getPhysicsBuoyancyComponentFromName);
		gameObject.def("getPhysicsTriggerComponentFromName", &getPhysicsTriggerComponentFromName);
		gameObject.def("getDistributedComponentFromName", &getDistributedComponentFromName);
		gameObject.def("getExitComponentFromName", &getExitComponentFromName);
		gameObject.def("getGameObjectTitleComponentFromName", &getGameObjectTitleComponentFromName);
		gameObject.def("getAiMoveComponentFromName", &getAiMoveComponentFromName);
		gameObject.def("getAiMoveRandomlyComponentFromName", &getAiMoveRandomlyComponentFromName);
		gameObject.def("getAiPathFollowComponentFromName", &getAiPathFollowComponentFromName);
		gameObject.def("getAiWanderComponentFromName", &getAiWanderComponentFromName);
		gameObject.def("getAiFlockingComponentFromName", &getAiFlockingComponentFromName);
		gameObject.def("getAiRecastPathNavigationComponentFromName", &getAiRecastPathNavigationComponentFromName);
		gameObject.def("getAiObstacleAvoidanceComponentFromName", &getAiObstacleAvoidanceComponentFromName);
		gameObject.def("getAiHideComponentFromName", &getAiHideComponentFromName);
		gameObject.def("getAiMoveComponent2DFromName", &getAiMoveComponent2D);
		gameObject.def("getAiPathFollowComponent2DFromName", &getAiPathFollowComponent2D);
		gameObject.def("getAiWanderComponent2DFromName", &getAiWanderComponent2D);
		gameObject.def("getCameraBehaviorBaseComponentFromName", &getCameraBehaviorBaseComponentFromName);
		gameObject.def("getCameraBehaviorFirstPersonComponentFromName", &getCameraBehaviorFirstPersonComponentFromName);
		gameObject.def("getCameraBehaviorThirdPersonComponentFromName", &getCameraBehaviorThirdPersonComponentFromName);
		gameObject.def("getCameraBehaviorFollow2DComponentFromName", &getCameraBehaviorFollow2DComponentFromName);
		gameObject.def("getCameraBehaviorZoomComponentFromName", &getCameraBehaviorZoomComponentFromName);
		gameObject.def("getCameraComponentFromName", &getCameraComponentFromName);
		gameObject.def("getCompositorEffectBloomComponentFromName", &getCompositorEffectBloomComponentFromName);
		gameObject.def("getCompositorEffectGlassComponentFromName", &getCompositorEffectGlassComponentFromName);
		gameObject.def("getCompositorEffectOldTvComponentFromName", &getCompositorEffectOldTvComponentFromName);
		gameObject.def("getCompositorEffectKeyholeComponentFromName", &getCompositorEffectKeyholeComponentFromName);
		gameObject.def("getCompositorEffectBlackAndWhiteComponentFromName", &getCompositorEffectBlackAndWhiteComponentFromName);
		gameObject.def("getCompositorEffectColorComponentFromName", &getCompositorEffectColorComponentFromName);
		gameObject.def("getCompositorEffectEmbossedComponentFromName", &getCompositorEffectEmbossedComponentFromName);
		gameObject.def("getCompositorEffectSharpenEdgesComponentFromName", &getCompositorEffectSharpenEdgesComponentFromName);
		gameObject.def("getHdrEffectComponentFromName", &getHdrEffectComponentFromName);
		gameObject.def("getDatablockPbsComponentFromName", &getDatablockPbsComponentFromName);
		gameObject.def("getDatablockUnlitComponentFromName", &getDatablockUnlitComponentFromName);
		gameObject.def("getDatablockTerraComponentFromName", &getDatablockTerraComponentFromName);
		gameObject.def("getJointComponentFromName", &getJointComponentFromName);
		gameObject.def("getJointHingeComponentFromName", &getJointHingeComponentFromName);
		gameObject.def("getJointHingeActuatorComponentFromName", &getJointHingeActuatorComponentFromName);
		gameObject.def("getJointBallAndSocketComponentFromName", &getJointBallAndSocketComponentFromName);
		gameObject.def("getJointPointToPointComponentFromName", &getJointPointToPointComponentFromName);
		gameObject.def("getJointPinComponentFromName", &getJointPinComponentFromName);
		gameObject.def("getJointPlaneComponentFromName", &getJointPlaneComponentFromName);
		gameObject.def("getJointSpringComponentFromName", &getJointSpringComponentFromName);
		gameObject.def("getJointAttractorComponentFromName", &getJointAttractorComponentFromName);
		gameObject.def("getJointCorkScrewComponentFromName", &getJointCorkScrewComponentFromName);
		gameObject.def("getJointPassiveSliderComponentFromName", &getJointPassiveSliderComponentFromName);
		gameObject.def("getJointSliderActuatorComponentFromName", &getJointSliderActuatorComponentFromName);
		gameObject.def("getJointSlidingContactComponentFromName", &getJointSlidingContactComponentFromName);
		gameObject.def("getJointActiveSliderComponentFromName", &getJointActiveSliderComponentFromName);
		gameObject.def("getJointMathSliderComponentFromName", &getJointMathSliderComponentFromName);
		gameObject.def("getJointKinematicComponentFromName", &getJointKinematicComponentFromName);
		gameObject.def("getJointDryRollingFrictionComponentFromName", &getJointDryRollingFrictionComponentFromName);
		gameObject.def("getJointPathFollowComponentFromName", &getJointPathFollowComponentFromName);
		gameObject.def("getJointGearComponentFromName", &getJointGearComponentFromName);
		gameObject.def("getJointRackAndPinionComponentFromName", &getJointRackAndPinionComponentFromName);
		gameObject.def("getJointWormGearComponentFromName", &getJointWormGearComponentFromName);
		gameObject.def("getJointPulleyComponentFromName", &getJointPulleyComponentFromName);
		gameObject.def("getJointUniversalComponentFromName", &getJointUniversalComponentFromName);
		gameObject.def("getJointUniversalActuatorComponentFromName", &getJointUniversalActuatorComponentFromName);
		gameObject.def("getJoint6DofComponentFromName", &getJoint6DofComponentFromName);
		gameObject.def("getJointMotorComponentFromName", &getJointMotorComponentFromName);
		gameObject.def("getJoinWheelComponentFromName", &getJointWheelComponentFromName);
		gameObject.def("getLightDirectionalComponentFromName", &getLightDirectionalComponentFromName);
		gameObject.def("getLightPointComponentFromName", &getLightPointComponentFromName);
		gameObject.def("getLightSpotComponentFromName", &getLightSpotComponentFromName);
		gameObject.def("getLightAreaComponentFromName", &getLightAreaComponentFromName);
		gameObject.def("getFadeComponentFromName", &getFadeComponentFromName);
		gameObject.def("getNavMeshComponentFromName", &getNavMeshComponentFromName);
		gameObject.def("getParticleUniverseComponentFromName", &getParticleUniverseComponentFromName);
		gameObject.def("getPlayerControllerJumpNRunComponentFromName", &getPlayerControllerJumpNRunComponentFromName);
		gameObject.def("getPlayerControllerJumpNRunLuaComponentFromName", &getPlayerControllerJumpNRunLuaComponentFromName);
		gameObject.def("getPlayerControllerClickToPointComponentFromName", &getPlayerControllerClickToPointComponentFromName);
		gameObject.def("getPhysicsComponentFromName", &getPhysicsComponentFromName);
		gameObject.def("getPhysicsActiveComponentFromName", &getPhysicsActiveComponentFromName);
		gameObject.def("getPhysicsActiveCompoundComponentFromName", &getPhysicsActiveCompoundComponentFromName);
		gameObject.def("getPhysicsActiveDestructibleComponentFromName", &getPhysicsActiveDestructableComponentFromName);
		gameObject.def("getPhysicsActiveKinematicComponentFromName", &getPhysicsActiveKinematicComponentFromName);
		gameObject.def("getPhysicsArtifactComponentFromName", &getPhysicsArtifactComponentFromName);
		gameObject.def("getPhysicsRagDollComponentFromName", &getPhysicsRagDollComponentFromName);
		gameObject.def("getPhysicsCompoundConnectionComponentFromName", &getPhysicsCompoundConnectionComponentFromName);
		gameObject.def("getPhysicsMaterialComponentFromName", &getPhysicsMaterialComponentFromName);
		gameObject.def("getPhysicsPlayerControllerComponentFromName", &getPhysicsPlayerControllerComponentFromName);
		gameObject.def("getPlaneComponentFromName", &getPlaneComponentFromName);
		gameObject.def("getSimpleSoundComponentFromName", &getSimpleSoundComponentFromName);
		gameObject.def("getSoundComponentFromName", &getSoundComponentFromName);
		gameObject.def("getSpawnComponentFromName", &getSpawnComponentFromName);
		gameObject.def("getVehicleComponentFromName", &getVehicleComponentFromName);
		gameObject.def("getAiLuaComponentFromName", &getAiLuaComponentFromName);
		gameObject.def("getPhysicsExplosionComponentFromName", &getPhysicsExplosionComponentFromName);
		gameObject.def("getMeshDecalComponentFromName", &getMeshDecalComponentFromName);
		gameObject.def("getTagPointComponentFromName", &getTagPointComponentFromName);
		gameObject.def("getTimeTriggerComponentFromName", &getTimeTriggerComponentFromName);
		gameObject.def("getTimeLineComponentFromName", &getTimeLineComponentFromName);
		gameObject.def("getMoveMathFunctionComponentFromName", &getMoveMathFunctionComponentFromName);
		gameObject.def("getTagChildNodeComponentFromName", &getTagChildNodeComponentFromName);
		gameObject.def("getNodeTrackComponentFromName", &getNodeTrackComponentFromName);
		gameObject.def("getLineComponentFromName", &getLineComponentFromName);
		gameObject.def("getLinesComponentFromName", &getLinesComponentFromName);
		gameObject.def("getNodeComponentFromName", &getNodeComponentFromName);
		gameObject.def("getManualObjectComponentFromName", &getManualObjectComponentFromName);
		gameObject.def("getRectangleComponentFromName", &getRectangleComponentFromName);
		gameObject.def("getValueBarComponentFromName", &getValueBarComponentFromName);
		gameObject.def("getBillboardComponentFromName", &getBillboardComponentFromName);
		gameObject.def("getRibbonTrailComponentFromName", &getRibbonTrailComponentFromName);
		gameObject.def("getMyGUIWindowComponentFromName", &getMyGUIWindowComponentFromName);
		gameObject.def("getMyGUIItemBoxComponentFromName", &getMyGUIItemBoxComponentFromName);
		gameObject.def("getInventoryItemComponentFromName", &getInventoryItemComponentFromName);
		gameObject.def("getMyGUITextComponentFromName", &getMyGUITextComponentFromName);
		gameObject.def("getMyGUIButtonComponentFromName", &getMyGUIButtonComponentFromName);
		gameObject.def("getMyGUICheckBoxComponentFromName", &getMyGUICheckBoxComponentFromName);
		gameObject.def("getMyGUIImageBoxComponentFromName", &getMyGUIImageBoxComponentFromName);
		gameObject.def("getMyGUIProgressBarComponentFromName", &getMyGUIProgressBarComponentFromName);
		gameObject.def("getMyGUIComboBoxComponentFromName", &getMyGUIComboBoxComponentFromName);
		gameObject.def("getMyGUIListBoxComponentFromName", &getMyGUIListBoxComponentFromName);
		gameObject.def("getMyGUIMessageBoxComponentFromName", &getMyGUIMessageBoxComponentFromName);
		gameObject.def("getMyGUIPositionControllerComponentFromName", &getMyGUIPositionControllerComponentFromName);
		gameObject.def("getMyGUIFadeAlphaControllerComponentFromName", &getMyGUIFadeAlphaControllerComponentFromName);
		gameObject.def("getMyGUIScrollingMessageControllerComponentFromName", &getMyGUIScrollingMessageControllerComponentFromName);
		gameObject.def("getMyGUIEdgeHideControllerComponentFromName", &getMyGUIEdgeHideControllerComponentFromName);
		gameObject.def("getMyGUIRepeatClickControllerComponentFromName", &getMyGUIRepeatClickControllerComponentFromName);
		gameObject.def("getMyGUIMiniMapComponentFromName", &getMyGUIMiniMapComponentFromName);
		gameObject.def("getLuaScriptComponentFromName", &getLuaScriptComponentFromName);

		AddClassToCollection("GameObject", "class", "Main NOWA type. A game object is a visual representation in the scene and can be composed of several components.");
		AddClassToCollection("GameObject", "string getId()", "Gets the id of this game object. Note: Id is the primary key, which uniquely identifies a game object.");
		AddClassToCollection("GameObject", "string getName()", "Gets the name of this game object.");
		AddClassToCollection("GameObject", "string getUniqueName()", "Gets the unique name, that is its name and the id.");
		AddClassToCollection("GameObject", "SceneManager getSceneManager()", "Gets the scene manager of this game object.");
		AddClassToCollection("GameObject", "SceneNode getSceneNode()", "Gets the scene node of this game object.");
		AddClassToCollection("GameObject", "Entity getEntity()", "Gets the entity of this game object.");
		AddClassToCollection("GameObject", "Vector3 getDefaultDirection()", "Gets the direction the game object's entity is modelled.");
		AddClassToCollection("GameObject", "string getCategory()", "Gets the category to which this game object does belong. Note: This is useful when doing ray-casts on graphics base or physics base or creating physics materials between categories.");
		AddClassToCollection("GameObject", "string getCategoryId()", "Gets the tag name of game object. Note: Tags are like sub-categories. E.g. several game objects may belong to the category 'Enemy', but one group may have a tag name 'Stone', the other 'Ship1', 'Ship2' etc. "
			"This is useful when doing ray-casts on graphics base or physics base or creating physics materials between categories, to further distinquish, which tag has been hit in order to remove different energy amount.");
		AddClassToCollection("GameObject", "string getTagName()", "Gets the category id to which this game object does belong. Note: Categories are specified in a level editor and are read from XML. This is useful when doing ray-casts on graphics base or physics base or creating physics materials between categories.");
		AddClassToCollection("GameObject", "void changeCategory(string oldCategory, string newCategory)", "Changes the category name, this game object belongs to.");
		AddClassToCollection("GameObject", "void changeCategory2(string newCategory)", "Changes the category name, this game object belongs to.");
		AddClassToCollection("GameObject", "setAttributePosition(Vector3 position)", "Sets the position of this game object and actualizes the position attribute.");
		AddClassToCollection("GameObject", "setAttributeScale(Vector3 scale)", "Sets the scale of this game object and actualizes the scale attribute.");
		AddClassToCollection("GameObject", "setAttributeOrientation(Quaternion orientation)", "Sets the orientation of this game object and actualizes the orientation attribute.");
		AddClassToCollection("GameObject", "Vector3 getPosition()", "Gets the position of this game object.");
		AddClassToCollection("GameObject", "Vector3 getScale()", "Gets the scale of this game object.");
		AddClassToCollection("GameObject", "Quaternion getOrientation()", "Gets the Orientation of this game object.");
		AddClassToCollection("GameObject", "number getControlledByClientID()", "Gets the client id, by which this game object will be controller. This is used in a network scenario.");
		AddClassToCollection("GameObject", "Vector3 getSize()", "Gets the bounding box size of this game object.");
		AddClassToCollection("GameObject", "Vector3 getCenterOffset()", "Gets offset vector by which the game object is away from the center of its bounding box.");
		AddClassToCollection("GameObject", "void setVisible(bool visible)", "Sets whether this game object should be visible.");
		AddClassToCollection("GameObject", "bool getVisible()", "Gets whether this game object is visible.");
		AddClassToCollection("GameObject", "void setClampY(bool clampY)", "Sets whether to clamp the y coordinate at the next lower game object. Note: If there is no one the next above game object is taken into accout.");
		AddClassToCollection("GameObject", "bool getClampY()", "Gets whether to clamp the y coordinate of this game object.");
		AddClassToCollection("GameObject", "void setReferenceId(String referenceId)", "Sets the reference id. Note: If this game object is referenced by another game object, a component with the same id as this game object can be get for activation etc.");
		AddClassToCollection("GameObject", "String getReferenceId()", "Gets the reference if another game object does reference it.");
		AddClassToCollection("GameObject", "void setDynamic(bool dynamic)", "Sets whether this game object is frequently moved (dynamic), if not set false here, this will increase the rendering performance.");
		AddClassToCollection("GameObject", "void setUseReflection(bool useReflection)", "Sets whether this game object uses shader reflection for visual effects.");
		AddClassToCollection("GameObject", "bool getUseReflection(bool getUseReflection)", "Gets whether this game object uses shader reflection for visual effects.");
		AddClassToCollection("GameObject", "bool showBoundingBox(bool show)", "Shows a bounding box for this game object if set to true.");
		AddClassToCollection("GameObject", "bool showDebugData()", "Shows a debug data (if exists) for this game object and all its components. If called a second time debug data will not be shown.");
		AddClassToCollection("GameObject", "GameObjectComponent getGameObjectComponentByIndex(unsigned int index)", "Gets the game object component by the given index. If the index is out of bounds, nil will be delivered.");
		AddClassToCollection("GameObject", "GameObjectComponent getComponentByIndex(GameObject gameObject, int index)", "Gets the component from the game objects by the index.");
		AddClassToCollection("GameObject", "number getIndexFromComponent(GameObjectComponent component)", "Gets index of the given component.");
		AddClassToCollection("GameObject", "bool deleteComponent(string componentClassName, unsigned int occurrenceIndex)", "Deletes the component by the given class name and optionally with its occurrence index.");
		AddClassToCollection("GameObject", "bool deleteComponentByIndex(unsigned int index)", "Deletes the component by the given index.");
		AddClassToCollection("GameObject", "GameObjectTitleComponent getGameObjectTitleComponent()", "Gets the game object title component.");
		AddClassToCollection("GameObject", "AnimationComponent getAnimationComponent2(unsigned int occurrenceIndex)", "Gets the animation component by the given occurence index, since a game object may have besides other components several animation components.");
		AddClassToCollection("GameObject", "AnimationComponent getAnimationComponent()", "Gets the animation component. This can be used if the game object just has one animation component.");
		AddClassToCollection("GameObject", "AttributesComponent getAttributesComponent()", "Gets the attributes component.");
		AddClassToCollection("GameObject", "AttributeEffectComponent getAttributeEffectComponentComponent(unsigned int occurrenceIndex)", "Gets the attribute effect component by the given occurence index, since a game object may have besides other components several animation components.");
		AddClassToCollection("GameObject", "AttributeEffectComponent getAttributeEffectComponentComponent2()", "Gets the animation component. This can be used if the game object just has one attribute effect component.");
		AddClassToCollection("GameObject", "BuoyancyComponent getBuoyancyComponent()", "Gets the physics buoyancy component.");
		AddClassToCollection("GameObject", "DistributedComponent getDistributedComponent()", "Gets the distributed component. Usage in a network scenario.");
		AddClassToCollection("GameObject", "ExitComponent getExitComponent()", "Gets the exit component.");
		AddClassToCollection("GameObject", "AiMoveComponent getAiMoveComponent()", "Gets the ai move component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiMoveRandomlyComponent getAiMoveRandomlyComponent()", "Gets the ai move randomly component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiPathFollowComponent getAiPathFollowComponent()", "Gets the ai path follow component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiWanderComponent getAiWanderComponent()", "Gets the ai wander component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiFlockingComponent getAiFlockingComponent()", "Gets the ai flocking component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiRecastPathNavigationComponent getAiRecastPathNavigationComponent()", "Gets the ai recast path navigation component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiObstacleAvoidanceComponent getAiObstacleAvoidanceComponent()", "Gets the ai obstacle avoidance component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiHideComponent getAiHideComponent()", "Gets the ai hide component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiMoveComponent2D getAiMoveComponent2D()", "Gets the ai move component for 2D. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiPathFollowComponent2D getAiPathFollowComponent2D()", "Gets the ai path follow component for 2D. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiWanderComponent2D getAiWanderComponent2D()", "Gets the ai wander component for 2D. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "CameraBehaviorBaseComponent getCameraBehaviorBaseComponent()", "Gets the camera behavior base component.");
		AddClassToCollection("GameObject", "CameraBehaviorFirstPersonComponent getCameraBehaviorFirstPersonComponent()", "Gets the camera behavior first person component.");
		AddClassToCollection("GameObject", "CameraBehaviorThirdPersonComponent getCameraBehaviorThirdPersonComponent()", "Gets the camera third first person component.");
		AddClassToCollection("GameObject", "CameraBehaviorFollow2DComponent getCameraBehaviorFollow2DComponent()", "Gets the camera follow 2D component.");
		AddClassToCollection("GameObject", "CameraBehaviorZoomComponent getCameraBehaviorZoomComponent()", "Gets the camera zoom component.");
		AddClassToCollection("GameObject", "CameraComponent getCameraComponent()", "Gets the camera component.");
		AddClassToCollection("GameObject", "CompositorEffectBloomComponent getCompositorEffectBloomComponent()", "Gets the compositor effect bloom component.");
		AddClassToCollection("GameObject", "CompositorEffectGlassComponent getCompositorEffectGlassComponent()", "Gets the compositor effect glass component.");
		AddClassToCollection("GameObject", "CompositorEffectBlackAndWhiteComponent getCompositorEffectBlackAndWhiteComponent()", "Gets the compositor effect black and white component.");
		AddClassToCollection("GameObject", "CompositorEffectColorComponent getCompositorEffectColorComponent()", "Gets the compositor effect color component.");
		AddClassToCollection("GameObject", "CompositorEffectEmbossedComponent getCompositorEffectEmbossedComponent()", "Gets the compositor effect embossed component.");
		AddClassToCollection("GameObject", "CompositorEffectSharpenEdgesComponent getCompositorEffectSharpenEdgesComponent()", "Gets the compositor effect sharpen edges component.");
		AddClassToCollection("GameObject", "HdrEffectComponent getHdrEffectComponent()", "Gets hdr effect component.");
		AddClassToCollection("GameObject", "DatablockPbsComponent getDatablockPbsComponent()", "Gets the datablock pbs (physically based shading) component.");
		AddClassToCollection("GameObject", "DatablockUnlitComponent getDatablockUnlitComponent()", "Gets the datablock unlit (without lighting) component.");
		AddClassToCollection("GameObject", "DatablockTerraComponent getDatablockTerraComponent()", "Gets the datablock terra component.");
		AddClassToCollection("GameObject", "JointComponent getJointComponent()", "Gets the joint root component. Requirements: A physics component.");
		AddClassToCollection("GameObject", "JointHingeComponent getJointHingeComponent()", "Gets the joint hinge component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointHingeActuatorComponent getJointHingeActuatorComponent()", "Gets the joint hinge actuator component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointBallAndSocketComponent getJointBallAndSocketComponent()", "Gets the joint ball and socket component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPointToPointComponent getJointPointToPointComponent()", "Gets the joint point to point component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPinComponent getJointPinComponent()", "Gets the joint pin component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPlaneComponent getJointPlaneComponent()", "Gets the joint plane component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointSpringComponent getJointSpringComponent()", "Gets the joint spring component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointAttractorComponent getJointAttractorComponent()", "Gets the joint attractor component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointCorkScrewComponent getJointCorkScrewComponent()", "Gets the joint cork screw component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPassiveSliderComponent getJointPassiveSliderComponent()", "Gets the joint passive slider component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointSliderActuatorComponent getJointSliderActuatorComponent()", "Gets the joint slider actuator component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointSlidingContactComponent getJointSlidingContactComponent()", "Gets the joint sliding contact component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointActiveSliderComponent getJointActiveSliderComponent()", "Gets the joint active slider component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointMathSliderComponent getJointMathSliderComponent()", "Gets the joint math slider component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointKinematicComponent getJointKinematicComponent()", "Gets the joint kinematic component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPathFollowComponent getJointPathFollowComponent()", "Gets the joint path follow component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointDryRollingFrictionComponent getJointDryRollingFrictionComponent()", "Gets the joint dry rolling friction component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointGearComponent getJointGearComponent()", "Gets the joint gear component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointRackAndPinionComponent getJointRackAndPinionComponent()", "Gets the joint rack and pinion component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointWormGearComponent getJointWormGearComponent()", "Gets the joint worm gear component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPulleyComponent getJointPulleyComponent()", "Gets the joint pulley component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointUniversalComponent getJointUniversalComponent()", "Gets the joint universal component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointUniversalActuatorComponent getJointUniversalActuatlorComponent()", "Gets the joint universal actuator component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "Joint6DofComponent getJoint6DofComponent()", "Gets the joint 6 DOF component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointMotorComponent getJointMotorComponent()", "Gets the joint motor component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointWheelComponent getJointWheelComponent()", "Gets the joint wheel component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "LightDirectionalComponent getLightDirectionalComponent()", "Gets the light directional component.");
		AddClassToCollection("GameObject", "LightPointComponent getLightPointComponent()", "Gets the light point component.");
		AddClassToCollection("GameObject", "LightSpotComponent getLightSpotComponent()", "Gets the light spot component.");
		AddClassToCollection("GameObject", "LightAreaComponent getLightAreaComponent()", "Gets the light area component.");
		AddClassToCollection("GameObject", "FadeComponent getFadeComponent()", "Gets the fade component.");
		AddClassToCollection("GameObject", "NavMeshComponent getNavMeshComponent()", "Gets the nav mesh component. Requirements: OgreRecast navigation must be activated.");
		AddClassToCollection("GameObject", "ParticleUniverseComponent getParticleUniverseComponent(unsigned int occurrenceIndex)", "Gets the particle universe component by the given occurence index, since a game object may have besides other components several particle universe components.");
		AddClassToCollection("GameObject", "ParticleUniverseComponent getParticleUniverseComponent2()", "Gets the particle universe component. This can be used if the game object just has one particle universe component.");
		AddClassToCollection("GameObject", "PlayerControllerJumpNRunComponent getPlayerControllerJumpNRunComponent()", "Gets the player controller Jump N Run component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "PlayerControllerJumpNRunLuaComponent getPlayerControllerJumpNRunLuaComponent()", "Gets the player controller Jump N Run Lua component. Requirements: A physics active component and a LuaScriptComponent.");
		AddClassToCollection("GameObject", "PlayerControllerClickToPointComponent getPlayerControllerClickToPointComponent()", "Gets the player controller click to point component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "PhysicsComponent getPhysicsComponent()", "Gets the physics component, which is the base for all other physics components.");
		AddClassToCollection("GameObject", "PhysicsActiveComponent getPhysicsActiveComponent()", "Gets the physics active component.");
		AddClassToCollection("GameObject", "PhysicsActiveCompoundComponent getPhysicsActiveCompoundComponent()", "Gets the physics active compound component.");
		AddClassToCollection("GameObject", "PhysicsActiveDestructibleComponent getPhysicsActiveDestructibleComponent()", "Gets the physics active destructable component.");
		AddClassToCollection("GameObject", "PhysicsArtifactComponent getPhysicsArtifactComponent()", "Gets the physics artifact component.");
		AddClassToCollection("GameObject", "PhysicsRagDollComponent getPhysicsRagDollComponent()", "Gets the physics ragdoll component.");
		AddClassToCollection("GameObject", "PhysicsCompoundConnectionComponent getPhysicsCompoundConnectionComponent()", "Gets the physics compound connection component.");
		AddClassToCollection("GameObject", "PhysicsMaterialComponent getPhysicsMaterialComponent2(unsigned int occurrenceIndex)", "Gets the physics material component by the given occurence index, since a game object may have besides other components several physics material components.");
		AddClassToCollection("GameObject", "PhysicsMaterialComponent getPhysicsMaterialComponent()", "Gets the physics material component. This can be used if the game object just has one physics material component.");
		AddClassToCollection("GameObject", "PhysicsPlayerControllerComponent getPhysicsPlayerControllerComponent()", "Gets the physics player controller component.");
		AddClassToCollection("GameObject", "PlaneComponent getPlaneComponent()", "Gets the plane component.");
		AddClassToCollection("GameObject", "SimpleSoundComponent getSimpleSoundComponent2(unsigned int occurrenceIndex)", "Gets the simple sound component by the given occurence index, since a game object may have besides other components several simple sound components.");
		AddClassToCollection("GameObject", "SimpleSoundComponent getSimpleSoundComponent()", "Gets the simple sound component. This can be used if the game object just has one simple sound component.");
		AddClassToCollection("GameObject", "SoundComponent getSoundComponent2(unsigned int occurrenceIndex)", "Gets the sound component by the given occurence index, since a game object may have besides other components several sound components.");
		AddClassToCollection("GameObject", "SoundComponent getSoundComponent()", "Gets the sound component. This can be used if the game object just has one sound component.");
		AddClassToCollection("GameObject", "SpawnComponent getSpawnComponent()", "Gets the spawn component.");
		AddClassToCollection("GameObject", "VehicleComponent getVehicleComponent()", "Gets the physics vehicle component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiLuaComponent getAiLuaComponent()", "Gets the ai lua script component. Requirements: A physics active component and a lua script component.");
		AddClassToCollection("GameObject", "PhysicsExplosionComponent getPhysicsExplosionComponent()", "Gets the physics explosion component.");
		AddClassToCollection("GameObject", "MeshDecalComponent getMeshDecalComponent()", "Gets the mesh decal component.");
		AddClassToCollection("GameObject", "TagPointComponent getTagPointComponent2(unsigned int occurrenceIndex)", "Gets the tag point component by the given occurence index, since a game object may have besides other components several tag point components.");
		AddClassToCollection("GameObject", "TagPointComponent getTagPointComponent()", "Gets the tag point component. This can be used if the game object just has one tag point component.");
		AddClassToCollection("GameObject", "TimeTriggerComponent getTimeTriggerComponent2(unsigned int occurrenceIndex)", "Gets the time trigger component by the given occurence index, since a game object may have besides other components several tag point components.");
		AddClassToCollection("GameObject", "TimeTriggerComponent getTimeTriggerComponent()", "Gets the time trigger component. This can be used if the game object just has one time trigger component.");
		AddClassToCollection("GameObject", "TimeLineComponent getTimeLineComponent()", "Gets the time line component.");
		AddClassToCollection("GameObject", "MoveMathFunctionComponent getMoveMathFunctionComponent()", "Gets the move math function component.");
		AddClassToCollection("GameObject", "TagChildNodeComponent getTagChildNodeComponent(unsigned int occurrenceIndex)", "Gets the tag child node component by the given occurence index, since a game object may have besides other components several tag child node components.");
		AddClassToCollection("GameObject", "TagChildNodeComponent getTagChildNodeComponent2()", "Gets the tag child node component. This can be used if the game object just has one tag child node component.");
		AddClassToCollection("GameObject", "NodeTrackComponent getNodeTrackComponent()", "Gets the node track component.");
		AddClassToCollection("GameObject", "LineComponent getLineComponent()", "Gets the line component.");
		AddClassToCollection("GameObject", "LinesComponent getLinesComponent()", "Gets the lines component.");
		AddClassToCollection("GameObject", "NodeComponent getNodeComponent()", "Gets the node component.");
		AddClassToCollection("GameObject", "ManualObjectComponent getManualObjectComponent()", "Gets the manual object component.");
		AddClassToCollection("GameObject", "RectangleComponent getRectangleComponent()", "Gets the rectangle component.");
		AddClassToCollection("GameObject", "ValueBarComponent getValueBarComponent()", "Gets the value bar component.");
		AddClassToCollection("GameObject", "BillboardComponent getBillboardComponent()", "Gets the billboard component.");
		AddClassToCollection("GameObject", "RibbonTrailComponent getRibbonTrailComponent()", "Gets the ribbon trail component.");
		AddClassToCollection("GameObject", "MyGUIWindowComponent getMyGUIWindowComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI window component by the given occurence index, since a game object may have besides other components several MyGUI window components.");
		AddClassToCollection("GameObject", "MyGUIWindowComponent getMyGUIWindowComponent()", "Gets the MyGUI window component. This can be used if the game object just has one MyGUI window component.");
		AddClassToCollection("GameObject", "MyGUIItemBoxComponent getMyGUIItemBoxComponent()", "Gets the MyGUI item box component. This can be used for inventory item in conjunction with InventoryItemComponent.");
		AddClassToCollection("GameObject", "InventoryItemComponent getInventoryItemComponent()", "Gets the inventory item component. This can be used for inventory item in conjunction with MyGUIItemBoxComponent.");
		AddClassToCollection("GameObject", "MyGUITextComponent getMyGUITextComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI text component by the given occurence index, since a game object may have besides other components several MyGUI text components.");
		AddClassToCollection("GameObject", "MyGUITextComponent getMyGUITextComponent()", "Gets the MyGUI text component. This can be used if the game object just has one MyGUI text component.");
		AddClassToCollection("GameObject", "MyGUIButtonComponent getMyGUIButtonComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI button component by the given occurence index, since a game object may have besides other components several MyGUI button components.");
		AddClassToCollection("GameObject", "MyGUIButtonComponent getMyGUIButtonComponent()", "Gets the MyGUI button component. This can be used if the game object just has one MyGUI button component.");
		AddClassToCollection("GameObject", "MyGUICheckBoxComponent getMyGUICheckBoxComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI check box component by the given occurence index, since a game object may have besides other components several MyGUI check box components.");
		AddClassToCollection("GameObject", "MyGUICheckBoxComponent getMyGUICheckBoxComponent()", "Gets the MyGUI check box component. This can be used if the game object just has one MyGUI check box component.");
		AddClassToCollection("GameObject", "MyGUIImageBoxComponent getMyGUIImageBoxComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI image box component by the given occurence index, since a game object may have besides other components several MyGUI image box components.");
		AddClassToCollection("GameObject", "MyGUIImageBoxComponent getMyGUIImageBoxComponent()", "Gets the MyGUI image box component. This can be used if the game object just has one MyGUI image box component.");
		AddClassToCollection("GameObject", "MyGUIProgressBarComponent getMyGUIProgressBarComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI progress bar component by the given occurence index, since a game object may have besides other components several MyGUI progress bar components.");
		AddClassToCollection("GameObject", "MyGUIProgressBarComponent getMyGUIProgressBarComponent()", "Gets the MyGUI progress bar component. This can be used if the game object just has one MyGUI progress bar component.");
		AddClassToCollection("GameObject", "MyGUIListBoxComponent getMyGUIListBoxComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI list box component by the given occurence index, since a game object may have besides other components several MyGUI list box components.");
		AddClassToCollection("GameObject", "MyGUIListBoxComponent getMyGUIListBoxComponent()", "Gets the MyGUI list box component. This can be used if the game object just has one MyGUI list box component.");
		AddClassToCollection("GameObject", "MyGUIComboBoxComponent getMyGUIComboBoxComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI combo box component by the given occurence index, since a game object may have besides other components several MyGUI combo box components.");
		AddClassToCollection("GameObject", "MyGUIComboBoxComponent getMyGUIComboBoxComponent()", "Gets the MyGUI combo box component. This can be used if the game object just has one MyGUI combo box component.");
		AddClassToCollection("GameObject", "MyGUIMessageBoxComponent getMyGUIMessageBoxComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI message box component by the given occurence index, since a game object may have besides other components several MyGUI message box components.");
		AddClassToCollection("GameObject", "MyGUIMessageBoxComponent getMyGUIMessageBoxComponent()", "Gets the MyGUI message box component. This can be used if the game object just has one MyGUI message box component.");
		AddClassToCollection("GameObject", "MyGUIPositionControllerComponent getMyGUIPositionControllerComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI position controller component by the given occurence index, since a game object may have besides other components several MyGUI position controller components.");
		AddClassToCollection("GameObject", "MyGUIPositionControllerComponent getMyGUIPositionControllerComponent()", "Gets the MyGUI position controller component. This can be used if the game object just has one MyGUI position controller component.");
		AddClassToCollection("GameObject", "MyGUIFadeAlphaControllerComponent getMyGUIFadeAlphaControllerComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI fade alpha controller component by the given occurence index, since a game object may have besides other components several MyGUI fade alpha components.");
		AddClassToCollection("GameObject", "MyGUIFadeAlphaControllerComponent getMyGUIFadeAlphaControllerComponent()", "Gets the MyGUI fade alpha controller component. This can be used if the game object just has one MyGUI fade alpha controller component.");
		AddClassToCollection("GameObject", "MyGUIScrollingMessageControllerComponent getMyGUIScrollingMessageControllerComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI scrolling message controller component by the given occurence index, since a game object may have besides other components several MyGUI scrolling message components.");
		AddClassToCollection("GameObject", "MyGUIScrollingMessageControllerComponent getMyGUIFScrollingMessageControllerComponent()", "Gets the MyGUI scrolling message controller component. This can be used if the game object just has one MyGUI scrolling message controller component.");
		AddClassToCollection("GameObject", "MyGUIEdgeHideControllerComponent getMyGUIEdgeHideControllerComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI hide edge controller component by the given occurence index, since a game object may have besides other components several MyGUI hide edge components.");
		AddClassToCollection("GameObject", "MyGUIEdgeHideControllerComponent getMyGUIEdgeHideControllerComponent()", "Gets the MyGUI hide edge controller component. This can be used if the game object just has one MyGUI hide edge controller component.");
		AddClassToCollection("GameObject", "MyGUIRepeatClickControllerComponent getMyGUIRepeatClickControllerComponent2(unsigned int occurrenceIndex)", "Gets the MyGUI repeat click controller component by the given occurence index, since a game object may have besides other components several MyGUI repeat click components.");
		AddClassToCollection("GameObject", "MyGUIRepeatClickControllerComponent getMyGUIRepeatClickControllerComponent()", "Gets the MyGUI repeat click controller component. This can be used if the game object just has one MyGUI repeat click controller component.");
		AddClassToCollection("GameObject", "MyGUIMiniMapComponent getMyGUIMiniMapComponent()", "Gets the MyGUI mini map component. Note: There can only be one mini map.");
		AddClassToCollection("GameObject", "LuaScriptComponent getLuaScriptComponent()", "Gets the lua script component.");

		AddClassToCollection("GameObject", "AnimationComponent getAnimationComponentFromName(String name)", "Gets the animation component.");
		AddClassToCollection("GameObject", "AttributesComponent getAttributesComponentFromName(String name)", "Gets the attributes component.");
		AddClassToCollection("GameObject", "AttributeEffectComponent getAttributeEffectComponentComponentFromName(String nameunsigned int occurrenceIndex)", "Gets the attribute effect component by the given occurence index, since a game object may have besides other components several animation components.");
		AddClassToCollection("GameObject", "BuoyancyComponent getBuoyancyComponentFromName(String name)", "Gets the physics buoyancy component.");
		AddClassToCollection("GameObject", "DistributedComponent getDistributedComponentFromName(String name)", "Gets the distributed component. Usage in a network scenario.");
		AddClassToCollection("GameObject", "ExitComponent getExitComponentFromName(String name)", "Gets the exit component.");
		AddClassToCollection("GameObject", "AiMoveComponent getAiMoveComponentFromName(String name)", "Gets the ai move component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiMoveRandomlyComponent getAiMoveRandomlyComponentFromName(String name)", "Gets the ai move randomly component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiPathFollowComponent getAiPathFollowComponentFromName(String name)", "Gets the ai path follow component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiWanderComponent getAiWanderComponentFromName(String name)", "Gets the ai wander component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiFlockingComponent getAiFlockingComponentFromName(String name)", "Gets the ai flocking component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiRecastPathNavigationComponent getAiRecastPathNavigationComponentFromName(String name)", "Gets the ai recast path navigation component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiObstacleAvoidanceComponent getAiObstacleAvoidanceComponentFromName(String name)", "Gets the ai obstacle avoidance component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiHideComponent getAiHideComponentFromName(String name)", "Gets the ai hide component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiMoveComponent2D getAiMoveComponent2D()", "Gets the ai move component for 2D. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiPathFollowComponent2D getAiPathFollowComponent2D()", "Gets the ai path follow component for 2D. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiWanderComponent2D getAiWanderComponent2D()", "Gets the ai wander component for 2D. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "CameraBehaviorBaseComponent getCameraBehaviorBaseComponentFromName(String name)", "Gets the camera behavior base component.");
		AddClassToCollection("GameObject", "CameraBehaviorFirstPersonComponent getCameraBehaviorFirstPersonComponentFromName(String name)", "Gets the camera behavior first person component.");
		AddClassToCollection("GameObject", "CameraBehaviorThirdPersonComponent getCameraBehaviorThirdPersonComponentFromName(String name)", "Gets the camera third first person component.");
		AddClassToCollection("GameObject", "CameraBehaviorFollow2DComponent getCameraBehaviorFollow2DComponentFromName(String name)", "Gets the camera follow 2D component.");
		AddClassToCollection("GameObject", "CameraBehaviorZoomComponent getCameraBehaviorZoomComponentFromName(String name)", "Gets the camera zoom component.");
		AddClassToCollection("GameObject", "CameraComponent getCameraComponentFromName(String name)", "Gets the camera component.");
		AddClassToCollection("GameObject", "CompositorEffectBloomComponent getCompositorEffectBloomComponentFromName(String name)", "Gets the compositor effect bloom component.");
		AddClassToCollection("GameObject", "CompositorEffectGlassComponent getCompositorEffectGlassComponentFromName(String name)", "Gets the compositor effect glass component.");
		AddClassToCollection("GameObject", "CompositorEffectBlackAndWhiteComponent getCompositorEffectBlackAndWhiteComponentFromName(String name)", "Gets the compositor effect black and white component.");
		AddClassToCollection("GameObject", "CompositorEffectColorComponent getCompositorEffectColorComponentFromName(String name)", "Gets the compositor effect color component.");
		AddClassToCollection("GameObject", "CompositorEffectEmbossedComponent getCompositorEffectEmbossedComponentFromName(String name)", "Gets the compositor effect embossed component.");
		AddClassToCollection("GameObject", "CompositorEffectSharpenEdgesComponent getCompositorEffectSharpenEdgesComponentFromName(String name)", "Gets the compositor effect sharpen edges component.");
		AddClassToCollection("GameObject", "HdrEffectComponent getHdrEffectComponentFromName(String name)", "Gets hdr effect component.");
		AddClassToCollection("GameObject", "DatablockPbsComponent getDatablockPbsComponentFromName(String name)", "Gets the datablock pbs (physically based shading) component.");
		AddClassToCollection("GameObject", "DatablockUnlitComponent getDatablockUnlitComponentFromName(String name)", "Gets the datablock unlit (without lighting) component.");
		AddClassToCollection("GameObject", "DatablockTerraComponent getDatablockTerraComponentFromName(String name)", "Gets the datablock terra component.");
		AddClassToCollection("GameObject", "JointComponent getJointComponentFromName(String name)", "Gets the joint root component. Requirements: A physics component.");
		AddClassToCollection("GameObject", "JointHingeComponent getJointHingeComponentFromName(String name)", "Gets the joint hinge component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointHingeActuatorComponent getJointHingeActuatorComponentFromName(String name)", "Gets the joint hinge actuator component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointBallAndSocketComponent getJointBallAndSocketComponentFromName(String name)", "Gets the joint ball and socket component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPointToPointComponent getJointPointToPointComponentFromName(String name)", "Gets the joint point to point component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPinComponent getJointPinComponentFromName(String name)", "Gets the joint pin component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPlaneComponent getJointPlaneComponentFromName(String name)", "Gets the joint plane component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointSpringComponent getJointSpringComponentFromName(String name)", "Gets the joint spring component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointAttractorComponent getJointAttractorComponentFromName(String name)", "Gets the joint attractor component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointCorkScrewComponent getJointCorkScrewComponentFromName(String name)", "Gets the joint cork screw component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPassiveSliderComponent getJointPassiveSliderComponentFromName(String name)", "Gets the joint passive slider component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointSliderActuatorComponent getJointSliderActuatorComponentFromName(String name)", "Gets the joint slider actuator component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointSlidingContactComponent getJointSlidingContactComponentFromName(String name)", "Gets the joint sliding contact component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointActiveSliderComponent getJointActiveSliderComponentFromName(String name)", "Gets the joint active slider component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointMathSliderComponent getJointMathSliderComponentFromName(String name)", "Gets the joint math slider component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointKinematicComponent getJointKinematicComponentFromName(String name)", "Gets the joint kinematic component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPathFollowComponent getJointPathFollowComponentFromName(String name)", "Gets the joint path follow component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointDryRollingFrictionComponent getJointDryRollingFrictionComponentFromName(String name)", "Gets the joint dry rolling friction component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointGearComponent getJointGearComponentFromName(String name)", "Gets the joint gear component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointRackAndPinionComponent getJointRackAndPinionComponentFromName(String name)", "Gets the joint rack and pinion component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointWormGearComponent getJointWormGearComponentFromName(String name)", "Gets the joint worm gear component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointPulleyComponent getJointPulleyComponentFromName(String name)", "Gets the joint pulley component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointUniversalComponent getJointUniversalComponentFromName(String name)", "Gets the joint universal component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointUniversalActuatorComponent getJointUniversalActuatlorComponentFromName(String name)", "Gets the joint universal actuator component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "Joint6DofComponent getJoint6DofComponentFromName(String name)", "Gets the joint 6 DOF component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointMotorComponent getJointMotorComponentFromName(String name)", "Gets the joint motor component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "JointWheelComponent getJointWheelComponentFromName(String name)", "Gets the joint wheel component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "LightDirectionalComponent getLightDirectionalComponentFromName(String name)", "Gets the light directional component.");
		AddClassToCollection("GameObject", "LightPointComponent getLightPointComponentFromName(String name)", "Gets the light point component.");
		AddClassToCollection("GameObject", "LightSpotComponent getLightSpotComponentFromName(String name)", "Gets the light spot component.");
		AddClassToCollection("GameObject", "LightAreaComponent getLightAreaComponentFromName(String name)", "Gets the light area component.");
		AddClassToCollection("GameObject", "FadeComponent getFadeComponentFromName(String name)", "Gets the fade component.");
		AddClassToCollection("GameObject", "NavMeshComponent getNavMeshComponentFromName(String name)", "Gets the nav mesh component. Requirements: OgreRecast navigation must be activated.");
		AddClassToCollection("GameObject", "ParticleUniverseComponent getParticleUniverseComponentFromName(String nameunsigned int occurrenceIndex)", "Gets the particle universe component by the given occurence index, since a game object may have besides other components several particle universe components.");
		AddClassToCollection("GameObject", "PlayerControllerJumpNRunComponent getPlayerControllerJumpNRunComponentFromName(String name)", "Gets the player controller Jump N Run component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "PlayerControllerJumpNRunLuaComponent getPlayerControllerJumpNRunLuaComponentFromName(String name)", "Gets the player controller Jump N Run Lua component. Requirements: A physics active component and a LuaScriptComponent.");
		AddClassToCollection("GameObject", "PlayerControllerClickToPointComponent getPlayerControllerClickToPointComponentFromName(String name)", "Gets the player controller click to point component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "PhysicsComponent getPhysicsComponentFromName(String name)", "Gets the physics component, which is the base for all other physics components.");
		AddClassToCollection("GameObject", "PhysicsActiveComponent getPhysicsActiveComponentFromName(String name)", "Gets the physics active component.");
		AddClassToCollection("GameObject", "PhysicsActiveCompoundComponent getPhysicsActiveCompoundComponentFromName(String name)", "Gets the physics active compound component.");
		AddClassToCollection("GameObject", "PhysicsActiveDestructibleComponent getPhysicsActiveDestructibleComponentFromName(String name)", "Gets the physics active destructable component.");
		AddClassToCollection("GameObject", "PhysicsArtifactComponent getPhysicsArtifactComponentFromName(String name)", "Gets the physics artifact component.");
		AddClassToCollection("GameObject", "PhysicsRagDollComponent getPhysicsRagDollComponentFromName(String name)", "Gets the physics ragdoll component.");
		AddClassToCollection("GameObject", "PhysicsCompoundConnectionComponent getPhysicsCompoundConnectionComponentFromName(String name)", "Gets the physics compound connection component.");
		AddClassToCollection("GameObject", "PhysicsMaterialComponent getPhysicsMaterialComponentFromName(String name)", "Gets the physics material component.");
		AddClassToCollection("GameObject", "PhysicsPlayerControllerComponent getPhysicsPlayerControllerComponentFromName(String name)", "Gets the physics player controller component.");
		AddClassToCollection("GameObject", "PlaneComponent getPlaneComponentFromName(String name)", "Gets the plane component.");
		AddClassToCollection("GameObject", "SimpleSoundComponent getSimpleSoundComponentFromName(String name)", "Gets the simple sound component.");
		AddClassToCollection("GameObject", "SoundComponent getSoundComponentFromName(String name)", "Gets the sound component.");
		AddClassToCollection("GameObject", "SpawnComponent getSpawnComponentFromName(String name)", "Gets the spawn component.");
		AddClassToCollection("GameObject", "VehicleComponent getVehicleComponentFromName(String name)", "Gets the physics vehicle component. Requirements: A physics active component.");
		AddClassToCollection("GameObject", "AiLuaComponent getAiLuaComponentFromName(String name)", "Gets the ai lua script component. Requirements: A physics active component and a lua script component.");
		AddClassToCollection("GameObject", "PhysicsExplosionComponent getPhysicsExplosionComponentFromName(String name)", "Gets the physics explosion component.");
		AddClassToCollection("GameObject", "MeshDecalComponent getMeshDecalComponentFromName(String name)", "Gets the mesh decal component.");
		AddClassToCollection("GameObject", "TagPointComponent getTagPointComponentFromName(String name)", "Gets the tag point component.");
		AddClassToCollection("GameObject", "TimeTriggerComponent getTimeTriggerComponentFromName(String name)", "Gets the time trigger component.");
		AddClassToCollection("GameObject", "TimeLineComponent getTimeLineComponentFromName(String name)", "Gets the time line component.");
		AddClassToCollection("GameObject", "MoveMathFunctionComponent getMoveMathFunctionComponentFromName(String name)", "Gets the mvoe math function component.");
		AddClassToCollection("GameObject", "TagChildNodeComponent getTagChildNodeComponentFromName(String nameunsigned int occurrenceIndex)", "Gets the tag child node component by the given occurence index, since a game object may have besides other components several tag child node components.");
		AddClassToCollection("GameObject", "NodeTrackComponent getNodeTrackComponentFromName(String name)", "Gets the node track component.");
		AddClassToCollection("GameObject", "LineComponent getLineComponentFromName(String name)", "Gets the line component.");
		AddClassToCollection("GameObject", "LinesComponent getLinesComponentFromName(String name)", "Gets the lines component.");
		AddClassToCollection("GameObject", "NodeComponent getNodeComponentFromName(String name)", "Gets the node component.");
		AddClassToCollection("GameObject", "ManualObjectComponent getManualObjectComponentFromName(String name)", "Gets the manual object component.");
		AddClassToCollection("GameObject", "RectangleComponent getRectangleComponentFromName(String name)", "Gets the rectangle component.");
		AddClassToCollection("GameObject", "ValueBarComponent getValueBarComponentFromName(String name)", "Gets the value bar component.");
		AddClassToCollection("GameObject", "BillboardComponent getBillboardComponentFromName(String name)", "Gets the billboard component.");
		AddClassToCollection("GameObject", "RibbonTrailComponent getRibbonTrailComponentFromName(String name)", "Gets the ribbon trail component.");
		AddClassToCollection("GameObject", "MyGUIWindowComponent getMyGUIWindowComponentFromName(String name)", "Gets the MyGUI window component.");
		AddClassToCollection("GameObject", "MyGUIItemBoxComponent getMyGUIItemBoxComponentFromName(String name)", "Gets the MyGUI item box component. This can be used for inventory item in conjunction with InventoryItemComponent.");
		AddClassToCollection("GameObject", "InventoryItemComponent getInventoryItemComponentFromName(String name)", "Gets the inventory item component. This can be used for inventory item in conjunction with MyGUIItemBoxComponent.");
		AddClassToCollection("GameObject", "MyGUITextComponent getMyGUITextComponentFromName(String name)", "Gets the MyGUI text component.");
		AddClassToCollection("GameObject", "MyGUIButtonComponent getMyGUIButtonComponentFromName(String name)", "Gets the MyGUI button component.");
		AddClassToCollection("GameObject", "MyGUICheckBoxComponent getMyGUICheckBoxComponentFromName(String name)", "Gets the MyGUI check box component.");
		AddClassToCollection("GameObject", "MyGUIImageBoxComponent getMyGUIImageBoxComponentFromName(String name)", "Gets the MyGUI image box component.");
		AddClassToCollection("GameObject", "MyGUIProgressBarComponent getMyGUIProgressBarComponentFromName(String name)", "Gets the MyGUI progress bar component.");
		AddClassToCollection("GameObject", "MyGUIListBoxComponent getMyGUIListBoxComponentFromName(String name)", "Gets the MyGUI list box component.");
		AddClassToCollection("GameObject", "MyGUIComboBoxComponent getMyGUIComboBoxComponentFromName(String name)", "Gets the MyGUI combo box component.");
		AddClassToCollection("GameObject", "MyGUIMessageBoxComponent getMyGUIMessageBoxComponentFromName(String name)", "Gets the MyGUI message box component.");
		AddClassToCollection("GameObject", "MyGUIPositionControllerComponent getMyGUIPositionControllerComponentFromName(String name)", "Gets the MyGUI position controller component.");
		AddClassToCollection("GameObject", "MyGUIFadeAlphaControllerComponent getMyGUIFadeAlphaControllerComponentFromName(String name)", "Gets the MyGUI fade alpha controller component.");
		AddClassToCollection("GameObject", "MyGUIScrollingMessageControllerComponent getMyGUIFScrollingMessageControllerComponentFromName(String name)", "Gets the MyGUI scrolling message controller component.");
		AddClassToCollection("GameObject", "MyGUIEdgeHideControllerComponent getMyGUIEdgeHideControllerComponentFromName(String name)", "Gets the MyGUI hide edge controller component.");
		AddClassToCollection("GameObject", "MyGUIRepeatClickControllerComponent getMyGUIRepeatClickControllerComponentFromName(String name)", "Gets the MyGUI repeat click controller component.");
		AddClassToCollection("GameObject", "MyGUIMiniMapComponent getMyGUIMiniMapComponentFromName(String name)", "Gets the MyGUI mini map component. Note: There can only be one mini map.");
		AddClassToCollection("GameObject", "LuaScriptComponent getLuaScriptComponentFromName(String name)", "Gets the lua script component.");
	}

	// Note: Shared Ptr are to complicated, they won't be deleted somehow, even when forceGarbageCollection is called and when the whole application will be ended
	// a crash occurs, because lua tries to clean a game object shared ptr! Thats not everything: When GameObject is extented with shared_ptr<GameObject> when defined
	// its no more possible to work with raw GameObject's. So to not give ownership to scripted game objects!
	// When using smart pointers, at the end of the script the game object must be set to nil!
	GameObject* getGameObjectFromId(GameObjectController* instance, const Ogre::String& id)
	{
		auto& gameObject = instance->getGameObjectFromId(Ogre::StringConverter::parseUnsignedLong(id));
		if (nullptr != gameObject)
		{
			return gameObject.get();
		}

		currentErrorGameObject = id;
		currentCalledFunction = "getGameObjectFromId";

		return nullptr;
	}

	GameObject* clone(GameObjectController* instance, const Ogre::String& originalGameObjectId, Ogre::SceneNode* parentNode, const Ogre::String& targetId, const Ogre::Vector3& targetPosition,
		const Ogre::Quaternion& targetOrientation, const Ogre::Vector3& targetScale)
	{
		auto& clonedGameObjectPtr = instance->clone(Ogre::StringConverter::parseUnsignedLong(originalGameObjectId), parentNode, Ogre::StringConverter::parseUnsignedLong(targetId), targetPosition, targetOrientation, targetScale);
		if (nullptr != clonedGameObjectPtr)
		{
			// Connect the object
			clonedGameObjectPtr->connect();
			return clonedGameObjectPtr.get();
		}

		currentErrorGameObject = originalGameObjectId;
		currentCalledFunction = "clone";

		return nullptr;
	}

	GameObject* getClonedGameObjectFromPriorId(GameObjectController* instance, const Ogre::String& priorId)
	{
		auto& gameObject = instance->getClonedGameObjectFromPriorId(Ogre::StringConverter::parseUnsignedLong(priorId));
		if (nullptr != gameObject)
			return gameObject.get();

		currentErrorGameObject = priorId;
		currentCalledFunction = "getClonedGameObjectFromPriorId";

		return nullptr;
	}

	GameObject* getGameObjectFromName(GameObjectController* instance, const Ogre::String& name)
	{
		auto& gameObject = instance->getGameObjectFromName(name);
		if (nullptr != gameObject)
			return gameObject.get();

		currentErrorGameObject = name;
		currentCalledFunction = "getGameObjectFromName";

		return nullptr;
	}

	GameObject* getGameObjectFromNamePrefix(GameObjectController* instance, const Ogre::String& pattern)
	{
		auto& gameObject = instance->getGameObjectFromNamePrefix(pattern);
		if (nullptr != gameObject)
			return gameObject.get();

		currentErrorGameObject = pattern;
		currentCalledFunction = "getGameObjectFromNamePrefix";

		return nullptr;
	}

	/// Note: return_stl_iterator returning std::vector directly is not possible, because the vector is locally created in that methods
	// and a copy would be return which causes 'incompatbile iterators' exception. So the iterator is as in/out param
	// There is no solution, so use luabind::object as container for transformation c++ data to lua data

	luabind::object getGameObjectsFromCategory(GameObjectController* instance, const Ogre::String& category)
	{
		// Game objects need to be transformed to a lua object, which is a table
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		unsigned int i = 0;
		for (auto& it = instance->getGameObjects()->cbegin(); it != instance->getGameObjects()->cend(); ++it)
		{
			if (it->second->getCategory() == category)
			{
				obj[i++] = it->second.get();
			}
		}

		currentErrorGameObject = category;
		currentCalledFunction = "getGameObjectsFromCategory";

		return obj;
	}

	luabind::object getVUPointsData(SimpleSoundComponent* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		OgreAL::Sound* sound = instance->getSound();
		if (nullptr != sound)
		{
			unsigned short j = 0;
			for (unsigned short i = 0; i < sound->getSpectrumProcessingSize() * 2; i++)
			{
				obj[j++] = sound->getSpectrumParameter()->VUpoints[i];
			}
		}
		return obj;
	}

	luabind::object getAmplitudeData(SimpleSoundComponent* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		OgreAL::Sound* sound = instance->getSound();
		if (nullptr != sound)
		{
			unsigned short j = 0;
			for (unsigned short i = 0; i < sound->getSpectrumProcessingSize(); i++)
			{
				obj[j++] = sound->getSpectrumParameter()->amplitude[i];
			}
		}
		return obj;
	}

	luabind::object getLevelData(SimpleSoundComponent* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		OgreAL::Sound* sound = instance->getSound();
		if (nullptr != sound)
		{
			unsigned short j = 0;
			for (unsigned short i = 0; i < sound->getSpectrumNumberOfBands(); i++)
			{
				obj[j++] = sound->getSpectrumParameter()->level[i];
			}
		}
		return obj;
	}

	luabind::object getFrequencyData(SimpleSoundComponent* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		OgreAL::Sound* sound = instance->getSound();
		if (nullptr != sound)
		{
			unsigned short j = 0;
			for (unsigned short i = 0; i < sound->getSpectrumProcessingSize(); i++)
			{
				obj[j++] = sound->getSpectrumParameter()->frequency[i];
			}
		}
		return obj;
	}

	luabind::object getPhaseData(SimpleSoundComponent* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		OgreAL::Sound* sound = instance->getSound();
		if (nullptr != sound)
		{
			unsigned short j = 0;
			for (unsigned short i = 0; i < sound->getSpectrumProcessingSize(); i++)
			{
				obj[j++] = sound->getSpectrumParameter()->phase[i];
			}
		}
		return obj;
	}

	luabind::object getGameObjectsFromNamePrefix(GameObjectController* instance, const Ogre::String& pattern)
	{
		// Game objects need to be transformed to a lua object, which is a table
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		unsigned int i = 0;
		for (auto& it = instance->getGameObjects()->cbegin(); it != instance->getGameObjects()->cend(); ++it)
		{
			if (Ogre::StringUtil::match(it->second->getSceneNode()->getName(), pattern, false))
			{
				obj[i++] = it->second.get();
			}
		}

		return obj;
	}

	luabind::object getGameObjects(GameObjectController* instance)
	{
		// Game objects need to be transformed to a lua object, which is a table
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		unsigned int i = 0;
		for (auto& it = instance->getGameObjects()->cbegin(); it != instance->getGameObjects()->cend(); ++it)
		{
			obj[i++] = it->second.get();
		}

		return obj;
	}

	luabind::object getIdsFromCategory(GameObjectController* instance, const Ogre::String& category)
	{
		// Game objects need to be transformed to a lua object, which is a table
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		unsigned int i = 0;
		for (auto& it = instance->getGameObjects()->cbegin(); it != instance->getGameObjects()->cend(); ++it)
		{
			if (it->second->getCategory() == category)
			{
				obj[i++] = it->second->getId();
			}
		}

		return obj;
	}

	luabind::object getOtherIdsFromCategory(GameObjectController* instance, GameObject* excludedGameObject, const Ogre::String& category)
	{
		// Game objects need to be transformed to a lua object, which is a table
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		unsigned int i = 0;
		for (auto& it = instance->getGameObjects()->cbegin(); it != instance->getGameObjects()->cend(); ++it)
		{
			if (it->second->getCategory() == category && it->second.get() != excludedGameObject)
			{
				obj[i++] = Ogre::StringConverter::toString(it->second->getId());
			}
		}

		return obj;
	}

	luabind::object getGameObjectsControlledByClientId(GameObjectController* instance, unsigned int clientID)
	{
		// Game objects need to be transformed to a lua object, which is a table
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		// only search for game objects that are controlled by a client. Client id 0 means, it is not controlled
		if (0 == clientID)
		{
			return obj;
		}

		unsigned int i = 0;
		for (auto& it = instance->getGameObjects()->cbegin(); it != instance->getGameObjects()->cend(); ++it)
		{
			if (it->second->getControlledByClientID() == clientID)
			{
				obj[i++] = it->second.get();
			}
		}

		return obj;
	}

	luabind::object getGameObjectsFromTagName(GameObjectController* instance, const Ogre::String& tagName)
	{
		// Game objects need to be transformed to a lua object, which is a table
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		// only search for game objects that are controlled by a client. Client id 0 means, it is not controlled
		if (true == tagName.empty())
		{
			return obj;
		}

		unsigned int i = 0;
		for (auto& it = instance->getGameObjects()->cbegin(); it != instance->getGameObjects()->cend(); ++it)
		{
			if (it->second->getTagName() == tagName)
			{
				obj[i++] = it->second.get();
			}
		}

		return obj;
	}

	GameObject* getGameObjectFromReferenceId(GameObjectController* instance, const Ogre::String& referenceId)
	{
		auto& gameObject = instance->getGameObjectFromId(Ogre::StringConverter::parseUnsignedLong(referenceId));
		if (nullptr != gameObject)
		{
			return gameObject.get();
		}

		currentErrorGameObject = referenceId;
		currentCalledFunction = "getGameObjectFromReferenceId";

		return nullptr;
	}

	luabind::object getGameObjectComponentsFromReferenceId(GameObjectController* instance, const Ogre::String& referenceId)
	{
		// Game objects need to be transformed to a lua object, which is a table
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		unsigned int i = 0;

		std::vector<GameObjectCompPtr> vec;

		unsigned long lReferenceId = Ogre::StringConverter::parseUnsignedLong(referenceId);
		auto& gameObjectPtr = instance->getGameObjectFromId(lReferenceId);
		if (nullptr == gameObjectPtr)
		{
			currentErrorGameObject = referenceId;
			currentCalledFunction = "getGameObjectComponentsFromReferenceId";

			return obj;
		}

		// Search for a component with the same id
		for (auto& it = instance->getGameObjects()->cbegin(); it != instance->getGameObjects()->cend(); ++it)
		{
			if (it->second->getReferenceId() == lReferenceId)
			{
				obj[i++] = it->second.get();
			}
		}

		return obj;
	}

	MyGUI::TextBox* createTextBox(int left, int right, int width, int height, MyGUI::Align align, const Ogre::String& layer, const Ogre::String& name = "")
	{
		return MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(left, right, width, height), align, layer, name);
	}

	MyGUI::TextBox* createRealTextBox(float left, float right, float width, float height, MyGUI::Align align, const Ogre::String& layer, const Ogre::String& name = "")
	{
		return MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::TextBox>("TextBox", MyGUI::FloatCoord(left, right, width, height), align, layer, name);
	}

	void setMyGUIColor(MyGUI::Widget* instance, const Ogre::Vector4& color)
	{
		instance->setColour(MyGUI::Colour(color.x, color.y, color.z, color.w));
	}

	void setMyGUITextColor(MyGUI::EditBox* instance, const Ogre::Vector4& color)
	{
		instance->setTextColour(MyGUI::Colour(color.x, color.y, color.z, color.w));
	}

	Ogre::Vector4 getMyGUITextColor(MyGUI::EditBox* instance)
	{
		MyGUI::Colour color = instance->getTextColour();
		return Ogre::Vector4(color.red, color.green, color.blue, color.alpha);
	}

	void setMyGUITextShadowColor(MyGUI::EditBox* instance, const Ogre::Vector4& color)
	{
		instance->setTextShadowColour(MyGUI::Colour(color.x, color.y, color.z, color.w));
	}

	Ogre::Vector4 getMyGUITextShadowColor(MyGUI::EditBox* instance)
	{
		MyGUI::Colour color = instance->getTextShadowColour();
		return Ogre::Vector4(color.red, color.green, color.blue, color.alpha);
	}

	void bindMyGUI(lua_State* lua)
	{
		/*module(lua)
		[
			class_<MyGUI::UString>("UString")
			.def(constructor<const MyGUI::UString&>())
			.def(constructor<const wchar_t*>())
			.def(constructor<const std::wstring&>())
			.def(constructor<const char*>())
			.def(constructor<const std::string&>())
			.def(self == other<const MyGUI::UString&>())
			.def("size", &MyGUI::UString::size)
			.def("length", &MyGUI::UString::length)
			.def("empty", &MyGUI::UString::empty)
			.def("clear", &MyGUI::UString::clear)
		];*/

		module(lua)
			[
				class_<MyGUI::Align>("Align")
				.enum_("Enum")
			[
				value("H_CENTER", MyGUI::Align::HCenter),
				value("V_CENTER", MyGUI::Align::VCenter),
				value("CENTER", MyGUI::Align::Center),
				value("LEFT", MyGUI::Align::Left),
				value("RIGHT", MyGUI::Align::Right),
				value("H_STRECH", MyGUI::Align::HStretch),
				value("TOP", MyGUI::Align::Top),
				value("BOTTOM", MyGUI::Align::Bottom),
				value("V_STRETCH", MyGUI::Align::VStretch),
				value("STRETCH", MyGUI::Align::Stretch),
				value("DEFAULT", MyGUI::Align::Default)
			]
			];

		AddClassToCollection("Align", "class", "Align class for MyGUI widget positioning.");
		AddClassToCollection("Align", "H_CENTER", "Horizontal center.");
		AddClassToCollection("Align", "V_CENTER", "Vertical center.");
		AddClassToCollection("Align", "CENTER", "Center.");
		AddClassToCollection("Align", "LEFT", "Left.");
		AddClassToCollection("Align", "RIGHT", "Right.");
		AddClassToCollection("Align", "H_STRECH", "Horizontal stretch.");
		AddClassToCollection("Align", "TOP", "Top.");
		AddClassToCollection("Align", "Bottom", "Bottom.");
		AddClassToCollection("Align", "V_STRETCH", "Vertical stretch.");
		AddClassToCollection("Align", "STRETCH", "Stretch.");
		AddClassToCollection("Align", "DEFAULT", "Default (LEFT | TOP).");


		module(lua)
			[
				class_<MyGUI::WidgetStyle>("WidgetStyle")
				.enum_("Enum")
			[
				value("CHILD", MyGUI::WidgetStyle::Child),
				value("POPUP", MyGUI::WidgetStyle::Popup),
				value("OVERLAPPED", MyGUI::WidgetStyle::Overlapped)
			]
			];

		AddClassToCollection("WidgetStyle", "class", "WidgetStyle class.");
		AddClassToCollection("WidgetStyle", "CHILD", "Child.");
		AddClassToCollection("WidgetStyle", "POPUP", "Popup.");
		AddClassToCollection("WidgetStyle", "OVERLAPPED", "Overlapped.");

		module(lua)
			[
				class_<MyGUI::IntCoord>("IntCoord")
				.def(constructor<int, int, int, int>())
			.def(self == other<MyGUI::IntCoord>())
			.def(self + other<MyGUI::IntCoord>())
			.def(self - other<MyGUI::IntCoord>())
			.def("right", &MyGUI::IntCoord::right)
			.def("bottom", &MyGUI::IntCoord::bottom)
			.def("clear", &MyGUI::IntCoord::clear)
			.def("set", &MyGUI::IntCoord::set)
			];
		AddClassToCollection("IntCoord", "class", "Absolute coordinates class.");
		AddClassToCollection("IntCoord", "int right()", "The right absolute coordinate.");
		AddClassToCollection("IntCoord", "int bottom()", "The bottom absolute coordinate.");
		AddClassToCollection("IntCoord", "void clear()", "Clears the coordinate.");
		AddClassToCollection("IntCoord", "void set(int x1, int y1, int x2, int y2)", "Sets the absolute coordinate.");

		module(lua)
			[
				class_<MyGUI::FloatCoord>("FloatCoord")
				.def(constructor<float, float, float, float>())
			.def(self == other<MyGUI::FloatCoord>())
			.def(self + other<MyGUI::FloatCoord>())
			.def(self - other<MyGUI::FloatCoord>())
			.def("right", &MyGUI::FloatCoord::right)
			.def("bottom", &MyGUI::FloatCoord::bottom)
			.def("clear", &MyGUI::FloatCoord::clear)
			.def("set", &MyGUI::FloatCoord::set)
			];
		AddClassToCollection("FloatCoord", "class", "Relative coordinates class.");
		AddClassToCollection("FloatCoord", "float right()", "The right relative coordinate.");
		AddClassToCollection("FloatCoord", "float bottom()", "The bottom relative coordinate.");
		AddClassToCollection("FloatCoord", "void clear()", "Clears the coordinate.");
		AddClassToCollection("FloatCoord", "void set(float x1, float y1, float x2, float y2)", "Sets the relative coordinate.");

		module(lua)
			[
				class_<MyGUI::IntSize>("IntSize")
				.def(constructor<int, int>())
			.def(self == other<MyGUI::IntSize>())
			.def(self + other<MyGUI::IntSize>())
			.def(self - other<MyGUI::IntSize>())
			.def("clear", &MyGUI::IntSize::clear)
			.def("set", &MyGUI::IntSize::set)
			];
		AddClassToCollection("IntSize", "class", "Absolute size class.");
		AddClassToCollection("IntSize", "void clear()", "Clears the size.");
		AddClassToCollection("IntSize", "void set(int width, int height)", "Sets the width and height.");

		module(lua)
			[
				class_<MyGUI::FloatSize>("FloatSize")
				.def(constructor<float, float>())
			.def(self == other<MyGUI::FloatSize>())
			.def(self + other<MyGUI::FloatSize>())
			.def(self - other<MyGUI::FloatSize>())
			.def("clear", &MyGUI::FloatSize::clear)
			.def("set", &MyGUI::FloatSize::set)
			];

		AddClassToCollection("FloatSize", "class", "Relative size class.");
		AddClassToCollection("FloatSize", "void clear()", "Clears the size.");
		AddClassToCollection("FloatSize", "void set(float width, float height)", "Sets the width and height.");

		module(lua)
			[
				class_<MyGUI::Widget>("Widget")
				.def("setPosition", (void (MyGUI::Widget::*)(int, int)) & MyGUI::Widget::setPosition)
			.def("setSize", (void (MyGUI::Widget::*)(int, int)) & MyGUI::Widget::setSize)
			.def("setCoord", (void (MyGUI::Widget::*)(int, int, int, int)) & MyGUI::Widget::setCoord)
			.def("setRealPosition", (void (MyGUI::Widget::*)(float, float)) & MyGUI::Widget::setRealPosition)
			.def("setRealSize", (void (MyGUI::Widget::*)(float, float)) & MyGUI::Widget::setRealSize)
			.def("setRealCoord", (void (MyGUI::Widget::*)(float, float, float, float)) & MyGUI::Widget::setRealCoord)
			.def("setVisible", &MyGUI::Widget::setVisible)
			.def("getVisible", &MyGUI::Widget::getVisible)
			.def("setDepth", &MyGUI::Widget::setDepth)
			.def("getDepth", &MyGUI::Widget::getDepth)
			.def("setAlign", &MyGUI::Widget::setAlign)
			.def("getAlign", &MyGUI::Widget::getAlign)
			.def("setColor", &setMyGUIColor)
			.def("getParent", &MyGUI::Widget::getParent)
			.def("getParentSize", &MyGUI::Widget::getParentSize)
			.def("getChildCount", &MyGUI::Widget::getChildCount)
			.def("getChildAt", &MyGUI::Widget::getChildAt)
			.def("findWidget", &MyGUI::Widget::findWidget)
			.def("setEnabled", &MyGUI::Widget::setEnabled)
			.def("getEnabled", &MyGUI::Widget::getEnabled)
			// .def("getClientWidget", &MyGUI::Widget::getClientWidget)
			.def("setWidgetStyle", &MyGUI::Widget::setWidgetStyle)
			.def("getWidgetStyle", &MyGUI::Widget::getWidgetStyle)
			.def("setProperty", &MyGUI::Widget::setProperty)
			.def("_getItemIndex", &MyGUI::Widget::_getItemIndex)
			];

		AddClassToCollection("Widget", "class", "MyGUI widget class.");
		AddClassToCollection("Widget", "void setPosition(int x, int y)", "Sets the absolute position of the widget.");
		AddClassToCollection("Widget", "void setSize(int width, int height)", "Sets the absolute size of the widget.");
		AddClassToCollection("Widget", "void setCoord(int x1, int y1, int x2, int y2)", "Sets the absolute position and size of the widget.");
		AddClassToCollection("Widget", "void setRealPosition(float x, float y)", "Sets the relative position of the widget.");
		AddClassToCollection("Widget", "void setRealSize(float width, float height)", "Sets the relative size of the widget.");
		AddClassToCollection("Widget", "void setRealCoord(float x1, float y1, float x2, float y2)", "Sets the relative position and size of the widget.");
		AddClassToCollection("Widget", "void setVisible(bool visible)", "Sets whether to show the widget or not.");
		AddClassToCollection("Widget", "bool getVisible()", "Gets whether the widget is visible or not.");
		AddClassToCollection("Widget", "void setDepth(int depth)", "Sets the rendering depth (ordering).");
		AddClassToCollection("Widget", "int getDepth()", "Gets the rendering depth (ordering).");
		AddClassToCollection("Widget", "void setAlign(Align align)", "Sets the align of the widget.");
		AddClassToCollection("Widget", "Align getAlign()", "Gets the align of the widget.");
		AddClassToCollection("Widget", "void setColor(Vector4 color)", "Sets the color of the widget.");
		AddClassToCollection("Widget", "Widget getParent()", "Gets parent widget of this widget. Note: If this is the root widget and there is no parent, nil will be delivered.");
		AddClassToCollection("Widget", "IntSize getParentSize()", "Gets the parent widget size of this widget.");
		AddClassToCollection("Widget", "int getChildCount()", "Gets the count of children of this widget.");
		AddClassToCollection("Widget", "Widget getChildAt(int index)", "Gets the child widget of this widget by the given index.");
		AddClassToCollection("Widget", "Widget findWidget(String Name)", "Finds a widget by the given name.");
		AddClassToCollection("Widget", "void setEnabled(bool enable)", "Sets whether to enable the widget or not.");
		AddClassToCollection("Widget", "bool getEnabled()", "Gets whether the widget is enabled or not.");
		// AddClassToCollection("Widget", "Widget getClientWidget(?)", "?");
		AddClassToCollection("Widget", "void setWidgetStyle(WidgetStyle align)", "Sets the widget style.");
		AddClassToCollection("Widget", "WidgetStyle getWidgetStyle()", "Gets the widget style.");
		AddClassToCollection("Widget", "void setProperty(String key, Stirng value)", "Sets a user property for this widget.");
		AddClassToCollection("Widget", "number getItemIndex()", "Gets the index of the item.");

		module(lua)
			[
				class_<MyGUI::EditBox, MyGUI::Widget>("EditBox")
				.def("getTextRegion", &MyGUI::EditBox::getTextRegion)
			.def("getTextSize", &MyGUI::EditBox::getTextSize)
			.def("setCaption", &MyGUI::EditBox::setOnlyText)
			.def("getCaption", &MyGUI::EditBox::getOnlyText)
			.def("setFontName", &MyGUI::EditBox::setFontName)
			.def("getFontName", &MyGUI::EditBox::getFontName)
			.def("setFontHeight", &MyGUI::EditBox::setFontHeight)
			.def("getFontHeight", &MyGUI::EditBox::getFontHeight)
			.def("setTextAlign", &MyGUI::EditBox::setTextAlign)
			.def("getTextAlign", &MyGUI::EditBox::getTextAlign)
			.def("setTextColor", &setMyGUITextColor)
			.def("getTextColor", &getMyGUITextColor)
			.def("setCaptionWithReplacing", &MyGUI::EditBox::setCaptionWithReplacing)
			.def("setTextShadowColor", &setMyGUITextShadowColor)
			.def("getTextShadowColor", &getMyGUITextShadowColor)
			.def("setTextShadow", &MyGUI::EditBox::setTextShadow)
			.def("getTextShadow", &MyGUI::EditBox::getTextShadow)
			.def("setEditStatic", &MyGUI::EditBox::setEditStatic)
			.def("getEditStatic", &MyGUI::EditBox::getEditStatic)
			];

		AddClassToCollection("EditBox", "class", "MyGUI edit box class.");
		AddClassToCollection("EditBox", "IntCoord getTextRegion()", "Gets the text region.");
		AddClassToCollection("EditBox", "IntSize getTextSize()", "Gets the size of the text.");
		AddClassToCollection("EditBox", "void setCaption(string caption)", "Sets the caption of the text.");
		AddClassToCollection("EditBox", "string getCaption()", "Gets the caption of the text.");
		AddClassToCollection("EditBox", "void setFontName(string fontName)", "Sets the font name of the text. Note: The corresponding font must be installed.");
		AddClassToCollection("EditBox", "string getFontName()", "Gets the font name of the text.");
		AddClassToCollection("EditBox", "void setFontHeight(string fontName)", "Sets the font height of the text. Note: The corresponding font must be installed.");
		AddClassToCollection("EditBox", "string getFontHeight()", "Gets the font height of the text.");
		AddClassToCollection("EditBox", "void setTextAlign(Align align)", "Sets the align of the text.");
		AddClassToCollection("EditBox", "Align getTextAlign()", "Gets the align of the text.");
		AddClassToCollection("EditBox", "void setTextColor(Vector4 color)", "Sets the color of the text.");
		AddClassToCollection("EditBox", "Vector4 getTextColor()", "Gets the color of the text.");
		AddClassToCollection("EditBox", "void setCaptionWithReplacing(string textWithReplacing)", "Sets the caption of the text and keywords are replaced, from a translation table (e.g. printing text in the currently defined language.");
		AddClassToCollection("EditBox", "void setTextShadowColor(Vector3 shadowColor)", "Sets the shadow color of the text.");
		AddClassToCollection("EditBox", "Vector3 getTextColor()", "Gets the shadow color of the text.");
		AddClassToCollection("EditBox", "void setEditStatic(bool editStatic)", "Sets whether the text can be edited or not.");
		AddClassToCollection("EditBox", "bool getEditStatic()", "Gets whether the text can be edited or not.");

		module(lua)
			[
				class_<MyGUI::Button, MyGUI::Widget>("Button")
				.def("setStateCheck", &MyGUI::Button::setStateCheck)
			.def("getStateCheck", &MyGUI::Button::getStateCheck)
			];

		AddClassToCollection("Button", "class", "MyGUI button class.");
		AddClassToCollection("Button", "void setStateCheck(bool check)", "Sets whether the button is checked (check box functionality)");
		AddClassToCollection("Button", "bool getStateCheck()", "Gets whether the button is checked (check box functionality)");

		module(lua)
			[
				class_<MyGUI::Window, MyGUI::Widget>("Window")
				.def("setMovable", &MyGUI::Window::setMovable)
			.def("getMovable", &MyGUI::Window::getMovable)
			];
		AddClassToCollection("Window", "class", "MyGUI window class.");
		AddClassToCollection("Window", "void setMovable(bool movable)", "Sets whether thw window can be moved.");
		AddClassToCollection("Window", "bool getMovable()", "Gets whether the window can be moved");

		module(lua)
			[
				class_<MyGUI::ImageBox, MyGUI::Widget>("ImageBox")
				.def("setImageTexture", &MyGUI::ImageBox::setImageTexture)
			];
		AddClassToCollection("ImageBox", "class", "MyGUI image box class.");
		AddClassToCollection("ImageBox", "void setImageTexture(string textureName)", "Sets the texture name for the image box. Note: The image texture must be known by the resource management of Ogre.");

		module(lua)
			[
				class_<MyGUI::ListBox, MyGUI::Widget>("ListBox")
				.def("getItemCount", &MyGUI::ListBox::getItemCount)
			.def("insertItemAt", &MyGUI::ListBox::insertItemAt)
			.def("addItem", &MyGUI::ListBox::addItem)
			.def("removeItemAt", &MyGUI::ListBox::removeItemAt)
			.def("removeAllItems", &MyGUI::ListBox::removeAllItems)
			.def("swapItemsAt", &MyGUI::ListBox::swapItemsAt)
			.def("findItemIndexWith", &MyGUI::ListBox::findItemIndexWith)
			.def("getIndexSelected", &MyGUI::ListBox::getIndexSelected)
			.def("setItemNameAt", &MyGUI::ListBox::setItemNameAt)
			.def("getItemNameAt", &MyGUI::ListBox::getItemNameAt)
			];
		AddClassToCollection("ListBox", "class", "MyGUI list box class.");
		AddClassToCollection("ListBox", "int getItemCount()", "Gets the count of entries of the list box");
		AddClassToCollection("ListBox", "void insertItemAt(?)", "Inserts an item at the given index.");
		AddClassToCollection("ListBox", "void addItem(String name)", "Adds an item.");
		AddClassToCollection("ListBox", "void removeItemAt(int index)", "Removes an item at the given index.");
		AddClassToCollection("ListBox", "void removeAllItems()", "Removes all items.");
		AddClassToCollection("ListBox", "void swapItemsAt(int index1, int index2)", "Swaps two items at the given index.");
		AddClassToCollection("ListBox", "int findItemIndexWith(String name)", "Finds an item with the given index.");
		AddClassToCollection("ListBox", "int getIndexSelected()", "Gets the selected index.");
		AddClassToCollection("ListBox", "void setItemNameAt(int index, String name)", "Sets the name at the given index.");
		AddClassToCollection("ListBox", "String getItemNameAt(int index)", "Gets the name by the given index.");

		//module(lua)
		//[
		//	class_<MyGUI::Gui>("Gui")
		//	// .def("getInstancePtr", &MyGUI::Gui::getInstancePtr)
		//	.def("createTextBox", &createTextBox)
		//	.def("createRealTextBox", &createRealTextBox)
		//	.def("destroyWidget", &MyGUI::Gui::destroyWidget)
		//	// .def("findWidget", &MyGUI::Gui::findWidget)
		//];
		//AddClassToCollection("Gui", "TextBox createTextBox(?)", "Creates a text box with absolute coordinates.");
		//AddClassToCollection("Gui", "TextBox createRealTextBox(?)", "Creates a text box with relative coordinates.");
		//AddClassToCollection("Gui", "? destroyWidget(Widget widget)", "Destroys a widget.");

		module(lua)
			[
				class_<MyGUI::PointerManager>("PointerManager")
				// .def("getInstancePtr", &MyGUI::PointerManager::getInstancePtr)
			.def("showMouse", &MyGUI::PointerManager::setVisible)
			];

		AddClassToCollection("PointerManager", "class", "MyGUI pointer manager singleton class.");
		AddClassToCollection("PointerManager", "void showMouse(bool show)", "Show the MyGUI mouse cursor or hides the mouse cursor.");

		// object globalVars = globals(lua);
		// globalVars["Gui"] = MyGUI::Gui::getInstancePtr();
		// AddClassToCollection("Gui", "Gui Gui()", "Gets the Gui singleton instance.");

		object globalVars = globals(lua);
		globalVars["PointerManager"] = MyGUI::PointerManager::getInstancePtr();

		// MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::PopupMenu>
		/*module(lua)
		[
			class_<MyGUI::Canvas, MyGUI::Widget>("Canvas")
			.def("setStateSelected", &MyGUI::Button::setStateSelected)
			.def("getStateSelected", &MyGUI::Button::getStateSelected)

		];*/
	}

	void saveValue(GameProgressModule* instance, const Ogre::String& saveName, const Ogre::String& gameObjectId, unsigned int attributeIndex, bool crypted)
	{
		instance->saveValue(saveName, Ogre::StringConverter::parseUnsignedLong(gameObjectId), attributeIndex, crypted);
	}

	void saveValues(GameProgressModule* instance, const Ogre::String& saveName, const Ogre::String& gameObjectId, bool crypted)
	{
		instance->saveValues(saveName, Ogre::StringConverter::parseUnsignedLong(gameObjectId), crypted);
	}

	void loadValue(GameProgressModule* instance, const Ogre::String& saveName, const Ogre::String& gameObjectId, unsigned int attributeIndex)
	{
		instance->loadValue(saveName, Ogre::StringConverter::parseUnsignedLong(gameObjectId), attributeIndex);
	}

	void loadValues(GameProgressModule* instance, const Ogre::String& saveName, const Ogre::String& gameObjectId)
	{
		instance->loadValues(saveName, Ogre::StringConverter::parseUnsignedLong(gameObjectId));
	}

	void bindGameProgressModule(lua_State* lua)
	{
		module(lua)
			[
				class_<GameProgressModule>("GameProgressModule")
				// .def("getInstance", &GameProgressModule::getInstance) //returns static singleton instance
				// .def("addWorld", &GameProgressModule::addWorld)
				// .def("loadWorld", &GameProgressModule::loadWorld)
				// .def("loadWorldShowProgress", &GameProgressModule::loadWorldShowProgress)
				// .def("setPlayerName", &GameProgressModule::setPlayerName)
				// .def("determinePlayerStartLocation", &GameProgressModule::determinePlayerStartLocation)
			.def("getCurrentWorldName", &GameProgressModule::getCurrentWorldName)
			.def("saveProgress", &GameProgressModule::saveProgress)
			.def("saveValue", &saveValue)
			.def("saveValues", &saveValues)
			.def("loadProgress", &GameProgressModule::loadProgress)
			.def("loadValue", &loadValue)
			.def("loadValues", &loadValues)
			.def("getGlobalValue", &GameProgressModule::getGlobalValue)
			.def("setGlobalBoolValue", &GameProgressModule::setGlobalBoolValue)
			// Lua knows only about number!
			// .def("setGlobalIntValue", &GameProgressModule::setGlobalIntValue)
			// .def("setGlobalUIntValue", &GameProgressModule::setGlobalUIntValue)
			// .def("setGlobalULongValue", &GameProgressModule::setGlobalULongValue)
			.def("setGlobalNumberValue", &GameProgressModule::setGlobalRealValue)
			.def("setGlobalStringValue", &GameProgressModule::setGlobalStringValue)
			.def("setGlobalVector2Value", &GameProgressModule::setGlobalVector2Value)
			.def("setGlobalVector3Value", &GameProgressModule::setGlobalVector3Value)
			.def("setGlobalVector4Value", &GameProgressModule::setGlobalVector4Value)
			.def("changeWorld", &GameProgressModule::changeWorld)
			.def("changeWorldShowProgress", &GameProgressModule::changeWorldShowProgress)
			];

		// object globalVars = globals(lua);
		// globalVars["GameProgressModule"] = AppStateManager::getSingletonPtr()->getGameProgressModule();

		AddClassToCollection("GameProgressModule", "class", "Class that is reponsible for the game progress specific topics like switching a scene at runtime or loading saving the game.");
		AddClassToCollection("GameProgressModule", "String getCurrentWorldName()", "Gets the current world name.");
		AddClassToCollection("GameProgressModule", "void saveProgress(String saveName, bool crypted, bool sceneSnapshot)", "Saves the current progress for all game objects with its attribute components. "
			"Optionally crypts the content, so that it is not readable anymore. Optionally can saves a whole scene snapshot");
		AddClassToCollection("GameProgressModule", "String saveValue(String saveName, String gameObjectId, unsigned int attributeIndex, bool crypted)", "Saves a value for the given game object id and its attribute index. Optionally crypts the content, so that it is not readable anymore.");
		AddClassToCollection("GameProgressModule", "String saveValues(String saveName, String gameObjectId, bool crypted)", "Saves all values for the given game object id and its attribute components. Optionally crypts the content, so that it is not readable anymore.");
		AddClassToCollection("GameProgressModule", "bool loadProgress(String saveName, bool sceneSnapshot)", "Loads all values for all game objects with attributes components for the given save name. "
			"Optionally can loads a whole scene snapshot. Returns false, if no save file could be found.");
		AddClassToCollection("GameProgressModule", "bool loadValue(String saveName, String gameObjectId, unsigned int attributeIndex)", "Loads a value for the given game object id and attribute index and the given save name. Returns false, if no save file could be found or index or game object id is invalid.");
		AddClassToCollection("GameProgressModule", "bool loadValue(String saveName, String gameObjectId)", "Loads all values for the given game object id and the given save name. Returns false, if no save file could be found or game object id is invalid.");

		AddClassToCollection("GameProgressModule", "Variant getGlobalValue(String attributeName)", "Gets the Variant global value from attribute name. Note: Global values are stored directly in GameProgressModule. Note: Global values are stored directly in GameProgressModule."
			"They can be used for the whole game logic, like which boss has been defeated etc.");
		AddClassToCollection("GameProgressModule", "Variant setGlobalValue(String attributeName, bool value)", "Sets the bool value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. "
			"They can be used for the whole game logic, like which boss has been defeated etc.");
		AddClassToCollection("GameProgressModule", "Variant setGlobalValue(String attributeName, number value)", "Sets the number value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. "
			"They can be used for the whole game logic, like which boss has been defeated etc.");
		AddClassToCollection("GameProgressModule", "Variant setGlobalValue(String attributeName, String value)", "Sets the string value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. "
			"They can be used for the whole game logic, like which boss has been defeated etc.");
		AddClassToCollection("GameProgressModule", "Variant setGlobalValue(String attributeName, Vector2 value)", "Sets the Vector2 value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. "
			"They can be used for the whole game logic, like which boss has been defeated etc.");
		AddClassToCollection("GameProgressModule", "Variant setGlobalValue(String attributeName, Vector3 value)", "Sets the Vector3 value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. "
			"They can be used for the whole game logic, like which boss has been defeated etc.");
		AddClassToCollection("GameProgressModule", "Variant setGlobalValue(String attributeName, Vector4 value)", "Sets the Vector4 value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. "
			"They can be used for the whole game logic, like which boss has been defeated etc.");
		AddClassToCollection("GameProgressModule", "void changeWorld(String worldName)", "Changes the current world to the new given one.");
		AddClassToCollection("GameProgressModule", "void changeWorldShowProgress(String worldName)", "Changes the current world to the new given one. Also shows the loading progress.");
	}

	void deleteDelayedGameObject(GameObjectController* instance, const Ogre::String& id, Ogre::Real timeOffsetSec)
	{
		instance->deleteDelayedGameObject(Ogre::StringConverter::parseUnsignedLong(id), timeOffsetSec);
	}

	void deleteGameObject(GameObjectController* instance, const Ogre::String& id)
	{
		instance->deleteGameObject(Ogre::StringConverter::parseUnsignedLong(id));
	}

	void deleteGameObjectWithUndo(GameObjectController* instance, const Ogre::String& id)
	{
		instance->deleteGameObjectWithUndo(Ogre::StringConverter::parseUnsignedLong(id));
	}

#if 0
	void snapshotGameObjectsWithUndo(GameObjectController* instance, luabind::object gameObjectIds)
	{
		std::vector<unsigned long> gameObjectIdsULong;
		for (luabind::iterator it(gameObjectIds), end; it != end; ++it)
		{
			luabind::object val = *it;

			if (luabind::type(val) == LUA_TSTRING)
			{
				gameObjectIdsULong.emplace_back(Ogre::StringConverter::parseUnsignedLong(luabind::object_cast<Ogre::String>(val)));
			}
		}
		
		instance->snapshotGameObjectsWithUndo(gameObjectIdsULong);
	}
#endif

	void activateGameObjectComponentsFromReferenceId(GameObjectController* instance, const Ogre::String& referenceId, bool activate)
	{
		instance->activateGameObjectComponentsFromReferenceId(Ogre::StringConverter::parseUnsignedLong(referenceId), activate);
	}

	Ogre::String getCategoryId2(GameObjectController* instance, const Ogre::String& category)
	{
		return Ogre::StringConverter::toString(instance->getCategoryId(category));
	}

	void activatePlayerController(GameObjectController* instance, bool active, const Ogre::String& id, bool onlyOneActive)
	{
		instance->activatePlayerController(active, Ogre::StringConverter::parseUnsignedLong(id), onlyOneActive);
	}

	void bindGameObjectController(lua_State* lua, class_<GameObjectController>& gameObjectController)
	{
		gameObjectController.def("deleteDelayedGameObject", deleteDelayedGameObject);
		gameObjectController.def("deleteGameObject", deleteGameObject);
		gameObjectController.def("deleteGameObjectWithUndo", deleteGameObjectWithUndo);
		gameObjectController.def("undo", &GameObjectController::undo);
		gameObjectController.def("undoAll", &GameObjectController::undoAll);
		gameObjectController.def("redo", &GameObjectController::redo);
		gameObjectController.def("redoAll", &GameObjectController::redoAll);
		gameObjectController.def("clone", &clone);
		// gameObjectController.def("cloneGroup", &GameObjectController::cloneGroup);
		// gameObjectController.def("registerGameObject", &GameObjectController::registerGameObject);
		// gameObjectController.def("deleteAllGameObjects", &GameObjectController::deleteAllGameObjects);
		gameObjectController.def("getGameObjects", &getGameObjects);
		gameObjectController.def("getGameObjectFromId", &getGameObjectFromId);
		gameObjectController.def("getClonedGameObjectFromPriorId", &getClonedGameObjectFromPriorId);
		// gameObjectController.def("getGameObjectsFromCategory", &getGameObjectsFromCategory, out_value(boost::arg<1>(), return_stl_iterator)) // nothing does work here :(
		gameObjectController.def("getGameObjectsFromCategory", &getGameObjectsFromCategory);
		gameObjectController.def("getGameObjectsControlledByClientId", &getGameObjectsControlledByClientId);
		gameObjectController.def("getGameObjectsFromTagName", &getGameObjectsFromTagName);
		// gameObjectController.def("getAllCategoriesSoFar", &getAllCategoriesSoFar) does not work for lua at the moment, because the typeDB container is private and cannot be iterated here
		gameObjectController.def("getIdsFromCategory", &getIdsFromCategory);
		gameObjectController.def("getGameObjectFromReferenceId", &getGameObjectFromReferenceId);
		gameObjectController.def("getGameObjectComponentsFromReferenceId", &getGameObjectComponentsFromReferenceId);
		gameObjectController.def("activateGameObjectComponentsFromReferenceId", &activateGameObjectComponentsFromReferenceId);

		gameObjectController.def("getOtherIdsFromCategory", &getOtherIdsFromCategory);
		gameObjectController.def("getGameObjectFromName", &getGameObjectFromName);
		gameObjectController.def("getGameObjectsFromNamePrefix", &getGameObjectsFromNamePrefix);
		gameObjectController.def("getGameObjectFromNamePrefix", &getGameObjectFromNamePrefix);
		// gameObjectController.def("getCategoryId", &GameObjectController::getCategoryId);
		gameObjectController.def("getCategoryId", &getCategoryId2);
		//// gameObjectController.def("getAffectedCategories", &getAffectedCategories);
		// gameObjectController.def("getNextGameObjectId", &GameObjectController::getNextGameObjectId) Does not work, because the internal algorithm is here not usable -> must be somehow re-implemented here
		// gameObjectController.def("getMaterialID", &GameObjectController::getMaterialID);
		gameObjectController.def("getMaterialIDFromCategory", &GameObjectController::getMaterialID);
		// gameObjectController.def("activatePlayerController", &GameObjectController::activatePlayerController);
		gameObjectController.def("activatePlayerController", &activatePlayerController);
		gameObjectController.def("getNextGameObject", (GameObject* (GameObjectController::*)(unsigned int))&GameObjectController::getNextGameObject);
		gameObjectController.def("getNextGameObject", (GameObject* (GameObjectController::*)(const Ogre::String&))&GameObjectController::getNextGameObject);

		// gameObjectController.def("addJointProperties", &GameObjectController::addJointProperties);
		// gameObjectController.def("processJoints", &GameObjectController::processJoints);
		// gameObjectController.def("processCompoundCollisions", &GameObjectController::processCompoundCollisions);
		// gameObjectController.def("processVehicles", &GameObjectController::processVehicles);
		// gameObjectController.def("processGameObjectsData", &GameObjectController::processGameObjectsData);
		// gameObjectController.def("cleanUp", &GameObjectController::cleanUp);

		// gameObjectController.def("selectGameObject", (GameObject* (GameObjectController::*)(int, int, Ogre::Camera*, Ogre::RaySceneQuery*)) &GameObjectController::selectGameObject);
		// gameObjectController.def("selectGameObject", (GameObject* (GameObjectController::*)(int, int, Ogre::Camera*, OgreNewt::World*, unsigned int, Ogre::Real, bool)) &GameObjectController::selectGameObject);
		// gameObjectController.def("getTargetBodyPosition", &GameObjectController::getTargetBodyPosition);
		// gameObjectController.def("initAreaForActiveObjects", &GameObjectController::initAreaForActiveObjects);
		// gameObjectController.def("checkAreaForActiveObjects", &GameObjectController::checkAreaForActiveObjects);
		// gameObjectController.def("registerType", &GameObjectController::registerType);
		// For lua api auto complete when use functions, e.g. connect(gameObject), what is gameObject?
		gameObjectController.def("castGameObject", &GameObjectController::cast<GameObject>);
		gameObjectController.def("castContactData", &GameObjectController::cast<OgreNewt::Contact>);
		gameObjectController.def("castMouseButton", &GameObjectController::cast<OIS::MouseButtonID>);

		gameObjectController.def("castGameObjectComponent", &GameObjectController::cast<GameObjectComponent>);
		gameObjectController.def("castAnimationComponent", &GameObjectController::cast<AnimationComponent>);
		gameObjectController.def("castAttributesComponent", &GameObjectController::cast<AttributesComponent>);
		gameObjectController.def("castAttributeEffectComponent", &GameObjectController::cast<AttributeEffectComponent>);
		gameObjectController.def("castPhysicsBuoyancyComponent", &GameObjectController::cast<PhysicsBuoyancyComponent>);
		gameObjectController.def("castPhysicsTriggerComponent", &GameObjectController::cast<PhysicsTriggerComponent>);
		gameObjectController.def("castDistributedComponent", &GameObjectController::cast<DistributedComponent>);
		gameObjectController.def("castExitComponent", &GameObjectController::cast<ExitComponent>);
		gameObjectController.def("castGameObjectTitleComponent", &GameObjectController::cast<GameObjectTitleComponent>);
		gameObjectController.def("castAiMoveComponent", &GameObjectController::cast<AiMoveComponent>);
		gameObjectController.def("castAiMoveRandomlyComponent", &GameObjectController::cast<AiMoveRandomlyComponent>);
		gameObjectController.def("castAiPathFollowComponent", &GameObjectController::cast<AiPathFollowComponent>);
		gameObjectController.def("castAiWanderComponent", &GameObjectController::cast<AiWanderComponent>);
		gameObjectController.def("castAiFlockingComponent", &GameObjectController::cast<AiFlockingComponent>);
		gameObjectController.def("castAiRecastPathNavigationComponent", &GameObjectController::cast<AiRecastPathNavigationComponent>);
		gameObjectController.def("castAiObstacleAvoidanceComponent", &GameObjectController::cast<AiObstacleAvoidanceComponent>);
		gameObjectController.def("castAiHideComponent", &GameObjectController::cast<AiHideComponent>);
		gameObjectController.def("castAiMoveComponent2D", &GameObjectController::cast<AiMoveComponent2D>);
		gameObjectController.def("castAiPathFollowComponent2D", &GameObjectController::cast<AiPathFollowComponent2D>);
		gameObjectController.def("castAiWanderComponent2D", &GameObjectController::cast<AiWanderComponent2D>);
		gameObjectController.def("castCameraBehaviorBaseComponent", &GameObjectController::cast<CameraBehaviorBaseComponent>);
		gameObjectController.def("castCameraBehaviorFirstPersonComponent", &GameObjectController::cast<CameraBehaviorFirstPersonComponent>);
		gameObjectController.def("castCameraBehaviorThirdPersonComponent", &GameObjectController::cast<CameraBehaviorThirdPersonComponent>);
		gameObjectController.def("castCameraBehaviorFollow2DComponent", &GameObjectController::cast<CameraBehaviorFollow2DComponent>);
		gameObjectController.def("castCameraBehaviorZoomComponent", &GameObjectController::cast<CameraBehaviorZoomComponent>);
		gameObjectController.def("castCameraComponent", &GameObjectController::cast<CameraComponent>);
		gameObjectController.def("castCompositorEffectBloomComponent", &GameObjectController::cast<CompositorEffectBloomComponent>);
		gameObjectController.def("castCompositorEffectGlassComponent", &GameObjectController::cast<CompositorEffectGlassComponent>);
		gameObjectController.def("castCompositorEffectOldTvComponent", &GameObjectController::cast<CompositorEffectOldTvComponent>);
		gameObjectController.def("castCompositorEffectKeyholeComponent", &GameObjectController::cast<CompositorEffectKeyholeComponent>);
		gameObjectController.def("castCompositorEffectBlackAndWhiteComponent", &GameObjectController::cast<CompositorEffectBlackAndWhiteComponent>);
		gameObjectController.def("castCompositorEffectColorComponent", &GameObjectController::cast<CompositorEffectColorComponent>);
		gameObjectController.def("castCompositorEffectEmbossedComponent", &GameObjectController::cast<CompositorEffectEmbossedComponent>);
		gameObjectController.def("castCompositorEffectSharpenEdgesComponent", &GameObjectController::cast<CompositorEffectSharpenEdgesComponent>);
		gameObjectController.def("castHdrEffectComponent", &GameObjectController::cast<HdrEffectComponent>);
		gameObjectController.def("castDatablockPbsComponent", &GameObjectController::cast<DatablockPbsComponent>);
		gameObjectController.def("castDatablockUnlitComponent", &GameObjectController::cast<DatablockUnlitComponent>);
		gameObjectController.def("castDatablockTerraComponent", &GameObjectController::cast<DatablockTerraComponent>);
		gameObjectController.def("castJointComponent", &GameObjectController::cast<JointComponent>);
		gameObjectController.def("castJointHingeComponent", &GameObjectController::cast<JointHingeComponent>);
		gameObjectController.def("castJointHingeActuatorComponent", &GameObjectController::cast<JointHingeActuatorComponent>);
		gameObjectController.def("castJointBallAndSocketComponent", &GameObjectController::cast<JointBallAndSocketComponent>);
		gameObjectController.def("castJointPointToPointComponent", &GameObjectController::cast<JointPointToPointComponent>);
		gameObjectController.def("castJointPinComponent", &GameObjectController::cast<JointPinComponent>);
		gameObjectController.def("castJointPlaneComponent", &GameObjectController::cast<JointPlaneComponent>);
		gameObjectController.def("castJointSpringComponent", &GameObjectController::cast<JointSpringComponent>);
		gameObjectController.def("castJointAttractorComponent", &GameObjectController::cast<JointAttractorComponent>);
		gameObjectController.def("castJointCorkScrewComponent", &GameObjectController::cast<JointCorkScrewComponent>);
		gameObjectController.def("castJointPassiveSliderComponent", &GameObjectController::cast<JointPassiveSliderComponent>);
		gameObjectController.def("castJointSliderActuatorComponent", &GameObjectController::cast<JointSliderActuatorComponent>);
		gameObjectController.def("castJointSlidingContactComponent", &GameObjectController::cast<JointSlidingContactComponent>);
		gameObjectController.def("castJointActiveSliderComponent", &GameObjectController::cast<JointActiveSliderComponent>);
		gameObjectController.def("castJointMathSliderComponent", &GameObjectController::cast<JointMathSliderComponent>);
		gameObjectController.def("castJointKinematicComponent", &GameObjectController::cast<JointKinematicComponent>);
		gameObjectController.def("castJointDryRollingFrictionComponent", &GameObjectController::cast<JointDryRollingFrictionComponent>);
		gameObjectController.def("castJointPathFollowComponent", &GameObjectController::cast<JointPathFollowComponent>);
		gameObjectController.def("castJointGearComponent", &GameObjectController::cast<JointGearComponent>);
		gameObjectController.def("castJointRackAndPinionComponent", &GameObjectController::cast<JointRackAndPinionComponent>);
		gameObjectController.def("castJointWormGearComponent", &GameObjectController::cast<JointWormGearComponent>);
		gameObjectController.def("castJointPulleyComponent", &GameObjectController::cast<JointPulleyComponent>);
		gameObjectController.def("castJointUniversalComponent", &GameObjectController::cast<JointUniversalComponent>);
		gameObjectController.def("castJointUniversalActuatorComponent", &GameObjectController::cast<JointUniversalActuatorComponent>);
		gameObjectController.def("castJoint6DofComponent", &GameObjectController::cast<Joint6DofComponent>);
		gameObjectController.def("castJointMotorComponent", &GameObjectController::cast<JointMotorComponent>);
		gameObjectController.def("castJointWheelComponent", &GameObjectController::cast<JointWheelComponent>);
		gameObjectController.def("castLightDirectionalComponent", &GameObjectController::cast<LightDirectionalComponent>);
		gameObjectController.def("castLightPointComponent", &GameObjectController::cast<LightPointComponent>);
		gameObjectController.def("castLightSpotComponent", &GameObjectController::cast<LightSpotComponent>);
		gameObjectController.def("castLightAreaComponent", &GameObjectController::cast<LightAreaComponent>);
		gameObjectController.def("castFadeComponent", &GameObjectController::cast<FadeComponent>);
		gameObjectController.def("castNavMeshComponent", &GameObjectController::cast<NavMeshComponent>);
		gameObjectController.def("castParticleUniverseComponent", &GameObjectController::cast<ParticleUniverseComponent>);
		gameObjectController.def("castPlayerControllerJumpNRunComponent", &GameObjectController::cast<PlayerControllerJumpNRunComponent>);
		gameObjectController.def("castPlayerControllerJumpNRunLuaComponent", &GameObjectController::cast<PlayerControllerJumpNRunLuaComponent>);
		gameObjectController.def("castPlayerControllerClickToPointComponent", &GameObjectController::cast<PlayerControllerClickToPointComponent>);
		gameObjectController.def("castPhysicsComponent", &GameObjectController::cast<PhysicsComponent>);
		gameObjectController.def("castPhysicsActiveComponent", &GameObjectController::cast<PhysicsActiveComponent>);
		gameObjectController.def("castPhysicsActiveCompoundComponent", &GameObjectController::cast<PhysicsActiveCompoundComponent>);
		gameObjectController.def("castPhysicsActiveDestructableComponent", &GameObjectController::cast<PhysicsActiveDestructableComponent>);
		gameObjectController.def("castPhysicsActiveKinematicComponent", &GameObjectController::cast<PhysicsActiveKinematicComponent>);
		gameObjectController.def("castPhysicsArtifactComponent", &GameObjectController::cast<PhysicsArtifactComponent>);
		gameObjectController.def("castPhysicsRagDollComponent", &GameObjectController::cast<PhysicsRagDollComponent>);
		gameObjectController.def("castPhysicsCompoundConnectionComponent", &GameObjectController::cast<PhysicsCompoundConnectionComponent>);
		gameObjectController.def("castPhysicsMaterialComponent", &GameObjectController::cast<PhysicsMaterialComponent>);
		gameObjectController.def("castPhysicsPlayerControllerComponent", &GameObjectController::cast<PhysicsPlayerControllerComponent>);
		gameObjectController.def("castPlaneComponent", &GameObjectController::cast<PlaneComponent>);
		gameObjectController.def("castSimpleSoundComponent", &GameObjectController::cast<SimpleSoundComponent>);
		gameObjectController.def("castSoundComponent", &GameObjectController::cast<SoundComponent>);
		gameObjectController.def("castSpawnComponent", &GameObjectController::cast<SpawnComponent>);
		gameObjectController.def("castVehicleComponent", &GameObjectController::cast<VehicleComponent>);
		gameObjectController.def("castAiLuaComponent", &GameObjectController::cast<AiLuaComponent>);
		gameObjectController.def("castPhysicsExplosionComponent", &GameObjectController::cast<PhysicsExplosionComponent>);
		gameObjectController.def("castMeshDecalComponent", &GameObjectController::cast<MeshDecalComponent>);
		gameObjectController.def("castTagPointComponent", &GameObjectController::cast<TagPointComponent>);
		gameObjectController.def("castTimeTriggerComponent", &GameObjectController::cast<TimeTriggerComponent>);
		gameObjectController.def("castTimeLineComponent", &GameObjectController::cast<TimeLineComponent>);
		gameObjectController.def("castMoveMathFunctionComponent", &GameObjectController::cast<MoveMathFunctionComponent>);
		gameObjectController.def("castTagChildNodeComponent", &GameObjectController::cast<TagChildNodeComponent>);
		gameObjectController.def("castNodeTrackComponent", &GameObjectController::cast<NodeTrackComponent>);
		gameObjectController.def("castLineComponent", &GameObjectController::cast<LineComponent>);
		gameObjectController.def("castLinesComponent", &GameObjectController::cast<LinesComponent>);
		gameObjectController.def("castNodeComponent", &GameObjectController::cast<NodeComponent>);
		gameObjectController.def("castManualObjectComponent", &GameObjectController::cast<ManualObjectComponent>);
		gameObjectController.def("castRectangleComponent", &GameObjectController::cast<RectangleComponent>);
		gameObjectController.def("castValueBarComponent", &GameObjectController::cast<ValueBarComponent>);
		gameObjectController.def("castBillboardComponent", &GameObjectController::cast<BillboardComponent>);
		gameObjectController.def("castRibbonTrailComponent", &GameObjectController::cast<RibbonTrailComponent>);
		gameObjectController.def("castMyGUIWindowComponent", &GameObjectController::cast<MyGUIWindowComponent>);
		gameObjectController.def("castMyGUITextComponent", &GameObjectController::cast<MyGUITextComponent>);
		gameObjectController.def("castMyGUIButtonComponent", &GameObjectController::cast<MyGUIButtonComponent>);
		gameObjectController.def("castMyGUICheckBoxComponent", &GameObjectController::cast<MyGUICheckBoxComponent>);
		gameObjectController.def("castMyGUIImageBoxComponent", &GameObjectController::cast<MyGUIImageBoxComponent>);
		gameObjectController.def("castMyGUIProgressBarComponent", &GameObjectController::cast<MyGUIProgressBarComponent>);
		gameObjectController.def("castMyGUIListBoxComponent", &GameObjectController::cast<MyGUIListBoxComponent>);
		gameObjectController.def("castMyGUIComboBoxComponent", &GameObjectController::cast<MyGUIComboBoxComponent>);
		gameObjectController.def("castMyGUIMessageBoxComponent", &GameObjectController::cast<MyGUIMessageBoxComponent>);
		gameObjectController.def("castMyGUIPositionControllerComponent", &GameObjectController::cast<MyGUIPositionControllerComponent>);
		gameObjectController.def("castMyGUIFadeAlphaControllerComponent", &GameObjectController::cast<MyGUIFadeAlphaControllerComponent>);
		gameObjectController.def("castMyGUIScrollingMessageControllerComponent", &GameObjectController::cast<MyGUIScrollingMessageControllerComponent>);
		gameObjectController.def("castMyGUIEdgeHideControllerComponent", &GameObjectController::cast<MyGUIEdgeHideControllerComponent>);
		gameObjectController.def("castMyGUIRepeatClickControllerComponent", &GameObjectController::cast<MyGUIRepeatClickControllerComponent>);
		gameObjectController.def("castMyGUIWindowComponent", &GameObjectController::cast<MyGUIWindowComponent>);
		gameObjectController.def("castMyGUIItemBoxComponent", &GameObjectController::cast<MyGUIItemBoxComponent>);
		gameObjectController.def("castMyGUIMiniMapComponent", &GameObjectController::cast<MyGUIMiniMapComponent>);

		AddClassToCollection("GameObjectController", "class", "The game object controller manages all game objects.");
		AddClassToCollection("GameObjectController", "void deleteDelayedGameObject(String gameObjectId, float delayMS)", "Deletes a game object by the given id after a delay of milliseconds.");
		AddClassToCollection("GameObjectController", "void deleteDelayedGameObject(String gameObjectId)", "Deletes a game object by the given id after 1000 milliseconds.");
		AddClassToCollection("GameObjectController", "void deleteGameObject(String gameObjectId)", "Deletes a game object by the given id immediately.");
		AddClassToCollection("GameObjectController", "void deleteGameObjectWithUndo(String gameObjectId)", "Deletes the game object by id.Note: The game object by the given id will not be deleted immediately but in the next update turn. "
														"This function can be used e.g. from a component of the game object, to delete it later, so that the component which calls this function will be deleted to.");
		AddClassToCollection("GameObjectController", "void clone(String originalGameObjectId, SceneNode parentNode, String targetId, Vector3 targetPosition, Quaternion targetOrientation, Vector3 targetScale)", "Clones a game object with all its components.");
		AddClassToCollection("GameObjectController", "Table[index][GameObject] getGameObjects()", "Gets all game objects as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.");
		AddClassToCollection("GameObjectController", "GameObject getGameObjectFromId(String gameObjectId)", "Gets a game object by the given id.");
		AddClassToCollection("GameObjectController", "GameObject getClonedGameObjectFromPriorId(String gameObjectId)", "Gets a cloned game object by its prior id. This can be used, when a game object has been cloned, it gets a new id, but a trace of its prior id can be made, so that a matching can be done, which game object has been cloned.");
		AddClassToCollection("GameObjectController", "Table[index][GameObject] getGameObjectsFromCategory(string category)", "Gets all game objects that are belonging to the given category as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.");
		AddClassToCollection("GameObjectController", "Table[index][GameObject] getGameObjectsControlledByClientId(unsigned int clientId)", "Gets all game objects that are belonging to the given client id in a network scenario as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.");
		AddClassToCollection("GameObjectController", "Table[index][GameObject] getGameObjectsFromTagName(string tagName)", "Gets all game objects that are belonging to the given tag name as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.");
		AddClassToCollection("GameObjectController", "GameObject getGameObjectFromReferenceId(String referenceId)", "Gets a game object by the given reference id.");
		AddClassToCollection("GameObjectController", "GameObjectComponent getGameObjectComponentsFromReferenceId(String referenceId)", "Gets a list of game object components by the given reference id.");
		AddClassToCollection("GameObjectController", "activateGameObjectComponentsFromReferenceId(String referenceId, bool activate)", "Activates all game object components, that have the same id, as the referenced game object.");
		AddClassToCollection("GameObjectController", "Table[index][category] getAllCategoriesSoFar()", "Gets all currently created categories as lua table. Which can be traversed via 'for key, category in categories do'.");
		AddClassToCollection("GameObjectController", "Table[index][id] getIdsFromCategory(string category)", "Gets all game object ids that are belonging to the given category as lua table. Which can be traversed via 'for key, category in categories do'.");
		AddClassToCollection("GameObjectController", "Table[index][id] getOtherIdsFromCategory(GameObject excludedGameObject, string category)", "Gets all game object ids besides the excluded one that are belonging to the given category as lua table. Which can be traversed via 'for key, category in categories do'.");
		AddClassToCollection("GameObjectController", "GameObject getGameObjectFromName(string name)", "Gets a game object from the given name.");
		AddClassToCollection("GameObjectController", "Table[index][GameObject] getGameObjectsFromNamePrefix(string pattern)", "Gets all game objects that are matching the given pattern as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.");
		AddClassToCollection("GameObjectController", "GameObject getGameObjectFromNamePrefix(string pattern)", "Gets first game object that is matching the given pattern.");
		AddClassToCollection("GameObjectController", "number getCategoryId(string categoryName)", "Gets id of the given category name.");
		AddClassToCollection("GameObjectController", "number getMaterialIDFromCategory(string categoryName)", "Gets physics material id of the given category name. In order to connect physics components together for collision detection etc.");
		AddClassToCollection("GameObjectController", "void activatePlayerController(bool active, String id, bool onlyOneActive)", "Activates the given player component from the given game object id. "
			"If set to true, the given player component will be activated, else deactivated. Sets whether only one player instance can be controller. If set to false more player can be controlled, that is each player, that is currently selected.");
		AddClassToCollection("GameObjectController", "GameObject getNextGameObject(int categoryIds)", "Gets the next game object from the given group id. "
			"The category ids for filtering. Using ALL_CATEGORIES_ID, everything is selectable.");
		AddClassToCollection("GameObjectController", "GameObject getNextGameObject(int categoryIds)", "Gets the next game object from the given group id as string. E.g. 'Player+Enemy'. "
			"The category ids for filtering. Using ALL_CATEGORIES_ID, everything is selectable.");
		
		AddClassToCollection("GameObjectController", "GameObject castGameObject(GameObject other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AnimationComponent castAnimationComponent(AnimationComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AttributesComponent castAttributesComponent(AttributesComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AttributeEffectComponent castAttributeEffectComponent(AttributeEffectComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsBuoyancyComponent castPhysicsBuoyancyComponent(PhysicsBuoyancyComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsTriggerComponent castPhysicsTriggerComponent(PhysicsTriggerComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "DistributedComponent castDistributedComponent(DistributedComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "ExitComponent castExitComponent(ExitComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "GameObjectTitleComponent castGameObjectTitleComponent(GameObjectTitleComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiMoveComponent castAiMoveComponent(AiMoveComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiMoveRandomlyComponent castAiMoveRandomlyComponent(AiMoveRandomlyComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiPathFollowComponent castAiPathFollowComponent(AiPathFollowComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiWanderComponent castAiWanderComponent(AiWanderComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiFlockingComponent castAiFlockingComponent(AiFlockingComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiRecastPathNavigationComponent castAiRecastPathNavigationComponent(AiRecastPathNavigationComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiObstacleAvoidanceComponent castAiObstacleAvoidanceComponent(AiObstacleAvoidanceComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiHideComponent castAiHideComponent(AiHideComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiMoveComponent2D castAiMoveComponent2D(AiMoveComponent2D other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiPathFollowComponent2D castAiPathFollowComponent2D(AiPathFollowComponent2D other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiWanderComponent2D castAiWanderComponent2D(AiWanderComponent2D other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CameraBehaviorBaseComponent castCameraBehaviorBaseComponent(CameraBehaviorBaseComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CameraBehaviorFirstPersonComponent castCameraBehaviorFirstPersonComponent(CameraBehaviorFirstPersonComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CameraBehaviorThirdPersonComponent castCameraBehaviorThirdPersonComponent(CameraBehaviorThirdPersonComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CameraBehaviorFollow2DComponent castCameraBehaviorFollow2DComponent(CameraBehaviorFollow2DComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CameraBehaviorZoomComponent castCameraBehaviorZoomComponent(CameraBehaviorZoomComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CameraComponent castCameraComponent(CameraComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CompositorEffectBloomComponent castCompositorEffectBloomComponent(CompositorEffectBloomComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CompositorEffectGlassComponent castCompositorEffectGlassComponent(CompositorEffectGlassComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CompositorEffectOldTvComponent castCompositorEffectOldTvComponent(CompositorEffectOldTvComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CompositorEffectKeyholeComponent castCompositorEffectKeyholeComponentt(CompositorEffectKeyholeComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CompositorEffectBlackAndWhiteComponent castCompositorEffectBlackAndWhiteComponent(CompositorEffectBlackAndWhiteComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CompositorEffectColorComponent castCompositorEffectColorComponent(CompositorEffectColorComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CompositorEffectEmbossedComponent castCompositorEffectEmbossedComponent(CompositorEffectEmbossedComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "CompositorEffectSharpenEdgesComponent castCompositorEffectSharpenEdgesComponent(CompositorEffectSharpenEdgesComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "HdrEffectComponent castHdrEffectComponent(HdrEffectComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "DatablockPbsComponent castDatablockPbsComponent(DatablockPbsComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "DatablockUnlitComponent castDatablockUnlitComponent(DatablockUnlitComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "DatablockTerraComponent castDatablockTerraComponent(DatablockTerraComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointComponent castJointComponent(JointComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointHingeComponent castJointHingeComponent(JointHingeComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointHingeActuatorComponent castJointHingeActuatorComponent(JointHingeActuatorComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointBallAndSocketComponent castJointBallAndSocketComponent(JointBallAndSocketComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointPointToPointComponent castJointPointToPointComponent(JointPointToPointComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointPinComponent castJointPinComponent(JointPinComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointPlaneComponent castJointPlaneComponent(JointPlaneComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointSpringComponent castJointSpringComponent(JointSpringComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointAttractorComponent castJointAttractorComponent(JointAttractorComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointCorkScrewComponent castJointCorkScrewComponent(JointCorkScrewComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointPassiveSliderComponent castJointPassiveSliderComponent(JointPassiveSliderComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointSliderActuatorComponent castJointSliderActuatorComponent(JointSliderActuatorComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointSlidingContactComponent castJointSlidingContactComponent(JointSlidingContactComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointActiveSliderComponent castJointActiveSliderComponent(JointActiveSliderComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointMathSliderComponent castJointMathSliderComponent(JointMathSliderComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointKinematicComponent castJointKinematicComponent(JointKinematicComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointDryRollingFrictionComponent castJointDryRollingFrictionComponent(JointDryRollingFrictionComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointPathFollowComponent castJointPathFollowComponent(JointPathFollowComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointGearComponent castJointGearComponent(JointGearComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointRackAndPinionComponent castJointRackAndPinionComponent(JointRackAndPinionComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointWormGearComponent castJointWormGearComponent(JointWormGearComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointPulleyComponent castJointPulleyComponent(JointPulleyComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointUniversalComponent castJointUniversalComponent(JointUniversalComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointUniversalActuatorComponent castJointUniversalActuatorComponent(JointUniversalActuatorComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "Joint6DofComponent castJoint6DofComponent(Joint6DofComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointMotorComponent castJointMotorComponent(JointMotorComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "JointWheelComponent castJointWheelComponent(JointWheelComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "LightDirectionalComponent castLightDirectionalComponent(LightDirectionalComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "LightPointComponent castLightPointComponent(LightPointComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "LightSpotComponent castLightSpotComponent(LightSpotComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "LightAreaComponent castLightAreaComponent(LightAreaComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "FadeComponent castFadeComponent(FadeComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "NavMeshComponent castNavMeshComponent(NavMeshComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "ParticleUniverseComponent castParticleUniverseComponent(ParticleUniverseComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PlayerControllerJumpNRunComponent castPlayerControllerJumpNRunComponent(PlayerControllerJumpNRunComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PlayerControllerJumpNRunLuaComponent castPlayerControllerJumpNRunLuaComponent(PlayerControllerJumpNRunLuaComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PlayerControllerClickToPointComponent castPlayerControllerClickToPointComponent(PlayerControllerClickToPointComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsComponent castPhysicsComponent(PhysicsComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsActiveComponent castPhysicsActiveComponent(PhysicsActiveComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsActiveCompoundComponent castPhysicsActiveCompoundComponent(PhysicsActiveCompoundComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsActiveDestructableComponent castPhysicsActiveDestructableComponent(PhysicsActiveDestructableComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsActiveKinematicComponent castPhysicsActiveKinematicComponent(PhysicsActiveKinematicComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsArtifactComponent castPhysicsArtifactComponent(PhysicsArtifactComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsRagDollComponent castPhysicsRagDollComponent(PhysicsRagDollComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsCompoundConnectionComponent castPhysicsCompoundConnectionComponent(PhysicsCompoundConnectionComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsMaterialComponent castPhysicsMaterialComponent(PhysicsMaterialComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsPlayerControllerComponent castPhysicsPlayerControllerComponent(PhysicsPlayerControllerComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PlaneComponent castPlaneComponent(PlaneComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "SimpleSoundComponent castSimpleSoundComponent(SimpleSoundComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "SoundComponent castSoundComponent(SoundComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "SpawnComponent castSpawnComponent(SpawnComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "VehicleComponent castVehicleComponent(VehicleComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "AiLuaComponent castAiLuaComponent(AiLuaComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "PhysicsExplosionComponent castPhysicsExplosionComponent(PhysicsExplosionComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MeshDecalComponent castMeshDecalComponent(MeshDecalComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "TagPointComponent castTagPointComponent(TagPointComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "TimeTriggerComponent castTimeTriggerComponent(TimeTriggerComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "TimeLineComponent castTimeLineComponent(TimeLineComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MoveMathFunctionComponent castMoveMathFunctionComponent(MoveMathFunctionComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "TagChildNodeComponent castTagChildNodeComponent(TagChildNodeComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "NodeTrackComponent castNodeTrackComponent(NodeTrackComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "LineComponent castLineComponent(LineComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "LinesComponent castLinesComponent(LinesComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "NodeComponent castNodeComponent(NodeComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "ManualObjectComponent castManualObjectComponent(ManualObjectComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "RectangleComponent castRectangleComponent(RectangleComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "ValueBarComponent castValueBarComponent(ValueBarComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "BillboardComponent castBillboardComponent(BillboardComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "RibbonTrailComponent castRibbonTrailComponent(RibbonTrailComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIWindowComponent castMyGUIWindowComponent(MyGUIWindowComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUITextComponent castMyGUITextComponent(MyGUITextComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIButtonComponent castMyGUIButtonComponent(MyGUIButtonComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUICheckBoxComponent castMyGUICheckBoxComponent(MyGUICheckBoxComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIImageBoxComponent castMyGUIImageBoxComponent(MyGUIImageBoxComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIProgressBarComponent castMyGUIProgressBarComponent(MyGUIProgressBarComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIListBoxComponent castMyGUIListBoxComponent(MyGUIListBoxComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIComboBoxComponent castMyGUIComboBoxComponent(MyGUIComboBoxComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIMessageBoxComponent castMyGUIMessageBoxComponent(MyGUIMessageBoxComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIPositionControllerComponent castMyGUIPositionControllerComponent(MyGUIPositionControllerComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIFadeAlphaControllerComponent castMyGUIFadeAlphaControllerComponent(MyGUIFadeAlphaControllerComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIScrollingMessageControllerComponent castMyGUIScrollingMessageControllerComponent(MyGUIScrollingMessageControllerComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIEdgeHideControllerComponent castMyGUIEdgeHideControllerComponent(MyGUIEdgeHideControllerComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIRepeatClickControllerComponent castMyGUIRepeatClickControllerComponent(MyGUIRepeatClickControllerComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIWindowComponent castMyGUIWindowComponent(MyGUIWindowComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIItemBoxComponent castMyGUIItemBoxComponent(MyGUIItemBoxComponent other)", "Casts an incoming type from function for lua auto completion.");
		AddClassToCollection("GameObjectController", "MyGUIMiniMapComponent castMyGUIMiniMapComponent(MyGUIMiniMapComponent other)", "Casts an incoming type from function for lua auto completion.");

		/*luabind::module(lua);
		[
			luabind::class_<std::vector<GameObject*>>("GameObjectVector");
			gameObjectController.def(luabind::constructor<>());
			gameObjectController.def("at", (GameObject*&(std::vector<GameObject*>::*)(size_t))&std::vector<GameObject*>::at);
			gameObjectController.def("size", &std::vector<GameObject*>::size);
		];*/

		object globalVars = globals(lua);
		// globalVars["GameObjectController"] = AppStateManager::getSingletonPtr()->getGameObjectController();
		globalVars["ALL_CATEGORIES_ID"] = Ogre::StringConverter::toString(GameObjectController::ALL_CATEGORIES_ID);
		globalVars["MAIN_GAMEOBJECT_ID"] = Ogre::StringConverter::toString(GameObjectController::MAIN_GAMEOBJECT_ID);
		globalVars["MAIN_CAMERA_ID"] = Ogre::StringConverter::toString(GameObjectController::MAIN_CAMERA_ID);
		globalVars["MAIN_LIGHT_ID"] = Ogre::StringConverter::toString(GameObjectController::MAIN_LIGHT_ID);

		AddClassToCollection("GlobalId", "ALL_CATEGORIES_ID", "All categories id.");
		AddClassToCollection("GlobalId", "MAIN_GAMEOBJECT_ID", "The main game object id.");
		AddClassToCollection("GlobalId", "MAIN_CAMERA_ID", "The main camera object id.");
		AddClassToCollection("GlobalId", "MAIN_LIGHT_ID", "The main light object id.");
	}

	GameObject* getOwner(GameObjectComponent* instance)
	{
		return instance->getOwner().get();
	}

	void setReferenceId(GameObjectComponent* instance, const Ogre::String& referenceId)
	{
		instance->setReferenceId(Ogre::StringConverter::parseUnsignedLong(referenceId));
	}

	Ogre::String getReferenceId(GameObjectComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getReferenceId());
	}

	void bindGameObjectComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<GameObjectComponent>("GameObjectComponent")
				.def("getOwner", &getOwner)
			// .def("init", &GameObjectComponent::init)
			// .def("postInit", &GameObjectComponent::postInit)
			.def("connect", &GameObjectComponent::connect)
			.def("disconnect", &GameObjectComponent::disconnect)
			// .def("onCloned", &GameObjectComponent::onCloned)
			// .def("onRemoveComponent", &GameObjectComponent::onRemoveComponent)
			// .def("onOtherComponentRemoved", &GameObjectComponent::onOtherComponentRemoved)
			// .def("handleEvent", &GameObjectComponent::handleEvent)
			// .def("update", &GameObjectComponent::update)
			// .def("writeXML", &GameObjectComponent::writeXML)
			// .def("getClassName", &GameObjectComponent::getClassName)
			.def("getParentClassName", &GameObjectComponent::getParentClassName)
			// .def("getClassId", &GameObjectComponent::getClassId)
			// .def("getParentClassId", &GameObjectComponent::getParentClassId)
			//// .def("clone", &GameObjectComponent::clone)
			.def("getPosition", &GameObjectComponent::getPosition)
			.def("getOrientation", &GameObjectComponent::getOrientation)
			.def("showDebugData", &GameObjectComponent::showDebugData)
			.def("setActivated", &GameObjectComponent::setActivated)
			// .def("setReferenceId", &GameObjectComponent::setReferenceId)
			// .def("getReferenceId", &GameObjectComponent::getReferenceId)
			.def("setReferenceId", &setReferenceId)
			.def("getReferenceId", &getReferenceId)
			.def("getOccurrenceIndex", &GameObjectComponent::getOccurrenceIndex)
			.def("getIndex", &GameObjectComponent::getIndex)
			];

		AddClassToCollection("GameObjectComponent", "class", "This is the base class for all components.");
		AddClassToCollection("GameObjectComponent", "GameObject getOwner()", "Gets owner game object of this component.");
		AddClassToCollection("GameObjectComponent", "bool connect()", "Connects this game object for simulation start. Normally there is no need to call this function.");
		AddClassToCollection("GameObjectComponent", "bool disconnect()", "Disconnects this game object when simulation has ended. Normally there is no need to call this function.");
		// AddClassToCollection("GameObjectComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// // AddClassToCollection(("GameObjectComponent"), "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("GameObjectComponent", "String getParentClassName()", "Gets the parent class name (the one this component is derived from) of this component as string.");
		// AddClassToCollection("GameObjectComponent", "number getClassId()", "Gets the class id of this component.");
		// AddClassToCollection("GameObjectComponent", "number getParentClassId()", "Gets the parent class id (the one this component is derived from) of this component.");
		AddClassToCollection("GameObjectComponent", "Vector3 getPosition()", "Gets the position of this game object component.");
		AddClassToCollection("GameObjectComponent", "void setActivated(bool activated)", "Sets whether this game object component should be activated or not.");
		AddClassToCollection("GameObject", "void setReferenceId(String referenceId)", "Sets the reference id. Note: If this game object is referenced by another game object, a component with the same id as this game object can be get for activation etc.");
		AddClassToCollection("GameObject", "String getReferenceId()", "Gets the reference if another game object does reference it.");
		AddClassToCollection("GameObjectComponent", "number getOccurrenceIndex()", "Gets the occurence index of game object component. That is, if a game object has several components of the same time, each component that occurs more than once has a different occurence index than 0.");
		AddClassToCollection("GameObjectComponent", "number getIndex()", "Gets the index of game object component (at which position the component is in the list.");

		module(lua)
			[
				class_<NodeComponent, GameObjectComponent>("NodeComponent")
			];
	}

	void bindAnimationComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<Ogre::v1::AnimationState>("AnimationState")
				.def("getTimePosition", &Ogre::v1::AnimationState::getTimePosition)
			.def("getLength", &Ogre::v1::AnimationState::getLength)
			.def("setTimePosition", &Ogre::v1::AnimationState::setTimePosition)
			];
		AddClassToCollection("AnimationState", "class", "The v1 animation state.");
		AddClassToCollection("AnimationState", "float getTimePosition()", "Gets the current time position of this animation state.");
		AddClassToCollection("AnimationState", "float getLength()", "Gets the animation length.");
		AddClassToCollection("AnimationState", "void setTimePosition(float timePosition)", "Sets the time position of this animation state.");

		module(lua)
		[
			class_<AnimationBlender>("AnimationBlender")
			.enum_("BlendingTransition")
			[
				value("BLEND_SWITCH", AnimationBlender::BlendSwitch),
				value("BLEND_WHILE_ANIMATING", AnimationBlender::BlendWhileAnimating),
				value("BLEND_THEN_ANIMATE", AnimationBlender::BlendThenAnimate)
			]
			.enum_("AnimID")
			[
				value("ANIM_IDLE_1", AnimationBlender::ANIM_IDLE_1),
				value("ANIM_IDLE_2", AnimationBlender::ANIM_IDLE_2),
				value("ANIM_IDLE_3", AnimationBlender::ANIM_IDLE_3),
				value("ANIM_IDLE_4", AnimationBlender::ANIM_IDLE_4),
				value("ANIM_IDLE_5", AnimationBlender::ANIM_IDLE_5),
				value("ANIM_WALK_NORTH", AnimationBlender::ANIM_WALK_NORTH),
				value("ANIM_WALK_SOUTH", AnimationBlender::ANIM_WALK_SOUTH),
				value("ANIM_WALK_WEST", AnimationBlender::ANIM_WALK_WEST),
				value("ANIM_WALK_EAST", AnimationBlender::ANIM_WALK_EAST),
				value("ANIM_RUN", AnimationBlender::ANIM_RUN),
				value("ANIM_CLIMB", AnimationBlender::ANIM_CLIMB),
				value("ANIM_SNEAK", AnimationBlender::ANIM_SNEAK),
				value("ANIM_HANDS_CLOSED", AnimationBlender::ANIM_HANDS_CLOSED),
				value("ANIM_HANDS_RELAXED", AnimationBlender::ANIM_HANDS_RELAXED),
				value("ANIM_DRAW_WEAPON", AnimationBlender::ANIM_DRAW_WEAPON),
				value("ANIM_SLICE_VERTICAL", AnimationBlender::ANIM_SLICE_VERTICAL),
				value("ANIM_SLICE_HORIZONTAL", AnimationBlender::ANIM_SLICE_HORIZONTAL),
				value("ANIM_JUMP_START", AnimationBlender::ANIM_JUMP_START),
				value("ANIM_JUMP_LOOP", AnimationBlender::ANIM_JUMP_LOOP),
				value("ANIM_JUMP_END", AnimationBlender::ANIM_JUMP_END),
				value("ANIM_HIGH_JUMP_END", AnimationBlender::ANIM_HIGH_JUMP_END),
				value("ANIM_JUMP_WALK", AnimationBlender::ANIM_JUMP_WALK),
				value("ANIM_FALL", AnimationBlender::ANIM_FALL),
				value("ANIM_EAT_1", AnimationBlender::ANIM_EAT_1),
				value("ANIM_EAT_2", AnimationBlender::ANIM_EAT_2),
				value("ANIM_PICKUP_1", AnimationBlender::ANIM_PICKUP_1),
				value("ANIM_PICKUP_2", AnimationBlender::ANIM_PICKUP_2),
				value("ANIM_ATTACK_1", AnimationBlender::ANIM_ATTACK_1),
				value("ANIM_ATTACK_2", AnimationBlender::ANIM_ATTACK_2),
				value("ANIM_ATTACK_3", AnimationBlender::ANIM_ATTACK_3),
				value("ANIM_ATTACK_4", AnimationBlender::ANIM_ATTACK_4),
				value("ANIM_SWIM", AnimationBlender::ANIM_SWIM),
				value("ANIM_THROW_1", AnimationBlender::ANIM_THROW_1),
				value("ANIM_THROW_2", AnimationBlender::ANIM_THROW_2),
				value("ANIM_DEAD_1", AnimationBlender::ANIM_DEAD_1),
				value("ANIM_DEAD_2", AnimationBlender::ANIM_DEAD_2),
				value("ANIM_DEAD_3", AnimationBlender::ANIM_DEAD_3),
				value("ANIM_SPEAK_1", AnimationBlender::ANIM_SPEAK_1),
				value("ANIM_SPEAK_2", AnimationBlender::ANIM_SPEAK_2),
				value("ANIM_SLEEP", AnimationBlender::ANIM_SLEEP),
				value("ANIM_DANCE", AnimationBlender::ANIM_DANCE),
				value("ANIM_DUCK", AnimationBlender::ANIM_DUCK),
				value("ANIM_CROUCH", AnimationBlender::ANIM_CROUCH),
				value("ANIM_HALT", AnimationBlender::ANIM_HALT),
				value("ANIM_ROAR", AnimationBlender::ANIM_ROAR),
				value("ANIM_SIGH", AnimationBlender::ANIM_SIGH),
				value("ANIM_GREETINGS", AnimationBlender::ANIM_GREETINGS),
				value("ANIM_NO_IDEA", AnimationBlender::ANIM_NO_IDEA),
				value("ANIM_ACTION_1", AnimationBlender::ANIM_ACTION_1),
				value("ANIM_ACTION_2", AnimationBlender::ANIM_ACTION_2),
				value("ANIM_ACTION_3", AnimationBlender::ANIM_ACTION_3),
				value("ANIM_ACTION_4", AnimationBlender::ANIM_ACTION_4),
				value("ANIM_NONE", AnimationBlender::ANIM_NONE)
			]

			.def("init1", (void (AnimationBlender::*)(AnimationBlender::AnimID, bool)) & AnimationBlender::init)
			.def("init2", (void (AnimationBlender::*)(const Ogre::String&, bool)) & AnimationBlender::init)
			.def("blend1", (void (AnimationBlender::*)(AnimationBlender::AnimID, AnimationBlender::BlendingTransition)) & AnimationBlender::blend)
			.def("blend2", (void (AnimationBlender::*)(const Ogre::String&, AnimationBlender::BlendingTransition)) & AnimationBlender::blend)
			.def("blend3", (void (AnimationBlender::*)(AnimationBlender::AnimID, AnimationBlender::BlendingTransition, bool)) & AnimationBlender::blend)
			.def("blend4", (void (AnimationBlender::*)(const Ogre::String&, AnimationBlender::BlendingTransition, bool)) & AnimationBlender::blend)
			.def("blend5", (void (AnimationBlender::*)(AnimationBlender::AnimID, AnimationBlender::BlendingTransition, Ogre::Real, bool)) & AnimationBlender::blend)
			.def("blend6", (void (AnimationBlender::*)(const Ogre::String&, AnimationBlender::BlendingTransition, Ogre::Real, bool)) & AnimationBlender::blend)
			.def("blendExclusive1", (void (AnimationBlender::*)(AnimationBlender::AnimID, AnimationBlender::BlendingTransition)) & AnimationBlender::blendExclusive)
			.def("blendExclusive2", (void (AnimationBlender::*)(const Ogre::String&, AnimationBlender::BlendingTransition)) & AnimationBlender::blendExclusive)
			.def("blendExclusive3", (void (AnimationBlender::*)(AnimationBlender::AnimID, AnimationBlender::BlendingTransition, bool)) & AnimationBlender::blendExclusive)
			.def("blendExclusive4", (void (AnimationBlender::*)(const Ogre::String&, AnimationBlender::BlendingTransition, bool)) & AnimationBlender::blendExclusive)
			.def("blendExclusive5", (void (AnimationBlender::*)(AnimationBlender::AnimID, AnimationBlender::BlendingTransition, Ogre::Real, bool)) & AnimationBlender::blendExclusive)
			.def("blendExclusive6", (void (AnimationBlender::*)(const Ogre::String&, AnimationBlender::BlendingTransition, Ogre::Real, bool)) & AnimationBlender::blendExclusive)
			.def("blendAndContinue1", (void (AnimationBlender::*)(AnimationBlender::AnimID)) & AnimationBlender::blendAndContinue)
			.def("blendAndContinue2", (void (AnimationBlender::*)(const Ogre::String&)) & AnimationBlender::blendAndContinue)
			.def("blendAndContinue3", (void (AnimationBlender::*)(AnimationBlender::AnimID, Ogre::Real)) & AnimationBlender::blendAndContinue)
			.def("blendAndContinue4", (void (AnimationBlender::*)(const Ogre::String&, Ogre::Real)) & AnimationBlender::blendAndContinue)
			.def("getProgress", &AnimationBlender::getProgress)
			.def("getSource", &AnimationBlender::getSource)
			.def("getTarget", &AnimationBlender::getTarget)
			.def("isComplete", &AnimationBlender::isComplete)
			.def("registerAnimation", &AnimationBlender::registerAnimation)
			.def("getAnimationIdFromString", &AnimationBlender::getAnimationIdFromString)
			.def("hasAnimation", (bool (AnimationBlender::*)(AnimationBlender::AnimID)) & AnimationBlender::hasAnimation)
			.def("hasAnimation", (bool (AnimationBlender::*)(const Ogre::String&)) & AnimationBlender::hasAnimation)
			.def("isAnimationActive", &AnimationBlender::isAnimationActive)
			.def("addTime", &AnimationBlender::addTime)
			.def("setTimePosition", &AnimationBlender::setTimePosition)
			.def("getTimePosition", &AnimationBlender::getTimePosition)
			.def("getLength", &AnimationBlender::getLength)
			.def("setWeight", &AnimationBlender::setWeight)
			.def("getWeight", &AnimationBlender::getWeight)
			.def("getBone", &AnimationBlender::getBone)
			.def("resetBones", &AnimationBlender::resetBones)
			.def("setBoneWeight", &AnimationBlender::setBoneWeight)
			.def("setDebugLog", &AnimationBlender::setDebugLog)
			.def("getLocalToWorldPosition", &AnimationBlender::getLocalToWorldPosition)
			.def("getLocalToWorldOrientation", &AnimationBlender::getLocalToWorldOrientation)

			/*.def("setSpeed", &AnimationBlender::setSpeed)
			.def("getSpeed", &AnimationBlender::getSpeed)
			.def("setRepeat", &AnimationBlender::setRepeat)
			.def("getRepeat", &AnimationBlender::getRepeat)*/
		];

		AddClassToCollection("AnimationBlender", "class", "This class can be used for more complex animations and transitions between them.");
		AddClassToCollection("BlendingTransition", "BLEND_SWITCH", "Ends the current animation and start a new one.");
		AddClassToCollection("BlendingTransition", "BLEND_WHILE_ANIMATING", "Fades from current animation to a new one.");
		AddClassToCollection("BlendingTransition", "BLEND_THEN_ANIMATE", "Fades the current animation to the first frame of the new one, after that execute the new animation.");

		AddClassToCollection("AnimID", "ANIM_IDLE_1", "Idle 1 animation.");
		AddClassToCollection("AnimID", "ANIM_IDLE_2", "Idle 2 animation.");
		AddClassToCollection("AnimID", "ANIM_IDLE_3", "Idle 3 animation.");
		AddClassToCollection("AnimID", "ANIM_IDLE_4", "Idle 4 animation.");
		AddClassToCollection("AnimID", "ANIM_IDLE_5", "Idle 5 animation.");
		AddClassToCollection("AnimID", "ANIM_WALK_NORTH", "Walk north animation.");
		AddClassToCollection("AnimID", "ANIM_WALK_SOUTH", "Walk south animation.");
		AddClassToCollection("AnimID", "ANIM_WALK_WEST", "Walk west animation.");
		AddClassToCollection("AnimID", "ANIM_WALK_EAST", "Walk east animation.");
		AddClassToCollection("AnimID", "ANIM_RUN", "Run animation.");
		AddClassToCollection("AnimID", "ANIM_CLIMB", "Climb animation.");
		AddClassToCollection("AnimID", "ANIM_SNEAK", "Sneak animation.");
		AddClassToCollection("AnimID", "ANIM_HANDS_CLOSED", "Hands closed animation.");
		AddClassToCollection("AnimID", "ANIM_HANDS_RELAXED", "Hands relaxed animation.");
		AddClassToCollection("AnimID", "ANIM_DRAW_WEAPON", "Draw weapon animation.");
		AddClassToCollection("AnimID", "ANIM_SLICE_VERTICAL", "Slice vertical animation.");
		AddClassToCollection("AnimID", "ANIM_SLICE_HORIZONTAL", "Slice horizontal animation.");
		AddClassToCollection("AnimID", "ANIM_JUMP_START", "Jump start animation.");
		AddClassToCollection("AnimID", "ANIM_JUMP_LOOP", "Jump loop (in air) animation.");
		AddClassToCollection("AnimID", "ANIM_JUMP_END", "Jump end animation.");
		AddClassToCollection("AnimID", "ANIM_HIGH_JUMP_END", "High jump end animation.");
		AddClassToCollection("AnimID", "ANIM_JUMP_WALK", "Jump walk animation.");
		AddClassToCollection("AnimID", "ANIM_FALL", "Fall animation.");
		AddClassToCollection("AnimID", "ANIM_EAT_1", "Eat 1 animation.");
		AddClassToCollection("AnimID", "ANIM_EAT_2", "Eat 1 animation.");
		AddClassToCollection("AnimID", "ANIM_PICKUP_1", "Pickup 1 animation.");
		AddClassToCollection("AnimID", "ANIM_PICKUP_2", "Pickup 2 animation.");
		AddClassToCollection("AnimID", "ANIM_ATTACK_1", "Attack 1 animation.");
		AddClassToCollection("AnimID", "ANIM_ATTACK_2", "Attack 2 animation.");
		AddClassToCollection("AnimID", "ANIM_ATTACK_3", "Attack 3 animation.");
		AddClassToCollection("AnimID", "ANIM_ATTACK_4", "Attack 4 animation.");
		AddClassToCollection("AnimID", "ANIM_SWIM", "Swim  animation.");
		AddClassToCollection("AnimID", "ANIM_THROW_1", "Throw 1 animation.");
		AddClassToCollection("AnimID", "ANIM_THROW_2", "Throw 2 animation.");
		AddClassToCollection("AnimID", "ANIM_DEAD_1", "Dead 1 animation.");
		AddClassToCollection("AnimID", "ANIM_DEAD_2", "Dead 2 animation.");
		AddClassToCollection("AnimID", "ANIM_DEAD_3", "Dead 3 animation.");
		AddClassToCollection("AnimID", "ANIM_SPEAK_1", "Speak 1 animation.");
		AddClassToCollection("AnimID", "ANIM_SPEAK_2", "Speak 2 animation.");
		AddClassToCollection("AnimID", "ANIM_SLEEP", "Sleep animation.");
		AddClassToCollection("AnimID", "ANIM_DANCE", "Dance animation.");
		AddClassToCollection("AnimID", "ANIM_DUCK", "Duck animation.");
		AddClassToCollection("AnimID", "ANIM_CROUCH", "Crouch animation.");
		AddClassToCollection("AnimID", "ANIM_HALT", "Halt animation.");
		AddClassToCollection("AnimID", "ANIM_ROAR", "Roar animation.");
		AddClassToCollection("AnimID", "ANIM_SIGH", "Sigh animation.");
		AddClassToCollection("AnimID", "ANIM_GREETINGS", "Greetings animation.");
		AddClassToCollection("AnimID", "ANIM_NO_IDEA", "No idea animation.");
		AddClassToCollection("AnimID", "ANIM_ACTION_1", "Action 1 animation.");
		AddClassToCollection("AnimID", "ANIM_ACTION_2", "Action 2 animation.");
		AddClassToCollection("AnimID", "ANIM_ACTION_3", "Action 3 animation.");
		AddClassToCollection("AnimID", "ANIM_ACTION_4", "Action 4 animation.");
		AddClassToCollection("AnimID", "ANIM_NONE", "None animation (default).");
		AddClassToCollection("AnimationBlender", "void init1(AnimID animationId, bool loop)", "Inits the animation blender, also sets and start the first current animation id.");
		AddClassToCollection("AnimationBlender", "void init2(String animationName, bool loop)", "Inits the animation blender, also sets and start the first current animation name.");
		AddClassToCollection("AnimationBlender", "void blend1(AnimID animationId, BlendingTransition transition)", "Blends from the current animation to the new animationId.");
		AddClassToCollection("AnimationBlender", "void blend2(String animationName, BlendingTransition transition)", "Blends from the current animation to the new animation name.");
		AddClassToCollection("AnimationBlender", "void blend3(AnimID animationId, BlendingTransition transition, bool loop)", "Blends from the current animation to the new animationId and sets whether the animation should be looped.");
		AddClassToCollection("AnimationBlender", "void blend4(String animationName, BlendingTransition transition, bool loop)", "Blends from the current animation to the new animation name and sets whether the animation should be looped.");
		AddClassToCollection("AnimationBlender", "void blend5(AnimID animationId, BlendingTransition blendingTransition, float duration, bool loop)", "Blends to the given animation id with a transition. Sets how long the animation should be played and if the animation should be looped.");
		AddClassToCollection("AnimationBlender", "void blend6(String animationName, BlendingTransition blendingTransition, float duration, bool loop)", "Blends to the given animation name with a transition. Sets how long the animation should be played and if the animation should be looped.");
		AddClassToCollection("AnimationBlender", "void blendAndContinue1(AnimID animationId)", "Blends to the given animation id with a transition. After the animation is finished, the previous one will continue.");
		AddClassToCollection("AnimationBlender", "void blendAndContinue2(String animationName)", "Blends to the given animation name with a transition. After the animation is finished, the previous one will continue.");
		AddClassToCollection("AnimationBlender", "void blendAndContinue3(AnimID animationId, float duration)", "Blends to the given animation id with a transition. Sets how long the animation should be played. After the animation is finished, the previous one will continue.");
		AddClassToCollection("AnimationBlender", "void blendAndContinue4(String animationName, float duration)", "Blends to the given animation name with a transition. Sets how long the animation should be played. After the animation is finished, the previous one will continue.");
		AddClassToCollection("AnimationBlender", "void blendExclusive1(AnimID animationId, BlendingTransition blendingTransition)", "Blends to the given animation id with a transition exclusively. The animation will only be blend, if it is not active currently.");
		AddClassToCollection("AnimationBlender", "void blendExclusive2(String animationName, BlendingTransition blendingTransition)", "Blends to the given animation name with a transition exclusively. The animation will only be blend, if it is not active currently.");
		AddClassToCollection("AnimationBlender", "void blendExclusive3(AnimID animationId, BlendingTransition blendingTransition, bool loop)", "Blends to the given animation id with a transition exclusively and optionally loops the animation. The animation will only be blend, if it is not active currently.");
		AddClassToCollection("AnimationBlender", "void blendExclusive4(String animationName, BlendingTransition blendingTransition, bool loop)", "Blends to the given animation name with a transition exclusively and optionally loops the animation. The animation will only be blend, if it is not active currently.");
		AddClassToCollection("AnimationBlender", "void blendExclusive5(AnimID animationId, BlendingTransition blendingTransition, float duration)", "Blends to the given animation id with a transition exclusively. Sets how long the animation should be played. The animation will only be blend, if it is not active currently.");
		AddClassToCollection("AnimationBlender", "void blendExclusive6(String animationName, BlendingTransition blendingTransition, float duration)", "Blends to the given animation name with a transition exclusively. Sets how long the animation should be played. The animation will only be blend, if it is not active currently.");
		AddClassToCollection("AnimationBlender", "float getProgress()", "Gets the progress of the currently played animation in ms.");
		AddClassToCollection("AnimationBlender", "bool isCompleted()", "Gets whether the currently played animation has completed or not.");
		AddClassToCollection("AnimationBlender", "void registerAnimation(String animationName, AnimID animationId)", "Registers the animation name and maps it with the given animation id.");
		AddClassToCollection("AnimationBlender", "AnimID getAnimationIdFromString(String animationName)", "Gets the mapped animation id from the given animation name.");
		AddClassToCollection("AnimationBlender", "bool hasAnimation(String animationName)", "Gets whether the given animation name does exist.");
		AddClassToCollection("AnimationBlender", "bool isAnimationActive(String animationName)", "Gets whether the given animation name is being currently played.");
		AddClassToCollection("AnimationBlender", "void setSpeed(float speed)", "Sets the animation speed. E.g. setting speed = 2, the animation will be played two times faster.");
		AddClassToCollection("AnimationBlender", "float getSpeed()", "Gets the animation speed.");
		AddClassToCollection("AnimationBlender", "void setRepeat(bool repeat)", "Sets whether to repeat the animation.");
		AddClassToCollection("AnimationBlender", "bool getRepeat()", "Gets gets whether the animation is played in a loop.");
		AddClassToCollection("AnimationBlender", "void setTimePosition(float timePosition)", "Sets the time position for the animation.");
		AddClassToCollection("AnimationBlender", "float getTimePosition()", "Gets the current animation time position.");
		AddClassToCollection("AnimationBlender", "float getLength()", "Gets the animation length.");
		AddClassToCollection("AnimationBlender", "void setWeight(float weight)", "Sets the animation weight. The more less the weight the more less all bones are moved");
		AddClassToCollection("AnimationBlender", "float getWeight()", "Gets the current animation weight.");

		module(lua)
		[
			class_<AnimationComponent, GameObjectComponent>("AnimationComponent")
			// .def("getClassName", &AnimationComponent::getClassName)
			.def("getParentClassName", &AnimationComponent::getParentClassName)
			// .def("clone", &AnimationComponent::clone)
			// .def("getClassId", &AnimationComponent::getClassId)
			// .def("getParentClassId", &AnimationComponent::getParentClassId)
			.def("setActivated", &AnimationComponent::setActivated)
			.def("isActivated", &AnimationComponent::isActivated)
			.def("setAnimationName", &AnimationComponent::setAnimationName)
			.def("getAnimationName", &AnimationComponent::getAnimationName)
			.def("setSpeed", &AnimationComponent::setSpeed)
			.def("getSpeed", &AnimationComponent::getSpeed)
			.def("setRepeat", &AnimationComponent::setRepeat)
			.def("getRepeat", &AnimationComponent::getRepeat)
			.def("isComplete", &AnimationComponent::isComplete)
			.def("getAnimationBlender", &AnimationComponent::getAnimationBlender)
			.def("getBone", &AnimationComponent::getBone)
			.def("setTimePosition", &AnimationComponent::setTimePosition)
			.def("getTimePosition", &AnimationComponent::getTimePosition)
			.def("getLength", &AnimationComponent::getLength)
			.def("setWeight", &AnimationComponent::setWeight)
			.def("getWeight", &AnimationComponent::getWeight)
			.def("getLocalToWorldPosition", &AnimationComponent::getLocalToWorldPosition)
			.def("getLocalToWorldOrientation", &AnimationComponent::getLocalToWorldOrientation)
		];

		AddClassToCollection("AnimationComponent", "class inherits GameObjectComponent", AnimationComponent::getStaticInfoText());
		// AddClassToCollection("AnimationComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("AnimationComponent", "String getClassName()", "Gets the class name of this component as string.");
		// // AddClassToCollection(("AnimationComponent", "String getParentClassName()", "Gets the parent class name (the one this component is derived from) of this component as string.");
		// AddClassToCollection("AnimationComponent", "number getClassId()", "Gets the class id of this component.");
		// AddClassToCollection("AnimationComponent", "number getParentClassId()", "Gets the parent class id (the one this component is derived from) of this component.");
		AddClassToCollection("AnimationComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not (Start the animations).");
		AddClassToCollection("AnimationComponent", "bool isActivated()", "Gets whether this component is activated.");
		AddClassToCollection("AnimationComponent", "void setAnimationName(String animationName)", "Sets the to be played animation name. If it does not exist, the animation cannot be played later.");
		AddClassToCollection("AnimationComponent", "String getAnimationName()", "Gets currently used animation name.");
		AddClassToCollection("AnimationComponent", "void setSpeed(float speed)", "Sets the animation speed for the current animation.");
		AddClassToCollection("AnimationComponent", "float getSpeed()", "Gets the animation speed for currently used animation.");
		AddClassToCollection("AnimationComponent", "void setRepeat(bool repeat)", "Sets whether the current animation should be repeated when finished.");
		AddClassToCollection("AnimationComponent", "bool getRepeat()", "Gets whether the current animation will be repeated when finished.");
		AddClassToCollection("AnimationComponent", "bool isComplete()", "Gets whether the current animation has finished.");
		AddClassToCollection("AnimationComponent", "AnmationBlender getAnimationBlender()", "Gets animation blender to manipulate animations directly.");
		AddClassToCollection("AnimationComponent", "Bone getBone(String boneName)", "Gets the bone by the given bone name for direct manipulation. Nil is delivered, if the bone name does not exist.");
		AddClassToCollection("AnimationComponent", "void setTimePosition(float timePosition)", "Sets the time position for the animation.");
		AddClassToCollection("AnimationComponent", "float getTimePosition()", "Gets the current animation time position.");
		AddClassToCollection("AnimationComponent", "float getLength()", "Gets the animation length.");
		AddClassToCollection("AnimationComponent", "void setWeight(float weight)", "Sets the animation weight. The more less the weight the more less all bones are moved.");
		AddClassToCollection("AnimationComponent", "float getWeight()", "Gets the current animation weight.");
	}

	void addAttributeBool(AttributesComponent* instance, const Ogre::String& name, bool value)
	{
		instance->addAttribute<bool>(name, value, Variant::VAR_BOOL);
	}

	void addAttributeFloat(AttributesComponent* instance, const Ogre::String& name, unsigned int value)
	{
		instance->addAttribute<unsigned int>(name, value, Variant::VAR_UINT);
	}

	void addAttributeInt(AttributesComponent* instance, const Ogre::String& name, int value)
	{
		instance->addAttribute<int>(name, value, Variant::VAR_INT);
	}

	void addAttributeUInt(AttributesComponent* instance, const Ogre::String& name, unsigned int value)
	{
		instance->addAttribute<unsigned int>(name, value, Variant::VAR_INT);
	}

	void addAttributeULong(AttributesComponent* instance, const Ogre::String& name, unsigned long value)
	{
		// Attention: Danger, as lua cannot work with huge numbers
		instance->addAttribute<unsigned long>(name, value, Variant::VAR_ULONG);
	}

	void addAttributeString(AttributesComponent* instance, const Ogre::String& name, const Ogre::String& value)
	{
		instance->addAttribute<Ogre::String>(name, value, Variant::VAR_STRING);
	}

	void addAttributeVector2(AttributesComponent* instance, const Ogre::String& name, const Ogre::Vector2& value)
	{
		instance->addAttribute<Ogre::Vector2>(name, value, Variant::VAR_VEC2);
	}

	void addAttributeVector3(AttributesComponent* instance, const Ogre::String& name, const Ogre::Vector3& value)
	{
		instance->addAttribute<Ogre::Vector3>(name, value, Variant::VAR_VEC3);
	}

	void addAttributeVector4(AttributesComponent* instance, const Ogre::String& name, const Ogre::Vector4& value)
	{
		instance->addAttribute<Ogre::Vector4>(name, value, Variant::VAR_VEC4);
	}

	void changeValueBool(AttributesComponent* instance, const Ogre::String& name, bool value)
	{
		instance->changeValue<bool>(name, value);
	}

	void changeValueInt(AttributesComponent* instance, const Ogre::String& name, int value)
	{
		instance->changeValue<int>(name, value);
	}

	void changeValueUInt(AttributesComponent* instance, const Ogre::String& name, unsigned int value)
	{
		instance->changeValue<unsigned int>(name, value);
	}

	void changeValueULong(AttributesComponent* instance, const Ogre::String& name, unsigned long value)
	{
		instance->changeValue<unsigned long>(name, value);
	}

	void changeValueFloat(AttributesComponent* instance, const Ogre::String& name, int value)
	{
		instance->changeValue<unsigned int>(name, value);
	}

	void changeValueString(AttributesComponent* instance, const Ogre::String& name, const Ogre::String& value)
	{
		instance->changeValue<Ogre::String>(name, value);
	}

	void changeValueVector2(AttributesComponent* instance, const Ogre::String& name, const Ogre::Vector2& value)
	{
		instance->changeValue<Ogre::Vector2>(name, value);
	}

	void changeValueVector3(AttributesComponent* instance, const Ogre::String& name, const Ogre::Vector3& value)
	{
		instance->changeValue<Ogre::Vector3>(name, value);
	}

	void changeValueVector4(AttributesComponent* instance, const Ogre::String& name, const Ogre::Vector4& value)
	{
		instance->changeValue<Ogre::Vector4>(name, value);
	}

	void bindAttributesComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<Variant>("Variant")
				// more constructors must be made with different parameters if necessary
			.def(constructor<>())
			// Operators -> Lua does not support >=, > why? http://www.rasterbar.com/products/luabind/docs.html
			/*.def(self < other<Variant>())
			.def(self > other<Variant>())
			.def(self <= other<Variant>())
			.def(self >= other<Variant>())
			.def(self == other<Variant>())
			.def(self != other<Variant>())
			*/
			// .def("assign", (void (Variant::*)(const Ogre::String&)) &Variant::assign)
			.def("assign", (void (Variant::*)(const Variant&)) & Variant::assign)
			// .def("add", (void (Variant::*)(const Ogre::String&)) &Variant::add)
			// .def("add2", (void (Variant::*)(const Variant&)) &Variant::add)
			// .def("inverse", (void (Variant::*)(const Ogre::String&)) &Variant::inverse)
			// .def("inverse2", (void (Variant::*)(const Variant&)) &Variant::inverse)
			// .def("toggle", (void (Variant::*)(const Ogre::String&)) &Variant::toggle)
			// .def("toggle2", (void (Variant::*)(const Variant&)) &Variant::toggle)
			// .def("hasInverse", &Variant::hasInverse)

			.def("getType", &Variant::getType)
			.def("getName", &Variant::getName)
			// .def("setReadOnly", &Variant::setReadOnly)
			// .def("isReadOnly", &Variant::isReadOnly)
			// .def("clone", &Variant::clone)
			// .def("setValueType", (void (Variant::*)(int, const Ogre::String&)) &Variant::setValue)
			// .def("setValueFloat", (void (Variant::*)(Ogre::Real)) &Variant::setValue)
			.def("setValueNumber", (void (Variant::*)(Ogre::Real)) & Variant::setValue)
			.def("setValueBool", (void (Variant::*)(bool)) & Variant::setValue)
			// .def("setValueInt", (void (Variant::*)(int)) &Variant::setValue)
			// .def("setValueUInt", (void (Variant::*)(unsigned int)) &Variant::setValue)
			.def("setValueString", (void (Variant::*)(const Ogre::String&)) & Variant::setValue)
			.def("setValueId", (void (Variant::*)(const Ogre::String&)) & Variant::setValue)
			.def("setValueVector2", (void (Variant::*)(const Ogre::Vector2&)) & Variant::setValue)
			.def("setValueVector3", (void (Variant::*)(const Ogre::Vector3&)) & Variant::setValue)
			.def("setValueVector4", (void (Variant::*)(const Ogre::Vector4&)) & Variant::setValue)
			// .def("setValueList", (void (Variant::*)(const std::vector<Ogre::String>&)) &Variant::setValue) does not work yet
			// .def("setValueQuaternion", (void (Variant::*)(const Ogre::Quaternion&)) &Variant::setValue)
			// .def("setValueMatrix3", (void (Variant::*)(const Ogre::Matrix3&)) &Variant::setValue)
			// .def("setValueMatrix4", (void (Variant::*)(const Ogre::Matrix4&)) &Variant::setValue)

			// .def("setValueVariant", (void (Variant::*)(const Variant&)) &Variant::setValue)

			// .def("getValueFloat", &Variant::getReal)
			.def("getValueNumber", &Variant::getReal)
			.def("getValueBool", &Variant::getBool)
			// .def("getValueInt", &Variant::getInt)
			// .def("getValueUInt", &Variant::getUInt)
			.def("getValueString", &Variant::getString)
			.def("getValueId", &Variant::getString)
			.def("getValueVector2", &Variant::getVector2)
			.def("getValueVector3", &Variant::getVector3)
			.def("getValueVector4", &Variant::getVector4)
			// .def("getValueQuaternion", &Variant::getQuaternion)
			// .def("getValueMatrix3", &Variant::getMatrix3)
			// .def("getValueMatrix4", &Variant::getMatrix4)
			];

		AddClassToCollection("Variant", "class", "The class Variant is a common datatype for any attribute key and value.");
		AddClassToCollection("Variant", "void assign(String other)", "Assings another value to this Variant.");
		AddClassToCollection("Variant", "int getType()", "Gets the type of this variant.");
		AddClassToCollection("Variant", "String getName()", "Gets the name of this variant.");
		AddClassToCollection("Variant", "void setValueNumber(number value)", "Sets the number value.");
		AddClassToCollection("Variant", "void setValueBool(bool value)", "Sets the bool value.");
		AddClassToCollection("Variant", "void setValueString(String value)", "Sets the String value.");
		AddClassToCollection("Variant", "void setValueId(String id)", "Sets the Game Object ID value.");
		AddClassToCollection("Variant", "void setValueVector2(Vector2 value)", "Sets the Vector2 value.");
		AddClassToCollection("Variant", "void setValueVector3(Vector3 value)", "Sets the Vector3 value.");
		AddClassToCollection("Variant", "void setValueVector4(Vector4 value)", "Sets the Vector4 value.");
		// AddClassToCollection("Variant", "void setValueQuaternion(Quaternion value)", "Sets the Quaternion value.");
		// AddClassToCollection("Variant", "void setValueMatrix3(Matrix3 value)", "Sets the Matrix3 value.");
		// AddClassToCollection("Variant", "void setValueMatrix4(Matrix4 value)", "Sets the Matrix4 value.");
		AddClassToCollection("Variant", "number getValueNumber()", "Gets the number value.");
		AddClassToCollection("Variant", "String getValueString()", "Gets the string value.");
		AddClassToCollection("Variant", "String getValueId()", "Gets the id value.");
		AddClassToCollection("Variant", "bool getValueBool()", "Gets the bool value.");
		AddClassToCollection("Variant", "Vector2 getValueVector2()", "Gets the Vector2 value.");
		AddClassToCollection("Variant", "Vector3 getValueVector3()", "Gets the Vector3 value.");
		AddClassToCollection("Variant", "Vector4 getValueVector4()", "Gets the Vector4 value.");
		// AddClassToCollection("Variant", "Quaternion getValueQuaternion()", "Gets the Quaternion value.");
		// AddClassToCollection("Variant", "Matrix3 getValueMatrix3()", "Gets the Matrix3 value.");
		// AddClassToCollection("Variant", "Matrix4 getValueMatrix4()", "Gets the Matrix4 value.");

		LUA_CONST_START(Variant)
			LUA_CONST(Variant, VAR_NULL);
		LUA_CONST(Variant, VAR_BOOL);
		LUA_CONST(Variant, VAR_REAL);
		LUA_CONST(Variant, VAR_INT);
		LUA_CONST(Variant, VAR_VEC2);
		LUA_CONST(Variant, VAR_VEC3);
		LUA_CONST(Variant, VAR_VEC4);
		LUA_CONST(Variant, VAR_QUAT);
		LUA_CONST(Variant, VAR_MAT3);
		LUA_CONST(Variant, VAR_MAT4);
		LUA_CONST(Variant, VAR_STRING);
		LUA_CONST_END;

		AddClassToCollection("Variant", "VAR_NULL", "Null type");
		AddClassToCollection("Variant", "VAR_BOOL", "Bool type");
		AddClassToCollection("Variant", "VAR_REAL", "Number type");
		AddClassToCollection("Variant", "VAR_INT", "Number type");
		AddClassToCollection("Variant", "VAR_VEC2", "Vector2 type");
		AddClassToCollection("Variant", "VAR_VEC3", "Vector3 type");
		AddClassToCollection("Variant", "VAR_VEC4", "Vector4 type");
		AddClassToCollection("Variant", "VAR_QUAT", "Quaternion type");
		AddClassToCollection("Variant", "VAR_MAT3", "Matrix3 type");
		AddClassToCollection("Variant", "VAR_MAT4", "Matrix4 type");
		AddClassToCollection("Variant", "VAR_STRING", "String type");

		module(lua)
			[
				class_<AttributesComponent, GameObjectComponent>("AttributesComponent")
				.def("getAttributeValueByIndex", &AttributesComponent::getAttributeValueByIndex)
			.def("getAttributeValueByName", &AttributesComponent::getAttributeValueByName)
			// .def("addAttribute", &AttributesComponent::addAttribute)
			// .def("deleteAttribute", &AttributesComponent::deleteAttribute)
			// .def("getAttributes", &AttributesComponent::getAttributes)
			// .def("deleteAllAttributes", &AttributesComponent::deleteAllAttributes)
			// .def("setCloneWithInitValues", &AttributesComponent::setCloneWithInitValues)
			.def("getAttributeValueBool", (bool (AttributesComponent::*)(unsigned int)) & AttributesComponent::getAttributeValueBool)
			// .def("getAttributeValueInt", (int (AttributesComponent::*)(unsigned int)) &AttributesComponent::getAttributeValueInt)
			// .def("getAttributeValueUInt", (unsigned int (AttributesComponent::*)(unsigned int)) &AttributesComponent::getAttributeValueUInt)
			// .def("getAttributeValueULong", (unsigned long (AttributesComponent::*)(unsigned int)) &AttributesComponent::getAttributeValueULong)
			// .def("getAttributeValueReal", (Ogre::Real (AttributesComponent::*)(unsigned int)) &AttributesComponent::getAttributeValueReal)
			.def("getAttributeValueNumber", (Ogre::Real(AttributesComponent::*)(unsigned int)) & AttributesComponent::getAttributeValueReal)
			.def("getAttributeValueString", (Ogre::String(AttributesComponent::*)(unsigned int)) & AttributesComponent::getAttributeValueString)
			.def("getAttributeValueId", (Ogre::String(AttributesComponent::*)(unsigned int)) & AttributesComponent::getAttributeValueString)
			.def("getAttributeValueVector2", (Ogre::Vector2(AttributesComponent::*)(unsigned int)) & AttributesComponent::getAttributeValueVector2)
			.def("getAttributeValueVector3", (Ogre::Vector3(AttributesComponent::*)(unsigned int)) & AttributesComponent::getAttributeValueVector3)
			.def("getAttributeValueVector4", (Ogre::Vector4(AttributesComponent::*)(unsigned int)) & AttributesComponent::getAttributeValueVector4)

			.def("getAttributeValueBool2", (bool (AttributesComponent::*)(const Ogre::String&)) & AttributesComponent::getAttributeValueBool)
			// .def("getAttributeValueInt2", (int (AttributesComponent::*)(const Ogre::String&)) &AttributesComponent::getAttributeValueInt)
			// .def("getAttributeValueUInt2", (unsigned int (AttributesComponent::*)(const Ogre::String&)) &AttributesComponent::getAttributeValueUInt)
			// .def("getAttributeValueULong2", (unsigned long (AttributesComponent::*)(const Ogre::String&)) &AttributesComponent::getAttributeValueULong)
			// .def("getAttributeValueReal2", (Ogre:Real (AttributesComponent::*)(const Ogre::String&)) &AttributesComponent::getAttributeValueReal)
			.def("getAttributeValueNumber2", (Ogre::Real(AttributesComponent::*)(const Ogre::String&)) & AttributesComponent::getAttributeValueReal)
			.def("getAttributeValueString2", (Ogre::String(AttributesComponent::*)(const Ogre::String&)) & AttributesComponent::getAttributeValueString)
			.def("getAttributeValueId2", (Ogre::String(AttributesComponent::*)(const Ogre::String&)) & AttributesComponent::getAttributeValueString)
			.def("getAttributeValueVector22", (Ogre::Vector2(AttributesComponent::*)(const Ogre::String&)) & AttributesComponent::getAttributeValueVector2)
			.def("getAttributeValueVector32", (Ogre::Vector3(AttributesComponent::*)(const Ogre::String&)) & AttributesComponent::getAttributeValueVector3)
			.def("getAttributeValueVector42", (Ogre::Vector4(AttributesComponent::*)(const Ogre::String&)) & AttributesComponent::getAttributeValueVector4)

			.def("addAttributeBool", &addAttributeBool)
			.def("addAttributeNumber", &addAttributeFloat)
			// .def("addAttributeFloat", &addAttributeFloat)
			// .def("addAttributeInt", &addAttributeInt)
			// .def("addAttributeUInt", &addAttributeUInt)
			// .def("addAttributeULong", &addAttributeULong)
			.def("addAttributeString", &addAttributeString)
			.def("addAttributeId", &addAttributeString)
			.def("addAttributeVector2", &addAttributeVector2)
			.def("addAttributeVector3", &addAttributeVector3)
			.def("addAttributeVector4", &addAttributeVector4)

			.def("changeValueBool", &changeValueBool)
			.def("changeValueNumber", &changeValueFloat)
			// .def("changeValueFloat", &changeValueFloat)
			// .def("changeValueInt", &changeValueInt)
			// .def("changeValueUInt", &changeValueUInt)
			// .def("changeValueULong", &changeValueULong)
			.def("changeValueString", &changeValueString)
			.def("changeValueId", &changeValueString)
			.def("changeValueVector2", &changeValueVector2)
			.def("changeValueVector3", &changeValueVector3)
			.def("changeValueVector4", &changeValueVector4)

			.def("saveValues", &AttributesComponent::saveValues)
			.def("saveValue", (void (AttributesComponent::*)(const Ogre::String&, unsigned int, bool)) & AttributesComponent::saveValue)
			.def("saveValue", (void (AttributesComponent::*)(const Ogre::String&, Variant*, bool)) & AttributesComponent::saveValue)
			.def("loadValues", &AttributesComponent::loadValues)
			.def("loadValue", (bool (AttributesComponent::*)(const Ogre::String&, unsigned int)) & AttributesComponent::loadValue)
			.def("loadValue", (bool (AttributesComponent::*)(const Ogre::String&, Variant*)) & AttributesComponent::loadValue)
			];

		AddClassToCollection("AttributesComponent", "class inherits GameObjectComponent", AttributesComponent::getStaticInfoText());
		AddClassToCollection("AttributesComponent", "Variant getAttributeValueByIndex(unsigned int index)", "Gets the attribute value as variant by the given index which corresponds the occurence in the attributes component.");
		AddClassToCollection("AttributesComponent", "Variant getAttributeValueByName(String name)", "Gets the attribute value as variant by the given name.");
		AddClassToCollection("AttributesComponent", "bool getAttributeValueBool(unsigned int index)", "Gets the bool value by the given index which corresponds the occurence in the attributes component.");
		AddClassToCollection("AttributesComponent", "number getAttributeValueNumber(unsigned int index)", "Gets the number value by the given index which corresponds the occurence in the attributes component.");
		AddClassToCollection("AttributesComponent", "String getAttributeValueString(unsigned int index)", "Gets the string value by the given index which corresponds the occurence in the attributes component.");
		AddClassToCollection("AttributesComponent", "String getAttributeValueId(unsigned int index)", "Gets the id value by the given index which corresponds the occurence in the attributes component.");
		AddClassToCollection("AttributesComponent", "Vector2 getAttributeValueVector2(unsigned int index)", "Gets the Vector2 value by the given index which corresponds the occurence in the attributes component.");
		AddClassToCollection("AttributesComponent", "Vector3 getAttributeValueVector3(unsigned int index)", "Gets the Vector3 value by the given index which corresponds the occurence in the attributes component.");
		AddClassToCollection("AttributesComponent", "Vector4 getAttributeValueVector4(unsigned int index)", "Gets the Vector4 value by the given index which corresponds the occurence in the attributes component.");
		AddClassToCollection("AttributesComponent", "bool getAttributeValueBool(String name)", "Gets the bool value by the given name");
		AddClassToCollection("AttributesComponent", "number getAttributeValueNumber(String name)", "Gets the number value by the given name");
		AddClassToCollection("AttributesComponent", "String getAttributeValueString(String name)", "Gets the string value by the given name");
		AddClassToCollection("AttributesComponent", "String getAttributeValueId(String name)", "Gets the id value by the given name");
		AddClassToCollection("AttributesComponent", "Vector2 getAttributeValueVector2(String name", "Gets the Vector2 value by the given name");
		AddClassToCollection("AttributesComponent", "Vector3 getAttributeValueVector3(String name)", "Gets the Vector3 value by the given name");
		AddClassToCollection("AttributesComponent", "Vector4 getAttributeValueVector4(String name)", "Gets the Vector4 value by the given name");

		AddClassToCollection("AttributesComponent", "void addAttributeBool(String name, bool value)", "Adds an attribute bool with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void addAttributeNumber(String name, number value)", "Adds an attribute number with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void addAttributeString(String name, String value)", "Adds an attribute string with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void addAttributeId(String name, String id)", "Adds an attribute id with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void addAttributeVector2(String name, Vector2 value)", "Adds an attribute Vector2 with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void addAttributeVector3(String name, Vector3 value)", "Adds an attribute Vector3 with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void addAttributeVector4(String name, Vector4 value)", "Adds an attribute Vector4 with the name and the given value.");

		AddClassToCollection("AttributesComponent", "void changeValueBool(String name, bool value)", "Changes the attribute bool with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void changeValueNumber(String name, number value)", "Changes the attribute number with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void changeValueString(String name, String value)", "Changes the attribute string with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void changeValueId(String name, String id)", "Changes the attribute id with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void changeValueVector2(String name, Vector2 value)", "Changes the attribute Vector2 with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void changeValueVector3(String name, Vector3 value)", "Changes the attribute Vector3 with the name and the given value.");
		AddClassToCollection("AttributesComponent", "void changeValueVector4(String name, Vector4 value)", "Changes the attribute Vector4 with the name and the given value.");

		AddClassToCollection("AttributesComponent", "void saveValues(String saveName, bool crypted)", "Saves all values from this component. Note: if crypted is set to true, the content will by set in a not readable form.");
		AddClassToCollection("AttributesComponent", "void saveValue(String saveName, unsigned int index, bool crypted)", "Save the value by the given index. Note: if crypted is set to true, the content will by set in a not readable form.");
		AddClassToCollection("AttributesComponent", "void saveValue2(String saveName, String name, bool crypted)", "Save the value by the given name. Note: if crypted is set to true, the content will by set in a not readable form.");

		AddClassToCollection("AttributesComponent", "bool loadValues(String saveName)", "Loads all values for this component.");
		AddClassToCollection("AttributesComponent", "bool loadValue(String saveName, unsigned int index)", "Loads the value by the given index.");
		AddClassToCollection("AttributesComponent", "bool loadValue2(String saveName, String name)", "Loads the value by the given name.");
	}

	void bindAttributeEffectComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<AttributeEffectComponent, GameObjectComponent>("AttributeEffectComponent")
			.def("setActivated", &AttributeEffectComponent::setActivated)
			.def("isActivated", &AttributeEffectComponent::isActivated)
			// .def("getMaxLength", &AttributeEffectComponent::getMaxLength)
			.def("reactOnEndOfEffect", (void (AttributeEffectComponent::*)(luabind::object, Ogre::Real, bool)) & AttributeEffectComponent::reactOnEndOfEffect)
			.def("reactOnEndOfEffect", (void (AttributeEffectComponent::*)(luabind::object, bool)) & AttributeEffectComponent::reactOnEndOfEffect)
		];

		AddClassToCollection("AttributeEffectComponent", "class inherits GameObjectComponent", AttributeEffectComponent::getStaticInfoText());
		AddClassToCollection("AttributeEffectComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not (Start math function calculation).");
		AddClassToCollection("AttributeEffectComponent", "bool isActivated()", "Gets whether this component is activated.");
		// AddClassToCollection("AttributeEffectComponent", "float getMaxLength()", "Gets max length, at which the function is at the end.");
		AddClassToCollection("AttributeEffectComponent", "void reactOnEndOfEffect(func closureFunction, float notificationValue, bool oneTime)",
			"Sets whether the target game object should be notified at the end of the attribute effect. One time means, that the nofication is done only once.");
		AddClassToCollection("AttributeEffectComponent", "void reactOnEndOfEffect(func closureFunction, bool oneTime)",
			"Sets whether the target game object should be notified at the end of the attribute effect. One time means, that the nofication is done only once.");
	}

	luabind::object getAttributes(DistributedComponent* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		const auto& attributes = instance->getAttributes();

		// unsigned int i = 0;
		for (auto& it = attributes.cbegin(); it != attributes.cend(); ++it)
		{
			obj[it->first] = it->second;
		}

		return obj;
	}

	void bindDistributedComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<DistributedComponent, GameObjectComponent>("DistributedComponent")
				// .def("getClassName", &DistributedComponent::getClassName)
			.def("getParentClassName", &DistributedComponent::getParentClassName)
			// .def("clone", &DistributedComponent::clone)
			// .def("getClassId", &DistributedComponent::getClassId)
			// .def("getParentClassId", &DistributedComponent::getParentClassId)
			.def("getAttributeValue", &DistributedComponent::getAttributeValue)
			.def("addAttribute", &DistributedComponent::addAttribute)
			.def("changeValue", (void (DistributedComponent::*)(const Ogre::String&, const Ogre::String&, Variant::Types)) & DistributedComponent::changeValue)
			.def("changeValue", (void (DistributedComponent::*)(const Ogre::String&)) & DistributedComponent::changeValue)
			.def("removeAttribute", &DistributedComponent::removeAttribute)
			.def("getAttributes", &getAttributes)
			];

		AddClassToCollection("DistributedComponent", "class inherits GameObjectComponent", DistributedComponent::getStaticInfoText());
		// AddClassToCollection("DistributedComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("DistributedComponent", "String getParentClassName()", "Gets the parent class name (the one this component is derived from) of this component as string.");
		// AddClassToCollection("DistributedComponent", "number getClassId()", "Gets the class id of this component.");
		// AddClassToCollection("DistributedComponent", "number getParentClassId()", "Gets the parent class id (the one this component is derived from) of this component.");
		//TODO: Component not finished!
	}

	void bindGameObjectTitleComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<GameObjectTitleComponent, GameObjectComponent>("GameObjectTitleComponent")
				// .def("getClassName", &GameObjectTitleComponent::getClassName)
			.def("getParentClassName", &GameObjectTitleComponent::getParentClassName)
			// .def("clone", &GameObjectTitleComponent::clone)
			// .def("getClassId", &GameObjectTitleComponent::getClassId)
			// .def("getParentClassId", &GameObjectTitleComponent::getParentClassId)
			// .def("setFontName", &GameObjectTitleComponent::setFontName)
			.def("setCaption", &GameObjectTitleComponent::setCaption)
			.def("setAlwaysPresent", &GameObjectTitleComponent::setAlwaysPresent)
			.def("setCharHeight", &GameObjectTitleComponent::setCharHeight)
			.def("setColor", &GameObjectTitleComponent::setColor)
			.def("setAlignment", &GameObjectTitleComponent::setAlignment)
			.def("setOffsetPosition", &GameObjectTitleComponent::setOffsetPosition)
			.def("setOffsetOrientation", &GameObjectTitleComponent::setOffsetOrientation)
			.def("getOffsetPosition", &GameObjectTitleComponent::getOffsetPosition)
			.def("getOffsetOrientation", &GameObjectTitleComponent::getOffsetOrientation)
			.def("setLookAtCamera", &GameObjectTitleComponent::setLookAtCamera)
			];

		AddClassToCollection("GameObjectTitleComponent", "class inherits GameObjectComponent", GameObjectTitleComponent::getStaticInfoText());
		// AddClassToCollection("GameObjectTitleComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("GameObjectTitleComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("GameObjectTitleComponent", "String getParentClassName()", "Gets the parent class name (the one this component is derived from) of this component as string.");
		// AddClassToCollection("GameObjectTitleComponent", "number getClassId()", "Gets the class id of this component.");
		// AddClassToCollection("GameObjectTitleComponent", "number getParentClassId()", "Gets the parent class id (the one this component is derived from) of this component.");
		// AddClassToCollection("GameObjectTitleComponent", "void setFontName(string fontName)", "Sets font name.");
		AddClassToCollection("GameObjectTitleComponent", "void setCaption(string caption)", "Sets the caption text to be displayed.");
		AddClassToCollection("GameObjectTitleComponent", "void setAlwaysPresent(bool alwaysPresent)", "Sets whether the object title text should always be visible, even if the game object is hidden behind another one.");
		AddClassToCollection("GameObjectTitleComponent", "void setCharHeight(float charHeight)", "Sets the height of the characters (default value is 0.2).");
		AddClassToCollection("GameObjectTitleComponent", "void setColor(Vector4 color)", "Sets the color (with alpha) for the title.");
		AddClassToCollection("GameObjectTitleComponent", "void setAlignment(Vector2 alignment)", "Sets the alignment for the title (default value is Vector2(1, 1)).");
		AddClassToCollection("GameObjectTitleComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets the offset position for the title. Note: Normally the title is placed at the same position as its carrying game object. Hence setting an offset is necessary.");
		AddClassToCollection("GameObjectTitleComponent", "Vector3 getOffsetPosition()", "Gets the offset position of the title.");
		AddClassToCollection("GameObjectTitleComponent", "void setOffsetOrientation(Vector3 offsetOrientation)", "Sets the offset orientation vector in degrees for the title. Note: Orientation is set in the form: (degreeX, degreeY, degreeZ).");
		AddClassToCollection("GameObjectTitleComponent", "Vector3 getOffsetOrientation()", "Gets the offset orientation vector in degrees of the title. Orientation is in the form: (degreeX, degreeY, degreeZ)");
		AddClassToCollection("GameObjectTitleComponent", "void setLookAtCamera(bool lookAtCamera)", "Sets whether to orientate the text always at the camera.");
	}

	void bindBuoyancyComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<PhysicsBuoyancyComponent, PhysicsComponent>("PhysicsBuoyancyComponent")
			.def("setWaterToSolidVolumeRatio", &PhysicsBuoyancyComponent::setWaterToSolidVolumeRatio)
			.def("getWaterToSolidVolumeRatio", &PhysicsBuoyancyComponent::getWaterToSolidVolumeRatio)
			.def("setViscosity", &PhysicsBuoyancyComponent::setViscosity)
			.def("getViscosity", &PhysicsBuoyancyComponent::getViscosity)
			.def("setBuoyancyGravity", &PhysicsBuoyancyComponent::setBuoyancyGravity)
			.def("getBuoyancyGravity", &PhysicsBuoyancyComponent::getBuoyancyGravity)
			.def("setOffsetHeight", &PhysicsBuoyancyComponent::setOffsetHeight)
			.def("getOffsetHeight", &PhysicsBuoyancyComponent::getOffsetHeight)
			// .def("destroyBuoyancy", &PhysicsBuoyancyComponent::destroyBuoyancy)
			.def("reactOnEnter", &PhysicsBuoyancyComponent::reactOnEnter)
			.def("reactOnInside", &PhysicsBuoyancyComponent::reactOnInside)
			.def("reactOnLeave", &PhysicsBuoyancyComponent::reactOnLeave)
		];

		AddClassToCollection("PhysicsBuoyancyComponent", "class inherits PhysicsComponent", PhysicsBuoyancyComponent::getStaticInfoText());
		AddClassToCollection("PhysicsBuoyancyComponent", "void setWaterToSolidVolumeRatio(float waterToSolidVolumeRatio)", "Sets the water to solid volume ratio (default is 0.9).");
		AddClassToCollection("PhysicsBuoyancyComponent", "float getWaterToSolidVolumeRatio()", "Gets the water to solid volume ratio (default is 0.9).");
		AddClassToCollection("PhysicsBuoyancyComponent", "void setViscosity(float viscosity)", "Sets the viscosity for the water (default is 0.995).");
		AddClassToCollection("PhysicsBuoyancyComponent", "float getViscosity()", "Gets the viscosity for the water (default is 0.995).");
		AddClassToCollection("PhysicsBuoyancyComponent", "void setBuoyancyGravity(float speed)", "Sets the gravity in this area (default value is Vector3(0, -15, 0)).");
		AddClassToCollection("PhysicsBuoyancyComponent", "float getBuoyancyGravity()", "Gets the gravity in this area (default value is Vector3(0, -15, 0)).");
		AddClassToCollection("PhysicsBuoyancyComponent", "void setOffsetHeight(float offsetHeight)", "Sets the offset height, at which the area begins (default value is 0).");
		AddClassToCollection("PhysicsBuoyancyComponent", "float getOffsetHeight()", "Gets the offset height, at which the area begins (default value is 0).");
		// AddClassToCollection("PhysicsBuoyancyComponent", "void destroyBuoyancy()", "Destroys the buoyancy trigger, so that no fluid physics take place anymore.");
		AddClassToCollection("PhysicsBuoyancyComponent", "class inherits PhysicsComponent", PhysicsTriggerComponent::getStaticInfoText());
		// AddClassToCollection("PhysicsBuoyancyComponent", "void destroyTrigger()", "Destroys the trigger.");
		AddClassToCollection("PhysicsBuoyancyComponent", "void reactOnEnter(func closure, visitorGameObject)",
							 "Lua closure function gets called in order to react when a game object enters the trigger area.");
		AddClassToCollection("PhysicsBuoyancyComponent", "void reactOnInside(func closure, visitorGameObject)",
							 "Lua closure function gets called in order to react when a game object is inside the trigger area. Note: This function should only be used if really necessary, because this function gets triggered permanently.");
		AddClassToCollection("PhysicsBuoyancyComponent", "void reactOnVanish(func closure, visitorGameObject)",
							 "Lua closure function gets called in order to react when a game object leaves the trigger area.");
	}

	void bindTriggerComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<PhysicsTriggerComponent, PhysicsComponent>("PhysicsTriggerComponent")
			// .def("destroyTrigger", &PhysicsTriggerComponent::destroyTrigger)
			.def("reactOnEnter", &PhysicsTriggerComponent::reactOnEnter)
			.def("reactOnInside", &PhysicsTriggerComponent::reactOnInside)
			.def("reactOnLeave", &PhysicsTriggerComponent::reactOnLeave)
		];

		AddClassToCollection("PhysicsTriggerComponent", "class inherits PhysicsComponent", PhysicsTriggerComponent::getStaticInfoText());
		// AddClassToCollection("PhysicsTriggerComponent", "void destroyTrigger()", "Destroys the trigger.");
		AddClassToCollection("PhysicsTriggerComponent", "void reactOnEnter(func closure, visitorGameObject)",
														  "Lua closure function gets called in order to react when a game object enters the trigger area.");
		AddClassToCollection("PhysicsTriggerComponent", "void reactOnInside(func closure, visitorGameObject)",
							 "Lua closure function gets called in order to react when a game object is inside the trigger area. Note: This function should only be used if really necessary, because this function gets triggered permanently.");
		AddClassToCollection("PhysicsTriggerComponent", "void reactOnVanish(func closure, visitorGameObject)",
														  "Lua closure function gets called in order to react when a game object leaves the trigger area.");
	}

	void bindParticleUniverseComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<ParticleUniverseComponent, GameObjectComponent>("ParticleUniverseComponent")
				// .def("getClassName", &ParticleUniverseComponent::getClassName)
				// .def("clone", &ParticleUniverseComponent::clone)
				// .def("getClassId", &ParticleUniverseComponent::getClassId)
			.def("setActivated", &ParticleUniverseComponent::setActivated)
			.def("isActivated", &ParticleUniverseComponent::isActivated)
			.def("setTemplateName", &ParticleUniverseComponent::setParticleTemplateName)
			.def("getTemplateName", &ParticleUniverseComponent::getParticleTemplateName)
			.def("setRepeat", &ParticleUniverseComponent::setRepeat)
			.def("getRepeat", &ParticleUniverseComponent::getRepeat)
			.def("setPlayTimeMS", &ParticleUniverseComponent::setParticlePlayTimeMS)
			.def("getPlayTimeMS", &ParticleUniverseComponent::getParticlePlayTimeMS)
			.def("setOffsetPosition", &ParticleUniverseComponent::setParticleOffsetPosition)
			.def("getOffsetPosition", &ParticleUniverseComponent::getParticleOffsetPosition)
			.def("setOffsetOrientation", &ParticleUniverseComponent::setParticleOffsetOrientation)
			.def("getOffsetOrientation", &ParticleUniverseComponent::getParticleOffsetOrientation)
			.def("setScale", &ParticleUniverseComponent::setParticleScale)
			.def("getScale", &ParticleUniverseComponent::getParticleScale)
			.def("setPlaySpeed", &ParticleUniverseComponent::setParticlePlaySpeed)
			.def("getPlaySpeed", &ParticleUniverseComponent::getParticlePlaySpeed)
			];

		AddClassToCollection("ParticleUniverseComponent", "class inherits GameObjectComponent", ParticleUniverseComponent::getStaticInfoText());
		// AddClassToCollection("ParticleUniverseComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("ParticleUniverseComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("ParticleUniverseComponent", "String getParentClassName()", "Gets the parent class name (the one this component is derived from) of this component as string.");
		// AddClassToCollection("ParticleUniverseComponent", "number getClassId()", "Gets the class id of this component.");
		// AddClassToCollection("ParticleUniverseComponent", "number getParentClassId()", "Gets the parent class id (the one this component is derived from) of this component.");
		AddClassToCollection("ParticleUniverseComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not (Start the particle effect).");
		AddClassToCollection("ParticleUniverseComponent", "bool isActivated()", "Gets whether this component is activated.");
		AddClassToCollection("ParticleUniverseComponent", "void setTemplateName(string particleTemplateName)", "Sets the particle template name. The name must be recognized by the resource system, else the particle effect cannot be played.");
		AddClassToCollection("ParticleUniverseComponent", "string getTemplateName()", "Gets currently used particle template name.");
		AddClassToCollection("ParticleUniverseComponent", "void setRepeat(bool repeat)", "Sets whether the current particle effect should be repeated when finished.");
		AddClassToCollection("ParticleUniverseComponent", "bool getRepeat()", "Gets whether the current particle effect will be repeated when finished.");

		AddClassToCollection("ParticleUniverseComponent", "void setPlayTimeMS(float particlePlayTimeMS)", "Sets particle play time in milliseconds, how long the particle effect should be played.");
		AddClassToCollection("ParticleUniverseComponent", "float getPlayTimeMS()", "Gets particle play time in milliseconds.");
		AddClassToCollection("ParticleUniverseComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets offset position of the particle effect at which it should be played away from the game object.");
		AddClassToCollection("ParticleUniverseComponent", "Vector3 getOffsetPosition()", "Gets offset position of the particle effect.");
		AddClassToCollection("ParticleUniverseComponent", "void setOffsetOrientation(Vector3 orientation)", "Sets offset orientation (as vector3(degree, degree, degree)) of the particle effect at which it should be played away from the game object.");
		AddClassToCollection("ParticleUniverseComponent", "Vector3 getOffsetPosition()", "Gets offset orientation (as vector3(degree, degree, degree)) of the particle effect.");
		AddClassToCollection("ParticleUniverseComponent", "void setScale(Vector3 scale)", "Sets the scale (size) of the particle effect.");
		AddClassToCollection("ParticleUniverseComponent", "Vector3 getScale()", "Gets scale of the particle effect.");
		AddClassToCollection("ParticleUniverseComponent", "void setPlaySpeed(float playSpeed)", "Sets particle play speed. E.g. 2 will play the particle at double speed.");
		AddClassToCollection("ParticleUniverseComponent", "float getPlaySpeed()", "Gets particle play play speed.");
	}

	Ogre::String getCategoriesId(PlayerControllerClickToPointComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getCategoriesId());
	}

	void bindPlayerControllerComponents(lua_State* lua)
	{
		module(lua)
			[
				class_<PlayerControllerComponent, GameObjectComponent>("PlayerControllerComponent")
				// .def("getClassName", &PlayerControllerComponent::getClassName)
				// .def("clone", &PlayerControllerComponent::clone)
				// .def("getClassId", &PlayerControllerComponent::getClassId)
			.def("getAnimationBlender", &PlayerControllerComponent::getAnimationBlender)
			.def("setRotationSpeed", &PlayerControllerComponent::setRotationSpeed)
			.def("getRotationSpeed", &PlayerControllerComponent::getRotationSpeed)
			.def("setAnimationSpeed", &PlayerControllerComponent::setAnimationSpeed)
			.def("getAnimationSpeed", &PlayerControllerComponent::getAnimationSpeed)
			.def("setMoveWeight", &PlayerControllerComponent::setMoveWeight)
			.def("lockMovement", &PlayerControllerComponent::lockMovement)
			.def("getMoveWeight", &PlayerControllerComponent::getMoveWeight)
			.def("setJumpWeight", &PlayerControllerComponent::setJumpWeight)
			.def("getJumpWeight", &PlayerControllerComponent::getJumpWeight)
			.def("setIdle", &PlayerControllerComponent::setIdle)
			.def("isIdle", &PlayerControllerComponent::isIdle)
			.def("getPhysicsComponent", &PlayerControllerComponent::getPhysicsComponent)
			.def("getPhysicsRagDollComponent", &PlayerControllerComponent::getPhysicsRagDollComponent)
			.def("getCameraBehaviorComponent", &PlayerControllerComponent::getCameraBehaviorComponent)
			.def("setAcceleration", &PlayerControllerComponent::setAcceleration)
			.def("getAcceleration", &PlayerControllerComponent::getAcceleration)
			.def("getNormal", &PlayerControllerComponent::getNormal)
			.def("getHeight", &PlayerControllerComponent::getHeight)
			.def("getSlope", &PlayerControllerComponent::getSlope)
			.def("getHitGameObjectBelow", &PlayerControllerComponent::getHitGameObjectBelow)
			.def("getHitGameObjectFront", &PlayerControllerComponent::getHitGameObjectFront)
			.def("getHitGameObjectUp", &PlayerControllerComponent::getHitGameObjectUp)
			.def("reactOnAnimationFinished", &PlayerControllerComponent::reactOnAnimationFinished)
			];

		AddClassToCollection("PlayerControllerComponent", "class inherits GameObjectComponent", PlayerControllerComponent::getStaticInfoText());
		// AddClassToCollection("PlayerControllerComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("PlayerControllerComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("PlayerControllerComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("PlayerControllerComponent", "void setDefaultDirection(Vector3 direction)", "Sets the direction the player is modelled.");
		AddClassToCollection("PlayerControllerComponent", "AnimationBlender getAnimationBlender()", "Gets the used animation blender so that the animations may be manipulated manually.");
		AddClassToCollection("PlayerControllerComponent", "void setRotationSpeed(float rotationSpeed)", "Sets the rotation speed for the player.");
		AddClassToCollection("PlayerControllerComponent", "float getRotationSpeed()", "Gets the player rotation speed.");
		AddClassToCollection("PlayerControllerComponent", "void setAnimationSpeed(float animationSpeed)", "Sets the animation speed for the player.");
		AddClassToCollection("PlayerControllerComponent", "float getAnimationSpeed()", "Gets the player animation speed.");
		AddClassToCollection("PlayerControllerComponent", "Vector3 getNormal()", "Gets the player ground normal vector.");
		AddClassToCollection("PlayerControllerComponent", "float getHeight()", "Gets the player height from the ground. Note: If the player is in air and there is no ground, the height is always 500. This value can be used for checking.");
		AddClassToCollection("PlayerControllerComponent", "float getSlope()", "Gets the player slope between the player and the ground.");
		AddClassToCollection("PlayerControllerComponent", "void setMoveWeight(float moveWeight)", "Sets the move weight for the player.");
		AddClassToCollection("PlayerControllerComponent", "void requestMoveWeight(string ownerName, float moveWeight)", "Requests a move weight for the given owner name, "
			"so that the lowest move weight is always present. E.g. when performing a pickup animation, the move weight is set to 0, and only the owner can release the move weight again!");
		AddClassToCollection("PlayerControllerComponent", "void releaseMoveWeight(string ownerName)", "Release the move weight for the given owner name, "
			"so that the next higher move weight is present again.");
		AddClassToCollection("PlayerControllerComponent", "void setJumpWeight(float jumpWeight)", "Sets the jump weight for the player.");
		AddClassToCollection("PlayerControllerComponent", "void requestJumpWeight(string ownerName, float jumpWeight)", "Requests a jump weight for the given owner name, "
			"so that the lowest jump weight is always present. E.g. when performing a pickup animation, the jump weight is set to 0, and only the owner can release the jump weight again!");
		AddClassToCollection("PlayerControllerComponent", "void releaseJumpWeight(string ownerName)", "Release the jump weight for the given owner name, "
			"so that the next higher jump weight is present again.");
		AddClassToCollection("PlayerControllerComponent", "PhysicsActiveComponent getPhysicsComponent()", "Gets physics active component for direct manipulation.");
		AddClassToCollection("PlayerControllerComponent", "PhysicsRagDollComponent getPhysicsRagDollComponent()", "Gets physics ragdoll component for direct manipulation. Note: Only use this function if the game object has a ragdoll created.");
		AddClassToCollection("PlayerControllerComponent", "GameObject getHitGameObjectBelow()", "Gets the game object, that has been hit below the player. Note: Always check against nil.");
		AddClassToCollection("PlayerControllerComponent", "GameObject getHitGameObjectFront()", "Gets the game object, that has been hit in front of the player. Note: Always check against nil.");
		AddClassToCollection("PlayerControllerComponent", "GameObject getHitGameObjectBelow()", "Gets the game object, that has been hit up the player. Note: Always check against nil.");
		AddClassToCollection("PlayerControllerComponent", "void reactOnAnimationFinished(func closureFunction, bool oneTime)",
			"Sets whether to react when the given animation has finished.");


		// Declarations missing
		// ...
		// AddClassToCollection("PlayerControllerComponent", "Vector3 getCenterBottomOfPlayer()", "Gets center of bottom vector of the player.");
		// AddClassToCollection("PlayerControllerComponent", "Vector3 getCenterBottomOfPlayer()", "Gets middle vector of the player.");

		module(lua)
			[
				class_<PlayerControllerJumpNRunComponent, PlayerControllerComponent>("PlayerControllerJumpNRunComponent")
				.def("setJumpForce", &PlayerControllerJumpNRunComponent::setJumpForce)
			.def("getJumpForce", &PlayerControllerJumpNRunComponent::getJumpForce)
			];

		AddClassToCollection("PlayerControllerJumpNRunComponent", "class inherits PlayerControllerComponent", PlayerControllerJumpNRunComponent::getStaticInfoText());
		AddClassToCollection("PlayerControllerJumpNRunComponent", "void setJumpForce(float jumpForce)", "Sets the jump force for the player.");
		AddClassToCollection("PlayerControllerJumpNRunComponent", "float getJumpForce()", "Gets the jump force.");

		module(lua)
		[
			class_<PlayerControllerJumpNRunLuaComponent, PlayerControllerComponent>("PlayerControllerJumpNRunLuaComponent")
			.def("setStartStateName", &PlayerControllerJumpNRunLuaComponent::setStartStateName)
			.def("getStartStateName", &PlayerControllerJumpNRunLuaComponent::getStartStateName)
			.def("getStateMachine", &PlayerControllerJumpNRunLuaComponent::getStateMachine)
		];

		AddClassToCollection("PlayerControllerJumpNRunLuaComponent", "class inherits PlayerControllerComponent", PlayerControllerJumpNRunLuaComponent::getStaticInfoText());
		AddClassToCollection("PlayerControllerJumpNRunLuaComponent", "void setStartStateName(String startName)", "Sets start state name in lua script, that should be executed.");
		AddClassToCollection("PlayerControllerJumpNRunLuaComponent", "String getStartStateName()", "Gets start state name in lua script, that should be executed.");
		AddClassToCollection("PlayerControllerJumpNRunLuaComponent", "LuaStateMachine getStateMachine()", "Gets the state machine to switch between states etc. in lua script.");

		module(lua)
			[
				class_<PlayerControllerClickToPointComponent, PlayerControllerComponent>("PlayerControllerClickToPointComponent")
				.def("setCategories", &PlayerControllerClickToPointComponent::setCategories)
			.def("getCategories", &PlayerControllerClickToPointComponent::getCategories)
			// .def("getCategoriesId", &PlayerControllerClickToPointComponent::getCategoriesId)
			.def("getCategoriesId", &getCategoriesId)
			.def("setRange", &PlayerControllerClickToPointComponent::setRange)
			.def("getRange", &PlayerControllerClickToPointComponent::getRange)
			.def("setPathSlot", &PlayerControllerClickToPointComponent::setPathSlot)
			.def("getPathSlot", &PlayerControllerClickToPointComponent::getPathSlot)
			.def("getMovingBehavior", &PlayerControllerClickToPointComponent::getMovingBehavior)
			];

		AddClassToCollection("PlayerControllerClickToPointComponent", "class inherits PlayerControllerComponent", PlayerControllerClickToPointComponent::getStaticInfoText());
		AddClassToCollection("PlayerControllerClickToPointComponent", "void setCategories(String categories)", "Sets categories (may be composed: e.g. ALL or ALL-House+Floor a new click point can be placed via mouse.");
		AddClassToCollection("PlayerControllerClickToPointComponent", "String getCategories()", "Gets categories a click point can be placed via mouse.");
		AddClassToCollection("PlayerControllerClickToPointComponent", "number getCategoryIds()", "Gets category ids a click point can be placed via mouse.");
		AddClassToCollection("PlayerControllerClickToPointComponent", "void setRange(float range)", "Sets the maximum range a point can be placed away from the player.");
		AddClassToCollection("PlayerControllerClickToPointComponent", "float getRange()", "Gets the maximum range a point can be placed away from the player.");
		AddClassToCollection("PlayerControllerClickToPointComponent", "void setPathSlot(int pathSlot)", "Sets path slot, a path should be generated in.");
		AddClassToCollection("PlayerControllerClickToPointComponent", "float getPathSlot()", "Gets path slot a path is generated.");
		AddClassToCollection("PlayerControllerClickToPointComponent", "MovingBehavior getMovingBehavior()", "Gets moving behavior for direct ai manipulation.");
	}

	void setSourceId(TagPointComponent* instance, const Ogre::String& sourceId)
	{
		instance->setSourceId(Ogre::StringConverter::parseUnsignedLong(sourceId));
	}

	Ogre::String getSourceId(TagPointComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getSourceId());
	}

	void bindTagPointComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<TagPointComponent, GameObjectComponent>("TagPointComponent")
				// .def("getClassName", &TagPointComponent::getClassName)
				// .def("clone", &TagPointComponent::clone)
				// .def("getClassId", &TagPointComponent::getClassId)
			.def("setTagPointName", &TagPointComponent::setTagPointName)
			.def("getTagPointName", &TagPointComponent::getTagPointName)
			// .def("setSourceId", &TagPointComponent::setSourceId)
			// .def("getSourceId", &TagPointComponent::getSourceId)
			.def("setSourceId", &setSourceId)
			.def("getSourceId", &getSourceId)
			.def("setOffsetPosition", &TagPointComponent::setOffsetPosition)
			.def("getOffsetPosition", &TagPointComponent::getOffsetPosition)
			.def("setOffsetOrientation", &TagPointComponent::setOffsetOrientation)
			.def("getOffsetOrientation", &TagPointComponent::getOffsetOrientation)
			];

		AddClassToCollection("TagPointComponent", "class inherits GameObjectComponent", TagPointComponent::getStaticInfoText());
		// AddClassToCollection("TagPointComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("TagPointComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("TagPointComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("TagPointComponent", "void setTagPointName(String tagName)", "Sets the tag point name the source game object should be attached to.");
		AddClassToCollection("TagPointComponent", "String getTagPointName()", "Gets the current active tag point name.");
		AddClassToCollection("TagPointComponent", "void setSourceId(String sourceId)", "Sets source id for the game object that should be attached to this tag point.");
		AddClassToCollection("TagPointComponent", "String getSourceId()", "Gets the source id for the game object that is attached to this tag point.");
		AddClassToCollection("TagPointComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets an offset position at which the source game object should be attached.");
		AddClassToCollection("TagPointComponent", "Vector3 getOffsetPosition()", "Gets the offset position at which the source game object is attached.");
		AddClassToCollection("TagPointComponent", "void setOffsetOrientation(Vector3 offsetPosition)", "Sets an offset orientation at which the source game object should be attached.");
		AddClassToCollection("TagPointComponent", "Vector3 getOffsetOrientation()", "Gets the offset orientation at which the source game object is attached.");
	}

	void bindTimeTriggerComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<TimeTriggerComponent, GameObjectComponent>("TimeTriggerComponent")
				.def("setActivated", &TimeTriggerComponent::setActivated)
			.def("isActivated", &TimeTriggerComponent::isActivated)
			];

		AddClassToCollection("TimeTriggerComponent", "class inherits GameObjectComponent", TimeTriggerComponent::getStaticInfoText());
		AddClassToCollection("TimeTriggerComponent", "void setActivated(bool activated)", "Sets whether time trigger can start or not.");
		AddClassToCollection("TimeTriggerComponent", "bool isActivated()", "Gets whether this time trigger is activated or not.");
	}

	void bindTimeLineComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<TimeLineComponent, GameObjectComponent>("TimeLineComponent")
				.def("setActivated", &TimeLineComponent::setActivated)
			.def("isActivated", &TimeLineComponent::isActivated)
			.def("setCurrentTimeSec", &TimeLineComponent::setCurrentTimeSec)
			.def("getCurrentTimeSec", &TimeLineComponent::getCurrentTimeSec)
			.def("getMaxTimeLineDuration", &TimeLineComponent::getMaxTimeLineDuration)
			];

		AddClassToCollection("TimeLineComponent", "class inherits GameObjectComponent", TimeLineComponent::getStaticInfoText());
		AddClassToCollection("TimeLineComponent", "void setActivated(bool activated)", "Sets whether time line can start or not.");
		AddClassToCollection("TimeLineComponent", "bool isActivated()", "Gets whether this time line is activated or not.");
		AddClassToCollection("TimeLineComponent", "bool setCurrentTimeSec(float timeSec)", "Sets the current time in seconds. Note: The next time point is determined, and the corresponding game object or lua function (if existing) called. Note: If the given time exceeds the overwhole time line duration, false is returned.");
		AddClassToCollection("TimeLineComponent", "float getCurrentTimeSec()", "Gets the current time in seconds, since this component is running.");
		AddClassToCollection("TimeLineComponent", "float getMaxTimeLineDuration()", "Calculates and gets the maximum time line duration in seconds. Note: Do not call this to often, because the max time is calculated each time from the scratch, in order to be as flexible as possible. E.g. a time point has been removed during runtime.");
	}

	void bindMoveMathFunctionComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<MoveMathFunctionComponent, GameObjectComponent>("MoveMathFunctionComponent")
			.def("setActivated", &MoveMathFunctionComponent::setActivated)
			.def("isActivated", &MoveMathFunctionComponent::isActivated)
			.def("reactOnFunctionFinished", &MoveMathFunctionComponent::reactOnFunctionFinished)
		];

		AddClassToCollection("MoveMathFunctionComponent", "class inherits GameObjectComponent", MoveMathFunctionComponent::getStaticInfoText());
		AddClassToCollection("MoveMathFunctionComponent", "void setActivated(bool activated)", "Sets whether move math function can start or not.");
		AddClassToCollection("MoveMathFunctionComponent", "bool isActivated()", "Gets whether this move math is activated or not.");
		AddClassToCollection("MoveMathFunctionComponent", "void reactOnFunctionFinished(func closureFunction)",
			"Sets wheather to react the game object function movement has finished.");
	}

	void setSourceId2(TagChildNodeComponent* instance, const Ogre::String& sourceId)
	{
		instance->setSourceId(Ogre::StringConverter::parseUnsignedLong(sourceId));
	}

	Ogre::String getSourceId2(TagChildNodeComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getSourceId());
	}

	void bindTagChildNodeComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<TagChildNodeComponent, GameObjectComponent>("TagChildNodeComponent")
				// .def("getClassName", &TagChildNodeComponent::getClassName)
				// .def("clone", &TagChildNodeComponent::clone)
				// .def("getClassId", &TagChildNodeComponent::getClassId)
				// .def("setSourceId", &TagChildNodeComponent::setSourceId)
				// .def("getSourceId", &TagChildNodeComponent::getSourceId)
			.def("setSourceId", &setSourceId2)
			.def("getSourceId", &getSourceId2)
			];

		AddClassToCollection("TagChildNodeComponent", "class inherits GameObjectComponent", TagChildNodeComponent::getStaticInfoText());
		// AddClassToCollection("TagChildNodeComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("TagChildNodeComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("TagChildNodeComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("TagChildNodeComponent", "void setSourceId(String sourceId)", "Sets source id for the game object that should be added as a child to this components game object.");
		AddClassToCollection("TagChildNodeComponent", "String getSourceId()", "Gets the source id for the game object that has been added as a child of this components game object.");
	}

	void setNodeTrackId(NodeTrackComponent* instance, unsigned int index, const Ogre::String& trackId)
	{
		instance->setNodeTrackId(index, Ogre::StringConverter::parseUnsignedLong(trackId));
	}

	Ogre::String getNodeTrackId(NodeTrackComponent* instance, unsigned int index)
	{
		return Ogre::StringConverter::toString(instance->getNodeTrackId(index));
	}

	void bindNodeTrackComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<NodeTrackComponent, GameObjectComponent>("NodeTrackComponent")
				// .def("getClassName", &NodeTrackComponent::getClassName)
				// .def("clone", &NodeTrackComponent::clone)
				// .def("getClassId", &NodeTrackComponent::getClassId)
			.def("setActivated", &NodeTrackComponent::setActivated)
			.def("isActivated", &NodeTrackComponent::isActivated)
			.def("setNodeTrackCount", &NodeTrackComponent::setNodeTrackCount)
			.def("getNodeTrackCount", &NodeTrackComponent::getNodeTrackCount)
			// .def("setNodeTrackId", &NodeTrackComponent::setNodeTrackId)
			// .def("getNodeTrackId", &NodeTrackComponent::getNodeTrackId)
			.def("setNodeTrackId", &setNodeTrackId)
			.def("getNodeTrackId", &getNodeTrackId)
			.def("setTimePosition", &NodeTrackComponent::setTimePosition)
			.def("getTimePosition", &NodeTrackComponent::getTimePosition)
			.def("setInterpolationMode", &NodeTrackComponent::setInterpolationMode)
			.def("getInterpolationMode", &NodeTrackComponent::getInterpolationMode)
			.def("setRotationMode", &NodeTrackComponent::setRotationMode)
			.def("getRotationMode", &NodeTrackComponent::getRotationMode)
			];

		AddClassToCollection("NodeTrackComponent", "class inherits GameObjectComponent", NodeTrackComponent::getStaticInfoText());
		// AddClassToCollection("NodeTrackComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("NodeTrackComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("NodeTrackComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("NodeTrackComponent", "void setActivated(bool activated)", "Sets whether this node track is activated or not.");
		AddClassToCollection("NodeTrackComponent", "bool isActivated()", "Gets whether this node track is activated or not.");
		AddClassToCollection("NodeTrackComponent", "void setNodeTrackCount(unsigned int nodeTrackCount)", "Sets the node track count (how many nodes are used for the tracking).");
		AddClassToCollection("NodeTrackComponent", "number getNodeTrackCount()", "Gets the node track count.");
		AddClassToCollection("NodeTrackComponent", "void setNodeTrackId(unsigned int index, String id)", "Sets the node track id for the given index in the node track list with @nodeTrackCount elements. Note: The order is controlled by the index, from which node to which node this game object will be tracked.");
		AddClassToCollection("NodeTrackComponent", "String getNodeTrackId(unsigned int index)", "Gets node track id from the given node track index from list.");
		AddClassToCollection("NodeTrackComponent", "void setTimePosition(unsigned int index, float timePosition)", "Sets time position in milliseconds after which this game object should be tracked at the node from the given index.");
		AddClassToCollection("NodeTrackComponent", "float getTimePosition(unsigned int index)", "Gets time position in milliseconds for the node with the given index.");
		AddClassToCollection("NodeTrackComponent", "void setInterpolationMode(String interpolationMode)", "Sets the curve interpolation mode how the game object will be moved. Possible values are: 'Spline', 'Linear'");
		AddClassToCollection("NodeTrackComponent", "String getInterpolationMode()", "Gets the curve interpolation mode how the game object is moved. Possible values are: 'Spline', 'Linear'");
		AddClassToCollection("NodeTrackComponent", "void setRotationMode(String rotationMode)", "Sets the rotation mode how the game object will be rotated during movement. Possible values are: 'Linear', 'Spherical'");
		AddClassToCollection("NodeTrackComponent", "String getRotationMode(void)", "Gets the rotation mode how the game object is rotated during movement. Possible values are: 'Linear', 'Spherical'");
	}

	void setTargetIdLine(LineComponent* instance, const Ogre::String& targetId)
	{
		instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(targetId));
	}

	Ogre::String getTargetIdLine(LineComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getTargetId());
	}

	void bindLineComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<LineComponent, GameObjectComponent>("LineComponent")
				// .def("getClassName", &LineComponent::getClassName)
				// .def("clone", &LineComponent::clone)
				// .def("getClassId", &LineComponent::getClassId)
				// .def("setTargetId", &LineComponent::setTargetId)
				// .def("getTargetId", &LineComponent::getTargetId)
			.def("setTargetId", &setTargetIdLine)
			.def("getTargetId", &getTargetIdLine)
			.def("setColor", &LineComponent::setColor)
			.def("getColor", &LineComponent::getColor)
			.def("setSourceOffsetPosition", &LineComponent::setSourceOffsetPosition)
			.def("getSourceOffsetPosition", &LineComponent::getSourceOffsetPosition)
			.def("setTargetOffsetPosition", &LineComponent::setTargetOffsetPosition)
			.def("getTargetOffsetPosition", &LineComponent::getTargetOffsetPosition)
			];

		AddClassToCollection("LineComponent", "class inherits GameObjectComponent", LineComponent::getStaticInfoText());
		// AddClassToCollection("LineComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("LineComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("LineComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("LineComponent", "void setTargetId(String targetId)", "Sets target id, to get the target game object to attach the lines end point at. The target id is optional.");
		AddClassToCollection("LineComponent", "String getTargetId()", "Gets target id of the target game object.");
		AddClassToCollection("LineComponent", "void setColor(Vector3 color)", "Sets color (r, g, b) of the line.");
		AddClassToCollection("LineComponent", "Vector3 getColor()", "Gets color (r, g, b) of the line.");
		AddClassToCollection("LineComponent", "void setSourceOffsetPosition(Vector3 sourceOffsetPosition)", "Sets the source offset position the line start point is attached at this source game object.");
		AddClassToCollection("LineComponent", "Vector3 getSourceOffsetPosition()", "Gets the source offset position the line start point.");
		AddClassToCollection("LineComponent", "void setTargetOffsetPosition(Vector3 targetOffsetPosition)", "Sets the target offset position the line end point is attached at this target game object. "
			"If the target game object does not exist, the target offset position is at global space.");
		AddClassToCollection("LineComponent", "Vector3 getTargetOffsetPosition()", "Gets the target offset position the line end point.");

		module(lua)
			[
				class_<LinesComponent, GameObjectComponent>("LinesComponent")
				// .def("getClassName", &LinesComponent::getClassName)
				// .def("clone", &LinesComponent::clone)
				// .def("getClassId", &LinesComponent::getClassId)
			.def("setConnected", &LinesComponent::setConnected)
			.def("getIsConnected", &LinesComponent::getIsConnected)
			.def("setOperationType", &LinesComponent::setOperationType)
			.def("getOperationType", &LinesComponent::getOperationType)
			.def("setLinesCount", &LinesComponent::setLinesCount)
			.def("getLinesCount", &LinesComponent::getLinesCount)
			.def("setColor", &LinesComponent::setColor)
			.def("getColor", &LinesComponent::getColor)
			.def("setStartPosition", &LinesComponent::setStartPosition)
			.def("getStartPosition", &LinesComponent::getStartPosition)
			.def("setEndPosition", &LinesComponent::setEndPosition)
			.def("getEndPosition", &LinesComponent::getEndPosition)
			];

		AddClassToCollection("LinesComponent", "class inherits GameObjectComponent", LinesComponent::getStaticInfoText());
		// AddClassToCollection("LinesComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("LinesComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("LinesComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("LinesComponent", "void setConnected(bool connected)", "Sets whether the lines should be connected together forming a more complex object.");
		AddClassToCollection("LinesComponent", "bool getConnected()", "Gets whether the lines are be connected together.");
		AddClassToCollection("LinesComponent", "void setOperationType(String operationType)", "Sets the operation type. Possible values are: A list of points, "
			"1 vertex per point OT_POINT_LIST = 1, A list of lines, 2 vertices per line OT_LINE_LIST = 2, A strip of connected lines, 1 vertex per line plus 1 start vertex "
			" OT_LINE_STRIP = 3, A list of triangles, 3 vertices per triangle OT_TRIANGLE_LIST = 4, A strip of triangles, 3 vertices for the first triangle, "
			" and 1 per triangle after that OT_TRIANGLE_STRIP = 5");
		AddClassToCollection("LinesComponent", "String getOperationType()", "Gets used operation type.");
		AddClassToCollection("LinesComponent", "void setLinesCount(unsigned int linesCount)", "Sets the lines count.");
		AddClassToCollection("LinesComponent", "number getLinesCount()", "Gets the lines count.");
		AddClassToCollection("LinesComponent", "void setColor(unsigned int index, Vector3 color)", "Sets color for the line (r, g, b).");
		AddClassToCollection("LinesComponent", "Vector3 getColor()", "Gets line color.");
		AddClassToCollection("LinesComponent", "void setStartPosition(unsigned int index, Vector3 startPosition)", "Sets the start position for the line.");
		AddClassToCollection("LinesComponent", "Vector3 getStartPosition()", "Gets the start position of the line.");
		AddClassToCollection("LinesComponent", "void setEndPosition(unsigned int index, Vector3 endPosition)", "Sets the end position for the line.");
		AddClassToCollection("LinesComponent", "Vector3 getEndPosition()", "Gets the end position of the line.");

		module(lua)
			[
				class_<ManualObjectComponent, GameObjectComponent>("ManualObjectComponent")
				// .def("getClassName", &ManualObjectComponent::getClassName)
				// .def("clone", &ManualObjectComponent::clone)
				// .def("getClassId", &ManualObjectComponent::getClassId)
			.def("setConnected", &ManualObjectComponent::setConnected)
			.def("getIsConnected", &ManualObjectComponent::getIsConnected)
			.def("setOperationType", &ManualObjectComponent::setOperationType)
			.def("getOperationType", &ManualObjectComponent::getOperationType)
			.def("setLinesCount", &ManualObjectComponent::setLinesCount)
			.def("getLinesCount", &ManualObjectComponent::getLinesCount)
			.def("setColor", &ManualObjectComponent::setColor)
			.def("getColor", &ManualObjectComponent::getColor)
			.def("setStartPosition", &ManualObjectComponent::setStartPosition)
			.def("getStartPosition", &ManualObjectComponent::getStartPosition)
			.def("setEndPosition", &ManualObjectComponent::setEndPosition)
			.def("getEndPosition", &ManualObjectComponent::getEndPosition)
			];

		AddClassToCollection("ManualObjectComponent", "class inherits GameObjectComponent", ManualObjectComponent::getStaticInfoText());
		// AddClassToCollection("ManualObjectComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("ManualObjectComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("ManualObjectComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("ManualObjectComponent", "void setConnected(bool connected)", "Sets whether the manual objects should be connected together forming a more complex object.");
		AddClassToCollection("ManualObjectComponent", "bool getConnected()", "Gets whether the manual objects are be connected together.");
		AddClassToCollection("ManualObjectComponent", "void setOperationType(String operationType)", "Sets the operation type. Possible values are: A list of points, "
			"1 vertex per point OT_POINT_LIST = 1, A list of lines, 2 vertices per line OT_LINE_LIST = 2, A strip of connected lines, 1 vertex per line plus 1 start vertex "
			" OT_LINE_STRIP = 3, A list of triangles, 3 vertices per triangle OT_TRIANGLE_LIST = 4, A strip of triangles, 3 vertices for the first triangle, "
			" and 1 per triangle after that OT_TRIANGLE_STRIP = 5");
		AddClassToCollection("ManualObjectComponent", "String getOperationType()", "Gets used operation type.");
		AddClassToCollection("ManualObjectComponent", "void setLinesCount(unsigned int linesCount)", "Sets the manual object count.");
		AddClassToCollection("ManualObjectComponent", "number getLinesCount()", "Gets the lines count.");
		AddClassToCollection("ManualObjectComponent", "void setColor(unsigned int index, Vector3 color)", "Sets color for the manual object (r, g, b).");
		AddClassToCollection("ManualObjectComponent", "Vector3 getColor()", "Gets the manual object color.");
		AddClassToCollection("ManualObjectComponent", "void setStartPosition(unsigned int index, Vector3 startPosition)", "Sets the start position for the manual object.");
		AddClassToCollection("ManualObjectComponent", "Vector3 getStartPosition()", "Gets the start position of the manual object.");
		AddClassToCollection("ManualObjectComponent", "void setEndPosition(unsigned int index, Vector3 endPosition)", "Sets the end position for the manual object.");
		AddClassToCollection("ManualObjectComponent", "Vector3 getEndPosition()", "Gets the end position of the manual object.");

		module(lua)
			[
				class_<RectangleComponent, GameObjectComponent>("RectangleComponent")
				// .def("getClassName", &RectangleComponent::getClassName)
				// .def("clone", &RectangleComponent::clone)
				// .def("getClassId", &RectangleComponent::getClassId)
			.def("setTwoSided", &RectangleComponent::setTwoSided)
			.def("getTwoSided", &RectangleComponent::getTwoSided)
			.def("setRectanglesCount", &RectangleComponent::setRectanglesCount)
			.def("getRectanglesCount", &RectangleComponent::getRectanglesCount)
			.def("setColor1", &RectangleComponent::setColor1)
			.def("getColor1", &RectangleComponent::getColor1)
			.def("setColor2", &RectangleComponent::setColor2)
			.def("getColor2", &RectangleComponent::getColor2)
			.def("setStartPosition", &RectangleComponent::setStartPosition)
			.def("getStartPosition", &RectangleComponent::getStartPosition)
			.def("setStartOrientation", &RectangleComponent::setStartOrientation)
			.def("getStartOrientation", &RectangleComponent::getStartOrientation)
			.def("setWidth", &RectangleComponent::setWidth)
			.def("getWidth", &RectangleComponent::getWidth)
			.def("setHeight", &RectangleComponent::setHeight)
			.def("getHeight", &RectangleComponent::getHeight)
			];

		AddClassToCollection("RectangleComponent", "class inherits GameObjectComponent", RectangleComponent::getStaticInfoText());
		// AddClassToCollection("RectangleComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("RectangleComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("RectangleComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("RectangleComponent", "void setTwoSided(bool two sided)", "Sets whether to render the rectangle on two sides.");
		AddClassToCollection("RectangleComponent", "bool getTwoSided()", "Gets whether the rectangle is rendered on two sides.");
		AddClassToCollection("RectangleComponent", "void setRectanglesCount(unsigned int rectanglesCount)", "Sets the rectangles count.");
		AddClassToCollection("RectangleComponent", "number getRectanglesCount()", "Gets the rectangles count.");
		AddClassToCollection("RectangleComponent", "void setColor1(unsigned int index, Vector3 color)", "Sets color 1 for the rectangle (r, g, b).");
		AddClassToCollection("RectangleComponent", "Vector3 getColor2()", "Gets rectangle color 2.");
		AddClassToCollection("RectangleComponent", "void setColor2(unsigned int index, Vector3 color)", "Sets color 2 for the rectangle (r, g, b).");
		AddClassToCollection("RectangleComponent", "Vector3 getColor1()", "Gets rectangle color 1.");
		AddClassToCollection("RectangleComponent", "void setStartPosition(unsigned int index, Vector3 startPosition)", "Sets the start position for the rectangle.");
		AddClassToCollection("RectangleComponent", "Vector3 getStartPosition()", "Gets the start position of the rectangle.");
		AddClassToCollection("RectangleComponent", "void setStartOrientation(unsigned int index, Vector3 startOrientation)", "Sets the start orientation vector in degrees for the rectangle. Note: Orientation is set in the form: (degreeX, degreeY, degreeZ).");
		AddClassToCollection("RectangleComponent", "Vector3 getStartOrientation()", "Gets the start orientation vector in degrees of the rectangle. Orientation is in the form: (degreeX, degreeY, degreeZ)");
		AddClassToCollection("RectangleComponent", "void setWidth(unsigned int index, float width)", "Sets the width of the rectangle.");
		AddClassToCollection("RectangleComponent", "float getWidth(unsigned int index)", "Gets the width of the rectangle.");
		AddClassToCollection("RectangleComponent", "void setHeight(unsigned int index, float height)", "Sets the height of the rectangle.");
		AddClassToCollection("RectangleComponent", "float getHeight(unsigned int index)", "Gets the height of the rectangle.");

		module(lua)
			[
				class_<ValueBarComponent, GameObjectComponent>("ValueBarComponent")
				// .def("getClassName", &ValueBarComponent::getClassName)
				// .def("clone", &ValueBarComponent::clone)
				// .def("getClassId", &ValueBarComponent::getClassId)
			.def("setTwoSided", &ValueBarComponent::setTwoSided)
			.def("getTwoSided", &ValueBarComponent::getTwoSided)
			.def("setInnerColor", &ValueBarComponent::setInnerColor)
			.def("getInnerColor", &ValueBarComponent::getInnerColor)
			.def("setOuterColor", &ValueBarComponent::setOuterColor)
			.def("getOuterColor", &ValueBarComponent::getOuterColor)
			.def("setBorderSize", &ValueBarComponent::setBorderSize)
			.def("getBorderSize", &ValueBarComponent::getBorderSize)
			// .def("setOrientationTargetId", &ValueBarComponent::setOrientationTargetId)
			// .def("getOrientationTargetId", &ValueBarComponent::getOrientationTargetId)

			// .def("setShowCurrentValue", &ValueBarComponent::setShowCurrentValue)
			// .def("getShowCurrentValue", &ValueBarComponent::getShowCurrentValue)
			.def("setOffsetPosition", &ValueBarComponent::setOffsetPosition)
			.def("getOffsetPosition", &ValueBarComponent::getOffsetPosition)
			.def("setOffsetOrientation", &ValueBarComponent::setOffsetOrientation)
			.def("getOffsetOrientation", &ValueBarComponent::getOffsetOrientation)
			.def("setWidth", &ValueBarComponent::setWidth)
			.def("getWidth", &ValueBarComponent::getWidth)
			.def("setHeight", &ValueBarComponent::setHeight)
			.def("getHeight", &ValueBarComponent::getHeight)
			.def("setMaxValue", &ValueBarComponent::setMaxValue)
			.def("getMaxValue", &ValueBarComponent::getMaxValue)
			.def("setCurrentValue", &ValueBarComponent::setCurrentValue)
			.def("getCurrentValue", &ValueBarComponent::getCurrentValue)
			];

		AddClassToCollection("ValueBarComponent", "class inherits GameObjectComponent", ValueBarComponent::getStaticInfoText());
		// AddClassToCollection("ValueBarComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("ValueBarComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("ValueBarComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("ValueBarComponent", "void setTwoSided(bool two sided)", "Sets whether to render the value bar on two sides.");
		AddClassToCollection("ValueBarComponent", "bool getTwoSided()", "Gets whether the value bar is rendered on two sides.");
		AddClassToCollection("ValueBarComponent", "void setInnerColor(Vector3 color)", "Sets the inner color for the value bar (r, g, b).");
		AddClassToCollection("ValueBarComponent", "Vector3 getInnerColor()", "Gets the inner color for the value bar (r, g, b).");
		AddClassToCollection("ValueBarComponent", "void setOuterColor(Vector3 color)", "Sets the outer color for the value bar (r, g, b).");
		AddClassToCollection("ValueBarComponent", "Vector3 getOuterColor()", "Gets the outer color for the value bar (r, g, b).");
		AddClassToCollection("ValueBarComponent", "void setBorderSize(number borderSize)", "Sets border size of the outer rectangle of the value bar.");
		AddClassToCollection("ValueBarComponent", "number getBorderSize()", "Gets border size of the outer rectangle of the value bar.");
		// AddClassToCollection("ValueBarComponent", "void setOrientationTargetId(string targetId)", "Sets the orientation target id, at which this enery bar should be automatically orientated.");
		// AddClassToCollection("ValueBarComponent", "string getOrientationTargetId()", "Gets border size of the outer border of the value bar.");
		// AddClassToCollection("ValueBarComponent", "void setShowCurrentValue(bool showCurrentValue, Vector3 textColor, number textSize, string currentValueText)", "Sets whether to show the current value as text in the middle of the value bar.");
		// AddClassToCollection("ValueBarComponent", "bool getShowCurrentValue()", "Gets whether to show the current value as text in the middle of the value bar.");
		AddClassToCollection("ValueBarComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets the offset position for the value bar. Note: Normally the value bar is placed at the same position as its carrying game object. Hence setting an offset is necessary.");
		AddClassToCollection("ValueBarComponent", "Vector3 getOffsetPosition()", "Gets the offset position of the value bar.");
		AddClassToCollection("ValueBarComponent", "void setOffsetOrientation(Vector3 offsetOrientation)", "Sets the offset orientation vector in degrees for the value bar. Note: Orientation is set in the form: (degreeX, degreeY, degreeZ).");
		AddClassToCollection("ValueBarComponent", "Vector3 getOffsetOrientation()", "Gets the offset orientation vector in degrees of the value bar. Orientation is in the form: (degreeX, degreeY, degreeZ)");
		AddClassToCollection("ValueBarComponent", "void setWidth(number width)", "Sets the width of the rectangle.");
		AddClassToCollection("ValueBarComponent", "number getWidth()", "Gets the width of the rectangle.");
		AddClassToCollection("ValueBarComponent", "void setHeight(number height)", "Sets the height of the rectangle.");
		AddClassToCollection("ValueBarComponent", "number getHeight()", "Gets the height of the rectangle.");
		AddClassToCollection("ValueBarComponent", "void setMaxValue(number maxValue)", "Sets the maximum value for the bar.");
		AddClassToCollection("ValueBarComponent", "number getMaxValue()", "Gets the maximum value for the bar.");
		AddClassToCollection("ValueBarComponent", "void setCurrentValue(number currentValue)", "Sets the current absolute value for the bar. Note: The value is clamped if it exceeds the maximum value or is < 0.");
		AddClassToCollection("ValueBarComponent", "number getCurrentValue()", "Gets the current value for the bar.");
	}

	void bindBillboardComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<BillboardComponent, GameObjectComponent>("BillboardComponent")
				// .def("getClassName", &BillboardComponent::getClassName)
				// .def("clone", &BillboardComponent::clone)
				// .def("getClassId", &BillboardComponent::getClassId)
			.def("setActivated", &BillboardComponent::setActivated)
			.def("isActivated", &BillboardComponent::isActivated)
			.def("setDatablockName", &BillboardComponent::setDatablockName)
			.def("getDatablockName", &BillboardComponent::getDatablockName)
			.def("setPosition", &BillboardComponent::setPosition)
			.def("getPosition", &BillboardComponent::getPosition)
			// .def("setOrigin", &BillboardComponent::setOrigin)
			// .def("getOrigin", &BillboardComponent::getOrigin)
			// .def("setRotationType", &BillboardComponent::setRotationType)
			// .def("getRotationType", &BillboardComponent::getRotationType)
			// .def("setType", &BillboardComponent::setType)
			// .def("getType", &BillboardComponent::getType)
			.def("setDimensions", &BillboardComponent::setDimensions)
			.def("getDimensions", &BillboardComponent::getDimensions)
			.def("setColor", &BillboardComponent::setColor)
			.def("getColor", &BillboardComponent::getColor)
			];

		AddClassToCollection("BillboardComponent", "class inherits GameObjectComponent", BillboardComponent::getStaticInfoText());
		// AddClassToCollection("BillboardComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("BillboardComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("BillboardComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("BillboardComponent", "void setActivated(bool activated)", "Sets whether this billboard is activated or not.");
		AddClassToCollection("BillboardComponent", "bool isActivated()", "Gets whether this billboard is activated or not.");
		AddClassToCollection("BillboardComponent", "void setDatablockName(String datablockName)", "Sets the data block name to used for the billboard representation. Note: It must be a unlit datablock.");
		AddClassToCollection("BillboardComponent", "String getDatablockName()", "Gets the used datablock name.");
		AddClassToCollection("BillboardComponent", "void setPosition(Vector3 position)", "Sets relative position offset where the billboard should be painted.");
		AddClassToCollection("BillboardComponent", "Vector3 getPosition()", "Gets the relative position offset of the billboard.");
		AddClassToCollection("BillboardComponent", "void setDimensions(Vector2 dimensions)", "Sets the dimensions (size x, y) of the billboard.");
		AddClassToCollection("BillboardComponent", "Vecto2 getDimensions()", "Gets the dimensions of the billboard.");
		AddClassToCollection("BillboardComponent", "void setColor(Vector3 color)", "Sets the color (r, g, b) of the billboard.");
		AddClassToCollection("BillboardComponent", "Vecto2 getDimensions()", "Gets the color (r, g, b) of the billboard.");
	}

	/*
	.enum_("SceneMemoryMgrTypes")
	[
			value("SceneMemoryMgrTypes", SCENE_DYNAMIC),
			value("SceneMemoryMgrTypes", SCENE_STATIC),
	]
	*/

	void bindProcedural(lua_State* lua)
	{
		module(lua)
			[
				class_<Procedural::BoxGenerator>("BoxGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::BoxGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::BoxGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::BoxGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::BoxGenerator::setTextureRectangle)

			.def("setSize", &Procedural::BoxGenerator::setSize)
			.def("setNumSegX", &Procedural::BoxGenerator::setNumSegX)
			.def("setNumSegY", &Procedural::BoxGenerator::setNumSegY)
			.def("setNumSegZ", &Procedural::BoxGenerator::setNumSegZ)
			.def("realizeMesh", &Procedural::BoxGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::BoxGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::BoxGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("BoxGenerator", "class", "Generates a procedural mesh box.");
		AddClassToCollection("BoxGenerator", "void setEnableNormals(bool enableNormals)", "Sets whether normals are enabled.");
		AddClassToCollection("BoxGenerator", "void setNumTexCoordSet(int numTexCoordSet)", "Sets the number of texture coordinate sets.");
		AddClassToCollection("BoxGenerator", "void setSwitchUV(bool switchUV)", "Sets whether the uv coordinates should be switched.");

		module(lua)
			[
				class_<Procedural::ConeGenerator>("ConeGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::ConeGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::ConeGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::ConeGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::ConeGenerator::setTextureRectangle)

			.def("setRadius", &Procedural::ConeGenerator::setRadius)
			.def("setHeight", &Procedural::ConeGenerator::setHeight)
			.def("realizeMesh", &Procedural::ConeGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::ConeGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::ConeGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("ConeGenerator", "class", "Generates a procedural mesh cone.");

		module(lua)
			[
				class_<Procedural::CylinderGenerator>("CylinderGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::CylinderGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::CylinderGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::CylinderGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::CylinderGenerator::setTextureRectangle)

			.def("setNumSegBase", &Procedural::CylinderGenerator::setNumSegBase)
			.def("setNumSegHeight", &Procedural::CylinderGenerator::setNumSegHeight)
			.def("setRadius", &Procedural::CylinderGenerator::setRadius)
			.def("setHeight", &Procedural::CylinderGenerator::setHeight)
			.def("setCapped", &Procedural::CylinderGenerator::setCapped)
			.def("realizeMesh", &Procedural::CylinderGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::CylinderGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::CylinderGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("CylinderGenerator", "class", "Generates a procedural mesh cylinder.");

		module(lua)
			[
				class_<Procedural::IcoSphereGenerator>("IcoSphereGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::IcoSphereGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::IcoSphereGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::IcoSphereGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::IcoSphereGenerator::setTextureRectangle)

			.def("setRadius", &Procedural::IcoSphereGenerator::setRadius)
			.def("setNumIterations", &Procedural::IcoSphereGenerator::setNumIterations)
			.def("realizeMesh", &Procedural::IcoSphereGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::IcoSphereGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::IcoSphereGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("IcoSphereGenerator", "class", "Generates a procedural mesh ico sphere.");

		module(lua)
			[
				class_<Procedural::PlaneGenerator>("PlaneGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::PlaneGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::PlaneGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::PlaneGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::PlaneGenerator::setTextureRectangle)

			.def("setNumSegX", &Procedural::PlaneGenerator::setNumSegX)
			.def("setNumSegY", &Procedural::PlaneGenerator::setNumSegY)
			.def("setNormal", &Procedural::PlaneGenerator::setNormal)
			.def("setSize", &Procedural::PlaneGenerator::setSize)
			.def("realizeMesh", &Procedural::PlaneGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::PlaneGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::PlaneGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("PlaneGenerator", "class", "Generates a procedural mesh plane.");

		module(lua)
			[
				class_<Procedural::PrismGenerator>("PrismGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::PrismGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::PrismGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::PrismGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::PrismGenerator::setTextureRectangle)

			.def("setNumSegHeight", &Procedural::PrismGenerator::setNumSegHeight)
			.def("setRadius", &Procedural::PrismGenerator::setRadius)
			.def("setHeight", &Procedural::PrismGenerator::setHeight)
			.def("setCapped", &Procedural::PrismGenerator::setCapped)
			.def("setNumSides", &Procedural::PrismGenerator::setNumSides)
			.def("realizeMesh", &Procedural::PrismGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::PrismGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::PrismGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("PrismGenerator", "class", "Generates a procedural mesh prism.");

		module(lua)
			[
				class_<Procedural::RoundedBoxGenerator>("RoundedBoxGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::RoundedBoxGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::RoundedBoxGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::RoundedBoxGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::RoundedBoxGenerator::setTextureRectangle)

			.def("setSize", &Procedural::RoundedBoxGenerator::setSize)
			.def("setNumSegX", &Procedural::RoundedBoxGenerator::setNumSegX)
			.def("setNumSegY", &Procedural::RoundedBoxGenerator::setNumSegY)
			.def("setNumSegZ", &Procedural::RoundedBoxGenerator::setNumSegZ)
			.def("setChamferSize", &Procedural::RoundedBoxGenerator::setChamferSize)
			.def("realizeMesh", &Procedural::RoundedBoxGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::RoundedBoxGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::RoundedBoxGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("RoundedBoxGenerator", "class", "Generates a procedural mesh rounded box.");

		module(lua)
			[
				class_<Procedural::SphereGenerator>("SphereGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::SphereGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::SphereGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::SphereGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::SphereGenerator::setTextureRectangle)

			.def("setNumRings", &Procedural::SphereGenerator::setNumRings)
			.def("setNumSegments", &Procedural::SphereGenerator::setNumSegments)
			.def("setRadius", &Procedural::SphereGenerator::setRadius)
			.def("realizeMesh", &Procedural::SphereGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::SphereGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::SphereGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("SphereGenerator", "class", "Generates a procedural mesh sphere.");

		module(lua)
			[
				class_<Procedural::CapsuleGenerator>("CapsuleGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::CapsuleGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::CapsuleGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::CapsuleGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::CapsuleGenerator::setTextureRectangle)

			.def("setNumRings", &Procedural::CapsuleGenerator::setNumRings)
			.def("setNumSegments", &Procedural::CapsuleGenerator::setNumSegments)
			.def("setRadius", &Procedural::CapsuleGenerator::setRadius)
			.def("setHeight", &Procedural::CapsuleGenerator::setHeight)
			.def("setNumSegHeight", &Procedural::CapsuleGenerator::setNumSegHeight)
			.def("realizeMesh", &Procedural::CapsuleGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::CapsuleGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::CapsuleGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("CapsuleGenerator", "class", "Generates a procedural mesh capsule.");

		module(lua)
			[
				class_<Procedural::SpringGenerator>("SpringGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::SpringGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::SpringGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::SpringGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::SpringGenerator::setTextureRectangle)

			.def("setRadiusCircle", &Procedural::SpringGenerator::setRadiusCircle)
			.def("setHeight", &Procedural::SpringGenerator::setHeight)
			.def("setNumSegPath", &Procedural::SpringGenerator::setNumSegPath)
			.def("setNumRound", &Procedural::SpringGenerator::setNumRound)
			.def("realizeMesh", &Procedural::SpringGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::SpringGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::SpringGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("SpringGenerator", "class", "Generates a procedural mesh spring.");

		module(lua)
			[
				class_<Procedural::TorusGenerator>("TorusGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::TorusGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::TorusGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::TorusGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::TorusGenerator::setTextureRectangle)

			.def("setRadius", &Procedural::TorusGenerator::setRadius)
			.def("setNumSegSection", &Procedural::TorusGenerator::setNumSegSection)
			.def("setNumSegCircle", &Procedural::TorusGenerator::setNumSegCircle)
			.def("setSectionRadius", &Procedural::TorusGenerator::setSectionRadius)
			.def("realizeMesh", &Procedural::TorusGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::TorusGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::TorusGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("TorusGenerator", "class", "Generates a procedural mesh torus.");

		module(lua)
			[
				class_<Procedural::TorusKnotGenerator>("TorusKnotGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::TorusKnotGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::TorusKnotGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::TorusKnotGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::TorusKnotGenerator::setTextureRectangle)

			.def("setRadius", &Procedural::TorusKnotGenerator::setRadius)
			.def("setNumSegSection", &Procedural::TorusKnotGenerator::setNumSegSection)
			.def("setNumSegCircle", &Procedural::TorusKnotGenerator::setNumSegCircle)
			.def("setSectionRadius", &Procedural::TorusKnotGenerator::setSectionRadius)
			.def("setP", &Procedural::TorusKnotGenerator::setP)
			.def("setQ", &Procedural::TorusKnotGenerator::setQ)
			.def("realizeMesh", &Procedural::TorusKnotGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::TorusKnotGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::TorusKnotGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("TorusKnotGenerator", "class", "Generates a procedural mesh torus knot.");

		module(lua)
			[
				class_<Procedural::TubeGenerator>("TubeGenerator")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("setEnableNormals", &Procedural::TubeGenerator::setEnableNormals)
			.def("setNumTexCoordSet", &Procedural::TubeGenerator::setNumTexCoordSet)
			.def("setSwitchUV", &Procedural::TubeGenerator::setSwitchUV)
			.def("setTextureRectangle", &Procedural::TubeGenerator::setTextureRectangle)

			.def("setNumSegBase", &Procedural::TubeGenerator::setNumSegBase)
			.def("setNumSegHeight", &Procedural::TubeGenerator::setNumSegHeight)
			.def("setHeight", &Procedural::TubeGenerator::setHeight)
			.def("setInnerRadius", &Procedural::TubeGenerator::setInnerRadius)
			.def("setOuterRadius", &Procedural::TubeGenerator::setOuterRadius)
			.def("realizeMesh", &Procedural::TubeGenerator::realizeMesh)
			.def("addToTriangleBuffer", &Procedural::TubeGenerator::addToTriangleBuffer)
			.def("buildTriangleBuffer", &Procedural::TubeGenerator::buildTriangleBuffer)
			];

		AddClassToCollection("TubeGenerator", "class", "Generates a procedural mesh tube.");

		module(lua)
			[
				class_<Procedural::Boolean>("Boolean")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.enum_("BooleanOperation")
			[
				value("BT_UNION", Procedural::Boolean::BT_UNION),
				value("BT_INTERSECTION", Procedural::Boolean::BT_INTERSECTION),
				value("BT_DIFFERENCE", Procedural::Boolean::BT_DIFFERENCE)
			]

		.def("setMesh1", &Procedural::Boolean::setMesh1)
			.def("setMesh2", &Procedural::Boolean::setMesh2)
			.def("setBooleanOperation", &Procedural::Boolean::setBooleanOperation)
			.def("addToTriangleBuffer", &Procedural::Boolean::addToTriangleBuffer)
			];

		AddClassToCollection("Boolean", "class", "Uses a boolean operation on two procedural meshes.");

		module(lua)
			[
				class_<Procedural::Extruder>("Extruder")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("addToTriangleBuffer", &Procedural::Extruder::addToTriangleBuffer)
			.def("setShapeToExtrude", &Procedural::Extruder::setShapeToExtrude)
			.def("setMultiShapeToExtrude", &Procedural::Extruder::setMultiShapeToExtrude)
			.def("setExtrusionPath", (Procedural::Extruder & (Procedural::Shape::*)(const Procedural::Path*)) & Procedural::Extruder::setExtrusionPath)
			.def("setExtrusionPath", (Procedural::Extruder & (Procedural::Shape::*)(const Procedural::MultiPath*)) & Procedural::Extruder::setExtrusionPath)
			.def("setRotationTrack", &Procedural::Extruder::setRotationTrack)
			.def("setScaleTrack", &Procedural::Extruder::setScaleTrack)
			.def("setShapeTextureTrack", &Procedural::Extruder::setShapeTextureTrack)
			.def("setPathTextureTrack", &Procedural::Extruder::setPathTextureTrack)
			.def("setCapped", &Procedural::Extruder::setCapped)
			];

		AddClassToCollection("Extruder", "class", "Extrudes a procedural meshes.");

		module(lua)
			[
				class_<Procedural::Lathe>("Lathe")
				// .def(constructor<Shape*, unsigned int>())
			.def("setNumSeg", &Procedural::Lathe::setNumSeg)
			.def("setAngleBegin", &Procedural::Lathe::setAngleBegin)
			.def("setAngleEnd", &Procedural::Lathe::setAngleEnd)
			.def("setClosed", &Procedural::Lathe::setClosed)
			.def("setCapped", &Procedural::Lathe::setCapped)
			.def("setShapeToExtrude", &Procedural::Lathe::setShapeToExtrude)
			.def("setMultiShapeToExtrude", &Procedural::Lathe::setMultiShapeToExtrude)
			.def("addToTriangleBuffer", &Procedural::Lathe::addToTriangleBuffer)
			];

		AddClassToCollection("Lathe", "class", "Performs a lathe operation on a procedural mesh.");

		module(lua)
			[
				class_<Procedural::MeshLinearTransform>("MeshLinearTransform")
				.def("modify", &Procedural::MeshLinearTransform::modify)
			.def("setTranslation", &Procedural::MeshLinearTransform::setTranslation)
			.def("setRotation", &Procedural::MeshLinearTransform::setRotation)
			];

		AddClassToCollection("MeshLinearTransform", "class", "Performs a linear transform on a procedural mesh.");

		module(lua)
			[
				class_<Procedural::MeshUVTransform>("MeshUVTransform")
				.def("setTile", &Procedural::MeshUVTransform::setTile)
			.def("setOrigin", &Procedural::MeshUVTransform::setOrigin)
			.def("setSwitchUV", &Procedural::MeshUVTransform::setSwitchUV)
			];

		AddClassToCollection("MeshUVTransform", "class", "Transforms UV on a procedural mesh.");

		module(lua)
			[
				class_<Procedural::SpherifyModifier>("SpherifyModifier")
				.def("setInputTriangleBuffer", &Procedural::SpherifyModifier::setInputTriangleBuffer)
			.def("setRadius", &Procedural::SpherifyModifier::setRadius)
			.def("setCenter", &Procedural::SpherifyModifier::setCenter)
			.def("modify", &Procedural::SpherifyModifier::modify)
			];

		AddClassToCollection("SpherifyModifier", "class", "Modifies a sphere on a procedural mesh.");

		module(lua)
			[
				class_<Procedural::CalculateNormalsModifier>("CalculateNormalsModifier")
				.enum_("NormalComputeMode")
			[
				value("NCM_VERTEX", Procedural::CalculateNormalsModifier::NCM_VERTEX),
				value("NCM_TRIANGLE", Procedural::CalculateNormalsModifier::NCM_TRIANGLE)
			]
		.def("setComputeMode", &Procedural::CalculateNormalsModifier::setComputeMode)
			.def("setInputTriangleBuffer", &Procedural::CalculateNormalsModifier::setInputTriangleBuffer)
			.def("setMustWeldUnweldFirst", &Procedural::CalculateNormalsModifier::setMustWeldUnweldFirst)
			.def("modify", &Procedural::CalculateNormalsModifier::modify)
			];

		AddClassToCollection("CalculateNormalsModifier", "class", "Calculates and modifies normals on a procedural mesh.");

		module(lua)
			[
				class_<Procedural::WeldVerticesModifier>("WeldVerticesModifier")
				.def("setInputTriangleBuffer", &Procedural::WeldVerticesModifier::setInputTriangleBuffer)
			.def("setTolerance", &Procedural::WeldVerticesModifier::setTolerance)
			.def("modify", &Procedural::WeldVerticesModifier::modify)
			];

		AddClassToCollection("WeldVerticesModifier", "class", "Welds vertices on a procedural mesh.");

		module(lua)
			[
				class_<Procedural::UnweldVerticesModifier>("UnweldVerticesModifier")
				.def("setInputTriangleBuffer", &Procedural::UnweldVerticesModifier::setInputTriangleBuffer)
			.def("modify", &Procedural::UnweldVerticesModifier::modify)
			];

		AddClassToCollection("UnweldVerticesModifier", "class", "Unwelds vertices on a procedural mesh.");

		module(lua)
			[
				class_<Procedural::PlaneUVModifier>("PlaneUVModifier")
				.def("setPlaneNormal", &Procedural::PlaneUVModifier::setPlaneNormal)
			.def("setInputTriangleBuffer", &Procedural::PlaneUVModifier::setInputTriangleBuffer)
			.def("setPlaneCenter", &Procedural::PlaneUVModifier::setPlaneCenter)
			.def("setPlaneSize", &Procedural::PlaneUVModifier::setPlaneSize)
			.def("modify", &Procedural::PlaneUVModifier::modify)
			];

		AddClassToCollection("PlaneUVModifier", "class", "Modifies UV on a procedural plane.");

		module(lua)
			[
				class_<Procedural::SphereUVModifier>("SphereUVModifier")
				.def("setInputTriangleBuffer", &Procedural::SphereUVModifier::setInputTriangleBuffer)
			.def("modify", &Procedural::SphereUVModifier::modify)
			];

		AddClassToCollection("SphereUVModifier", "class", "Modifies UV on a procedural sphere.");

		module(lua)
			[
				class_<Procedural::HemisphereUVModifier>("HemisphereUVModifier")
				.def("setInputTriangleBuffer", &Procedural::HemisphereUVModifier::setInputTriangleBuffer)
			.def("setTextureRectangleTop", &Procedural::HemisphereUVModifier::setTextureRectangleTop)
			.def("setTextureRectangleBottom", &Procedural::HemisphereUVModifier::setTextureRectangleBottom)
			.def("modify", &Procedural::HemisphereUVModifier::modify)
			];

		AddClassToCollection("HemisphereUVModifier", "class", "Modifies UV on a procedural hemisphere.");

		module(lua)
			[
				class_<Procedural::CylinderUVModifier>("CylinderUVModifier")
				.def("setInputTriangleBuffer", &Procedural::CylinderUVModifier::setInputTriangleBuffer)
			.def("setRadius", &Procedural::CylinderUVModifier::setRadius)
			.def("setHeight", &Procedural::CylinderUVModifier::setHeight)
			.def("modify", &Procedural::CylinderUVModifier::modify)
			];

		AddClassToCollection("CylinderUVModifier", "class", "Modifies UV on a procedural cylinder.");

		module(lua)
			[
				class_<Procedural::BoxUVModifier>("BoxUVModifier")
				.enum_("MappingType")
			[
				value("MT_FULL", Procedural::BoxUVModifier::MT_FULL),
				value("MT_CROSS", Procedural::BoxUVModifier::MT_CROSS),
				value("MT_PACKED", Procedural::BoxUVModifier::MT_PACKED)
			]
		.def("setInputTriangleBuffer", &Procedural::BoxUVModifier::setInputTriangleBuffer)
			.def("setBoxSize", &Procedural::BoxUVModifier::setBoxSize)
			.def("setBoxCenter", &Procedural::BoxUVModifier::setBoxCenter)
			.def("setMappingType", &Procedural::BoxUVModifier::setMappingType)
			.def("modify", &Procedural::BoxUVModifier::modify)
			];

		AddClassToCollection("BoxUVModifier", "class", "Modifies UV on a procedural box.");

		module(lua)
			[
				class_<Procedural::MultiShape>("MultiShape")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("addShape", &Procedural::MultiShape::addShape)
			.def("clear", &Procedural::MultiShape::clear)
			.def("getPoints", &Procedural::MultiShape::getPoints)
			.def("getShapeCount", &Procedural::MultiShape::getShapeCount)
			.def("addMultiShape", &Procedural::MultiShape::addMultiShape)
			.def("realizeMesh", &Procedural::MultiShape::realizeMesh)
			.def("isPointInside", &Procedural::MultiShape::isPointInside)
			.def("isClosed", &Procedural::MultiShape::isClosed)
			.def("close", &Procedural::MultiShape::close)
			.def("isOutsideRealOutside", &Procedural::MultiShape::isOutsideRealOutside)
			// .def("buildFromSegmentSoup", &Procedural::MultiShape::buildFromSegmentSoup)
			];

		AddClassToCollection("MultiShape", "class", "Combines procedural shapes.");

		/*module(lua)
		[
			class_<Procedural::TextShape>("TextShape")
			.def("setText", &Procedural::TextShape::setText)
			.def("setFont", &Procedural::TextShape::setFont)
			.def("realizeShapes", &Procedural::TextShape::realizeShapes)
			.def("modify", &Procedural::TextShape::modify)
		];*/

		//module(lua)
		//	[
		//		class_<Procedural::NoiseBase>("PerlinNoise") // abstract
		//		.def(constructor<unsigned int, Real, Real, Real>())
		//		.def("field1D", &Procedural::PerlinNoise::field1D)
		//		.def("field2D", &Procedural::PerlinNoise::field2D)
		//		.def("function1D", &Procedural::PerlinNoise::function1D)
		//		.def("function2D", &Procedural::PerlinNoise::function2D)
		//	];

		module(lua)
			[
				class_<Procedural::Path>("ProceduralPath")
				// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("addPoint", (Procedural::Path & (Procedural::Path::*)(const Vector3&)) & Procedural::Path::addPoint)
			.def("addPoint", (Procedural::Path & (Procedural::Path::*)(Real, Real, Real)) & Procedural::Path::addPoint)
			.def("insertPoint", (Procedural::Path & (Procedural::Path::*)(std::size_t, Real, Real, Real)) & Procedural::Path::insertPoint)
			.def("insertPoint", (Procedural::Path & (Procedural::Path::*)(std::size_t, const Vector3&)) & Procedural::Path::insertPoint)
			.def("appendPath", &Procedural::Path::appendPath)
			.def("appendPathRel", &Procedural::Path::appendPathRel)
			.def("reset", &Procedural::Path::reset)
			.def("close", &Procedural::Path::close)
			.def("isClosed", &Procedural::Path::isClosed)
			.def("getPoints", &Procedural::Path::getPoints)
			.def("getPoint", &Procedural::Path::getPoint)
			.def("getSegCount", &Procedural::Path::getSegCount)
			.def("getDirectionAfter", &Procedural::Path::getDirectionAfter)
			.def("getDirectionBefore", &Procedural::Path::getDirectionBefore)
			.def("getAvgDirection", &Procedural::Path::getAvgDirection)
			.def("getLengthAtPoint", &Procedural::Path::getLengthAtPoint)
			.def("getPosition", (Ogre::Vector3(Procedural::Path::*)(unsigned int, Real) const) & Procedural::Path::getPosition)
			.def("getPosition", (Ogre::Vector3(Procedural::Path::*)(Real) const) & Procedural::Path::getPosition)
			.def("realizeMesh", &Procedural::Path::realizeMesh)
			.def("mergeKeysWithTrack", &Procedural::Path::mergeKeysWithTrack)
			.def("translate", (Procedural::Path & (Procedural::Path::*)(const Vector3&)) & Procedural::Path::translate)
			.def("translate", (Procedural::Path & (Procedural::Path::*)(Real, Real, Real)) & Procedural::Path::translate)
			.def("scale", (Procedural::Path & (Procedural::Path::*)(const Vector3&)) & Procedural::Path::scale)
			.def("scale", (Procedural::Path & (Procedural::Path::*)(Real, Real, Real)) & Procedural::Path::scale)
			.def("scale", (Procedural::Path & (Procedural::Path::*)(Real)) & Procedural::Path::scale)
			.def("reflect", &Procedural::Path::reflect)
			.def("extractSubPath", &Procedural::Path::extractSubPath)
			.def("reverse", &Procedural::Path::reverse)
			.def("buildFromSegmentSoup", &Procedural::Path::buildFromSegmentSoup)
			.def("convertToShape", &Procedural::Path::convertToShape)
			];

		AddClassToCollection("ProceduralPath", "class", "Generates a procedural path.");

		module(lua)
			[
				class_<Procedural::MultiPath>("ProceduralMultiPath")
				.def("clear", &Procedural::MultiPath::clear)
			.def("addPath", &Procedural::MultiPath::addPath)
			.def("addMultiPath", &Procedural::MultiPath::addMultiPath)
			.def("setPath", &Procedural::MultiPath::setPath)
			.def("getPathCount", &Procedural::MultiPath::getPathCount)
			.def("getPath", &Procedural::MultiPath::getPath)
			.def("_calcIntersections", &Procedural::MultiPath::_calcIntersections)
			// .def("getIntersections", &Procedural::MultiPath::getIntersections)  // PathCoordinate struct must be announced
			.def("getNoIntersectionParts", &Procedural::MultiPath::getNoIntersectionParts)
			];

		AddClassToCollection("ProceduralMultiPath", "class", "Generates a procedural multi path.");

		// Here Ogre::SimpleSpline missing

		module(lua)
			[
				class_<Procedural::CatmullRomSpline3>("CatmullRomSpline3")
				.def(constructor<>())
			.def(constructor<SimpleSpline&>())
			.def("setNumSeg", &Procedural::CatmullRomSpline3::setNumSeg)
			.def("close", &Procedural::CatmullRomSpline3::close)

			.def("toSimpleSpline", &Procedural::CatmullRomSpline3::toSimpleSpline)
			.def("addPoint", (Procedural::CatmullRomSpline3 & (Procedural::CatmullRomSpline3::*)(const Vector3&)) & Procedural::CatmullRomSpline3::addPoint)
			.def("addPoint", (Procedural::CatmullRomSpline3 & (Procedural::CatmullRomSpline3::*)(Real, Real, Real)) & Procedural::CatmullRomSpline3::addPoint)
			.def("safeGetPoint", &Procedural::CatmullRomSpline3::safeGetPoint)
			.def("getPointCount", &Procedural::CatmullRomSpline3::getPointCount)
			.def("realizePath", &Procedural::CatmullRomSpline3::realizePath)
			];

		AddClassToCollection("CatmullRomSpline3", "class", "Generates a procedural catmull rom spline3.");

		module(lua)
			[
				class_<Procedural::CatmullRomSpline2>("CatmullRomSpline2")
				// .def(constructor<>())
				// .def(constructor<SimpleSpline&>())
			.def("setNumSeg", &Procedural::CatmullRomSpline2::setNumSeg)
			.def("close", &Procedural::CatmullRomSpline2::close)

			// .def("toSimpleSpline", &Procedural::CatmullRomSpline2::toSimpleSpline)
			.def("addPoint", (Procedural::CatmullRomSpline2 & (Procedural::CatmullRomSpline2::*)(const Vector2&)) & Procedural::CatmullRomSpline2::addPoint)
			.def("addPoint", (Procedural::CatmullRomSpline2 & (Procedural::CatmullRomSpline2::*)(Real, Real)) & Procedural::CatmullRomSpline2::addPoint)
			.def("safeGetPoint", &Procedural::CatmullRomSpline2::safeGetPoint)
			.def("getPointCount", &Procedural::CatmullRomSpline2::getPointCount)
			.def("realizeShape", &Procedural::CatmullRomSpline2::realizeShape)
			];

		AddClassToCollection("CatmullRomSpline2", "class", "Generates a procedural catmull rom spline2.");

		module(lua)
			[
				class_<Procedural::CubicHermiteSplineAutoTangentMode>("keys")
				.enum_("CubicHermiteSplineAutoTangentMode")
			[
				value("AT_NONE", Procedural::AT_NONE),
				value("AT_STRAIGHT", Procedural::AT_STRAIGHT),
				value("AT_CATMULL", Procedural::AT_CATMULL)
			]
			];

		module(lua)
			[
				class_<Procedural::CubicHermiteSpline2>("CubicHermiteSpline2")
				// .def(constructor<>())
				// .def(constructor<SimpleSpline&>())
			.def("setNumSeg", &Procedural::CubicHermiteSpline2::setNumSeg)
			.def("close", &Procedural::CubicHermiteSpline2::close)

			// .def("toSimpleSpline", &Procedural::CubicHermiteSpline2::toSimpleSpline)
			.def("addPoint", (Procedural::CubicHermiteSpline2 & (Procedural::CubicHermiteSpline2::*)(const Vector2&, const Vector2&, const Vector2&)) & Procedural::CubicHermiteSpline2::addPoint)
			.def("addPoint", (Procedural::CubicHermiteSpline2 & (Procedural::CubicHermiteSpline2::*)(const Vector2&, Procedural::CubicHermiteSplineAutoTangentMode)) & Procedural::CubicHermiteSpline2::addPoint)
			.def("addPoint", (Procedural::CubicHermiteSpline2 & (Procedural::CubicHermiteSpline2::*)(Real, Real, Procedural::CubicHermiteSplineAutoTangentMode)) & Procedural::CubicHermiteSpline2::addPoint)
			.def("safeGetPoint", &Procedural::CubicHermiteSpline2::safeGetPoint)
			.def("getPointCount", &Procedural::CubicHermiteSpline2::getPointCount)
			.def("realizeShape", &Procedural::CubicHermiteSpline2::realizeShape)
			];

		AddClassToCollection("CubicHermiteSpline2", "class", "Generates a procedural cubic hermite spline2.");

		module(lua)
			[
				class_<Procedural::KochanekBartelsSpline2>("KochanekBartelsSpline2")
				// .def(constructor<>())
				// .def(constructor<SimpleSpline&>())
			.def("setNumSeg", &Procedural::KochanekBartelsSpline2::setNumSeg)
			.def("close", &Procedural::KochanekBartelsSpline2::close)

			// .def("toSimpleSpline", &Procedural::KochanekBartelsSpline2::toSimpleSpline)
			.def("addPoint", (Procedural::KochanekBartelsSpline2 & (Procedural::KochanekBartelsSpline2::*)(Real, Real)) & Procedural::KochanekBartelsSpline2::addPoint)
			.def("addPoint", (Procedural::KochanekBartelsSpline2 & (Procedural::KochanekBartelsSpline2::*)(Vector2)) & Procedural::KochanekBartelsSpline2::addPoint)
			.def("addPoint", (Procedural::KochanekBartelsSpline2 & (Procedural::KochanekBartelsSpline2::*)(Vector2, Real, Real, Real)) & Procedural::KochanekBartelsSpline2::addPoint)
			.def("safeGetPoint", &Procedural::KochanekBartelsSpline2::safeGetPoint)
			.def("getPointCount", &Procedural::KochanekBartelsSpline2::getPointCount)
			.def("realizeShape", &Procedural::KochanekBartelsSpline2::realizeShape)
			];

		AddClassToCollection("KochanekBartelsSpline2", "class", "Generates a procedural kochanek bartels spline2.");

		module(lua)
			[
				class_<Procedural::CubicHermiteSpline3>("CubicHermiteSpline3")
				// .def(constructor<>())
				// .def(constructor<SimpleSpline&>())
			.def("setNumSeg", &Procedural::CubicHermiteSpline3::setNumSeg)
			.def("close", &Procedural::CubicHermiteSpline3::close)

			// .def("toSimpleSpline", &Procedural::CubicHermiteSpline3::toSimpleSpline)
			.def("addPoint", (Procedural::CubicHermiteSpline3 & (Procedural::CubicHermiteSpline3::*)(const Vector3&, const Vector3&, const Vector3&)) & Procedural::CubicHermiteSpline3::addPoint)
			.def("addPoint", (Procedural::CubicHermiteSpline3 & (Procedural::CubicHermiteSpline3::*)(const Vector3&, Procedural::CubicHermiteSplineAutoTangentMode)) & Procedural::CubicHermiteSpline3::addPoint)
			.def("addPoint", (Procedural::CubicHermiteSpline3 & (Procedural::CubicHermiteSpline3::*)(Real, Real, Real, Procedural::CubicHermiteSplineAutoTangentMode)) & Procedural::CubicHermiteSpline3::addPoint)
			.def("safeGetPoint", &Procedural::CubicHermiteSpline3::safeGetPoint)
			.def("getPointCount", &Procedural::CubicHermiteSpline3::getPointCount)
			.def("realizePath", &Procedural::CubicHermiteSpline3::realizePath)
			];

		AddClassToCollection("CubicHermiteSpline3", "class", "Generates a procedural cubic ermite spline3.");

		module(lua)
			[
				class_<Procedural::LinePath>("LinePath")
				.def("setPoint1", &Procedural::LinePath::setPoint1)
			.def("setPoint2", &Procedural::LinePath::setPoint2)

			.def("setNumSeg", &Procedural::LinePath::setNumSeg)
			.def("betweenPoints", &Procedural::LinePath::betweenPoints)
			.def("realizePath", &Procedural::LinePath::realizePath)
			];

		AddClassToCollection("LinePath", "class", "Generates a procedural line path.");

		module(lua)
			[
				class_<Procedural::RoundedCornerSpline2>("RoundedCornerSpline2")
				// .def(constructor<>())
				// .def(constructor<SimpleSpline&>())
			.def("setNumSeg", &Procedural::RoundedCornerSpline2::setNumSeg)
			.def("close", &Procedural::RoundedCornerSpline2::close)

			.def("setRadius", &Procedural::RoundedCornerSpline2::setRadius)
			.def("addPoint", (Procedural::RoundedCornerSpline2 & (Procedural::RoundedCornerSpline2::*)(const Vector2&)) & Procedural::RoundedCornerSpline2::addPoint)
			.def("addPoint", (Procedural::RoundedCornerSpline2 & (Procedural::RoundedCornerSpline2::*)(Real, Real)) & Procedural::RoundedCornerSpline2::addPoint)
			.def("safeGetPoint", &Procedural::RoundedCornerSpline2::safeGetPoint)
			.def("getPointCount", &Procedural::RoundedCornerSpline2::getPointCount)
			.def("realizeShape", &Procedural::RoundedCornerSpline2::realizeShape)
			];

		AddClassToCollection("RoundedCornerSpline2", "class", "Generates a procedural rounded corner spline2.");

		module(lua)
			[
				class_<Procedural::RoundedCornerSpline3>("RoundedCornerSpline3")
				// .def(constructor<>())
				// .def(constructor<SimpleSpline&>())
			.def("setNumSeg", &Procedural::RoundedCornerSpline3::setNumSeg)
			.def("close", &Procedural::RoundedCornerSpline3::close)

			.def("setRadius", &Procedural::RoundedCornerSpline3::setRadius)
			.def("addPoint", (Procedural::RoundedCornerSpline3 & (Procedural::RoundedCornerSpline3::*)(const Vector3&)) & Procedural::RoundedCornerSpline3::addPoint)
			.def("addPoint", (Procedural::RoundedCornerSpline3 & (Procedural::RoundedCornerSpline3::*)(Real, Real, Real)) & Procedural::RoundedCornerSpline3::addPoint)
			.def("safeGetPoint", &Procedural::RoundedCornerSpline3::safeGetPoint)
			.def("getPointCount", &Procedural::RoundedCornerSpline3::getPointCount)
			.def("realizePath", &Procedural::RoundedCornerSpline3::realizePath)
			];

		AddClassToCollection("RoundedCornerSpline3", "class", "Generates a procedural rounded corner spline3.");

		module(lua)
			[
				class_<Procedural::BezierCurve2>("BezierCurve2")
				// .def(constructor<>())
				// .def(constructor<SimpleSpline&>())
			.def("setNumSeg", &Procedural::BezierCurve2::setNumSeg)
			.def("close", &Procedural::BezierCurve2::close)

			.def("addPoint", (Procedural::BezierCurve2 & (Procedural::BezierCurve2::*)(const Vector2&)) & Procedural::BezierCurve2::addPoint)
			.def("addPoint", (Procedural::BezierCurve2 & (Procedural::BezierCurve2::*)(Real, Real)) & Procedural::BezierCurve2::addPoint)
			.def("safeGetPoint", &Procedural::BezierCurve2::safeGetPoint)
			.def("getPointCount", &Procedural::BezierCurve2::getPointCount)
			.def("realizeShape", &Procedural::BezierCurve2::realizeShape)
			];

		AddClassToCollection("BezierCurve2", "class", "Generates a procedural bezier curve spline2.");

		module(lua)
			[
				class_<Procedural::BezierCurve3>("BezierCurve3")
				// .def(constructor<>())
				// .def(constructor<SimpleSpline&>())
			.def("setNumSeg", &Procedural::BezierCurve3::setNumSeg)
			.def("close", &Procedural::BezierCurve3::close)

			.def("addPoint", (Procedural::BezierCurve3 & (Procedural::BezierCurve3::*)(const Vector3&)) & Procedural::BezierCurve3::addPoint)
			.def("addPoint", (Procedural::BezierCurve3 & (Procedural::BezierCurve3::*)(Real, Real, Real)) & Procedural::BezierCurve3::addPoint)
			.def("safeGetPoint", &Procedural::BezierCurve3::safeGetPoint)
			.def("getPointCount", &Procedural::BezierCurve3::getPointCount)
			.def("realizePath", &Procedural::BezierCurve3::realizePath)
			];

		AddClassToCollection("BezierCurve3", "class", "Generates a procedural bezier curve spline3.");

		module(lua)
			[
				class_<Procedural::HelixPath>("HelixPath")
				.def("setHeight", &Procedural::HelixPath::setHeight)
			.def("setRadius", &Procedural::HelixPath::setRadius)

			.def("setNumRound", &Procedural::HelixPath::setNumRound)
			.def("setNumSegPath", &Procedural::HelixPath::setNumSegPath)
			.def("realizePath", &Procedural::HelixPath::realizePath)
			];

		AddClassToCollection("HelixPath", "class", "Generates a procedural helix path.");

		module(lua)
			[
				class_<Procedural::SvgLoader>("SvgLoader")
				// Attention: May not work because of Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME const, define it!
							// void parseSvgFile(MultiShape& out, const Ogre::String& fileName, const Ogre::String& groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, int segmentsNumber = 8);
			.def("parseSvgFile", &Procedural::SvgLoader::parseSvgFile)

			];

		AddClassToCollection("SvgLoader", "class", "Generates a procedural from svg file.");

		module(lua)
			[
				class_<Procedural::Shape>("Shape")
				.enum_("Side")
			[
				value("SIDE_LEFT", Procedural::SIDE_LEFT),
				value("SIDE_RIGHT", Procedural::SIDE_RIGHT)
			]
		// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
		.def("addPoint", (Procedural::Shape & (Procedural::Shape::*)(const Vector2&)) & Procedural::Shape::addPoint)
			.def("addPoint", (Procedural::Shape & (Procedural::Shape::*)(Real, Real)) & Procedural::Shape::addPoint)
			.def("insertPoint", (Procedural::Shape & (Procedural::Shape::*)(std::size_t, Real, Real)) & Procedural::Shape::insertPoint)
			.def("insertPoint", (Procedural::Shape & (Procedural::Shape::*)(std::size_t, const Vector2&)) & Procedural::Shape::insertPoint)
			.def("addPointRel", (Procedural::Shape & (Procedural::Shape::*)(const Vector2&)) & Procedural::Shape::addPointRel)
			.def("addPointRel", (Procedural::Shape & (Procedural::Shape::*)(Real, Real)) & Procedural::Shape::addPointRel)
			.def("appendShape", &Procedural::Shape::appendShape)
			.def("appendShapeRel", &Procedural::Shape::appendShapeRel)
			.def("extractSubShape", &Procedural::Shape::extractSubShape)
			.def("reverse", &Procedural::Shape::reverse)
			.def("reset", &Procedural::Shape::reset)
			.def("convertToPath", &Procedural::Shape::convertToPath)
			.def("convertToTrack", &Procedural::Shape::convertToTrack)
			.def("getPoints", &Procedural::Shape::getPoints)
			// .def("getPointsReference", &Procedural::Shape::getPointsReference)
			.def("getPoint", &Procedural::Shape::getPoint)
			.def("getBoundedIndex", &Procedural::Shape::getBoundedIndex)
			.def("getPointCount", &Procedural::Shape::getPointCount)
			.def("close", &Procedural::Shape::close)
			.def("setOutSide", &Procedural::Shape::setOutSide)
			.def("getOutSide", &Procedural::Shape::getOutSide)
			.def("switchSide", &Procedural::Shape::switchSide)
			.def("getSegCount", &Procedural::Shape::getSegCount)
			.def("getDirectionAfter", &Procedural::Shape::getDirectionAfter)
			.def("getDirectionBefore", &Procedural::Shape::getDirectionBefore)
			.def("getAvgDirection", &Procedural::Shape::getAvgDirection)
			.def("getNormalAfter", &Procedural::Shape::getNormalAfter)
			.def("getNormalBefore", &Procedural::Shape::getNormalBefore)
			.def("getAvgNormal", &Procedural::Shape::getAvgNormal)
			.def("realizeMesh", &Procedural::Shape::realizeMesh)
			// .def("_appendToManualObject", &Procedural::Shape::_appendToManualObject)
			.def("booleanIntersect", &Procedural::Shape::booleanIntersect)
			.def("booleanUnion", &Procedural::Shape::booleanUnion)
			.def("booleanDifference", &Procedural::Shape::booleanDifference)
			.def("findRealOutSide", &Procedural::Shape::findRealOutSide)
			.def("isOutsideRealOutside", &Procedural::Shape::isOutsideRealOutside)
			.def("mergeKeysWithTrack", &Procedural::Shape::mergeKeysWithTrack)
			.def("translate", (Procedural::Shape & (Procedural::Shape::*)(const Vector2&)) & Procedural::Shape::translate)
			.def("translate", (Procedural::Shape & (Procedural::Shape::*)(Real, Real)) & Procedural::Shape::translate)
			.def("rotate", &Procedural::Shape::rotate)
			.def("scale", (Procedural::Shape & (Procedural::Shape::*)(const Vector2&)) & Procedural::Shape::scale)
			.def("scale", (Procedural::Shape & (Procedural::Shape::*)(Real, Real)) & Procedural::Shape::scale)
			.def("reflect", &Procedural::Shape::reflect)
			.def("mirror", (Procedural::Shape & (Procedural::Shape::*)(bool)) & Procedural::Shape::mirror)
			.def("mirror", (Procedural::Shape & (Procedural::Shape::*)(Real, Real, bool)) & Procedural::Shape::mirror)
			.def("mirrorAroundPoint", &Procedural::Shape::mirrorAroundPoint)
			.def("mirrorAroundAxis", &Procedural::Shape::mirrorAroundAxis)
			.def("getTotalLength", &Procedural::Shape::getTotalLength)
			.def("getPosition", (Ogre::Vector2(Procedural::Shape::*)(unsigned int, Real) const) & Procedural::Shape::getPosition)
			.def("getPosition", (Ogre::Vector2(Procedural::Shape::*)(Real) const) & Procedural::Shape::getPosition)
			.def("findBoundingRadius", &Procedural::Shape::findBoundingRadius)
			.def("thicken", &Procedural::Shape::thicken)
			];

		AddClassToCollection("Shape", "class", "Generates a custom procedural shape mesh.");

		module(lua)
			[
				class_<Procedural::RectangleShape>("RectangleShape")
				.def("setWidth", &Procedural::RectangleShape::setWidth)
			.def("setHeight", &Procedural::RectangleShape::setHeight)
			.def("realizeShape", &Procedural::RectangleShape::realizeShape)
			];

		AddClassToCollection("RectangleShape", "class", "Generates a procedural rectangle shape mesh.");

		module(lua)
			[
				class_<Procedural::CircleShape>("CircleShape")
				.def("setRadius", &Procedural::CircleShape::setRadius)
			.def("setNumSeg", &Procedural::CircleShape::setNumSeg)
			.def("realizeShape", &Procedural::CircleShape::realizeShape)
			];

		AddClassToCollection("CircleShape", "class", "Generates a procedural circle shape mesh.");

		module(lua)
			[
				class_<Procedural::EllipseShape>("EllipseShape")
				.def("setRadiusX", &Procedural::EllipseShape::setRadiusX)
			.def("setRadiusY", &Procedural::EllipseShape::setRadiusY)
			.def("setNumSeg", &Procedural::EllipseShape::setNumSeg)
			.def("realizeShape", &Procedural::EllipseShape::realizeShape)
			];

		AddClassToCollection("EllipseShape", "class", "Generates a procedural ellipse shape mesh.");

		module(lua)
			[
				class_<Procedural::TriangleShape>("TriangleShape")
				.def("setLength", &Procedural::TriangleShape::setLength)
			.def("setLengthA", &Procedural::TriangleShape::setLengthA)
			.def("setLengthB", &Procedural::TriangleShape::setLengthB)
			.def("setLengthB", &Procedural::TriangleShape::setLengthC)
			.def("realizeShape", &Procedural::TriangleShape::realizeShape)
			];

		AddClassToCollection("TriangleShape", "class", "Generates a procedural triangle shape mesh.");

		module(lua)
			[
				class_<Procedural::TextureBuffer>("TextureBuffer")
				.def("setPixel", (void (Procedural::TextureBuffer::*)(size_t, size_t, Ogre::ColourValue)) & Procedural::TextureBuffer::setPixel)
			.def("setPixel", (void (Procedural::TextureBuffer::*)(size_t, size_t, unsigned char, unsigned char, unsigned char, unsigned char)) & Procedural::TextureBuffer::setPixel)
			.def("setPixel", (void (Procedural::TextureBuffer::*)(size_t, size_t, Real, Real, Real, Real)) & Procedural::TextureBuffer::setPixel)
			.def("setRed", (void (Procedural::TextureBuffer::*)(size_t, size_t, unsigned char)) & Procedural::TextureBuffer::setRed)
			.def("setRed", (void (Procedural::TextureBuffer::*)(size_t, size_t, Real)) & Procedural::TextureBuffer::setRed)
			.def("setGreen", (void (Procedural::TextureBuffer::*)(size_t, size_t, unsigned char)) & Procedural::TextureBuffer::setGreen)
			.def("setGreen", (void (Procedural::TextureBuffer::*)(size_t, size_t, Real)) & Procedural::TextureBuffer::setGreen)
			.def("setBlue", (void (Procedural::TextureBuffer::*)(size_t, size_t, unsigned char)) & Procedural::TextureBuffer::setBlue)
			.def("setBlue", (void (Procedural::TextureBuffer::*)(size_t, size_t, Real)) & Procedural::TextureBuffer::setBlue)
			.def("setAlpha", (void (Procedural::TextureBuffer::*)(size_t, size_t, unsigned char)) & Procedural::TextureBuffer::setAlpha)
			.def("setAlpha", (void (Procedural::TextureBuffer::*)(size_t, size_t, Real)) & Procedural::TextureBuffer::setAlpha)
			.def("setData", (void (Procedural::TextureBuffer::*)(size_t, size_t, unsigned char*)) & Procedural::TextureBuffer::setData)

			.def("getPixel", &Procedural::TextureBuffer::getPixel)
			.def("getPixelRedByte", &Procedural::TextureBuffer::getPixelRedByte)
			.def("getPixelGreenByte", &Procedural::TextureBuffer::getPixelGreenByte)
			.def("getPixelBlueByte", &Procedural::TextureBuffer::getPixelBlueByte)
			.def("getPixelAlphaByte", &Procedural::TextureBuffer::getPixelAlphaByte)
			.def("getPixelRedReal", &Procedural::TextureBuffer::getPixelRedReal)
			.def("getPixelGreenReal", &Procedural::TextureBuffer::getPixelGreenReal)
			.def("getPixelBlueReal", &Procedural::TextureBuffer::getPixelBlueReal)
			.def("getPixelAlphaReal", &Procedural::TextureBuffer::getPixelAlphaReal)

			// // .def("clone", &Procedural::TextureBuffer::clone) will not work maybe because of ptr, use anonym function to get raw pointer out of ptr
			.def("getWidth", &Procedural::TextureBuffer::getWidth)
			.def("getHeight", &Procedural::TextureBuffer::getHeight)
			// .def("getImage", &Procedural::TextureBuffer::getImage) Ogre::Image not announced, necessary?
			.def("saveImage", &Procedural::TextureBuffer::saveImage)
			// .def("createTexture", &Procedural::TextureBuffer::createTexture) Ogre::TexturePtr missing and ptr must be converted to raw pointer, necessary?
			];

		AddClassToCollection("TextureBuffer", "class", "Textures a procedural mesh.");

		// Attention: From ProceduralTextureGenerator.h nothing will work at the moment, because how to set the constructor with TextureBufferPtr? TextureBuffer must be announced and how to get a raw pointer here??
		// Same for ProceduralTextureModifiers.h
		/*module(lua)
		[
			class_<Procedural::Cell>("Cell")
			.def(constructor<TextureBufferPtr)
// Attention: Is that correct?
			.enum_("CELL_MODE")
			[
				value("MODE_GRID", MODE_GRID),
				value("MODE_CHESSBOARD", MODE_CHESSBOARD)
			]
			.enum_("CELL_PATTERN")
			[
				value("PATTERN_BOTH", PATTERN_BOTH),
				value("PATTERN_CROSS", PATTERN_CROSS),
				value("PATTERN_CONE", PATTERN_CONE)
			]
			.def("setInputTriangleBuffer", &Procedural::Cell::setInputTriangleBuffer)
			.def("setBoxSize", &Procedural::Cell::setBoxSize)
			.def("setBoxCenter", &Procedural::Cell::setBoxCenter)
			.def("setMappingType", &Procedural::Cell::setMappingType)
			.def("modify", &Procedural::Cell::modify)
		];*/

		module(lua)
			[
				class_<Procedural::TriangleBuffer>("TriangleBuffer")
				.def("append", &Procedural::TriangleBuffer::append)
			.def("beginSection", &Procedural::TriangleBuffer::beginSection)
			.def("endSection", &Procedural::TriangleBuffer::endSection)
			.def("getFullSection", &Procedural::TriangleBuffer::getFullSection)
			.def("rebaseOffset", &Procedural::TriangleBuffer::rebaseOffset)
			.def("transformToMesh", &Procedural::TriangleBuffer::transformToMesh)
			// Attention: Ogre::Vertex must be announced!
						// .def("vertex", (TriangleBuffer& (Procedural::TriangleBuffer::*)(const Vertex&)) &Procedural::TriangleBuffer::vertex)
						// .def("vertex", (TriangleBuffer& (Procedural::TriangleBuffer::*)(const Vector3&, const Vector3&, const Vector2&)) &Procedural::TriangleBuffer::vertex)
			.def("position", (Procedural::TriangleBuffer & (Procedural::TriangleBuffer::*)(const Vector3&)) & Procedural::TriangleBuffer::position)
			.def("position", (Procedural::TriangleBuffer & (Procedural::TriangleBuffer::*)(Real, Real, Real)) & Procedural::TriangleBuffer::position)

			.def("normal", &Procedural::TriangleBuffer::normal)
			.def("index", &Procedural::TriangleBuffer::index)
			.def("triangle", &Procedural::TriangleBuffer::triangle)
			.def("applyTransform", &Procedural::TriangleBuffer::applyTransform)
			.def("translate", (Procedural::TriangleBuffer & (Procedural::TriangleBuffer::*)(const Vector3&)) & Procedural::TriangleBuffer::translate)
			.def("translate", (Procedural::TriangleBuffer & (Procedural::TriangleBuffer::*)(Real, Real, Real)) & Procedural::TriangleBuffer::translate)
			.def("rotate", &Procedural::TriangleBuffer::rotate)
			.def("scale", (Procedural::TriangleBuffer & (Procedural::TriangleBuffer::*)(const Vector3&)) & Procedural::TriangleBuffer::scale)
			.def("scale", (Procedural::TriangleBuffer & (Procedural::TriangleBuffer::*)(Real, Real, Real)) & Procedural::TriangleBuffer::scale)
			.def("invertNormals", &Procedural::TriangleBuffer::invertNormals)
			.def("estimateVertexCount", &Procedural::TriangleBuffer::estimateVertexCount)
			.def("estimateIndexCount", &Procedural::TriangleBuffer::estimateIndexCount)
			];

		AddClassToCollection("TriangleBuffer", "class", "Triangulates a procedural mesh.");

		module(lua)
			[
				class_<Procedural::Triangulator>("Triangulator")
				.def("setShapeToTriangulate", &Procedural::Triangulator::setShapeToTriangulate)
			.def("setMultiShapeToTriangulate", &Procedural::Triangulator::setMultiShapeToTriangulate)

			.def("setSegmentListToTriangulate", &Procedural::Triangulator::setSegmentListToTriangulate)
			.def("setManualSuperTriangle", &Procedural::Triangulator::setManualSuperTriangle)
			.def("setRemoveOutside", &Procedural::Triangulator::setRemoveOutside)
			.def("triangulate", &Procedural::Triangulator::triangulate)
			.def("addToTriangleBuffer", &Procedural::Triangulator::addToTriangleBuffer)
			];

		AddClassToCollection("Triangulator", "class", "Triangulator manages trianulgation operations on a procedural mesh.");
	}

	void bindOgreNewt(lua_State* lua)
	{
		/*module(lua)
		[
			class_<OgreNewt::Body>("Body")
			// .def(constructor<const OgreNewt::World*, const OgreNewt::CollisionPtr&, Ogre::SceneMemoryMgrTypes memoryType, unsigned int>())
			.def("getOgreNode", &OgreNewt::Body::getOgreNode)
			.def("getWorld", &OgreNewt::Body::getWorld)
			// .def("setType", &OgreNewt::Body::setType)
			.def("getType", &OgreNewt::Body::getType)
			.def("attachNode", &OgreNewt::Body::attachNode)
			// .def("setStandardForceCallback", &OgreNewt::Body::setStandardForceCallback)
			.def("setPositionOrientation", &OgreNewt::Body::setPositionOrientation)
			.def("setMassMatrix", &OgreNewt::Body::setMassMatrix)
			.def("setCenterOfMass", &OgreNewt::Body::setCenterOfMass)
			.def("getCenterOfMass", &OgreNewt::Body::getCenterOfMass)
			.def("freeze", &OgreNewt::Body::freeze)
			.def("unFreeze", &OgreNewt::Body::unFreeze)
			.def("isFreezed", &OgreNewt::Body::isFreezed)
			.def("setMaterialGroupID", &OgreNewt::Body::setMaterialGroupID)
			.def("setContinuousCollisionMode", &OgreNewt::Body::setContinuousCollisionMode)
			.def("setJointRecursiveCollision", &OgreNewt::Body::setJointRecursiveCollision)
			.def("getNext", &OgreNewt::Body::getNext)
			.def("setOmega", &OgreNewt::Body::setOmega)
			.def("setVelocity", &OgreNewt::Body::setVelocity)
			.def("setLinearDamping", &OgreNewt::Body::setLinearDamping)
			.def("setAngularDamping", &OgreNewt::Body::setAngularDamping)
			.def("setAutoSleep", &OgreNewt::Body::setAutoSleep)
			.def("getAutoSleep", &OgreNewt::Body::getAutoSleep)
			// .def("getCollision", &OgreNewt::Body::getCollision)
			.def("getMaterialGroupID", &OgreNewt::Body::getMaterialGroupID)
			.def("getContinuousCollisionMode", &OgreNewt::Body::getContinuousCollisionMode)
			.def("getJointRecursiveCollision", &OgreNewt::Body::getJointRecursiveCollision)
			.def("getPositionOrientation", &OgreNewt::Body::getPositionOrientation)
			.def("getPosition", &OgreNewt::Body::getPosition)
			.def("getOrientation", &OgreNewt::Body::getOrientation)
			.def("getVisualPositionOrientation", &OgreNewt::Body::getVisualPositionOrientation)
			.def("getVisualPosition", &OgreNewt::Body::getVisualPosition)
			.def("getVisualOrientation", &OgreNewt::Body::getVisualOrientation)
			.def("getAABB", &OgreNewt::Body::getAABB)
			.def("getMassMatrix", &OgreNewt::Body::getMassMatrix)
			.def("getMass", &OgreNewt::Body::getMass)
			.def("getInertia", &OgreNewt::Body::getInertia)
			.def("getInvMass", &OgreNewt::Body::getVisualPositionOrientation)
			.def("getOmega", &OgreNewt::Body::getOmega)
			.def("getVelocity", &OgreNewt::Body::getVelocity)
			.def("getForce", &OgreNewt::Body::getForce)
			.def("getTorque", &OgreNewt::Body::getTorque)
			.def("getLinearDamping", &OgreNewt::Body::getLinearDamping)
			.def("getAngularDamping", &OgreNewt::Body::getAngularDamping)
			.def("calculateInverseDynamicsForce", &OgreNewt::Body::calculateInverseDynamicsForce)
			.def("addImpulse", &OgreNewt::Body::addImpulse)
		];

		module(lua)
		[
			class_<OgreNewt::MaterialID>("MaterialID")
			.def("getID", &OgreNewt::MaterialID::getID)
		];

		module(lua)
		[
			class_<OgreNewt::World>("World")
			.def("getDefaultMaterialID", &OgreNewt::World::getDefaultMaterialID)
			.def("getBodyCount", &OgreNewt::World::getBodyCount)
			.def("getConstraintCount", &OgreNewt::World::getConstraintCount)
			.def("getFirstBody", &OgreNewt::World::getFirstBody)
		];

		module(lua)
		[
			class_<OgreNewt::Contact>("Contact")
			.def("getFaceAttribute", &OgreNewt::Contact::getFaceAttribute)
			.def("getNormalSpeed", &OgreNewt::Contact::getNormalSpeed)
			.def("getForce", &OgreNewt::Contact::getForce)
			.def("getPositionAndNormal", &OgreNewt::Contact::getPositionAndNormal)
			.def("getTangentDirections", &OgreNewt::Contact::getTangentDirections)
			.def("getTangentSpeed", &OgreNewt::Contact::getTangentSpeed)
			.def("setSoftness", &OgreNewt::Contact::setSoftness)
			.def("setElasticity", &OgreNewt::Contact::setElasticity)
			.def("setFrictionState", &OgreNewt::Contact::setFrictionState)
			.def("setFrictionCoef", &OgreNewt::Contact::setFrictionCoef)
			.def("setTangentAcceleration", &OgreNewt::Contact::setTangentAcceleration)
			.def("rotateTangentDirections", &OgreNewt::Contact::rotateTangentDirections)
			.def("setNormalDirection", &OgreNewt::Contact::setNormalDirection)
			.def("setNormalAcceleration", &OgreNewt::Contact::setNormalAcceleration)
			.def("remove", &OgreNewt::Contact::remove)
		];*/

		// MaterialID class to be done
		// OgreNewt World to be done
		// CollisionPtr
	}

	JointComponent* getPredecessorJointComponent(JointComponent* instance)
	{
		auto& jointCompPtr = NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(instance->getId()));
		if (nullptr != jointCompPtr)
		{
			return jointCompPtr.get();
		}
		else
		{
			currentErrorGameObject = Ogre::StringConverter::toString(instance->getId());
			currentCalledFunction = "getPredecessorJointComponent";

			return nullptr;
		}
	}

	KI::MovingBehavior* getMovingBehavior(AiComponent* instance)
	{
		return makeStrongPtr(instance->getMovingBehavior()).get();
	}

	void setTargetIdAiMove(AiMoveComponent* instance, const Ogre::String& targetId)
	{
		instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(targetId));
	}

	Ogre::String getTargetIdAiMove(AiMoveComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getTargetId());
	}

	void setWaypointId(AiPathFollowComponent* instance, unsigned int index, const Ogre::String& waypointId)
	{
		instance->setWaypointId(index, Ogre::StringConverter::parseUnsignedLong(waypointId));
	}

	void addWaypointId(AiPathFollowComponent* instance, const Ogre::String& waypointId)
	{
		instance->addWaypointId(Ogre::StringConverter::parseUnsignedLong(waypointId));
	}

	Ogre::String getWaypointId(AiPathFollowComponent* instance, unsigned int index)
	{
		return Ogre::StringConverter::toString(instance->getWaypointId(index));
	}

	void setWaypointId2(AiPathFollowComponent2D* instance, unsigned int index, const Ogre::String& waypointId)
	{
		instance->setWaypointId(index, Ogre::StringConverter::parseUnsignedLong(waypointId));
	}

	void addWaypointId2(AiPathFollowComponent2D* instance, const Ogre::String& waypointId)
	{
		instance->addWaypointId(Ogre::StringConverter::parseUnsignedLong(waypointId));
	}

	Ogre::String getWaypointId2(AiPathFollowComponent2D* instance, unsigned int index)
	{
		return Ogre::StringConverter::toString(instance->getWaypointId(index));
	}

	void bindAiComponents(lua_State* lua)
	{
		module(lua)
		[
			class_<AiComponent, GameObjectComponent>("AiComponent")
			// .def("getClassName", &AiComponent::getClassName)
			.def("getParentClassName", &AiComponent::getParentClassName)
			.def("setActivated", &AiComponent::setActivated)
			.def("isActivated", &AiComponent::isActivated)
			.def("setRotationSpeed", &AiComponent::setRotationSpeed)
			.def("getRotationSpeed", &AiComponent::getRotationSpeed)
			.def("setFlyMode", &AiComponent::setFlyMode)
			.def("getFlyMode", &AiComponent::getIsInFlyMode)
			.def("getMovingBehavior", &getMovingBehavior)
			.def("reactOnPathGoalReached", &AiComponent::reactOnPathGoalReached)
			.def("reactOnAgentStuck", &AiComponent::reactOnAgentStuck)
			.def("setAutoOrientation", &AiComponent::setAutoOrientation)
			.def("setAutoAnimation", &AiComponent::setAutoAnimation)
		];

		AddClassToCollection("AiComponent", "class inherits GameObjectComponent", "Base class for ai components with some attributes. Note: The game Object must have a PhysicsActiveComponent.");
		// AddClassToCollection("AiComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("AiComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("AiComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("AiComponent", "void setActivated(bool activated)", "Sets the ai component is activated. If true, it will move according to its specified behavior.");
		AddClassToCollection("AiComponent", "bool isActivated()", "Gets whether ai component is activated. If true, it will move according to its specified behavior.");
		AddClassToCollection("AiComponent", "void setDefaultDirection(Vector3 direction)", "Sets the default mesh direction. According this direction, all ai behavior will move.");
		AddClassToCollection("AiComponent", "void setRotationSpeed(float rotationSpeed)", "Sets the rotation speed for the ai controller game object.");
		AddClassToCollection("AiComponent", "float getRotationSpeed()", "Gets the rotation speed for the ai controller game object.");
		AddClassToCollection("AiComponent", "void setFlyMode(bool flyMode)", "Sets the ai controller game object should be clamped at the floor or can fly.");
		AddClassToCollection("AiComponent", "float getIsInFlyMode()", "Gets whether the ai controller game object should is clamped at the floor or can fly.");
		AddClassToCollection("AiComponent", "MovingBehavior getMovingBehavior()", "Gets the moving behavior for direct ai manipulation.");
		AddClassToCollection("AiComponent", "void reactOnPathGoalReached(func closureFunction)",
			"Sets whether to react the agent reached the goal.");
		AddClassToCollection("AiComponent", "void reactOnAgentStuck(func closureFunction)",
			"Sets whether to react the agent got stuck.");
		AddClassToCollection("AiComponent", "void setAutoOrientation(bool autoOrientation)", "Sets whether the agent should be auto orientated during ai movement.");
		AddClassToCollection("AiComponent", "void setAutoAnimation(bool autoAnimation)", "Sets whether to use auto animation during ai movement. That is, the animation speed is adapted dynamically depending the velocity, which will create a much more realistic effect. Note: The game object must have a proper configured animation component.");


		module(lua)
			[
				class_<AiMoveComponent, AiComponent>("AiMoveComponent")
				// .def("clone", &AiMoveComponent::clone)
			.def("setBehaviorType", &AiMoveComponent::setBehaviorType)
			.def("getBehaviorType", &AiMoveComponent::getBehaviorType)
			// .def("setTargetId", &AiMoveComponent::setTargetId)
			// .def("getTargetId", &AiMoveComponent::getTargetId)
			.def("setTargetId", &setTargetIdAiMove)
			.def("getTargetId", &getTargetIdAiMove)
			];

		AddClassToCollection("AiMoveComponent", "class inherits AiComponent", AiMoveComponent::getStaticInfoText());
		AddClassToCollection("AiMoveComponent", "void setBehaviorType(String behaviorType)", "Sets the behavior type what this ai component should do. Possible values are 'Move', 'Seek', 'Flee', 'Arrive', 'Pursuit', 'Evade'");
		AddClassToCollection("AiMoveComponent", "String getBehaviorType()", "Gets the used behavior type. Possible values are 'Move', 'Seek', 'Flee', 'Arrive', 'Pursuit', 'Evade'.");
		AddClassToCollection("AiMoveComponent", "void setTargetId(String targetId)", "Sets the target game object id. This is used for 'Seek', 'Flee', 'Arrive', 'Pursuit', 'Evade'.");
		AddClassToCollection("AiMoveComponent", "String getTargetId()", "Gets the target game object id.");

		module(lua)
			[
				class_<AiMoveRandomlyComponent, AiComponent>("AiMoveRandomlyComponent")
				// .def("clone", &AiMoveRandomlyComponent::clone)
			];

		module(lua)
			[
				class_<AiPathFollowComponent, AiComponent>("AiPathFollowComponent")
				// .def("clone", &AiPathFollowComponent::clone)
			.def("setWaypointsCount", &AiPathFollowComponent::setWaypointsCount)
			.def("getWaypointsCount", &AiPathFollowComponent::getWaypointsCount)
			//.def("setWaypointId", &AiPathFollowComponent::setWaypointId)
			//.def("getWaypointId", &AiPathFollowComponent::getWaypointId)
			.def("setWaypointId", &setWaypointId)
			.def("addWaypointId", &addWaypointId)
			.def("getWaypointId", &getWaypointId)
			.def("setRepeat", &AiPathFollowComponent::setRepeat)
			.def("getRepeat", &AiPathFollowComponent::getRepeat)
			.def("setDirectionChange", &AiPathFollowComponent::setDirectionChange)
			.def("getDirectionChange", &AiPathFollowComponent::getDirectionChange)
			// .def("setInvertDirection", &AiPathFollowComponent::setInvertDirection)
			// .def("getInvertDirection", &AiPathFollowComponent::getInvertDirection)
			.def("setGoalRadius", &AiPathFollowComponent::setGoalRadius)
			.def("getGoalRadius", &AiPathFollowComponent::getGoalRadius)
			];

		AddClassToCollection("AiPathFollowComponent", "class inherits AiComponent", AiPathFollowComponent::getStaticInfoText());
		AddClassToCollection("AiPathFollowComponent", "void setWaypointsCount(int count)", "Sets the way points count.");
		AddClassToCollection("AiPathFollowComponent", "int getWaypointsCount()", "Gets the way points count.");
		AddClassToCollection("AiPathFollowComponent", "void setWaypointId(int index, String id)", "Sets the id of the GameObject with the NodeComponent for the given waypoint index.");
		AddClassToCollection("AiPathFollowComponent", "void addWaypointId(String id)", "Adds the id of the GameObject with the NodeComponent.");
		AddClassToCollection("AiPathFollowComponent", "String getWaypointId(int index)", "Gets the way point id from the given index.");
		AddClassToCollection("AiPathFollowComponent", "void setRepeat(bool repeat)", "Sets whether to repeat the path, when the game object reached the last way point.");
		AddClassToCollection("AiPathFollowComponent", "bool getRepeat()", "Gets whether to repeat the path, when the game object reached the last way point.");
		AddClassToCollection("AiPathFollowComponent", "void setDirectionChange(bool directionChange)", "Sets whether to change the direction of the path follow when the game object reached the last waypoint.");
		AddClassToCollection("AiPathFollowComponent", "bool getDirectionChange()", "Gets whether to change the direction of the path follow when the game object reached the last waypoint.");
		// AddClassToCollection("AiPathFollowComponent", "void setInvertDirection(int count)", "???");
		// AddClassToCollection("AiPathFollowComponent", "int getInvertDirection()",  "???");
		AddClassToCollection("AiPathFollowComponent", "void setGoalRadius(float radius)", "Sets the goal radius at which the game object is considered within the next waypoint range. Default value is 0.2.");
		AddClassToCollection("AiPathFollowComponent", "float getGoalRadius()", "Gets the goal radius at which the game object is considered within the next waypoint range.");

		module(lua)
			[
				class_<AiWanderComponent, AiComponent>("AiWanderComponent")
				// .def("clone", &AiWanderComponent::clone)
			.def("setWanderJitter", &AiWanderComponent::setWanderJitter)
			.def("getWanderJitter", &AiWanderComponent::getWanderJitter)
			.def("setWanderRadius", &AiWanderComponent::setWanderRadius)
			.def("getWanderRadius", &AiWanderComponent::getWanderRadius)
			.def("setWanderDistance", &AiWanderComponent::setWanderDistance)
			.def("getWanderDistance", &AiWanderComponent::getWanderDistance)
			];

		AddClassToCollection("AiWanderComponent", "class inherits AiComponent", AiWanderComponent::getStaticInfoText());
		AddClassToCollection("AiWanderComponent", "void setWanderJitter(float wanderJitter)", "Sets the wander jitter. That is how often the game object does change its direction. Default valus is 1.");
		AddClassToCollection("AiWanderComponent", "float getWanderJitter()", "Gets the wander jitter.");
		AddClassToCollection("AiWanderComponent", "void setWanderRadius(float wanderRadius)", "Sets wander radius. That is how much does the game object change its direction. Default valus is 1.2.");
		AddClassToCollection("AiWanderComponent", "float getWanderRadius(int index)", "Gets wander radius.");
		AddClassToCollection("AiWanderComponent", "void setWanderDistance(float wanderDistance)", "Sets the wander distance. That is how long does the game object follow a point until direction changes occur. Default valus is 20.");
		AddClassToCollection("AiWanderComponent", "float getWanderDistance()", "Gets the wander distance.");

		module(lua)
			[
				class_<AiFlockingComponent, AiComponent>("AiFlockingComponent")
				// .def("clone", &AiFlockingComponent::clone)
			.def("setNeighborDistance", &AiFlockingComponent::setNeighborDistance)
			.def("getNeighborDistance", &AiFlockingComponent::getNeighborDistance)
			.def("setCohesionBehavior", &AiFlockingComponent::setCohesionBehavior)
			.def("getCohesionBehavior", &AiFlockingComponent::getCohesionBehavior)
			.def("setSeparationBehavior", &AiFlockingComponent::setSeparationBehavior)
			.def("getSeparationBehavior", &AiFlockingComponent::getSeparationBehavior)
			.def("setAlignmentBehavior", &AiFlockingComponent::setAlignmentBehavior)
			.def("getAlignmentBehavior", &AiFlockingComponent::getAlignmentBehavior)
			.def("setBorderBehavior", &AiFlockingComponent::setBorderBehavior)
			.def("getBorderBehavior", &AiFlockingComponent::getBorderBehavior)
			.def("setObstacleBehavior", &AiFlockingComponent::setObstacleBehavior)
			.def("getObstacleBehavior", &AiFlockingComponent::getObstacleBehavior)
			.def("setFleeBehavior", &AiFlockingComponent::setFleeBehavior)
			.def("getFleeBehavior", &AiFlockingComponent::getFleeBehavior)
			.def("setSeekBehavior", &AiFlockingComponent::setSeekBehavior)
			.def("getSeekBehavior", &AiFlockingComponent::getSeekBehavior)
			];

		AddClassToCollection("AiFlockingComponent", "class inherits AiComponent", AiFlockingComponent::getStaticInfoText());
		AddClassToCollection("AiFlockingComponent", "void setNeighborDistance(float neighborDistance)", "Sets the minimum distance each flock does have to its neighbor. Default valus is 1.6. Works in conjunction with separation behavior.");
		AddClassToCollection("AiFlockingComponent", "float getNeighborDistance()", "Gets the minimum distance each flock does have to its neighbor.");
		AddClassToCollection("AiFlockingComponent", "void setCohesionBehavior(bool cohesion)", "Sets whether to use cohesion behavior. That is each game object goes towards a common center of mass of all other game objects.");
		AddClassToCollection("AiFlockingComponent", "bool getCohesionBehavior()", "Gets whether to use cohesion behavior.");
		AddClassToCollection("AiFlockingComponent", "void setSeparationBehavior(bool separation)", "Sets whether to use separation. That is the minimum distance each flock does have to its neighbor.");
		AddClassToCollection("AiFlockingComponent", "bool getSeparationBehavior()", "Gets twhether to use separation.");
		AddClassToCollection("AiFlockingComponent", "void setAlignmentBehavior(bool alignment)", "Sets whether to use alignment behavior. This is a common direction all flocks have.");
		AddClassToCollection("AiFlockingComponent", "bool getAlignmentBehavior()", "Gets whether to use alignment behavior.");
		AddClassToCollection("AiFlockingComponent", "void setObstacleBehavior(bool obstacle)", "Sets whether to use a obstacle behavior. That is, a flock will change its direction when comming near an obstacle.");
		AddClassToCollection("AiFlockingComponent", "bool getObstacleBehavior()", "Gets whether to use a obstacle behavior.");
		AddClassToCollection("AiFlockingComponent", "void setFleeBehavior(bool flee)", "Sets whether to use flee behavior. That is the flock does flee from a target game object (target id).");
		AddClassToCollection("AiFlockingComponent", "bool getFleeBehavior()", "Gets whether to use flee behavior.");
		AddClassToCollection("AiFlockingComponent", "void setSeekBehavior(bool seek)", "Sets whether to use seek behavior. That is the flock does seek a target game object (target id).");
		AddClassToCollection("AiFlockingComponent", "bool getSeekBehavior()", "Gets the wander distance.");
		AddClassToCollection("AiFlockingComponent", "void setTargetId(String id)", "Sets target id, that is used for flee or seek behavior.");
		AddClassToCollection("AiFlockingComponent", "String getTargetId()", "Gets the target id, that is used for flee or seek behavior.");

		module(lua)
			[
				class_<AiRecastPathNavigationComponent, AiComponent>("AiRecastPathNavigationComponent")
				// .def("clone", &AiRecastPathNavigationComponent::clone)
			.def("showDebugData", &AiRecastPathNavigationComponent::showDebugData)
			.def("setRepeat", &AiRecastPathNavigationComponent::setRepeat)
			.def("getRepeat", &AiRecastPathNavigationComponent::getRepeat)
			.def("setDirectionChange", &AiRecastPathNavigationComponent::setDirectionChange)
			.def("getDirectionChange", &AiRecastPathNavigationComponent::getDirectionChange)
			// .def("setInvertDirection", &AiRecastPathNavigationComponent::setInvertDirection)
			// .def("getInvertDirection", &AiRecastPathNavigationComponent::getInvertDirection)
			.def("setGoalRadius", &AiRecastPathNavigationComponent::setGoalRadius)
			.def("getGoalRadius", &AiRecastPathNavigationComponent::getGoalRadius)
			.def("setActualizePathDelay", &AiRecastPathNavigationComponent::setActualizePathDelaySec)
			.def("getActualizePathDelay", &AiRecastPathNavigationComponent::getActualizePathDelaySec)
			.def("setPathSlot", &AiRecastPathNavigationComponent::setPathSlot)
			.def("getPathSlot", &AiRecastPathNavigationComponent::getPathSlot)
			.def("setPathTargetSlot", &AiRecastPathNavigationComponent::setPathTargetSlot)
			.def("getPathTargetSlot", &AiRecastPathNavigationComponent::getPathTargetSlot)
			];

		AddClassToCollection("AiRecastPathNavigationComponent", "class inherits AiComponent", AiRecastPathNavigationComponent::getStaticInfoText());
		AddClassToCollection("AiRecastPathNavigationComponent", "void showDebugData(bool show)", "Shows the valid navigation area and the shortest path line from origin to goal.");
		AddClassToCollection("AiRecastPathNavigationComponent", "void setRepeat(bool repeat)", "Sets whether to repeat the path, when the game object reached the last way point.");
		AddClassToCollection("AiRecastPathNavigationComponent", "bool getRepeat()", "Gets whether to repeat the path, when the game object reached the last way point.");
		AddClassToCollection("AiRecastPathNavigationComponent", "void setDirectionChange(bool directionChange)", "Sets whether to change the direction of the path follow when the game object reached the last waypoint.");
		AddClassToCollection("AiRecastPathNavigationComponent", "bool getDirectionChange()", "Gets whether to change the direction of the path follow when the game object reached the last waypoint.");
		AddClassToCollection("AiRecastPathNavigationComponent", "void setGoalRadius(float radius)", "Sets the goal radius at which the game object is considered within the next waypoint range. Default value is 0.2.");
		AddClassToCollection("AiRecastPathNavigationComponent", "float getGoalRadius()", "Gets the goal radius at which the game object is considered within the next waypoint range.");
		AddClassToCollection("AiRecastPathNavigationComponent", "void setTargetId(String targetId)", "Sets a target id game object as goal to go to walk to that game object on shortest path. Note: The target game object may move and the path is recalculated at @getActualizePathDelay rate.");
		AddClassToCollection("AiRecastPathNavigationComponent", "String getTargetId()", "Gets the target id, which this game object does walk to.");
		AddClassToCollection("AiRecastPathNavigationComponent", "void setActualizePathDelay(float delayMS)", "Sets the path recalculation delay in milliseconds. This is the rate the path is recalculated due to performance reasons when the target id game object changes its position. Default is set to -1 (off)");
		AddClassToCollection("AiRecastPathNavigationComponent", "float getActualizePathDelay()", "Gets the path recalculation delay in milliseconds.");
		AddClassToCollection("AiRecastPathNavigationComponent", "void setPathSlot(int pathSlot)", "Sets path slot index. Note: This is necessary, since there can be several paths created for different slots.");
		AddClassToCollection("AiRecastPathNavigationComponent", "int getPathSlot()", "Gets the current path slot index.");
		AddClassToCollection("AiRecastPathNavigationComponent", "void setTargetPathSlot(int targetPathSlot)", "Sets target path slot index. Note: This is necessary, since there can be several paths created for different slots.");
		AddClassToCollection("AiRecastPathNavigationComponent", "int getTargetPathSlot()", "Gets the current target path slot index.");

		module(lua)
			[
				class_<AiObstacleAvoidanceComponent, AiComponent>("AiObstacleAvoidanceComponent")
				// .def("clone", &AiObstacleAvoidanceComponent::clone)
			.def("setAvoidanceRadius", &AiObstacleAvoidanceComponent::setAvoidanceRadius)
			.def("getAvoidanceRadius", &AiObstacleAvoidanceComponent::getAvoidanceRadius)
			.def("setObstacleCategories", &AiObstacleAvoidanceComponent::setObstacleCategories)
			.def("getObstacleCategories", &AiObstacleAvoidanceComponent::getObstacleCategories)
			];

		AddClassToCollection("AiObstacleAvoidanceComponent", "class inherits AiComponent", AiObstacleAvoidanceComponent::getStaticInfoText());
		AddClassToCollection("AiObstacleAvoidanceComponent", "void setAvoidanceRadius(float radius)", "Sets the radius, at which an obstacle is avoided.");
		AddClassToCollection("AiObstacleAvoidanceComponent", "float getAvoidanceRadius()", "Gets the radius, at which an obstacle is avoided.");
		AddClassToCollection("AiObstacleAvoidanceComponent", "void setObstacleCategories(string categories)", "Sets categories, that are considered as obstacle for hiding. Note: Combined categories can be used like 'ALL-Player' etc.");
		AddClassToCollection("AiObstacleAvoidanceComponent", "string getObstacleCategories()", "Gets categories, that are considered as obstacle for hiding.");

		module(lua)
			[
				class_<AiHideComponent, AiComponent>("AiHideComponent")
				// .def("clone", &AiHideComponent::clone)
			.def("setObstacleRangeRadius", &AiHideComponent::setObstacleRangeRadius)
			.def("getObstacleRangeRadius", &AiHideComponent::getObstacleRangeRadius)
			.def("setObstacleCategories", &AiHideComponent::setObstacleCategories)
			.def("getObstacleCategories", &AiHideComponent::getObstacleCategories)
			];

		AddClassToCollection("AiHideComponent", "class inherits AiComponent", AiHideComponent::getStaticInfoText());
		AddClassToCollection("AiHideComponent", "void setTargetId(String targetId)", "Sets a target id game object, this game object is hiding at.");
		AddClassToCollection("AiHideComponent", "String getTargetId()", "Gets the target id game object, this game object is hiding at.");
		AddClassToCollection("AiHideComponent", "void setObstacleRangeRadius(float radius)", "Sets the radius, at which the game object keeps distance at an obstacle.");
		AddClassToCollection("AiHideComponent", "float getObstacleRangeRadius()", "Gets the radius, at which the game object keeps distance at an obstacle.");
		AddClassToCollection("AiHideComponent", "void setObstacleCategories(string categories)", "Sets categories, that are considered as obstacle for keeping a distance. Note: Combined categories can be used like 'ALL-Player' etc.");
		AddClassToCollection("AiHideComponent", "string getObstacleCategories()", "Gets categories, that are considered as obstacle for keeping a distance.");

		module(lua)
			[
				class_<AiMoveComponent2D, AiComponent>("AiMoveComponent2D")
				// .def("clone", &AiMoveComponent2D::clone)
			.def("setBehaviorType", &AiMoveComponent2D::setBehaviorType)
			.def("getBehaviorType", &AiMoveComponent2D::getBehaviorType)
			];

		AddClassToCollection("AiMoveComponent2D", "class", AiMoveComponent2D::getStaticInfoText());
		AddClassToCollection("AiMoveComponent2D", "void setBehaviorType(String behaviorType)", "Sets the behavior type what this ai component should do. Possible values are 'Seek2D', 'Flee2D', 'Arrive2D'");
		AddClassToCollection("AiMoveComponent2D", "String getBehaviorType()", "Gets the used behavior type. Possible values are 'Seek2D', 'Flee2D', 'Arrive2D'.");
		AddClassToCollection("AiMoveComponent2D", "void setTargetId(String targetId)", "Sets the target game object id. This is used for 'Seek2D', 'Flee2D', 'Arrive2D'.");
		AddClassToCollection("AiMoveComponent2D", "String getTargetId()", "Gets the target game object id.");

		module(lua)
			[
				class_<AiPathFollowComponent2D, AiComponent>("AiPathFollowComponent2D")
				// .def("clone", &AiPathFollowComponent2D::clone)
			.def("setWaypointsCount", &AiPathFollowComponent2D::setWaypointsCount)
			.def("getWaypointsCount", &AiPathFollowComponent2D::getWaypointsCount)
			// .def("setWaypointId", &AiPathFollowComponent2D::setWaypointId)
			// .def("getWaypointId", &AiPathFollowComponent2D::getWaypointId)
			.def("setWaypointId", &setWaypointId2)
			.def("addWaypointId2", &addWaypointId2)
			.def("getWaypointId", &getWaypointId2)
			.def("setRepeat", &AiPathFollowComponent2D::setRepeat)
			.def("getRepeat", &AiPathFollowComponent2D::getRepeat)
			.def("setDirectionChange", &AiPathFollowComponent2D::setDirectionChange)
			.def("getDirectionChange", &AiPathFollowComponent2D::getDirectionChange)
			// .def("setInvertDirection", &AiPathFollowComponent2D::setInvertDirection)
			// .def("getInvertDirection", &AiPathFollowComponent2D::getInvertDirection)
			.def("setGoalRadius", &AiPathFollowComponent2D::setGoalRadius)
			.def("getGoalRadius", &AiPathFollowComponent2D::getGoalRadius)
			];

		AddClassToCollection("AiPathFollowComponent2D", "class", AiPathFollowComponent2D::getStaticInfoText());
		AddClassToCollection("AiPathFollowComponent2D", "void setWaypointsCount(int count)", "Sets the way points count.");
		AddClassToCollection("AiPathFollowComponent2D", "int getWaypointsCount()", "Gets the way points count.");
		AddClassToCollection("AiPathFollowComponent2D", "void setWaypointId(int index, String id)", "Sets the id of the GameObject with the NodeComponent for the given waypoint index.");
		AddClassToCollection("AiPathFollowComponent", "void addWaypointId(String id)", "Adds the id of the GameObject with the NodeComponent.");
		AddClassToCollection("AiPathFollowComponent2D", "String getWaypointId(int index)", "Gets the way point id from the given index.");
		AddClassToCollection("AiPathFollowComponent2D", "void setRepeat(bool repeat)", "Sets whether to repeat the path, when the game object reached the last way point.");
		AddClassToCollection("AiPathFollowComponent2D", "bool getRepeat()", "Gets whether to repeat the path, when the game object reached the last way point.");
		AddClassToCollection("AiPathFollowComponent2D", "void setDirectionChange(bool directionChange)", "Sets whether to change the direction of the path follow when the game object reached the last waypoint.");
		AddClassToCollection("AiPathFollowComponent2D", "bool getDirectionChange()", "Gets whether to change the direction of the path follow when the game object reached the last waypoint.");
		// AddClassToCollection("AiPathFollowComponent2D", "void setInvertDirection(int count)", "???");
		// AddClassToCollection("AiPathFollowComponent2D", "int getInvertDirection()",  "???");
		AddClassToCollection("AiPathFollowComponent2D", "void setGoalRadius(float radius)", "Sets the goal radius at which the game object is considered within the next waypoint range. Default value is 0.2.");
		AddClassToCollection("AiPathFollowComponent2D", "float getGoalRadius()", "Gets the goal radius at which the game object is considered within the next waypoint range.");

		module(lua)
			[
				class_<AiWanderComponent2D, AiComponent>("AiWanderComponent2D")
				// .def("clone", &AiWanderComponent2D::clone)
			.def("setWanderJitter", &AiWanderComponent2D::setWanderJitter)
			.def("getWanderJitter", &AiWanderComponent2D::getWanderJitter)
			.def("setWanderRadius", &AiWanderComponent2D::setWanderRadius)
			.def("getWanderRadius", &AiWanderComponent2D::getWanderRadius)
			.def("setWanderDistance", &AiWanderComponent2D::setWanderDistance)
			.def("getWanderDistance", &AiWanderComponent2D::getWanderDistance)
			];

		AddClassToCollection("AiWanderComponent2D", "class", AiWanderComponent2D::getStaticInfoText());
		AddClassToCollection("AiWanderComponent2D", "void setWanderJitter(float wanderJitter)", "Sets the wander jitter. That is how often the game object does change its direction. Default valus is 1.");
		AddClassToCollection("AiWanderComponent2D", "float getWanderJitter()", "Gets the wander jitter.");
		AddClassToCollection("AiWanderComponent2D", "void setWanderRadius(float wanderRadius)", "Sets wander radius. That is how much does the game object change its direction. Default valus is 1.2.");
		AddClassToCollection("AiWanderComponent2D", "float getWanderRadius(int index)", "Gets wander radius.");
		AddClassToCollection("AiWanderComponent2D", "void setWanderDistance(float wanderDistance)", "Sets the wander distance. That is how long does the game object follow a point until direction changes occur. Default valus is 20.");
		AddClassToCollection("AiWanderComponent2D", "float getWanderDistance()", "Gets the wander distance.");

		/// more to come...
	}

	void bindCameraBehaviorComponents(lua_State* lua)
	{
		module(lua)
			[
				class_<CameraBehaviorComponent, GameObjectComponent>("CameraBehaviorComponent")
				// .def("getClassName", &CameraBehaviorComponent::getClassName)
			.def("getParentClassName", &CameraBehaviorComponent::getParentClassName)
			.def("setActivated", &CameraBehaviorComponent::setActivated)
			.def("isActivated", &CameraBehaviorComponent::isActivated)
			.def("getCamera", &CameraBehaviorComponent::getCamera)
			.def("setCameraControlLocked", &CameraBehaviorComponent::setCameraControlLocked)
			.def("getCameraControlLocked", &CameraBehaviorComponent::getCameraControlLocked)
			];

		AddClassToCollection("CameraBehaviorComponent", "class inherits GameObjectComponent", CameraBehaviorComponent::getStaticInfoText());
		// AddClassToCollection("CameraBehaviorComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("CameraBehaviorComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("CameraBehaviorComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("CameraBehaviorComponent", "void setActivated(bool activated)", "Sets the camera behavior component is activated. If true, the camera will do its work according the used behavior.");
		AddClassToCollection("CameraBehaviorComponent", "bool isActivated()", "Gets whether camera behavior component is activated. If true, the camera will do its work according the used behavior.");
		AddClassToCollection("CameraBehaviorComponent", "Camera getCamera()", "Gets the used camera pointer for direct manipulation.");
		AddClassToCollection("CameraBehaviorComponent", "void setCameraControlLocked(bool locked)", "Sets whether the camera can be moved by keys. Note: This should be locked, if a behavior is active, since the camera will be moved automatically.");
		AddClassToCollection("CameraBehaviorComponent", "bool getCameraControlLocked()", "Gets the rotation speed for the ai controller game object. Note: This should be locked, if a behavior is active, since the camera will be moved automatically.");


		module(lua)
			[
				class_<CameraBehaviorBaseComponent, CameraBehaviorComponent>("CameraBehaviorBaseComponent")
				// .def("clone", &CameraBehaviorBaseComponent::clone)
			.def("setMoveSpeed", &CameraBehaviorBaseComponent::setMoveSpeed)
			.def("getMoveSpeed", &CameraBehaviorBaseComponent::getMoveSpeed)
			.def("setRotationSpeed", &CameraBehaviorBaseComponent::setRotationSpeed)
			.def("getRotationSpeed", &CameraBehaviorBaseComponent::getRotationSpeed)
			.def("setSmoothValue", &CameraBehaviorBaseComponent::setSmoothValue)
			.def("getSmoothValue", &CameraBehaviorBaseComponent::getSmoothValue)
			];
		AddClassToCollection("CameraBehaviorBaseComponent", "class inherits CameraBehaviorComponent", CameraBehaviorBaseComponent::getStaticInfoText());
		AddClassToCollection("CameraBehaviorBaseComponent", "void setMoveSpeed(float moveSpeed)", "Sets the camera move speed.");
		AddClassToCollection("CameraBehaviorBaseComponent", "float getMoveSpeed()", "Gets the camera move speed.");
		AddClassToCollection("CameraBehaviorBaseComponent", "void setRotationSpeed(float rotationSpeed)", "Sets the camera rotation speed.");
		AddClassToCollection("CameraBehaviorBaseComponent", "float getRotationSpeed()", "Gets the camera rotation speed.");
		AddClassToCollection("CameraBehaviorBaseComponent", "void setSmoothValue(float smoothValue)", "Sets the camera value for more smooth transform. Note: Setting to 0, camera transform is not smooth, setting to 1 would be to smooth and lag behind, a good value is 0.1");
		AddClassToCollection("CameraBehaviorBaseComponent", "float getSmoothValue()", "Gets the camera value for more smooth transform.");

		module(lua)
			[
				class_<CameraBehaviorFirstPersonComponent, CameraBehaviorComponent>("CameraBehaviorFirstPersonComponent")
				// .def("clone", &CameraBehaviorFirstPersonComponent::clone)
			.def("setSmoothValue", &CameraBehaviorFirstPersonComponent::setSmoothValue)
			.def("getSmoothValue", &CameraBehaviorFirstPersonComponent::getSmoothValue)
			.def("setRotationSpeed", &CameraBehaviorFirstPersonComponent::setRotationSpeed)
			.def("getRotationSpeed", &CameraBehaviorFirstPersonComponent::getRotationSpeed)
			.def("setOffsetPosition", &CameraBehaviorFirstPersonComponent::setOffsetPosition)
			.def("getOffsetPosition", &CameraBehaviorFirstPersonComponent::getOffsetPosition)
			];
		AddClassToCollection("CameraBehaviorFirstPersonComponent", "class inherits CameraBehaviorComponent", CameraBehaviorFirstPersonComponent::getStaticInfoText());
		AddClassToCollection("CameraBehaviorFirstPersonComponent", "void setSmoothValue(float smoothValue)", "Sets the camera value for more smooth transform. Note: Setting to 0, camera transform is not smooth, setting to 1 would be to smooth and lag behind, a good value is 0.1");
		AddClassToCollection("CameraBehaviorFirstPersonComponent", "float getSmoothValue()", "Gets the camera value for more smooth transform.");
		AddClassToCollection("CameraBehaviorFirstPersonComponent", "void setRotationSpeed(float rotationSpeed)", "Sets the camera rotation speed.");
		AddClassToCollection("CameraBehaviorFirstPersonComponent", "float getRotationSpeed()", "Gets the camera rotation speed.");
		AddClassToCollection("CameraBehaviorFirstPersonComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets the camera offset position, it should be away from the game object.");
		AddClassToCollection("CameraBehaviorFirstPersonComponent", "Vector3 getOffsetPosition()", "Gets the offset position, the camera is away from the game object.");

		module(lua)
			[
				class_<CameraBehaviorThirdPersonComponent, CameraBehaviorComponent>("CameraBehaviorThirdPersonComponent")
				// .def("clone", &CameraBehaviorThirdPersonComponent::clone)
			.def("setYOffset", &CameraBehaviorThirdPersonComponent::setYOffset)
			.def("getYOffset", &CameraBehaviorThirdPersonComponent::getYOffset)
			.def("setLookAtOffset", &CameraBehaviorThirdPersonComponent::setLookAtOffset)
			.def("getLookAtOffset", &CameraBehaviorThirdPersonComponent::getLookAtOffset)
			.def("setSpringForce", &CameraBehaviorThirdPersonComponent::setSpringForce)
			.def("getSpringForce", &CameraBehaviorThirdPersonComponent::getSpringForce)
			.def("setFriction", &CameraBehaviorThirdPersonComponent::setFriction)
			.def("getFriction", &CameraBehaviorThirdPersonComponent::getFriction)
			.def("setSpringLength", &CameraBehaviorThirdPersonComponent::setSpringLength)
			.def("getSpringLength", &CameraBehaviorThirdPersonComponent::getSpringLength)
			];
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "class inherits CameraBehaviorComponent", CameraBehaviorThirdPersonComponent::getStaticInfoText());
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "void setYOffset(float yOffset)", "Sets the camera y-offset, the camera is placed above the game object.");
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "float getYOffset()", "Gets the camera y-offset, the camera is placed above the game object.");
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "void setLookAtOffset(Vector3 lookAtOffset)", "Sets the camera look at game object offset.");
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "Vector3 getLookAtOffset()", "Gets the camera look at game object offset.");
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "void setSpringForce(float springForce)", "Sets the camera spring force, that is, when the game object is rotated the camera is moved to the same direction but with a spring effect.");
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "float getSpringForce()", "Gets the camera spring force.");
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "void setFriction(float friction)", "Sets the camera friction during movement.");
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "float getFriction()", "Gets the camera friction during movement.");
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "void setSpringLength(float springLength)", "Sets the camera spring length during movement.");
		AddClassToCollection("CameraBehaviorThirdPersonComponent", "float getSpringLength()", "Gets the camera spring length during movement.");

		module(lua)
			[
				class_<CameraBehaviorFollow2DComponent, CameraBehaviorComponent>("CameraBehaviorFollow2DComponent")
				// .def("clone", &CameraBehaviorFollow2DComponent::clone)
			.def("setSmoothValue", &CameraBehaviorFollow2DComponent::setSmoothValue)
			.def("getSmoothValue", &CameraBehaviorFollow2DComponent::getSmoothValue)
			.def("setOffsetPosition", &CameraBehaviorFollow2DComponent::setOffsetPosition)
			.def("getOffsetPosition", &CameraBehaviorFollow2DComponent::getOffsetPosition)
			];
		AddClassToCollection("CameraBehaviorFollow2DComponent", "class inherits CameraBehaviorComponent", CameraBehaviorFollow2DComponent::getStaticInfoText());
		AddClassToCollection("CameraBehaviorFollow2DComponent", "void setSmoothValue(float smoothValue)", "Sets the camera value for more smooth transform. Note: Setting to 0, camera transform is not smooth, setting to 1 would be to smooth and lag behind, a good value is 0.1");
		AddClassToCollection("CameraBehaviorFollow2DComponent", "float getSmoothValue()", "Gets the camera value for more smooth transform.");
		AddClassToCollection("CameraBehaviorFollow2DComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets the camera offset position, it should be away from the game object.");
		AddClassToCollection("CameraBehaviorFollow2DComponent", "Vector3 getOffsetPosition()", "Gets the offset position, the camera is away from the game object.");

		module(lua)
		[
			class_<CameraBehaviorZoomComponent, CameraBehaviorComponent>("CameraBehaviorZoomComponent")
			// .def("clone", &CameraBehaviorZoomComponent::clone)
			.def("setCategory", &CameraBehaviorZoomComponent::setCategory)
			.def("getCategory", &CameraBehaviorZoomComponent::getCategory)
			.def("setSmoothValue", &CameraBehaviorZoomComponent::setSmoothValue)
			.def("getSmoothValue", &CameraBehaviorZoomComponent::getSmoothValue)
			.def("setGrowMultiplicator", &CameraBehaviorZoomComponent::setGrowMultiplicator)
			.def("getGrowMultiplicator", &CameraBehaviorZoomComponent::getGrowMultiplicator)
		];
		AddClassToCollection("CameraBehaviorZoomComponent", "class inherits CameraBehaviorComponent", CameraBehaviorZoomComponent::getStaticInfoText());
		AddClassToCollection("CameraBehaviorZoomComponent", "void setCategory(string category)", "Sets the category, for which the bounds are calculated and the zoom adapted, so that all GameObjects of this category are visible.");
		AddClassToCollection("CameraBehaviorZoomComponent", "string getCategory()", "Gets the category, for which the bounds are calculated and the zoom adapted, so that all GameObjects of this category are visible.");
		AddClassToCollection("CameraBehaviorZoomComponent", "void setSmoothValue(float smoothValue)", "Sets the camera value for more smooth transform. Note: Setting to 0, camera transform is not smooth, setting to 1 would be to smooth and lag behind, a good value is 0.1");
		AddClassToCollection("CameraBehaviorZoomComponent", "float getSmoothValue()", "Gets the camera value for more smooth transform.");
		AddClassToCollection("CameraBehaviorZoomComponent", "void setGrowMultiplicator(float growMultiplicator)", "Sets a grow multiplicator how fast the orthogonal camera window size will increase, so that the game objects remain in view."
			"Play with this value, because it also depends e.g. how fast your game objects are moving.");
		AddClassToCollection("CameraBehaviorZoomComponent", "float getGrowMultiplicator()", "Gets a grow multiplicator how fast the orthogonal camera window size will increase, so that the game objects remain in view."
			"Play with this value, because it also depends e.g. how fast your game objects are moving.");

	}

	void bindCameraComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<CameraComponent, GameObjectComponent>("CameraComponent")
				// .def("getClassName", &CameraComponent::getClassName)
			.def("getParentClassName", &CameraComponent::getParentClassName)
			.def("setActivated", &CameraComponent::setActivated)
			.def("isActivated", &CameraComponent::isActivated)
			// .def("setActivatedFlag", &CameraComponent::setActivatedFlag)
			.def("setNearClipDistance", &CameraComponent::setNearClipDistance)
			.def("getNearClipDistance", &CameraComponent::getNearClipDistance)
			.def("setFarClipDistance", &CameraComponent::setFarClipDistance)
			.def("getFarClipDistance", &CameraComponent::getFarClipDistance)
			.def("setOrthographic", &CameraComponent::setOrthographic)
			.def("getIsOrthographic", &CameraComponent::getIsOrthographic)
			.def("setFovy", &CameraComponent::setFovy)
			.def("getFovy", &CameraComponent::getFovy)
			.def("getCamera", &CameraComponent::getCamera)
			.def("setCameraPosition", &CameraComponent::setCameraPosition)
			.def("getCameraPosition", &CameraComponent::getCameraPosition)
			.def("setCameraDegreeOrientation", &CameraComponent::setCameraDegreeOrientation)
			.def("getCameraDegreeOrientation", &CameraComponent::getCameraDegreeOrientation)
			];

		AddClassToCollection("CameraComponent", "class inherits GameObjectComponent", CameraComponent::getStaticInfoText());
		// AddClassToCollection("CameraComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("CameraComponent", "void setActivated(bool activated)", "Activates this camera and deactivates another camera (when was active).");
		AddClassToCollection("CameraComponent", "bool isActivated()", "Gets whether camera is activated or not.");
		AddClassToCollection("CameraComponent", "void setNearClipDistance(float nearClipDistance)", "Sets the near clip distance for the camera.");
		AddClassToCollection("CameraComponent", "float getNearClipDistance()", "Gets the near clip distance for the camera.");
		AddClassToCollection("CameraComponent", "void setFarClipDistance(float farClipDistance)", "Sets the far clip distance for the camera.");
		AddClassToCollection("CameraComponent", "float getFarClipDistance()", "Gets the far clip distance for the camera.");
		AddClassToCollection("CameraComponent", "void setOrthographic(bool orthographic)", "Sets whether to use orthographic projection. Default is off (3D perspective projection).");
		AddClassToCollection("CameraComponent", "bool getIsOrthographic()", "Gets whether to use orthographic projection. Default is off (3D perspective projection).");
		AddClassToCollection("CameraComponent", "void setFovy(float angle)", "Sets the field of view angle for the camera.");
		AddClassToCollection("CameraComponent", "float getFovy()", "Gets the field of view angle for the camera.");
		AddClassToCollection("CameraComponent", "Camera getCamera()", "Gets the used camera pointer for direct manipulation.");
		// AddClassToCollection("CameraComponent", "void setWorkspaceName(String workspaceName)", "Sets the workspace name to use for rendering. Possible values are e.g. NOWAPbsWorkspace, NOWASkyWorkspace, NOWASkyReflectionWorkspace, NOWASkyPlanarReflectionWorkspace, NOWABackgroundPlanarReflectionWorkspace, NOWATerraWorkspace, NOWABackgroundWorkspace");
		// AddClassToCollection("CameraComponent", "String getWorkspaceName()", "Gets the currently used workspace name to use for rendering.");
		// AddClassToCollection("CameraComponent", "void setSkyBoxName(String skyBoxName)", "Sets the sky box name. Note: Only usable when workspace is set to  NOWASkyWorkspace, NOWASkyReflectionWorkspace, NOWASkyPlanarReflectionWorkspace, NOWATerraWorkspace");
		// AddClassToCollection("CameraComponent", "String getSkyBoxName()", "Gets the currently used sky box name.");
		// AddClassToCollection("CameraComponent", "void setColor(Vector3 color)", "Sets the background color for the camera. Note: Does only make sense for NOWAPbsWorkspace");
		// AddClassToCollection("CameraComponent", "Vector3 getColor()", "Gets the currently used background color.");
	}

	void bindCompositorEffects(lua_State* lua)
	{
		module(lua)
			[
				class_<CompositorEffectBaseComponent, GameObjectComponent>("CompositorEffectBaseComponent")
				// .def("getClassName", &CompositorEffectBaseComponent::getClassName)
			.def("getParentClassName", &CompositorEffectBaseComponent::getParentClassName)
			.def("setActivated", &CompositorEffectBaseComponent::setActivated)
			.def("isActivated", &CompositorEffectBaseComponent::isActivated)
			];

		AddClassToCollection("CompositorEffectBaseComponent", "class inherits GameObjectComponent", CompositorEffectBaseComponent::getStaticInfoText());

		module(lua)
			[
				class_<CompositorEffectBloomComponent, CompositorEffectBaseComponent>("CompositorEffectBaseComponent")
				.def("setImageWeight", &CompositorEffectBloomComponent::setImageWeight)
			.def("getImageWeight", &CompositorEffectBloomComponent::getImageWeight)
			.def("setBlurWeight", &CompositorEffectBloomComponent::setBlurWeight)
			.def("getBlurWeight", &CompositorEffectBloomComponent::getBlurWeight)
			];

		AddClassToCollection("CompositorEffectBloomComponent", "class inherits CompositorEffectBaseComponent", CompositorEffectBloomComponent::getStaticInfoText());
		// AddClassToCollection("CompositorEffectBloomComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("CompositorEffectBloomComponent", "void setActivated(bool activated)", "Activates the given compositor effect.");
		AddClassToCollection("CompositorEffectBloomComponent", "bool isActivated()", "Gets whether the compositor effect is activated or not.");
		AddClassToCollection("CompositorEffectBloomComponent", "void setImageWeight(float imageWeight)", "Sets the image weight for the bloom effect.");
		AddClassToCollection("CompositorEffectBloomComponent", "float getImageWeight()", "Gets the image weight.");
		AddClassToCollection("CompositorEffectBloomComponent", "void setBlurWeight(float blurWeight)", "Sets the blur weight for the bloom effect.");
		AddClassToCollection("CompositorEffectBloomComponent", "float getblurWeight()", "Gets the blur weight.");

		module(lua)
			[
				class_<CompositorEffectGlassComponent, CompositorEffectBaseComponent>("CompositorEffectGlassComponent")
				.def("setGlassWeight", &CompositorEffectGlassComponent::setGlassWeight)
			.def("getGlassWeight", &CompositorEffectGlassComponent::getGlassWeight)
			];

		AddClassToCollection("CompositorEffectGlassComponent", "class inherits CompositorEffectBaseComponent", CompositorEffectGlassComponent::getStaticInfoText());
		// AddClassToCollection("CompositorEffectGlassComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("CompositorEffectGlassComponent", "void setActivated(bool activated)", "Activates the given compositor effect.");
		AddClassToCollection("CompositorEffectGlassComponent", "bool isActivated()", "Gets whether the compositor effect is activated or not.");
		AddClassToCollection("CompositorEffectGlassComponent", "void setGlassWeight(float glassWeight)", "Sets the glass weight for the glass effect.");
		AddClassToCollection("CompositorEffectGlassComponent", "float getGlassWeight()", "Gets the glass weight.");

		module(lua)
		[
			class_<CompositorEffectOldTvComponent, CompositorEffectBaseComponent>("CompositorEffectOldTvComponent")
			.def("setDistortionFrequency", &CompositorEffectOldTvComponent::setDistortionFrequency)
			.def("getDistortionFrequency", &CompositorEffectOldTvComponent::getDistortionFrequency)
			.def("setDistortionScale", &CompositorEffectOldTvComponent::setDistortionScale)
			.def("getDistortionScale", &CompositorEffectOldTvComponent::getDistortionScale)
			.def("setDistortionRoll", &CompositorEffectOldTvComponent::setDistortionRoll)
			.def("getDistortionRoll", &CompositorEffectOldTvComponent::getDistortionRoll)
			.def("setInterference", &CompositorEffectOldTvComponent::setInterference)
			.def("getInterference", &CompositorEffectOldTvComponent::getInterference)
			.def("setFrameLimit", &CompositorEffectOldTvComponent::setFrameLimit)
			.def("getFrameLimit", &CompositorEffectOldTvComponent::getFrameLimit)
			.def("setFrameShape", &CompositorEffectOldTvComponent::setFrameShape)
			.def("getFrameShape", &CompositorEffectOldTvComponent::getFrameShape)
			.def("setFrameSharpness", &CompositorEffectOldTvComponent::setFrameSharpness)
			.def("getFrameSharpness", &CompositorEffectOldTvComponent::getFrameSharpness)
			.def("setTime", &CompositorEffectOldTvComponent::setTime)
			.def("getTime", &CompositorEffectOldTvComponent::getTime)
			.def("setSinusTime", &CompositorEffectOldTvComponent::setSinusTime)
			.def("getSinusTime", &CompositorEffectOldTvComponent::getSinusTime)
		];

		AddClassToCollection("CompositorEffectOldTvComponent", "class inherits CompositorEffectBaseComponent", CompositorEffectOldTvComponent::getStaticInfoText());
		// AddClassToCollection("CompositorEffectOldTvComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setActivated(bool activated)", "Activates the given compositor effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "bool isActivated()", "Gets whether the compositor effect is activated or not.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setDistortionFrequency(float distortionFrequency)", "Sets the disortion frequency for the old tv effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "float getDistortionFrequency()", "Gets the disortion frequency.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setDistortionScale(float distortionScale)", "Sets the distortion scale for the old tv effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "float getDistortionScale()", "Gets the distortion scale.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setDistortionRoll(float distortionRoll)", "Sets the distortion roll for the old tv effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "float getDistortionRoll()", "Gets the distortion roll.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setInterference(float interference)", "Sets the interference for the old tv effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "float getInterference()", "Gets the interference.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setFrameLimit(float frameLimit)", "Sets the frame limit for the old tv effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "float getFrameLimit()", "Gets the frame limit.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setFrameShape(float frameShape)", "Sets the frame shape for the old tv effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "float getFrameShape()", "Gets the frame shape.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setFrameSharpness(float frameSharpness)", "Sets the frame sharpness for the old tv effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "float getFrameSharpness()", "Gets the frame sharpness.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setTime(Vector3 time)", "Sets the time frequency for the old tv effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "Vector3 getTime()", "Gets the time frequency.");
		AddClassToCollection("CompositorEffectOldTvComponent", "void setSinusTime(Vector3 sinusTime)", "Sets the since time frequency for the old tv effect.");
		AddClassToCollection("CompositorEffectOldTvComponent", "Vector3 getSinusTime()", "Gets the sinus time frequency.");

		module(lua)
		[
			class_<CompositorEffectKeyholeComponent, CompositorEffectBaseComponent>("CompositorEffectKeyholeComponent")
			.def("setFrameShape", &CompositorEffectOldTvComponent::setFrameShape)
			.def("getFrameShape", &CompositorEffectOldTvComponent::getFrameShape)
			// .def("setTime", &CompositorEffectKeyholeComponent::setTime)
			// .def("getTime", &CompositorEffectKeyholeComponent::getTime)
			// .def("setSinusTime", &CompositorEffectKeyholeComponent::setSinusTime)
			// .def("getSinusTime", &CompositorEffectKeyholeComponent::getSinusTime)
		];

		AddClassToCollection("CompositorEffectKeyholeComponent", "class inherits CompositorEffectBaseComponent", CompositorEffectKeyholeComponent::getStaticInfoText());
		AddClassToCollection("CompositorEffectKeyholeComponent", "void setActivated(bool activated)", "Activates the given compositor effect.");
		AddClassToCollection("CompositorEffectKeyholeComponent", "bool isActivated()", "Gets whether the compositor effect is activated or not.");
		AddClassToCollection("CompositorEffectKeyholeComponent", "void setFrameShape(float frameShape)", "Sets the frame shape for the key hole effect.");
		AddClassToCollection("CompositorEffectKeyholeComponent", "float getFrameShape()", "Gets the frame shape.");
		// AddClassToCollection("CompositorEffectKeyholeComponent", "void setTime(Vector3 time)", "Sets the time frequency for the old tv effect.");
		// AddClassToCollection("CompositorEffectKeyholeComponent", "Vector3 getTime()", "Gets the time frequency.");
		// AddClassToCollection("CompositorEffectKeyholeComponent", "void setSinusTime(Vector3 sinusTime)", "Sets the since time frequency for the old tv effect.");
		// AddClassToCollection("CompositorEffectKeyholeComponent", "Vector3 getSinusTime()", "Gets the sinus time frequency.");

		module(lua)
			[
				class_<CompositorEffectBlackAndWhiteComponent, CompositorEffectBaseComponent>("CompositorEffectBlackAndWhiteComponent")
				.def("setColor", &CompositorEffectBlackAndWhiteComponent::setColor)
			.def("getColor", &CompositorEffectBlackAndWhiteComponent::getColor)
			];

		AddClassToCollection("CompositorEffectBlackAndWhiteComponent", "class inherits CompositorEffectBaseComponent", CompositorEffectBlackAndWhiteComponent::getStaticInfoText());
		// AddClassToCollection("CompositorEffectBlackAndWhiteComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("CompositorEffectBlackAndWhiteComponent", "void setActivated(bool activated)", "Activates the given compositor effect.");
		AddClassToCollection("CompositorEffectBlackAndWhiteComponent", "bool isActivated()", "Gets whether the compositor effect is activated or not.");
		AddClassToCollection("CompositorEffectBlackAndWhiteComponent", "void setColor(Vector3 grayScale)", "Sets the gray scale color for the black and white effect.");
		AddClassToCollection("CompositorEffectBlackAndWhiteComponent", "float getColor()", "Gets the gray scale color.");

		module(lua)
			[
				class_<CompositorEffectColorComponent, CompositorEffectBaseComponent>("CompositorEffectColorComponent")
				.def("setColor", &CompositorEffectColorComponent::setColor)
			.def("getColor", &CompositorEffectColorComponent::getColor)
			];

		AddClassToCollection("CompositorEffectColorComponent", "class inherits CompositorEffectBaseComponent", CompositorEffectColorComponent::getStaticInfoText());
		// AddClassToCollection("CompositorEffectColorComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("CompositorEffectColorComponent", "void setActivated(bool activated)", "Activates the given compositor effect.");
		AddClassToCollection("CompositorEffectColorComponent", "bool isActivated()", "Gets whether the compositor effect is activated or not.");
		AddClassToCollection("CompositorEffectColorComponent", "void setColor(Vector3 grayScale)", "Sets the gray scale color for the color effect.");
		AddClassToCollection("CompositorEffectColorComponent", "float getColor()", "Gets the gray scale color.");

		module(lua)
			[
				class_<CompositorEffectEmbossedComponent, CompositorEffectBaseComponent>("CompositorEffectEmbossedComponent")
				.def("setWeight", &CompositorEffectEmbossedComponent::setWeight)
			.def("getWeight", &CompositorEffectEmbossedComponent::getWeight)
			];

		AddClassToCollection("CompositorEffectEmbossedComponent", "class inherits CompositorEffectBaseComponent", CompositorEffectEmbossedComponent::getStaticInfoText());
		// AddClassToCollection("CompositorEffectEmbossedComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("CompositorEffectEmbossedComponent", "void setActivated(bool activated)", "Activates the given compositor effect.");
		AddClassToCollection("CompositorEffectEmbossedComponent", "bool isActivated()", "Gets whether the compositor effect is activated or not.");
		AddClassToCollection("CompositorEffectEmbossedComponent", "void setWeight(float weight)", "Sets the embossed weight.");
		AddClassToCollection("CompositorEffectEmbossedComponent", "float getWeight()", "Gets the embossed weight.");

		module(lua)
			[
				class_<CompositorEffectSharpenEdgesComponent, CompositorEffectBaseComponent>("CompositorEffectSharpenEdgesComponent")
				.def("setWeight", &CompositorEffectSharpenEdgesComponent::setWeight)
			.def("getWeight", &CompositorEffectSharpenEdgesComponent::getWeight)
			];

		AddClassToCollection("CompositorEffectSharpenEdgesComponent", "class inherits CompositorEffectBaseComponent", CompositorEffectSharpenEdgesComponent::getStaticInfoText());
		// AddClassToCollection("CompositorEffectSharpenEdgesComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("CompositorEffectSharpenEdgesComponent", "void setActivated(bool activated)", "Activates the given compositor effect.");
		AddClassToCollection("CompositorEffectSharpenEdgesComponent", "bool isActivated()", "Gets whether the compositor effect is activated or not.");
		AddClassToCollection("CompositorEffectSharpenEdgesComponent", "void setWeight(float weight)", "Sets the sharpen edges weight.");
		AddClassToCollection("CompositorEffectSharpenEdgesComponent", "float getWeight()", "Gets the sharpen edges weight.");


		module(lua)
			[
				class_<HdrEffectComponent, GameObjectComponent>("HdrEffectComponent")
				.def("setEffectName", &HdrEffectComponent::setEffectName)
			.def("getEffectName", &HdrEffectComponent::getEffectName)
			.def("setSkyColor", &HdrEffectComponent::setSkyColor)
			.def("getSkyColor", &HdrEffectComponent::getSkyColor)
			.def("setUpperHemisphere", &HdrEffectComponent::setUpperHemisphere)
			.def("getUpperHemisphere", &HdrEffectComponent::getUpperHemisphere)
			.def("setLowerHemisphere", &HdrEffectComponent::setLowerHemisphere)
			.def("getLowerHemisphere", &HdrEffectComponent::getLowerHemisphere)
			.def("setSunPower", &HdrEffectComponent::setSunPower)
			.def("getSunPower", &HdrEffectComponent::getSunPower)
			.def("setExposure", &HdrEffectComponent::setExposure)
			.def("getExposure", &HdrEffectComponent::getExposure)
			.def("setMinAutoExposure", &HdrEffectComponent::setMinAutoExposure)
			.def("getMinAutoExposure", &HdrEffectComponent::getMinAutoExposure)
			.def("setMaxAutoExposure", &HdrEffectComponent::setMaxAutoExposure)
			.def("getMaxAutoExposure", &HdrEffectComponent::getMaxAutoExposure)
			.def("setBloom", &HdrEffectComponent::setBloom)
			.def("getBloom", &HdrEffectComponent::getBloom)
			.def("setEnvMapScale", &HdrEffectComponent::setEnvMapScale)
			.def("getEnvMapScale", &HdrEffectComponent::getEnvMapScale)
			];

		AddClassToCollection("HdrEffectComponent", "class inherits GameObjectComponent", HdrEffectComponent::getStaticInfoText());
		// AddClassToCollection("HdrEffectComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("HdrEffectComponent", "void setEffectName(string effectName)", "Sets the hdr effect name. Possible values are: "
			"'Bright, sunny day', 'Scary Night', 'Average, slightly hazy day', 'Heavy overcast day', 'Gibbous moon night', 'JJ Abrams style', 'Custom'"
			"Note: If any value is manipulated manually, the effect name will be set to 'Custom'.");
		AddClassToCollection("HdrEffectComponent", "string getEffectName()", "Gets currently set effect name. Possible values are : "
			"'Bright, sunny day', 'Scary Night', 'Average, slightly hazy day', 'Heavy overcast day', 'Gibbous moon night', 'JJ Abrams style', 'Custom'"
			"Note: If any value is manipulated manually, the effect name will be set to 'Custom'.");

		AddClassToCollection("HdrEffectComponent", "void setSkyColor(Vector3 skyColor)", "Sets sky color.");
		AddClassToCollection("HdrEffectComponent", "Vector3 getSkyColor()", "Gets the sky color.");
		AddClassToCollection("HdrEffectComponent", "void setUpperHemisphere(Vector3 upperHemisphere)", "Sets the ambient color when the surface normal is close to hemisphereDir.");
		AddClassToCollection("HdrEffectComponent", "Vector3 getUpperHemisphere()", "Gets the upper hemisphere color.");
		AddClassToCollection("HdrEffectComponent", "void setLowerHemisphere(Vector3 lowerHemisphere)", "Sets the ambient color when the surface normal is pointing away from hemisphereDir.");
		AddClassToCollection("HdrEffectComponent", "Vector3 getLowerHemisphere()", "Gets the sky color.");
		AddClassToCollection("HdrEffectComponent", "void setSunPower(float sunPower)", "Sets the sun power scale. Note: This is applied on the 'SunLight'.");
		AddClassToCollection("HdrEffectComponent", "float getSunPower()", "Gets the sun power scale.");
		AddClassToCollection("HdrEffectComponent", "void setExposure(float exposure)", "Modifies the HDR Materials for the new exposure parameters. "
			"By default the HDR implementation will try to auto adjust the exposure based on the scene's average luminance. "
			"If left unbounded, even the darkest scenes can look well lit and the brigthest scenes appear too normal. "
			"These parameters are useful to prevent the auto exposure from jumping too much from one extreme to the otherand provide "
			"a consistent experience within the same lighting conditions. (e.g.you may want to change the params when going from indoors to outdoors)"
			"The smaller the gap between minAutoExposure & maxAutoExposure, the less the auto exposure tries to auto adjust to the scene's lighting conditions. "
			"The first value is exposure. Valid range is [-4.9; 4.9]. Low values will make the picture darker. Higher values will make the picture brighter.");
		AddClassToCollection("HdrEffectComponent", "float getExposure()", "Gets the exposure.");
		AddClassToCollection("HdrEffectComponent", "void setMinAutoExposure(float minAutoExposure)", "Sets the min auto exposure. Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure darkens a bright scene. "
			"To prevent that looking at a very bright object makes the rest of the scene really dark, use higher values.");
		AddClassToCollection("HdrEffectComponent", "float getMinAutoExposure()", "Gets the min auto exposure.");
		AddClassToCollection("HdrEffectComponent", "void setMaxAutoExposure(float maxAutoExposure)", "Sets max auto exposure. Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure brightens a dark scene. "
			"To prevent that looking at a very dark object makes the rest of the scene really bright, use lower values.");
		AddClassToCollection("HdrEffectComponent", "float getMaxAutoExposure()", "Gets max auto exposure.");
		AddClassToCollection("HdrEffectComponent", "void setBloom(float bloom)", "Sets the bloom intensity. Scale is in lumens / 1024. Valid range is [0.01; 4.9].");
		AddClassToCollection("HdrEffectComponent", "float getBloom()", "Gets bloom intensity.");
		AddClassToCollection("HdrEffectComponent", "void setEnvMapScale(float envMapScale)", "Sets enivornmental scale. Its a global scale that applies to all environment maps (for relevant Hlms implementations, "
			"like PBS). The value will be stored in upperHemisphere.a. Use 1.0 to disable.");
		AddClassToCollection("HdrEffectComponent", "float getEnvMapScale()", "Gets the environmental map scale.");
	}

	void bindDatablockComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<DatablockPbsComponent, GameObjectComponent>("DatablockPbsComponent")
				.enum_("TransparencyModes")
			[
				value("NONE", Ogre::HlmsPbsDatablock::None),
				value("TRANSPARENT", Ogre::HlmsPbsDatablock::Transparent),
				value("FADE", Ogre::HlmsPbsDatablock::Fade)
			]
		// .def("getClassName", &DatablockPbsComponent::getClassName)
		.def("getParentClassName", &DatablockPbsComponent::getParentClassName)
			.def("setSubEntityIndex", &DatablockPbsComponent::setSubEntityIndex)
			.def("getSubEntityIndex", &DatablockPbsComponent::getSubEntityIndex)

			.def("setWorkflow", &DatablockPbsComponent::setWorkflow)
			.def("getWorkflow", &DatablockPbsComponent::getWorkflow)
			.def("setMetalness", &DatablockPbsComponent::setMetalness)
			.def("getMetalness", &DatablockPbsComponent::getMetalness)
			.def("setRoughness", &DatablockPbsComponent::setRoughness)
			.def("getRoughness", &DatablockPbsComponent::getRoughness)
			.def("setFresnel", &DatablockPbsComponent::setFresnel)
			.def("getFresnel", &DatablockPbsComponent::getFresnel)
			.def("setIndexOfRefraction", &DatablockPbsComponent::setIndexOfRefraction)
			.def("getIndexOfRefraction", &DatablockPbsComponent::getIndexOfRefraction)
			.def("setRefractionStrength", &DatablockPbsComponent::setRefractionStrength)
			.def("getRefractionStrength", &DatablockPbsComponent::getRefractionStrength)
			.def("setBrdf", &DatablockPbsComponent::setBrdf)
			.def("getBrdf", &DatablockPbsComponent::getBrdf)
			.def("setTwoSidedLighting", &DatablockPbsComponent::setTwoSidedLighting)
			.def("getTwoSidedLighting", &DatablockPbsComponent::getTwoSidedLighting)
			.def("setOneSidedShadowCastCullingMode", &DatablockPbsComponent::setOneSidedShadowCastCullingMode)
			.def("getOneSidedShadowCastCullingMode", &DatablockPbsComponent::getOneSidedShadowCastCullingMode)
			.def("setAlphaTestThreshold", &DatablockPbsComponent::setAlphaTestThreshold)
			.def("getAlphaTestThreshold", &DatablockPbsComponent::getAlphaTestThreshold)
			.def("setReceiveShadows", &DatablockPbsComponent::setReceiveShadows)
			.def("getReceiveShadows", &DatablockPbsComponent::getReceiveShadows)

			.def("setDiffuseColor", &DatablockPbsComponent::setDiffuseColor)
			.def("getDiffuseColor", &DatablockPbsComponent::getDiffuseColor)
			.def("setBackgroundColor", &DatablockPbsComponent::setBackgroundColor)
			.def("getBackgroundColor", &DatablockPbsComponent::getBackgroundColor)
			.def("setSpecularColor", &DatablockPbsComponent::setSpecularColor)
			.def("getSpecularColor", &DatablockPbsComponent::getSpecularColor)
			.def("setEmissiveColor", &DatablockPbsComponent::setEmissiveColor)
			.def("getEmissiveColor", &DatablockPbsComponent::getEmissiveColor)
			.def("setDiffuseTextureName", &DatablockPbsComponent::setDiffuseTextureName)
			.def("getDiffuseTextureName", &DatablockPbsComponent::getDiffuseTextureName)
			.def("setNormalTextureName", &DatablockPbsComponent::setNormalTextureName)
			.def("getNormalTextureName", &DatablockPbsComponent::getNormalTextureName)
			.def("setNormalMapWeight", &DatablockPbsComponent::setNormalMapWeight)
			.def("getNormalMapWeight", &DatablockPbsComponent::getNormalMapWeight)
			.def("setClearCoat", &DatablockPbsComponent::setClearCoat)
			.def("getClearCoat", &DatablockPbsComponent::getClearCoat)
			.def("setClearCoatRoughness", &DatablockPbsComponent::setClearCoatRoughness)
			.def("getClearCoatRoughness", &DatablockPbsComponent::getClearCoatRoughness)

			.def("setSpecularTextureName", &DatablockPbsComponent::setSpecularTextureName)
			.def("getSpecularTextureName", &DatablockPbsComponent::getSpecularTextureName)
			.def("setMetallicTextureName", &DatablockPbsComponent::setMetallicTextureName)
			.def("getMetallicTextureName", &DatablockPbsComponent::getMetallicTextureName)
			.def("setRoughnessTextureName", &DatablockPbsComponent::setRoughnessTextureName)
			.def("getRoughnessTextureName", &DatablockPbsComponent::getRoughnessTextureName)
			.def("setDetailWeightTextureName", &DatablockPbsComponent::setDetailWeightTextureName)
			.def("getDetailWeightTextureName", &DatablockPbsComponent::getDetailWeightTextureName)
			.def("setDetail0TextureName", &DatablockPbsComponent::setDetail0TextureName)
			.def("getDetail0TextureName", &DatablockPbsComponent::getDetail0TextureName)
			.def("setBlendMode0", &DatablockPbsComponent::setBlendMode0)
			.def("getBlendMode0", &DatablockPbsComponent::getBlendMode0)
			.def("setDetail1TextureName", &DatablockPbsComponent::setDetail1TextureName)
			.def("getDetail1TextureName", &DatablockPbsComponent::getDetail1TextureName)
			.def("setBlendMode1", &DatablockPbsComponent::setBlendMode1)
			.def("getBlendMode1", &DatablockPbsComponent::getBlendMode1)
			.def("setDetail2TextureName", &DatablockPbsComponent::setDetail2TextureName)
			.def("getDetail2TextureName", &DatablockPbsComponent::getDetail2TextureName)
			.def("setBlendMode2", &DatablockPbsComponent::setBlendMode2)
			.def("getBlendMode2", &DatablockPbsComponent::getBlendMode2)
			.def("setDetail3TextureName", &DatablockPbsComponent::setDetail3TextureName)
			.def("getDetail3TextureName", &DatablockPbsComponent::getDetail3TextureName)
			.def("setBlendMode3", &DatablockPbsComponent::setBlendMode3)
			.def("getBlendMode3", &DatablockPbsComponent::getBlendMode3)
			.def("setDetail0NMTextureName", &DatablockPbsComponent::setDetail0NMTextureName)
			.def("getDetail0NMTextureName", &DatablockPbsComponent::getDetail0NMTextureName)
			.def("setDetail1NMTextureName", &DatablockPbsComponent::setDetail1NMTextureName)
			.def("getDetail1NMTextureName", &DatablockPbsComponent::getDetail1NMTextureName)
			.def("setDetail2NMTextureName", &DatablockPbsComponent::setDetail2NMTextureName)
			.def("getDetail2NMTextureName", &DatablockPbsComponent::getDetail2NMTextureName)
			.def("setDetail3NMTextureName", &DatablockPbsComponent::setDetail3NMTextureName)
			.def("getDetail3NMTextureName", &DatablockPbsComponent::getDetail3NMTextureName)
			.def("setReflectionTextureName", &DatablockPbsComponent::setReflectionTextureName)
			.def("getReflectionTextureName", &DatablockPbsComponent::getReflectionTextureName)
			.def("setEmissiveTextureName", &DatablockPbsComponent::setEmissiveTextureName)
			.def("getEmissiveTextureName", &DatablockPbsComponent::getEmissiveTextureName)
			.def("setUseEmissiveAsLightMap", &DatablockPbsComponent::setUseEmissiveAsLightMap)
			.def("getUseEmissiveAsLightMap", &DatablockPbsComponent::getUseEmissiveAsLightMap)
			.def("setTransparencyMode", &DatablockPbsComponent::setTransparencyMode)
			.def("getTransparencyMode", &DatablockPbsComponent::getTransparencyMode)
			.def("setTransparency", &DatablockPbsComponent::setTransparency)
			.def("getTransparency", &DatablockPbsComponent::getTransparency)
			.def("setUseAlphaFromTextures", &DatablockPbsComponent::setUseAlphaFromTextures)
			.def("getUseAlphaFromTextures", &DatablockPbsComponent::getUseAlphaFromTextures)
			];

		AddClassToCollection("DatablockPbsComponent", "class inherits GameObjectComponent", DatablockPbsComponent::getStaticInfoText());
		// AddClassToCollection("DatablockPbsComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("DatablockPbsComponent", "void setSubEntityIndex(int index)", "Sets the sub entity index, this data block should be used for.");
		AddClassToCollection("DatablockPbsComponent", "int getSubEntityIndex()", "Gets the sub entity index this data block is used for.");
		AddClassToCollection("DatablockPbsComponent", "void setWorkflow(String workflow)", "Sets the data block work flow. Possible values are: 'SpecularWorkflow', 'SpecularAsFresnelWorkflow', 'MetallicWorkflow'");
		AddClassToCollection("DatablockPbsComponent", "float getWorkflow()", "Gets the data block work flow.");
		AddClassToCollection("DatablockPbsComponent", "void setMetalness(float metalness)", "Sets the metalness in a metallic workflow. Overrides any fresnel value. Only visible/usable when metallic workflow is active. Range: [0; 1].");
		AddClassToCollection("DatablockPbsComponent", "float getMetalness()", "Gets the metalness value.");
		AddClassToCollection("DatablockPbsComponent", "void setRoughness(float roughness)", "Sets roughness value. Range: [0; 1].");
		AddClassToCollection("DatablockPbsComponent", "float getRoughness()", "Gets the roughness value.");
		AddClassToCollection("DatablockPbsComponent", "void setFresnel(Vector3 fresnel, bool separateFresnel)", "Sets the fresnel (F0) directly. The fresnel of the material for each color component. When separateFresnel = false, the Y and Z components are ignored. Note: Only visible/usable when fresnel workflow is active.");
		AddClassToCollection("DatablockPbsComponent", "Vector4 getFresnel()", "Gets the fresnel values (Vector3 for fresnel color and bool if fresnes is separated).");
		AddClassToCollection("DatablockPbsComponent", "void setIndexOfRefraction(Vector3 colorIndex, bool separateFresnel)", "Calculates fresnel (F0 in most books) based on the IOR. Note: If 'separateFresnel' was different from the current setting, it will call flushRenderables. "
			"If the another shader must be created, it could cause a stall. The index of refraction of the material for each colour component. When separateFresnel = false, the Y and Z components are ignored. Whether to use the same fresnel term for RGB channel, or individual ones.");
		AddClassToCollection("DatablockPbsComponent", "Vector4 getIndexOfRefraction()", "Gets the fresnel values (Vector3 for fresnel color and bool if fresnes is separated).");
		AddClassToCollection("DatablockPbsComponent", "void setRefractionStrength(float refractionStrength)", "Sets the strength of the refraction, i.e. how much displacement in screen space. Note: This value is not physically based. Only used when 'setTransparency' was set to HlmsPbsDatablock::Refractive");
		AddClassToCollection("DatablockPbsComponent", "float getRefractionStrength()", "Gets the roughness value..");
		AddClassToCollection("DatablockPbsComponent", "void setBrdf(String brdf)", "Sets brdf mode. Possible values are: Default (Most physically accurate BRDF performance heavy), CookTorrance (Ideal for silk (use high roughness values), synthetic fabric), "
			"BlinnPhong (performance. It's cheaper, while still looking somewhat similar to Default), DefaultUncorrelated (auses edges to be dimmer and is less correct, like in Unity), "
			"DefaultSeparateDiffuseFresnel (for complex refractions and reflections like glass, transparent plastics, fur, and surface with refractions and multiple rescattering, achieve a perfect mirror effect, raise the fresnel term to 1 and the diffuse color to black), "
			"BlinnPhongSeparateDiffuseFresnel (like DefaultSeparateDiffuseFresnel, but uses BlinnPhong as base), BlinnPhongLegacyMath (looks more like a 2000-2005's game, Ignores fresnel completely)");
		AddClassToCollection("DatablockPbsComponent", "String getBrdf()", "Gets the currently used brdf model.");
		AddClassToCollection("DatablockPbsComponent", "void setTwoSidedLighting(bool twoSidedLighting)", "Allows support for two sided lighting. Disabled by default (faster)");
		AddClassToCollection("DatablockPbsComponent", "bool getTwoSidedLighting()", "Gets whether two sided lighting is used.");
		AddClassToCollection("DatablockPbsComponent", "void setOneSidedShadowCastCullingMode(String cullingMode)", "Sets the one sided shadow cast culling mode. While CULL_NONE is usually the 'correct' option, setting CULL_ANTICLOCKWISE can prevent ugly self-shadowing on interiors.");
		AddClassToCollection("DatablockPbsComponent", "String getOneSidedShadowCastCullingMode()", "Gets the currently used one sided shadow cast culling mode.");
		AddClassToCollection("DatablockPbsComponent", "void setAlphaTestThreshold(float threshold)", "Sets threshold for alpha. Alpha testing works on the alpha channel of the diffuse texture. If there is no diffuse texture, the first diffuse detail map after.");
		AddClassToCollection("DatablockPbsComponent", "float getAlphaTestThreshold()", "Gets the alpha threshold.");
		AddClassToCollection("DatablockPbsComponent", "void setReceiveShadows(bool receiveShadows)", "Sets whether the model of this data block should receive shadows or not.");
		AddClassToCollection("DatablockPbsComponent", "bool getReceiveShadows()", "Gets whether the model of this data block should receive shadows or not.");
		AddClassToCollection("DatablockPbsComponent", "void setDiffuseColor(Vector3 diffuseColor)", "Sets the diffuse color (final multiplier). The color will be divided by PI for energy conservation.");
		AddClassToCollection("DatablockPbsComponent", "Vector3 getDiffuseColor()", "Gets the diffuse color.");
		AddClassToCollection("DatablockPbsComponent", "void setBackgroundColor(Vector3 backgroundColor)", "Sets the diffuse background color. When no diffuse texture is present, this solid color replaces it, and can act as a background for the detail maps.");
		AddClassToCollection("DatablockPbsComponent", "Vector3 getBackgroundColor()", "Gets the background color.");
		AddClassToCollection("DatablockPbsComponent", "void setSpecularColor(Vector3 specularColor)", "Sets the specular color.");
		AddClassToCollection("DatablockPbsComponent", "Vector3 getSpecularColor()", "Gets the specular color.");
		AddClassToCollection("DatablockPbsComponent", "void setEmissiveColor(Vector3 emissiveColor)", "Sets the emissive color.");
		AddClassToCollection("DatablockPbsComponent", "Vector3 getEmissiveColor()", "Gets the emissive color.");
		AddClassToCollection("DatablockPbsComponent", "void setDiffuseTextureName(String diffuseTextureName)", "Sets the diffuse texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDiffuseTextureName()", "Gets the diffuse texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setNormalTextureName(String normalTextureName)", "Sets the normal texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getNormalTextureName()", "Gets the normal texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setNormalMapWeight(float normalMapWeight)", "Sets the normal mapping weight. Note: A value of 0 means no effect (tangent space normal is 0, 0, 1); and would be. A value of 1 means full normal map effect. A value outside the [0; 1] range extrapolates.");
		AddClassToCollection("DatablockPbsComponent", "float getNormalMapWeight()", "Gets the normal map weight.");
		AddClassToCollection("DatablockPbsComponent", "void setClearCoat(float clearCoat)", "Sets the strength of the of the clear coat layer. Note: This should be treated as a binary value, set to either 0 or 1. Intermediate values are useful to control transitions between parts of the surface that have a clear coat layers and parts that don't.");
		AddClassToCollection("DatablockPbsComponent", "float getClearCoat()", "Gets clear coat value.");
		AddClassToCollection("DatablockPbsComponent", "void setClearCoatRoughness(float clearCoatRoughness)", "Sets the roughness of the of the clear coat layer.");
		AddClassToCollection("DatablockPbsComponent", "float getClearCoatRoughness()", "Gets clear coat roughness.");
		AddClassToCollection("DatablockPbsComponent", "void setSpecularTextureName(String specularTextureName)", "Sets the specular texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getSpecularTextureName()", "Gets the specular texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setMetallicTextureName(String metallicTextureName)", "Sets the metallic texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getMetallicTextureName()", "Gets the metallic texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setRoughnessTextureName(String roughnessTextureName)", "Sets the roughness texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getRoughnessTextureName()", "Gets the roughness texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setDetailWeightTextureName(String detailWeightTextureName)", "Sets the detail weight texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDetailWeightTextureName()", "Gets the detail weight texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setDetail0TextureName(String detail0TextureName)", "Sets the detail 0 texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDetail0TextureName()", "Gets the detail 0 texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setBlendMode0(String blendMode0)", "Sets the blend mode 0. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL'"
			"'PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN'"
			"'PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'");
		AddClassToCollection("DatablockPbsComponent", "String getBlendMode0()", "Gets the blend mode 0.");
		AddClassToCollection("DatablockPbsComponent", "void setDetail1TextureName(String detail1TextureName)", "Sets the detail 1 texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDetail1TextureName()", "Gets the detail 1 texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setBlendMode1(String blendMode1)", "Sets the blend mode 1. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL'"
			"'PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN'"
			"'PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'");
		AddClassToCollection("DatablockPbsComponent", "String getBlendMode1()", "Gets the blend mode 1.");
		AddClassToCollection("DatablockPbsComponent", "void setDetail2TextureName(String detail2TextureName)", "Sets the detail 2 texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDetail2TextureName()", "Gets the detail 2 texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setBlendMode2(String blendMode2)", "Sets the blend mode 2. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL'"
			"'PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN'"
			"'PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'");
		AddClassToCollection("DatablockPbsComponent", "String getBlendMode2()", "Gets the blend mode 2.");
		AddClassToCollection("DatablockPbsComponent", "void setDetail3TextureName(String detail3TextureName)", "Sets the detail 3 texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDetail3TextureName()", "Gets the detail 3 texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setBlendMode3(String blendMode3)", "Sets the blend mode 3. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL'"
			"'PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN'"
			"'PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'");
		AddClassToCollection("DatablockPbsComponent", "String getBlendMode3()", "Gets the blend mode 3.");
		AddClassToCollection("DatablockPbsComponent", "void setDetail0NMTextureName(String detail0NMTextureName)", "Sets the detail 0 normal map texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDetail0NMTextureName()", "Gets the detail 0 normal map texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setDetail1NMTextureName(String detail1NMTextureName)", "Sets the detail 1 normal map texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDetail1NMTextureName()", "Gets the detail 1 normal map texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setDetail2NMTextureName(String detail2NMTextureName)", "Sets the detail 2 normal map texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDetail2NMTextureName()", "Gets the detail 2 normal map texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setDetail3NMTextureName(String detail3NMTextureName)", "Sets the detail 3 normal map texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getDetail3NMTextureName()", "Gets the detail 3 normal map texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setReflectionTextureName(String reflectionTextureName)", "Sets the reflection texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getReflectionTextureName()", "Gets the reflection texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setEmissiveTextureName(String emissiveTextureName)", "Sets the emissive texture name.");
		AddClassToCollection("DatablockPbsComponent", "String getEmissiveTextureName()", "Gets the emissive texture name.");
		AddClassToCollection("DatablockPbsComponent", "void setTransparencyMode(String transparencyMode)", "Sets the transparency mode. Possible values are: 'Transparent', 'Fade'");
		AddClassToCollection("DatablockPbsComponent", "String getTransparencyMode()", "Gets the transparency mode.");
		AddClassToCollection("DatablockPbsComponent", "void setTransparency(float transparency)", "Makes the material transparent, and sets the amount of transparency. Note: Value in range [0; 1] where 0 = full transparency and 1 = fully opaque");
		AddClassToCollection("DatablockPbsComponent", "float getTransparency()", "Gets the transparency value for the material.");
		AddClassToCollection("DatablockPbsComponent", "void setUseAlphaFromTextures(bool useAlphaFromTextures)", "Sets whether to use transparency from textures or not. "
			"When false, the alpha channel of the diffuse maps and detail maps will be ignored. It's a GPU performance optimization.");
		AddClassToCollection("DatablockPbsComponent", "bool getUseAlphaFromTextures()", "Gets whether to use transparency from textures or not.");

		AddClassToCollection("DatablockPbsComponent", "void setUseEmissiveAsLightMap(bool useEmissiveAsLightMap)", "When set, it treats the emissive map as a lightmap; which means it will be multiplied against the diffuse component.");
		AddClassToCollection("DatablockPbsComponent", "bool getUseEmissiveAsLightMap()", "Gets whether to use the emissive as light map.");

		module(lua)
			[
				class_<DatablockTerraComponent, GameObjectComponent>("DatablockTerraComponent")
				// .def("getClassName", &DatablockTerraComponent::getClassName)
			.def("getParentClassName", &DatablockTerraComponent::getParentClassName)

			.def("setDiffuseColor", &DatablockTerraComponent::setDiffuseColor)
			.def("getDiffuseColor", &DatablockTerraComponent::getDiffuseColor)
			.def("setDiffuseTextureName", &DatablockTerraComponent::setDiffuseTextureName)
			.def("getDiffuseTextureName", &DatablockTerraComponent::getDiffuseTextureName)
			.def("setDetail0TextureName", &DatablockTerraComponent::setDetail0TextureName)
			.def("getDetail0TextureName", &DatablockTerraComponent::getDetail0TextureName)
			/*.def("setBlendMode0", &DatablockTerraComponent::setBlendMode0)
			.def("getBlendMode0", &DatablockTerraComponent::getBlendMode0)*/
			.def("setDetail1TextureName", &DatablockTerraComponent::setDetail1TextureName)
			.def("getDetail1TextureName", &DatablockTerraComponent::getDetail1TextureName)
			//.def("setBlendMode1", &DatablockTerraComponent::setBlendMode1)
			//.def("getBlendMode1", &DatablockTerraComponent::getBlendMode1)
			.def("setDetail2TextureName", &DatablockTerraComponent::setDetail2TextureName)
			.def("getDetail2TextureName", &DatablockTerraComponent::getDetail2TextureName)
			//.def("setBlendMode2", &DatablockTerraComponent::setBlendMode2)
			//.def("getBlendMode2", &DatablockTerraComponent::getBlendMode2)
			.def("setDetail3TextureName", &DatablockTerraComponent::setDetail3TextureName)
			.def("getDetail3TextureName", &DatablockTerraComponent::getDetail3TextureName)
			//.def("setBlendMode3", &DatablockTerraComponent::setBlendMode3)
			//.def("getBlendMode3", &DatablockTerraComponent::getBlendMode3)
			.def("setDetail0NMTextureName", &DatablockTerraComponent::setDetail0NMTextureName)
			.def("getDetail0NMTextureName", &DatablockTerraComponent::getDetail0NMTextureName)
			.def("setDetail1NMTextureName", &DatablockTerraComponent::setDetail1NMTextureName)
			.def("getDetail1NMTextureName", &DatablockTerraComponent::getDetail1NMTextureName)
			.def("setDetail2NMTextureName", &DatablockTerraComponent::setDetail2NMTextureName)
			.def("getDetail2NMTextureName", &DatablockTerraComponent::getDetail2NMTextureName)
			.def("setDetail3NMTextureName", &DatablockTerraComponent::setDetail3NMTextureName)
			.def("getDetail3NMTextureName", &DatablockTerraComponent::getDetail3NMTextureName)
			.def("setDetail0RTextureName", &DatablockTerraComponent::setDetail0RTextureName)
			.def("getDetail0RTextureName", &DatablockTerraComponent::getDetail0RTextureName)
			.def("setDetail1RTextureName", &DatablockTerraComponent::setDetail1RTextureName)
			.def("getDetail1RTextureName", &DatablockTerraComponent::getDetail1RTextureName)
			.def("setDetail2RTextureName", &DatablockTerraComponent::setDetail2RTextureName)
			.def("getDetail2RTextureName", &DatablockTerraComponent::getDetail2RTextureName)
			.def("setDetail3RTextureName", &DatablockTerraComponent::setDetail3RTextureName)
			.def("getDetail3RTextureName", &DatablockTerraComponent::getDetail3RTextureName)
			.def("setDetail0MTextureName", &DatablockTerraComponent::setDetail0MTextureName)
			.def("getDetail0MTextureName", &DatablockTerraComponent::getDetail0MTextureName)
			.def("setDetail1MTextureName", &DatablockTerraComponent::setDetail1MTextureName)
			.def("getDetail1MTextureName", &DatablockTerraComponent::getDetail1MTextureName)
			.def("setDetail2MTextureName", &DatablockTerraComponent::setDetail2MTextureName)
			.def("getDetail2MTextureName", &DatablockTerraComponent::getDetail2MTextureName)
			.def("setDetail3MTextureName", &DatablockTerraComponent::setDetail3MTextureName)
			.def("getDetail3MTextureName", &DatablockTerraComponent::getDetail3MTextureName)
			.def("setReflectionTextureName", &DatablockTerraComponent::setReflectionTextureName)
			.def("getReflectionTextureName", &DatablockTerraComponent::getReflectionTextureName)
			];

		AddClassToCollection("DatablockTerraComponent", "class inherits GameObjectComponent", DatablockTerraComponent::getStaticInfoText());
		// AddClassToCollection("DatablockTerraComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("DatablockTerraComponent", "void setDiffuseColor(Vector3 diffuseColor)", "Sets the diffuse color (final multiplier). The color will be divided by PI for energy conservation.");
		AddClassToCollection("DatablockTerraComponent", "Vector3 getDiffuseColor()", "Gets the diffuse color.");
		AddClassToCollection("DatablockTerraComponent", "void setDiffuseTextureName(String diffuseTextureName)", "Sets the diffuse texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDiffuseTextureName()", "Gets the diffuse texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetailWeightTextureName(String detailWeightTextureName)", "Sets the detail weight texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetailWeightTextureName()", "Gets the detail weight texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail0TextureName(String detail0TextureName)", "Sets the detail 0 texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail0TextureName()", "Gets the detail 0 texture name.");
		// AddClassToCollection("DatablockTerraComponent", "void setBlendMode0(String blendMode0)", "Sets the blend mode 0. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL'"
		//   					"'PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN'"
		// 					"'PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'");
		AddClassToCollection("DatablockTerraComponent", "String getBlendMode0()", "Gets the blend mode 0.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail1TextureName(String detail1TextureName)", "Sets the detail 1 texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail1TextureName()", "Gets the detail 1 texture name.");
		// AddClassToCollection("DatablockTerraComponent", "void setBlendMode1(String blendMode1)", "Sets the blend mode 1. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL'"
		//   					"'PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN'"
		// 					"'PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'");
		AddClassToCollection("DatablockTerraComponent", "String getBlendMode1()", "Gets the blend mode 1.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail2TextureName(String detail2TextureName)", "Sets the detail 2 texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail2TextureName()", "Gets the detail 2 texture name.");
		// AddClassToCollection("DatablockTerraComponent", "void setBlendMode2(String blendMode2)", "Sets the blend mode 2. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL'"
		//   					"'PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN'"
		// 					"'PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'");
		AddClassToCollection("DatablockTerraComponent", "String getBlendMode2()", "Gets the blend mode 2.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail3TextureName(String detail3TextureName)", "Sets the detail 3 texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail3TextureName()", "Gets the detail 3 texture name.");
		// AddClassToCollection("DatablockTerraComponent", "void setBlendMode3(String blendMode3)", "Sets the blend mode 3. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL'"
		//   					"'PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN'"
		// 					"'PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'");
		AddClassToCollection("DatablockTerraComponent", "String getBlendMode3()", "Gets the blend mode 3.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail0NMTextureName(String detail0NMTextureName)", "Sets the detail 0 normal map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail0NMTextureName()", "Gets the detail 0 normal map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail1NMTextureName(String detail1NMTextureName)", "Sets the detail 1 normal map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail1NMTextureName()", "Gets the detail 1 normal map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail2NMTextureName(String detail2NMTextureName)", "Sets the detail 2 normal map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail2NMTextureName()", "Gets the detail 2 normal map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail3NMTextureName(String detail3NMTextureName)", "Sets the detail 3 normal map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail3NMTextureName()", "Gets the detail 3 normal map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail0RTextureName(String detail0RTextureName)", "Sets the detail 0 roughness map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail0RTextureName()", "Gets the detail 0 roughness map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail1RTextureName(String detail1RTextureName)", "Sets the detail 1 roughness map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail1RTextureName()", "Gets the detail 1 roughness map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail2RTextureName(String detail2RTextureName)", "Sets the detail 2 roughness map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail2RTextureName()", "Gets the detail 2 roughness map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail3RTextureName(String detail3RTextureName)", "Sets the detail 3 roughness map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail3RTextureName()", "Gets the detail 3 roughness map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail0MTextureName(String detail0MTextureName)", "Sets the detail 0 metallic map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail0MTextureName()", "Gets the detail 0 metallic map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail1MTextureName(String detail1MTextureName)", "Sets the detail 1 metallic map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail1MTextureName()", "Gets the detail 1 metallic map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail2MTextureName(String detail2MTextureName)", "Sets the detail 2 metallic map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail2MTextureName()", "Gets the detail 2 metallic map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setDetail3MTextureName(String detail3MTextureName)", "Sets the detail 3 metallic map texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getDetail3MTextureName()", "Gets the detail 3 metallic map texture name.");
		AddClassToCollection("DatablockTerraComponent", "void setReflectionTextureName(String reflectionTextureName)", "Sets the reflection texture name.");
		AddClassToCollection("DatablockTerraComponent", "String getReflectionTextureName()", "Gets the reflection texture name.");

		module(lua)
			[
				class_<DatablockUnlitComponent, GameObjectComponent>("DatablockUnlitComponent")
				// .def("getClassName", &DatablockUnlitComponent::getClassName)
			.def("getParentClassName", &DatablockUnlitComponent::getParentClassName)
			.def("setSubEntityIndex", &DatablockUnlitComponent::setSubEntityIndex)
			.def("getSubEntityIndex", &DatablockUnlitComponent::getSubEntityIndex)
			.def("setBackgroundColor", &DatablockUnlitComponent::setBackgroundColor)
			.def("getBackgroundColor", &DatablockUnlitComponent::getBackgroundColor)
			];

		AddClassToCollection("DatablockUnlitComponent", "class inherits GameObjectComponent", DatablockUnlitComponent::getStaticInfoText());
		// AddClassToCollection("DatablockUnlitComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("DatablockUnlitComponent", "void setSubEntityIndex(int index)", "Sets the sub entity index, this data block should be used for.");
		AddClassToCollection("DatablockUnlitComponent", "int getSubEntityIndex()", "Gets the sub entity index this data block is used for.");
		AddClassToCollection("DatablockUnlitComponent", "void setBackgroundColor(Vector3 backgroundColor)", "Sets the diffuse background color. When no diffuse texture is present, this solid color replaces it, and can act as a background for the detail maps.");
		AddClassToCollection("DatablockUnlitComponent", "Vector3 getBackgroundColor()", "Gets the background color.");
	}

	Ogre::String getId2(JointComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getId());
	}

	void setPredecessorId(JointComponent* instance, const Ogre::String& id)
	{
		instance->setPredecessorId(Ogre::StringConverter::parseUnsignedLong(id));
	}

	Ogre::String getPredecessorId(JointComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getPredecessorId());
	}

	void setTargetId2(JointComponent* instance, const Ogre::String& id)
	{
		instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(id));
	}

	Ogre::String getTargetId2(JointComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getTargetId());
	}

	luabind::object getMaxLinearAngleFriction(JointKinematicComponent* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		Ogre::Real linearFriction = 0.0f;
		Ogre::Real angleFriction = 0.0f;

		auto result = instance->getMaxLinearAngleFriction();

		obj[0] = std::get<0>(result);
		obj[1] = std::get<1>(result);;

		return obj;
	}

	void bindJointHandlers(lua_State* lua)
	{
		module(lua)
			[
				class_<JointComponent, GameObjectComponent>("JointComponent")
				// // .def("clone", &JointComponent::clone)
			.def("createJoint", &JointComponent::createJoint)
			.def("setActivated", &JointComponent::setActivated)
			// .def("getId", &JointComponent::getId)
			.def("getId", &getId2)
			.def("getType", &JointComponent::getType)
			// .def("getClassName", &JointComponent::getClassName)
			.def("getParentClassName", &JointComponent::getParentClassName)
			// .def("setPredecessorId", &JointComponent::setPredecessorId)
			// .def("getPredecessorId", &JointComponent::getPredecessorId)
			.def("setPredecessorId", &setPredecessorId)
			.def("getPredecessorId", &getPredecessorId)
			// .def("setTargetId", &JointComponent::setTargetId)
			// .def("getTargetId", &JointComponent::getTargetId)
			.def("setTargetId", &setTargetId2)
			.def("getTargetId", &getTargetId2)
			.def("getJoint", &JointComponent::getJoint)
			.def("getPredecessorJointComponent", &getPredecessorJointComponent)
			.def("getJointPosition", &JointComponent::getJointPosition)
			.def("setJointRecursiveCollisionEnabled", &JointComponent::setJointRecursiveCollisionEnabled)
			.def("getJointRecursiveCollisionEnabled", &JointComponent::getJointRecursiveCollisionEnabled)
			.def("setBodyMassScale", &JointComponent::setBodyMassScale)
			.def("getBodyMassScale", &JointComponent::getBodyMassScale)
			.def("releaseJoint", &JointComponent::releaseJoint)
			.def("showDebugData", &JointComponent::showDebugData)
			];

		AddClassToCollection("JointComponent", "class inherits GameObjectComponent", JointComponent::getStaticInfoText());
		AddClassToCollection("JointComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointComponent", "String getName()", "Gets the name of this joint.");
		AddClassToCollection("JointComponent", "String getType()", "Gets the type of this joint.");
		// AddClassToCollection("JointComponent", "String getClassName()", "Gets the class name of this component as string.");
		AddClassToCollection("JointComponent", "JointComponent getPredecessorJointComponent()", "Gets the predecessor joint component.");
		AddClassToCollection("JointComponent", "Vector3 getJointPosition()", "Gets the position of this joint.");
		AddClassToCollection("JointComponent", "void setJointRecursiveCollisionEnabled(bool enabled)", "Sets whether this joint should collide with its predecessors or not.");
		AddClassToCollection("JointComponent", "bool getJointRecursiveCollisionEnabled()", "Gets whether this joint should collide with its predecessors or not.");
		AddClassToCollection("JointComponent", "void releaseJoint()", "Releases this joint, so that it is no more connected with its predecessors and the joint is deleted. "
			"Note: This function is dangerous and may cause a crash, because an inconsistent behavior may happen.");

		module(lua)
			[
				class_<JointHingeComponent, JointComponent>("JointHingeComponent")
				// // .def("clone", &JointHingeComponent::clone)
			.def("setAnchorPosition", &JointHingeComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointHingeComponent::getAnchorPosition)
			.def("setPin", &JointHingeComponent::setPin)
			.def("getPin", &JointHingeComponent::getPin)
			.def("setLimitsEnabled", &JointHingeComponent::setLimitsEnabled)
			.def("getLimitsEnabled", &JointHingeComponent::getLimitsEnabled)
			.def("setMinMaxAngleLimit", &JointHingeComponent::setMinMaxAngleLimit)
			.def("getMinAngleLimit", &JointHingeComponent::getMinAngleLimit)
			.def("getMaxAngleLimit", &JointHingeComponent::getMaxAngleLimit)
			.def("setTorgue", &JointHingeComponent::setTorgue)
			.def("getTorgue", &JointHingeComponent::getTorgue)
			.def("setFriction", &JointHingeComponent::setFriction)
			.def("setSpring", (void (JointHingeComponent::*)(bool, bool)) & JointHingeComponent::setSpring)
			.def("setSpring", (void (JointHingeComponent::*)(bool, bool, Ogre::Real, Ogre::Real, Ogre::Real)) & JointHingeComponent::setSpring)

			// .def("getSpring", &JointHingeComponent::getSpring)
			.def("setBreakForce", &JointHingeComponent::setBreakForce)
			.def("getBreakForce", &JointHingeComponent::getBreakForce)
			.def("setBreakTorque", &JointHingeComponent::setBreakTorque)
			.def("getBreakTorque", &JointHingeComponent::getBreakTorque)
			];

		AddClassToCollection("JointHingeComponent", "class inherits JointComponent", JointHingeComponent::getStaticInfoText());
		AddClassToCollection("JointHingeComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointHingeComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointHingeComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointHingeComponent", "void setPin(Vector3 pin)", "Sets pin axis for the joint, to rotate around one dimension prependicular to the axis.");
		AddClassToCollection("JointHingeComponent", "Vector3 getPin()", "Gets joint pin axis.");
		AddClassToCollection("JointHingeComponent", "void setLimitsEnabled(bool enabled)", "Sets whether this joint should be limited by its rotation.");
		AddClassToCollection("JointHingeComponent", "bool getLimitsEnabled()", "Gets whether this joint is limited by its rotation.");
		AddClassToCollection("JointHingeComponent", "void setMinMaxAngleLimit(Degree minAngleLimit, Degree minAngleLimit)", "Sets the min and max angle limit in degree. Note: This will only be applied if 'setLimitsEnabled' is set to 'true'.");
		AddClassToCollection("JointHingeComponent", "Degree getMinAngleLimit()", "Gets the min angle limit.");
		AddClassToCollection("JointHingeComponent", "Degree getMaxAngleLimit()", "Gets the max angle limit.");
		AddClassToCollection("JointHingeComponent", "void setTorgue(float torque)", "Sets the torque rotation for this joint. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.");
		AddClassToCollection("JointHingeComponent", "void setFriction(float maxTorque)", "Sets the friction (max. torque) for torque. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.");
		AddClassToCollection("JointHingeComponent", "float getTorgue()", "Gets the torque rotation for this joint.");
		AddClassToCollection("JointHingeComponent", "void setSpring(bool asSpringDamper, bool massIndependent)", "Activates the spring with currently set spring values.");
		AddClassToCollection("JointHingeComponent", "void setSpring(bool asSpringDamper, bool massIndependent, float springDamperRelaxation, float springK, float springD)", "Sets the spring values for this joint. Note: When 'asSpringDamper' is activated the joint will use spring values for rotation.");
		AddClassToCollection("JointHingeComponent", "void setBreakForce(float breakForce)", "Sets the force, that is required to break this joint, so that the go will no longer use the hinge joint.");
		AddClassToCollection("JointHingeComponent", "float getBreakForce()", "Gets break force that is required to break this joint, so that the go will no longer use the hinge joint.");
		AddClassToCollection("JointHingeComponent", "void setBreakTorque(float breakForce)", "Sets the torque, that is required to break this joint, so that the go will no longer use the hinge joint.");
		AddClassToCollection("JointHingeComponent", "float getBreakTorque()", "Gets break torque that is required to break this joint, so that the go will no longer use the hinge joint.");

		module(lua)
			[
				class_<JointHingeActuatorComponent, JointComponent>("JointHingeActuatorComponent")
				// // .def("clone", &JointHingeActuatorComponent::clone)
			.def("setAnchorPosition", &JointHingeActuatorComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointHingeActuatorComponent::getAnchorPosition)
			.def("setPin", &JointHingeActuatorComponent::setPin)
			.def("getPin", &JointHingeActuatorComponent::getPin)
			.def("setTargetAngle", &JointHingeActuatorComponent::setTargetAngle)
			.def("getTargetAngle", &JointHingeActuatorComponent::getTargetAngle)
			.def("setAngleRate", &JointHingeActuatorComponent::setAngleRate)
			.def("getAngleRate", &JointHingeActuatorComponent::getAngleRate)
			.def("setMinAngleLimit", &JointHingeActuatorComponent::setMinAngleLimit)
			.def("getMinAngleLimit", &JointHingeActuatorComponent::getMinAngleLimit)
			.def("setMaxAngleLimit", &JointHingeActuatorComponent::setMaxAngleLimit)
			.def("getMaxAngleLimit", &JointHingeActuatorComponent::getMaxAngleLimit)
			.def("setMaxTorque", &JointHingeActuatorComponent::setMaxTorque)
			.def("getMaxTorque", &JointHingeActuatorComponent::getMaxTorque)
			.def("setDirectionChange", &JointHingeActuatorComponent::setDirectionChange)
			.def("getDirectionChange", &JointHingeActuatorComponent::getDirectionChange)
			.def("setRepeat", &JointHingeActuatorComponent::setRepeat)
			.def("getRepeat", &JointHingeActuatorComponent::getRepeat)
			.def("setSpring", (void (JointHingeActuatorComponent::*)(bool, bool)) & JointHingeActuatorComponent::setSpring)
			.def("setSpring", (void (JointHingeActuatorComponent::*)(bool, bool, Ogre::Real, Ogre::Real, Ogre::Real)) & JointHingeActuatorComponent::setSpring)
			];

		AddClassToCollection("JointHingeActuatorComponent", "class inherits JointComponent", JointHingeActuatorComponent::getStaticInfoText());
		AddClassToCollection("JointHingeActuatorComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointHingeActuatorComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointHingeActuatorComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointHingeActuatorComponent", "void setPin(Vector3 pin)", "Sets pin axis for the joint, to rotate around one dimension prependicular to the axis.");
		AddClassToCollection("JointHingeActuatorComponent", "Vector3 getPin()", "Gets joint pin axis.");
		AddClassToCollection("JointHingeActuatorComponent", "void setAngleRate(float angularRate)", "Sets the angle rate (rotation speed).");
		AddClassToCollection("JointHingeActuatorComponent", "float getAngleRate()", "Gets the angle rate (rotation speed) for this joint.");
		AddClassToCollection("JointHingeActuatorComponent", "void setTargetAngle(Degree targetAngle)", "Sets target angle, the game object will be rotated to. Note: This does only work, if 'setDirectionChange' or 'repeat' is off.");
		AddClassToCollection("JointHingeActuatorComponent", "Degree getTargetAngle()", "Gets target angle.");
		AddClassToCollection("JointHingeActuatorComponent", "void setMinAngleLimit(Degree minAngleLimit)", "Sets the min angle limit in degree. "
			"Note: If 'setDirectionChange' is enabled, this is the angle at which the rotation direction will be changed.");
		AddClassToCollection("JointHingeActuatorComponent", "Degree getMinAngleLimit()", "Gets the min angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "void setMaxAngleLimit(Degree maxAngleLimit)", "Sets the max angle limit in degree. "
			"Note: If 'setDirectionChange' is enabled, this is the angle at which the rotation direction will be changed.");
		AddClassToCollection("JointHingeActuatorComponent", "Degree getMaxAngleLimit()", "Gets the max angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "void setMaxTorque(float maxTorque)", "Sets max torque during rotation. Note: This will affect the rotation rate. "
			"Note: A high value will cause that the rotation will start slow and will be faster and at the end slow again.");
		AddClassToCollection("JointHingeActuatorComponent", "float getMaxTorque()", "Gets max torque that is used during rotation.");
		AddClassToCollection("JointHingeActuatorComponent", "void setDirectionChange(bool directionChange)", "Sets whether the direction should be changed, when the rotation reaches the min or max angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "bool getDirectionChange()", "Gets whether the direction is changed, when the rotation reaches the min or max angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "void setRepeat(bool repeat)", "Sets whether to repeat the direction changed rotation. Note: If set to false the rotation will only go from min angle limit and back to max angular limit once.");
		AddClassToCollection("JointHingeActuatorComponent", "bool getRepeat()", "Gets whether the direction changed rotation is repeated.");
		AddClassToCollection("JointHingeActuatorComponent", "void setSpring(bool asSpringDamper, bool massIndependent)", "Activates the spring with currently set spring values.");
		AddClassToCollection("JointHingeActuatorComponent", "void setSpring(bool asSpringDamper, bool massIndependent, float springDamperRelaxation, float springK, float springD)", "Sets the spring values for this joint. Note: When 'asSpringDamper' is activated the joint will use spring values for rotation.");

		module(lua)
			[
				class_<JointBallAndSocketComponent, JointComponent>("JointBallAndSocketComponent")
				// // .def("clone", &JointBallAndSocketComponent::clone)
			.def("setAnchorPosition", &JointBallAndSocketComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointBallAndSocketComponent::getAnchorPosition)
			.def("setTwistLimitsEnabled", &JointBallAndSocketComponent::setTwistLimitsEnabled)
			.def("getTwistLimitsEnabled", &JointBallAndSocketComponent::getTwistLimitsEnabled)
			.def("setConeLimitsEnabled", &JointBallAndSocketComponent::setConeLimitsEnabled)
			.def("getConeLimitsEnabled", &JointBallAndSocketComponent::getConeLimitsEnabled)
			.def("setMinMaxConeAngleLimit", &JointBallAndSocketComponent::setMinMaxConeAngleLimit)
			.def("getMinAngleLimit", &JointBallAndSocketComponent::getMinAngleLimit)
			.def("getMaxAngleLimit", &JointBallAndSocketComponent::getMaxAngleLimit)
			.def("getMaxConeLimit", &JointBallAndSocketComponent::getMaxConeLimit)
			.def("setTwistFriction", &JointBallAndSocketComponent::setTwistFriction)
			.def("getTwistFriction", &JointBallAndSocketComponent::getTwistFriction)
			.def("setConeFriction", &JointBallAndSocketComponent::setConeFriction)
			.def("getConeFriction", &JointBallAndSocketComponent::getConeFriction)
			];

		AddClassToCollection("JointBallAndSocketComponent", "class inherits JointComponent", JointBallAndSocketComponent::getStaticInfoText());
		AddClassToCollection("JointBallAndSocketComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointBallAndSocketComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointBallAndSocketComponent", "void setTwistLimitsEnabled(bool enabled)", "Sets whether the twist of this joint should be limited by its rotation.");
		AddClassToCollection("JointBallAndSocketComponent", "bool getTwistLimitsEnabled()", "Gets whether the twist of this joint is limited by its rotation.");
		AddClassToCollection("JointBallAndSocketComponent", "void setConeLimitsEnabled(bool enabled)", "Sets whether the cone of this joint should be limited by its 'hanging'.");
		AddClassToCollection("JointBallAndSocketComponent", "bool getConeLimitsEnabled()", "Gets whether the cone of this joint is limited by its 'hanging'.");
		AddClassToCollection("JointBallAndSocketComponent", "void setMinMaxConeAngleLimit(Degree minAngleLimit, Degree minAngleLimit, Degree maxConeLimit)", "Sets the min and max angle and cone limit in degree. "
			"Note: This will only be applied if 'setTwistLimitsEnabled' and 'setConeLimitsEnabled' are set to 'true'.");
		AddClassToCollection("JointBallAndSocketComponent", "Degree getMinAngleLimit()", "Gets the min angle limit.");
		AddClassToCollection("JointBallAndSocketComponent", "Degree getMaxAngleLimit()", "Gets the max angle limit.");
		AddClassToCollection("JointBallAndSocketComponent", "Degree getConeAngleLimit()", "Gets the max cone limit.");
		AddClassToCollection("JointBallAndSocketComponent", "void setTwistFriction(float friction)", "Sets the twist friction for this joint in order to prevent high peek values.");
		AddClassToCollection("JointBallAndSocketComponent", "float getTwistFriction()", "Gets the twist friction of this joint.");
		AddClassToCollection("JointBallAndSocketComponent", "void setConeFriction(float friction)", "Sets the cone friction for this joint in order to prevent high peek values.");
		AddClassToCollection("JointBallAndSocketComponent", "float getConeFriction()", "Gets the cone friction of this joint.");

		module(lua)
			[
				class_<JointPinComponent, JointComponent>("JointPinComponent")
				// // .def("clone", &JointPinComponent::clone)
			.def("setPin", &JointPinComponent::setPin)
			.def("getPin", &JointPinComponent::getPin)
			];
		AddClassToCollection("JointPinComponent", "class inherits JointComponent", "Derived class from JointComponent. This class can be used to limit rotation to one axis of freedom.");
		AddClassToCollection("JointPinComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointPinComponent", "void setPin(Vector3 pin)", "Sets pin axis for the joint, to limit rotation to one axis of freedom.");
		AddClassToCollection("JointPinComponent", "Vector3 getPin()", "Gets joint pin axis.");

		module(lua)
			[
				class_<JointPlaneComponent, JointComponent>("JointPlaneComponent")
				// // .def("clone", &JointPlaneComponent::clone)
			.def("setAnchorPosition", &JointPlaneComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointPlaneComponent::getAnchorPosition)
			.def("setNormal", &JointPlaneComponent::setNormal)
			.def("getNormal", &JointPlaneComponent::getNormal)
			];
		AddClassToCollection("JointPlaneComponent", "class inherits JointComponent", "Derived class from JointComponent. This class can be used to limit position to two axis.");
		AddClassToCollection("JointPlaneComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointPlaneComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointPlaneComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointPlaneComponent", "void setNormal(Vector3 normal)", "Sets normal axis for the joint, to limit position to two axis. Example: Setting normal to z, motion only at x and y is possible.");
		AddClassToCollection("JointPlaneComponent", "Vector3 getNormal()", "Gets joint normal axis.");

		module(lua)
			[
				class_<JointSpringComponent, JointComponent>("JointSpringComponent")
				// // .def("clone", &JointSpringComponent::clone)
			.def("setShowLine", &JointSpringComponent::setShowLine)
			.def("getShowLine", &JointSpringComponent::getShowLine)
			.def("setAnchorOffsetPosition", &JointSpringComponent::setAnchorOffsetPosition)
			.def("getAnchorOffsetPosition", &JointSpringComponent::getAnchorOffsetPosition)
			.def("setSpringOffsetPosition", &JointSpringComponent::setSpringOffsetPosition)
			.def("getSpringOffsetPosition", &JointSpringComponent::getSpringOffsetPosition)
			.def("releaseSpring", &JointSpringComponent::releaseSpring)
			// .def("drawLine", &JointSpringComponent::drawLine)
			];
		AddClassToCollection("JointSpringComponent", "class inherits JointComponent", "Derived class from JointComponent. With this joint its possible to connect two objects with a spring.");
		AddClassToCollection("JointSpringComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointSpringComponent", "void setShowLine(bool showLine)", "Sets whether to show the spring line.");
		AddClassToCollection("JointSpringComponent", "bool getShowLine()", "Gets whether the spring line is shown.");
		AddClassToCollection("JointSpringComponent", "void setAnchorOffsetPosition(Vector3 anchorOffsetPosition)", "Sets anchor offset position, where the spring should start.");
		AddClassToCollection("JointSpringComponent", "Vector3 getAnchorOffsetPosition()", "Gets anchor offset position, where the spring starts.");
		AddClassToCollection("JointSpringComponent", "void setSpringOffsetPosition(Vector3 springOffsetPosition)", "Sets spring offset position, where the spring should end.");
		AddClassToCollection("JointSpringComponent", "Vector3 getSpringOffsetPosition()", "Gets spring offset position, where the spring ends.");
		AddClassToCollection("JointSpringComponent", "void releaseSpring()", "Releases the spring, so that it is no more visible and no spring calculation takes places.");

		module(lua)
			[
				class_<JointAttractorComponent, JointComponent>("JointAttractorComponent")
				// // .def("clone", &JointAttractorComponent::clone)
			.def("setMagneticStrength", &JointAttractorComponent::setMagneticStrength)
			.def("getMagneticStrength", &JointAttractorComponent::getMagneticStrength)
			.def("setAttractionDistance", &JointAttractorComponent::setAttractionDistance)
			.def("getAttractionDistance", &JointAttractorComponent::getAttractionDistance)
			.def("setRepellerCategory", &JointAttractorComponent::setRepellerCategory)
			.def("getRepellerCategory", &JointAttractorComponent::getRepellerCategory)
			];
		AddClassToCollection("JointAttractorComponent", "class inherits JointComponent", "Derived class from JointComponent. With this joint its possible to influence other physics active game object (repeller) by magnetic strength. "
			"The to be attracted game object must belong to the specified repeller category.");
		AddClassToCollection("JointAttractorComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointAttractorComponent", "void setMagneticStrength(bool magneticStrength)", "Sets the magnetic strength.");
		AddClassToCollection("JointAttractorComponent", "bool getMagneticStrength()", "Gets the magnetic strength.");
		AddClassToCollection("JointAttractorComponent", "void setAttractionDistance(float attractionDistance)", "Sets attraction distance at which the object is influenced by the magnetic force.");
		AddClassToCollection("JointAttractorComponent", "float getAttractionDistance()", "Gets the attraction distance at which the object is influenced by the magnetic force.");
		AddClassToCollection("JointAttractorComponent", "void setRepellerCategory(String repellerCategory)", "Sets the repeller category. Note: All other game objects with a physics active component, that are belonging to this category, will be attracted.");
		AddClassToCollection("JointAttractorComponent", "String getRepellerCategory()", "Gets the repeller category.");

		module(lua)
			[
				class_<JointCorkScrewComponent, JointComponent>("JointCorkScrewComponent")
				// // .def("clone", &JointCorkScrewComponent::clone)
			.def("setAnchorPosition", &JointCorkScrewComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointCorkScrewComponent::getAnchorPosition)
			.def("setLinearLimitsEnabled", &JointCorkScrewComponent::setLinearLimitsEnabled)
			.def("getLinearLimitsEnabled", &JointCorkScrewComponent::getLinearLimitsEnabled)
			.def("setAngleLimitsEnabled", &JointCorkScrewComponent::setAngleLimitsEnabled)
			.def("getAngleLimitsEnabled", &JointCorkScrewComponent::getAngleLimitsEnabled)
			.def("setMinMaxStopDistance", &JointCorkScrewComponent::setMinMaxStopDistance)
			.def("getMinStopDistance", &JointCorkScrewComponent::getMinStopDistance)
			.def("getMaxStopDistance", &JointCorkScrewComponent::getMaxStopDistance)
			.def("setMinMaxAngleLimit", &JointCorkScrewComponent::setMinMaxAngleLimit)
			.def("getMinAngleLimit", &JointCorkScrewComponent::getMinAngleLimit)
			.def("getMaxAngleLimit", &JointCorkScrewComponent::getMaxAngleLimit)
			];
		AddClassToCollection("JointCorkScrewComponent", "class inherits JointComponent", "Derived class from JointComponent. This joint type is an enhanched version of a slider joint which allows the attached child body to not "
			"only slide up and down the axis, but also to rotate around this axis.");
		AddClassToCollection("JointCorkScrewComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointCorkScrewComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointCorkScrewComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");

		AddClassToCollection("JointCorkScrewComponent", "void setLinearLimitsEnabled(bool enabled)", "Sets whether the linear motion of this joint should be limited.");
		AddClassToCollection("JointCorkScrewComponent", "bool getLinearLimitsEnabled()", "Gets whether the linear motion of this joint is limited.");
		AddClassToCollection("JointCorkScrewComponent", "void setAngleLimitsEnabled(bool enabled)", "Sets whether the angle rotation of this joint should be limited.");
		AddClassToCollection("JointCorkScrewComponent", "bool getAngleLimitsEnabled()", "Gets whether the angle rotation of this joint is limited.");
		AddClassToCollection("JointCorkScrewComponent", "void setMinMaxStopDistance(float minStopDistance, float maxStopDistance)", "Sets the min and max distance limit. "
			"Note: This will only be applied if 'setLinearLimitsEnabled' is set to 'true'.");
		AddClassToCollection("JointCorkScrewComponent", "float getMinStopDistance()", "Gets min stop distance.");
		AddClassToCollection("JointCorkScrewComponent", "float getMaxStopDistance()", "Gets max stop distance.");
		AddClassToCollection("JointCorkScrewComponent", "void setMinMaxAngleLimit(Degree minAngleLimit, float maxAngleLimit)", "Sets the min and max angle limit. "
			"Note: This will only be applied if 'setAngleLimitsEnabled' is set to 'true'.");
		AddClassToCollection("JointCorkScrewComponent", "float getMinAngleLimit()", "Gets min angle limit.");
		AddClassToCollection("JointCorkScrewComponent", "float getMaxAngleLimit()", "Gets max angle limit.");

		module(lua)
			[
				class_<JointPassiveSliderComponent, JointComponent>("JointPassiveSliderComponent")
				// // .def("clone", &JointPassiveSliderComponent::clone)
			.def("setAnchorPosition", &JointPassiveSliderComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointPassiveSliderComponent::getAnchorPosition)
			.def("setPin", &JointPassiveSliderComponent::setPin)
			.def("getPin", &JointPassiveSliderComponent::getPin)
			.def("setLimitsEnabled", &JointPassiveSliderComponent::setLimitsEnabled)
			.def("getLimitsEnabled", &JointPassiveSliderComponent::getLimitsEnabled)
			.def("setMinMaxStopDistance", &JointPassiveSliderComponent::setMinMaxStopDistance)
			.def("getMinStopDistance", &JointPassiveSliderComponent::getMinStopDistance)
			.def("getMaxStopDistance", &JointPassiveSliderComponent::getMaxStopDistance)
			.def("setSpring", (void (JointPassiveSliderComponent::*)(bool)) & JointPassiveSliderComponent::setSpring)
			.def("setSpring", (void (JointPassiveSliderComponent::*)(bool, Ogre::Real, Ogre::Real, Ogre::Real)) & JointPassiveSliderComponent::setSpring)
			// .def("getSpring", &JointPassiveSliderComponent::getSpring)
			];
		AddClassToCollection("JointPassiveSliderComponent", "class inherits JointComponent", "Derived class from JointComponent. A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to.");
		AddClassToCollection("JointPassiveSliderComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointPassiveSliderComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointPassiveSliderComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointPassiveSliderComponent", "void setPin(Vector3 pin)", "Sets pin axis for the joint, in order to slide in that direction.");
		AddClassToCollection("JointPassiveSliderComponent", "Vector3 getPin()", "Gets joint pin axis.");
		AddClassToCollection("JointPassiveSliderComponent", "void setLimitsEnabled(bool enabled)", "Sets whether the linear motion of this joint should be limited.");
		AddClassToCollection("JointPassiveSliderComponent", "bool getLimitsEnabled()", "Gets whether the linear motion of this joint is limited.");
		AddClassToCollection("JointPassiveSliderComponent", "void setMinMaxStopDistance(float minStopDistance, float maxStopDistance)", "Sets the min and max stop distance. "
			"Note: This will only be applied if 'setLimitsEnabled' is set to 'true'.");
		AddClassToCollection("JointPassiveSliderComponent", "float getMinStopDistance()", "Gets min stop distance.");
		AddClassToCollection("JointPassiveSliderComponent", "float getMaxStopDistance()", "Gets max stop distance.");
		AddClassToCollection("JointPassiveSliderComponent", "void setSpring(bool asSpringDamper)", "Activates the spring with currently set spring values");
		AddClassToCollection("JointPassiveSliderComponent", "void setSpring2(bool asSpringDamper, float springDamperRelaxation, float springK, float springD)", "Sets the spring values for this joint. Note: When 'asSpringDamper' is activated the slider will use spring values for motion.");

		module(lua)
			[
				class_<JointSliderActuatorComponent, JointComponent>("JointSliderActuatorComponent")
				// // .def("clone", &JointSliderActuatorComponent::clone)
			.def("setAnchorPosition", &JointSliderActuatorComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointSliderActuatorComponent::getAnchorPosition)
			.def("setPin", &JointSliderActuatorComponent::setPin)
			.def("getPin", &JointSliderActuatorComponent::getPin)
			.def("setTargetPosition", &JointSliderActuatorComponent::setTargetPosition)
			.def("getTargetPosition", &JointSliderActuatorComponent::getTargetPosition)
			.def("setLinearRate", &JointSliderActuatorComponent::setLinearRate)
			.def("getLinearRate", &JointSliderActuatorComponent::getLinearRate)
			.def("setMinStopDistance", &JointSliderActuatorComponent::setMinStopDistance)
			.def("getMinStopDistance", &JointSliderActuatorComponent::getMinStopDistance)
			.def("setMaxStopDistance", &JointSliderActuatorComponent::setMaxStopDistance)
			.def("getMaxStopDistance", &JointSliderActuatorComponent::getMaxStopDistance)
			.def("setMinForce", &JointSliderActuatorComponent::setMinForce)
			.def("getMinForce", &JointSliderActuatorComponent::getMinForce)
			.def("setMaxForce", &JointSliderActuatorComponent::setMaxForce)
			.def("getMaxForce", &JointSliderActuatorComponent::getMaxForce)
			.def("setDirectionChange", &JointSliderActuatorComponent::setDirectionChange)
			.def("getDirectionChange", &JointSliderActuatorComponent::getDirectionChange)
			.def("setRepeat", &JointSliderActuatorComponent::setRepeat)
			.def("getRepeat", &JointSliderActuatorComponent::getRepeat)
			];
		AddClassToCollection("JointSliderActuatorComponent", "class inherits JointComponent", "Derived class from JointComponent. A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to. "
			"It will move automatically to the given target position, or when direction change is activated from min- to max stop distance and vice versa.");
		AddClassToCollection("JointSliderActuatorComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointSliderActuatorComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointSliderActuatorComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointSliderActuatorComponent", "void setPin(Vector3 pin)", "Sets pin axis for the joint, in order to slide in that direction.");
		AddClassToCollection("JointSliderActuatorComponent", "Vector3 getPin()", "Gets joint pin axis.");
		AddClassToCollection("JointSliderActuatorComponent", "void setTargetPosition(float targetPosition)", "Sets target position, the game object will move to. Note: This does only work, if 'setDirectionChange' is off.");
		AddClassToCollection("JointSliderActuatorComponent", "float getTargetPosition()", "Gets target position.");
		AddClassToCollection("JointSliderActuatorComponent", "void setMinStopDistance(float minStopDistance)", "Sets the min stop limit."
			"Note: If 'setDirectionChange' is enabled, this is the min stop limit at which the motion direction will be changed.");
		AddClassToCollection("JointSliderActuatorComponent", "float getMinStopDistance()", "Gets the min stop distance limit.");
		AddClassToCollection("JointSliderActuatorComponent", "void setMaxStopDistance(float maxStopDistance)", "Sets the max stop distance limit. "
			"Note: If 'setDirectionChange' is enabled, this is the max stop limit at which the motion direction will be changed.");
		AddClassToCollection("JointSliderActuatorComponent", "Degree getMaxStopDistance()", "Gets the max stop distance.");
		AddClassToCollection("JointSliderActuatorComponent", "void setMinForce(float minForce)", "Sets min force during motion. Note: This will affect the motion rate. "
			"Note: A high value will cause that the motion will start slow and will be faster and at the end slow again.");
		AddClassToCollection("JointSliderActuatorComponent", "float getMinForce()", "Gets min force that is used during motion.");
		AddClassToCollection("JointSliderActuatorComponent", "void setMaxForce(float maxForce)", "Sets max force during motion. Note: This will affect the motion rate. "
			"Note: A high value will cause that the motion will start slow and will be faster and at the end slow again.");
		AddClassToCollection("JointSliderActuatorComponent", "float getMaxForce()", "Gets max force that is used during motion.");
		AddClassToCollection("JointSliderActuatorComponent", "void setDirectionChange(bool directionChange)", "Sets whether the direction should be changed, when the motion reaches the min or max stop distance.");
		AddClassToCollection("JointSliderActuatorComponent", "bool getDirectionChange()", "Gets whether the direction is changed, when the motion reaches the min or max stop distance.");
		AddClassToCollection("JointSliderActuatorComponent", "void setRepeat(bool repeat)", "Sets whether to repeat the direction changed motion. Note: If set to false the motion will only go from min stop distance and back to max stop distance once.");
		AddClassToCollection("JointSliderActuatorComponent", "bool getRepeat()", "Gets whether the direction changed motion is repeated.");

		module(lua)
			[
				class_<JointSlidingContactComponent, JointComponent>("JointSlidingContactComponent")
				// // .def("clone", &JointSlidingContactComponent::clone)
			.def("setAnchorPosition", &JointSlidingContactComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointSlidingContactComponent::getAnchorPosition)
			.def("setPin", &JointSlidingContactComponent::setPin)
			.def("getPin", &JointSlidingContactComponent::getPin)
			.def("setLinearLimitsEnabled", &JointSlidingContactComponent::setLinearLimitsEnabled)
			.def("getLinearLimitsEnabled", &JointSlidingContactComponent::getLinearLimitsEnabled)
			.def("setAngleLimitsEnabled", &JointSlidingContactComponent::setAngleLimitsEnabled)
			.def("getAngleLimitsEnabled", &JointSlidingContactComponent::getAngleLimitsEnabled)
			.def("setMinMaxStopDistance", &JointSlidingContactComponent::setMinMaxStopDistance)
			.def("getMinStopDistance", &JointSlidingContactComponent::getMinStopDistance)
			.def("getMaxStopDistance", &JointSlidingContactComponent::getMaxStopDistance)
			.def("setMinMaxAngleLimit", &JointSlidingContactComponent::setMinMaxAngleLimit)
			.def("getMinAngleLimit", &JointSlidingContactComponent::getMinAngleLimit)
			.def("getMaxAngleLimit", &JointSlidingContactComponent::getMaxAngleLimit)
			.def("setSpring", (void (JointSlidingContactComponent::*)(bool)) & JointSlidingContactComponent::setSpring)
			.def("setSpring", (void (JointSlidingContactComponent::*)(bool, Ogre::Real, Ogre::Real, Ogre::Real)) & JointSlidingContactComponent::setSpring)
			// .def("getSpring", &JointSlidingContactComponent::getSpring)
			];
		AddClassToCollection("JointSlidingContactComponent", "class inherits JointComponent", "Derived class from JointComponent. A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to and rotate around given pin-axis.");
		AddClassToCollection("JointSlidingContactComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointSlidingContactComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointSlidingContactComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointSlidingContactComponent", "void setPin(Vector3 pin)", "Sets pin axis for the joint, in order to slide in that direction.");
		AddClassToCollection("JointSlidingContactComponent", "Vector3 getPin()", "Gets joint pin axis.");
		AddClassToCollection("JointSlidingContactComponent", "void setLinearLimitsEnabled(bool enabled)", "Sets whether the linear motion of this joint should be limited.");
		AddClassToCollection("JointSlidingContactComponent", "bool getLinearLimitsEnabled()", "Gets whether the linear motion of this joint is limited.");
		AddClassToCollection("JointSlidingContactComponent", "void setAngleLimitsEnabled(bool enabled)", "Sets whether the angle rotation of this joint should be limited.");
		AddClassToCollection("JointSlidingContactComponent", "bool getAngleLimitsEnabled()", "Gets whether the angle rotation of this joint is limited.");
		AddClassToCollection("JointSlidingContactComponent", "void setMinMaxStopDistance(float minStopDistance, float maxStopDistance)", "Sets the min and max stop distance. "
			"Note: This will only be applied if 'setLinearLimitsEnabled' is set to 'true'.");
		AddClassToCollection("JointSlidingContactComponent", "float getMinStopDistance()", "Gets min stop distance.");
		AddClassToCollection("JointSlidingContactComponent", "float getMaxStopDistance()", "Gets max stop distance.");
		AddClassToCollection("JointSlidingContactComponent", "void setMinMaxAngleLimit(Degree minAngleLimit, float maxAngleLimit)", "Sets the min and max angle limit. "
			"Note: This will only be applied if 'setAngleLimitsEnabled' is set to 'true'.");
		AddClassToCollection("JointSlidingContactComponent", "float getMinAngleLimit()", "Gets min angle limit.");
		AddClassToCollection("JointSlidingContactComponent", "float getMaxAngleLimit()", "Gets max angle limit.");
		AddClassToCollection("JointSlidingContactComponent", "void setSpring(bool asSpringDamper)", "Activates the spring with currently set spring values");
		AddClassToCollection("JointSlidingContactComponent", "void setSpring2(bool asSpringDamper, float springDamperRelaxation, float springK, float springD)", "Sets the spring values for this joint. Note: When 'asSpringDamper' is activated the slider will use spring values for motion.");

		module(lua)
			[
				class_<JointActiveSliderComponent, JointComponent>("JointActiveSliderComponent")
				// // .def("clone", &JointActiveSliderComponent::clone)
			.def("setAnchorPosition", &JointActiveSliderComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointActiveSliderComponent::getAnchorPosition)
			.def("setPin", &JointActiveSliderComponent::setPin)
			.def("getPin", &JointActiveSliderComponent::getPin)
			.def("setLimitsEnabled", &JointActiveSliderComponent::setLimitsEnabled)
			.def("getLimitsEnabled", &JointActiveSliderComponent::getLimitsEnabled)
			.def("setMinMaxStopDistance", &JointActiveSliderComponent::setMinMaxStopDistance)
			.def("getMinStopDistance", &JointActiveSliderComponent::getMinStopDistance)
			.def("getMaxStopDistance", &JointActiveSliderComponent::getMaxStopDistance)
			.def("getCurrentDirection", &JointActiveSliderComponent::getCurrentDirection)
			.def("setMoveSpeed", &JointActiveSliderComponent::setMoveSpeed)
			.def("getMoveSpeed", &JointActiveSliderComponent::getMoveSpeed)
			.def("setRepeat", &JointActiveSliderComponent::setRepeat)
			.def("getRepeat", &JointActiveSliderComponent::getRepeat)
			.def("setDirectionChange", &JointActiveSliderComponent::setDirectionChange)
			.def("getDirectionChange", &JointActiveSliderComponent::getDirectionChange)
			];
		AddClassToCollection("JointActiveSliderComponent", "class inherits JointComponent", "Derived class from JointComponent. A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to. "
			"It will move automatically from the min stop distance to the max stop distance by the specifed axis, or when direction change is activated from min- to max stop distance and vice versa.");
		AddClassToCollection("JointActiveSliderComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointActiveSliderComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointActiveSliderComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointActiveSliderComponent", "void setPin(Vector3 pin)", "Sets pin axis for the joint, in order to slide in that direction.");
		AddClassToCollection("JointActiveSliderComponent", "Vector3 getPin()", "Gets joint pin axis.");
		AddClassToCollection("JointActiveSliderComponent", "void setMinStopDistance(float minStopDistance)", "Sets the min stop limit."
			"Note: If 'setDirectionChange' is enabled, this is the min stop limit at which the motion direction will be changed.");
		AddClassToCollection("JointActiveSliderComponent", "float getMinStopDistance()", "Gets the min stop distance limit.");
		AddClassToCollection("JointActiveSliderComponent", "void setMaxStopDistance(float maxStopDistance)", "Sets the max stop distance limit. "
			"Note: If 'setDirectionChange' is enabled, this is the max stop limit at which the motion direction will be changed.");
		AddClassToCollection("JointActiveSliderComponent", "Degree getMaxStopDistance()", "Gets the max stop distance.");
		AddClassToCollection("JointActiveSliderComponent", "void setMoveSpeed(float moveSpeed)", "Sets the moving speed.");
		AddClassToCollection("JointActiveSliderComponent", "float getMoveSpeed()", "Gets moving speed.");
		AddClassToCollection("JointActiveSliderComponent", "void setDirectionChange(bool directionChange)", "Sets whether the direction should be changed, when the motion reaches the min or max stop distance.");
		AddClassToCollection("JointActiveSliderComponent", "bool getDirectionChange()", "Gets whether the direction is changed, when the motion reaches the min or max stop distance.");
		AddClassToCollection("JointActiveSliderComponent", "void setRepeat(bool repeat)", "Sets whether to repeat the direction changed motion. Note: If set to false the motion will only go from min stop distance and back to max stop distance once.");
		AddClassToCollection("JointActiveSliderComponent", "bool getRepeat()", "Gets whether the direction changed motion is repeated.");

		module(lua)
			[
				class_<JointMathSliderComponent, JointComponent>("JointMathSliderComponent")
				// // .def("clone", &JointMathSliderComponent::clone)
			.def("setLimitsEnabled", &JointMathSliderComponent::setLimitsEnabled)
			.def("getLimitsEnabled", &JointMathSliderComponent::getLimitsEnabled)
			.def("setMinMaxStopDistance", &JointMathSliderComponent::setMinMaxStopDistance)
			.def("getMinStopDistance", &JointMathSliderComponent::getMinStopDistance)
			.def("getMaxStopDistance", &JointMathSliderComponent::getMaxStopDistance)
			.def("setMoveSpeed", &JointMathSliderComponent::setMoveSpeed)
			.def("getMoveSpeed", &JointMathSliderComponent::getMoveSpeed)
			.def("setRepeat", &JointMathSliderComponent::setRepeat)
			.def("getRepeat", &JointMathSliderComponent::getRepeat)
			.def("setDirectionChange", &JointMathSliderComponent::setDirectionChange)
			.def("getDirectionChange", &JointMathSliderComponent::getDirectionChange)
			.def("setFunctionX", &JointMathSliderComponent::setFunctionX)
			.def("getFunctionX", &JointMathSliderComponent::getFunctionX)
			.def("setFunctionY", &JointMathSliderComponent::setFunctionY)
			.def("getFunctionY", &JointMathSliderComponent::getFunctionY)
			.def("setFunctionZ", &JointMathSliderComponent::setFunctionZ)
			.def("getFunctionZ", &JointMathSliderComponent::getFunctionZ)
			];
		AddClassToCollection("JointMathSliderComponent", "class inherits JointComponent", "Derived class from JointComponent. A child body attached via a slider joint can only slide by the given mathematical function.");
		AddClassToCollection("JointMathSliderComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointMathSliderComponent", "void setMinMaxStopDistance(float minStopDistance, float maxStopDistance)", "Sets the min and max stop limit."
			"Note: If 'setDirectionChange' is enabled, this is the min stop limit at which the motion direction will be changed.");
		AddClassToCollection("JointMathSliderComponent", "float getMinStopDistance()", "Gets the min stop distance limit.");
		AddClassToCollection("JointMathSliderComponent", "Degree getMaxStopDistance()", "Gets the max stop distance.");
		AddClassToCollection("JointMathSliderComponent", "void setMoveSpeed(float moveSpeed)", "Sets the moving speed.");
		AddClassToCollection("JointMathSliderComponent", "float getMoveSpeed()", "Gets moving speed.");
		AddClassToCollection("JointMathSliderComponent", "void setDirectionChange(bool directionChange)", "Sets whether the direction should be changed, when the motion reaches the min or max stop distance.");
		AddClassToCollection("JointMathSliderComponent", "bool getDirectionChange()", "Gets whether the direction is changed, when the motion reaches the min or max stop distance.");
		AddClassToCollection("JointMathSliderComponent", "void setRepeat(bool repeat)", "Sets whether to repeat the direction changed motion. Note: If set to false the motion will only go from min stop distance and back to max stop distance once.");
		AddClassToCollection("JointMathSliderComponent", "bool getRepeat()", "Gets whether the direction changed motion is repeated.");

		AddClassToCollection("JointMathSliderComponent", "void setFunctionX(String mx)", "Sets the math function for x.");
		AddClassToCollection("JointMathSliderComponent", "String getFunctionX()", "Gets the math function for x.");
		AddClassToCollection("JointMathSliderComponent", "void setFunctionY(String my)", "Sets the math function for y.");
		AddClassToCollection("JointMathSliderComponent", "String getFunctionY()", "Gets the math function for y.");
		AddClassToCollection("JointMathSliderComponent", "void setFunctionZ(String mz)", "Sets the math function for z.");
		AddClassToCollection("JointMathSliderComponent", "String getFunctionZ()", "Gets the math function for z.");

		module(lua)
			[
				class_<JointKinematicComponent, JointComponent>("JointKinematicComponent")
				// // .def("clone", &JointKinematicComponent::clone)
			.def("setActivated", &JointKinematicComponent::setActivated)
			.def("isActivated", &JointKinematicComponent::isActivated)
			.def("setAnchorPosition", &JointKinematicComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointKinematicComponent::getAnchorPosition)
			.def("setPickingMode", &JointKinematicComponent::setPickingMode)
			.def("getPickingMode", &JointKinematicComponent::getPickingMode)
			.def("setMaxLinearAngleFriction", &JointKinematicComponent::setMaxLinearAngleFriction)
			.def("getMaxLinearAngleFriction", &getMaxLinearAngleFriction)
			.def("setTargetPosition", &JointKinematicComponent::setTargetPosition)
			.def("getTargetPosition", &JointKinematicComponent::getTargetPosition)
			.def("setTargetRotation", &JointKinematicComponent::setTargetRotation)
			.def("getTargetRotation", &JointKinematicComponent::getTargetRotation)
			.def("backToOrigin", &JointKinematicComponent::backToOrigin)
			];
		AddClassToCollection("JointKinematicComponent", "class inherits JointComponent", JointKinematicComponent::getStaticInfoText());
		AddClassToCollection("JointKinematicComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointKinematicComponent", "void setActivated(bool activated)", "Sets whether to activate the kinematic motion.");
		AddClassToCollection("JointKinematicComponent", "bool isActivated()", "Gets whether the kinematic motion is activated.");
		AddClassToCollection("JointKinematicComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointKinematicComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointKinematicComponent", "void setPickingMode(string picking mode)", "Sets the picking mode. "
			"Possible values are: Linear, Full 6 Degree Of Freedom, Linear And Twist, Linear And Cone, Linear Plus Angluar Friction");
		AddClassToCollection("JointKinematicComponent", "String getPickingMode()", "Gets the currently set picking mode. "
			"Possible values are: Linear, Full 6 Degree Of Freedom, Linear And Twist, Linear And Cone, Linear Plus Angluar Friction");
		AddClassToCollection("JointKinematicComponent", "void setMaxLinearAngleFriction(float maxLinearFriction, float maxAngleFriction)", "Sets the max linear and angle friction.");
		AddClassToCollection("JointKinematicComponent", "Table[linearFriction][angleFriction] getMaxLinearAngleFriction()", "Gets max linear and angle friction.");
		AddClassToCollection("JointKinematicComponent", "void setTargetPosition(Vector3 targetPosition)", "Sets the target position.");
		AddClassToCollection("JointKinematicComponent", "Vector3 getTargetPosition()", "Gets the target position.");
		AddClassToCollection("JointKinematicComponent", "void setTargetRotation(Quaternion targetRotation)", "Sets the target rotation.");
		AddClassToCollection("JointKinematicComponent", "Quaternion getTargetRotation()", "Gets the target rotation.");
		AddClassToCollection("JointKinematicComponent", "void backToOrigin()", "Moves the game object back to its origin.");

		module(lua)
			[
				class_<JointDryRollingFrictionComponent, JointComponent>("JointDryRollingFrictionComponent")
				// // .def("clone", &JointDryRollingFrictionComponent::clone)
			.def("setRadius", &JointDryRollingFrictionComponent::setRadius)
			.def("getRadius", &JointDryRollingFrictionComponent::getRadius)
			.def("setRollingFrictionCoefficient", &JointDryRollingFrictionComponent::setRollingFrictionCoefficient)
			.def("getRollingFrictionCoefficient", &JointDryRollingFrictionComponent::getRollingFrictionCoefficient)
			];
		AddClassToCollection("JointDryRollingFrictionComponent", "class inherits JointComponent", "Derived class from JointComponent. This joint is usefully to simulate the rolling friction of a rolling ball over a flat surface. "
			" Normally this is not important for non spherical objects, but for games like poll, pinball, bolling, golf"
			" or any other where the movement of balls is the main objective the rolling friction is a real big problem."
			" See: http://newtondynamics.com/forum/viewtopic.php?f=15&t=1187&p=8158&hilit=dry+rolling+friction");
		AddClassToCollection("JointDryRollingFrictionComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointDryRollingFrictionComponent", "void setRadius(float radius)", "Sets radius of the ball.");
		AddClassToCollection("JointDryRollingFrictionComponent", "float getRadius()", "Gets the radius of the ball.");
		AddClassToCollection("JointDryRollingFrictionComponent", "void setRollingFrictionCoefficient(float rollingFrictionCoefficient)", "Sets the rolling friction coefficient.");
		AddClassToCollection("JointDryRollingFrictionComponent", "float getRollingFrictionCoefficient()", "Gets the rolling friction coefficient.");

		module(lua)
			[
				class_<JointGearComponent, JointComponent>("JointGearComponent")
				// // .def("clone", &JointGearComponent::clone)
			.def("setGearRatio", &JointGearComponent::setGearRatio)
			.def("getGearRatio", &JointGearComponent::getGearRatio)
			];
		AddClassToCollection("JointGearComponent", "class inherits JointComponent", "Derived class from JointComponent. This joint is for use in conjunction with Hinge of other spherical joints"
			" is useful for establishing synchronization between the phase angle other the"
			" relative angular velocity of two spinning disks according to the law of gears"
			" velErro = -(W0 * r0 + W1 *  r1)"
			" where w0 and W1 are the angular velocity"
			" r0 and r1 are the radius of the spinning disk");
		AddClassToCollection("JointGearComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointGearComponent", "void setGearRatio(float ratio)", "Sets the ratio between the first hinge rotation and the second hinge rotation.");
		AddClassToCollection("JointGearComponent", "float getGearRatio()", "Gets the ratio between the first hinge rotation and the second hinge rotation.");

		module(lua)
			[
				class_<JointWormGearComponent, JointComponent>("JointWormGearComponent")
				// // .def("clone", &JointWormGearComponent::clone)
			.def("setGearRatio", &JointWormGearComponent::setGearRatio)
			.def("getGearRatio", &JointWormGearComponent::getGearRatio)
			.def("setSlideRatio", &JointWormGearComponent::setSlideRatio)
			.def("getSlideRatio", &JointWormGearComponent::getSlideRatio)
			];
		AddClassToCollection("JointWormGearComponent", "class inherits JointComponent", "Derived class from JointComponent. This joint is for use in conjunction with Hinge of other slider joints"
			" is useful for establishing synchronization between the phase angle and a slider motion.");
		AddClassToCollection("JointWormGearComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointWormGearComponent", "void setGearRatio(float ratio)", "Sets the ratio between the first hinge rotation and the second hinge rotation.");
		AddClassToCollection("JointWormGearComponent", "float getGearRatio()", "Gets the ratio between the first hinge rotation and the second hinge rotation.");
		AddClassToCollection("JointWormGearComponent", "void setSlideRatio(float ratio)", "Sets the ratio between the hinge rotation and the slider motion.");
		AddClassToCollection("JointWormGearComponent", "float getSlideRatio()", "Gets the ratio between the hinge rotation and the slider motion.");

		module(lua)
			[
				class_<JointPulleyComponent, JointComponent>("JointPulleyComponent")
				// // .def("clone", &JointPulleyComponent::clone)
			.def("setPulleyRatio", &JointPulleyComponent::setPulleyRatio)
			.def("getPulleyRatio", &JointPulleyComponent::getPulleyRatio)
			];
		AddClassToCollection("JointPulleyComponent", "class inherits JointComponent", "Derived class from JointComponent. This joint is for use in conjunction with Slider of other linear joints"
			" it is useful for establishing synchronization between the relative position"
			" or relative linear velocity of two sliding bodies according to the law of pulley"
			" velErro = -(v0 + n * v1) where v0 and v1 are the linear velocity"
			" n is the number of turn on the pulley, in the case of this joint N could"
			" be a value with friction, as this joint is a generalization of the pulley idea.");
		AddClassToCollection("JointPulleyComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointPulleyComponent", "void setPulleyRatio(float ratio)", "Sets the ratio between the first slider motion and the second slider motion.");
		AddClassToCollection("JointPulleyComponent", "float getPulleyRatio()", "Gets the ratio between the first slider motion and the second slider motion.");

		module(lua)
			[
				class_<JointUniversalComponent, JointComponent>("JointUniversalComponent")
				// // .def("clone", &JointUniversalComponent::clone)
			.def("setAnchorPosition", &JointUniversalComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointUniversalComponent::getAnchorPosition)
			.def("setPin", &JointUniversalComponent::setPin)
			.def("getPin", &JointUniversalComponent::getPin)
			.def("setLimits0Enabled", &JointUniversalComponent::setLimits0Enabled)
			.def("getLimits0Enabled", &JointUniversalComponent::getLimits0Enabled)
			.def("setMinMaxAngleLimit0", &JointUniversalComponent::setMinMaxAngleLimit0)
			.def("getMinAngleLimit0", &JointUniversalComponent::getMinAngleLimit0)
			.def("getMaxAngleLimit0", &JointUniversalComponent::getMaxAngleLimit0)
			.def("setFriction0", &JointUniversalComponent::setFriction0)
			.def("getFriction0", &JointUniversalComponent::getFriction0)
			.def("setMotorEnabled", &JointUniversalComponent::setMotorEnabled)
			.def("getMotorEnabled", &JointUniversalComponent::getMotorEnabled)
			.def("setMotorSpeed", &JointUniversalComponent::setMotorSpeed)
			.def("getMotorSpeed", &JointUniversalComponent::getMotorSpeed)
			.def("setLimits1Enabled", &JointUniversalComponent::setLimits1Enabled)
			.def("getLimits1Enabled", &JointUniversalComponent::getLimits1Enabled)
			.def("setMinMaxAngleLimit1", &JointUniversalComponent::setMinMaxAngleLimit1)
			.def("getMinAngleLimit1", &JointUniversalComponent::getMinAngleLimit1)
			.def("getMaxAngleLimit1", &JointUniversalComponent::getMaxAngleLimit1)
			.def("setSpring0", (void (JointUniversalComponent::*)(bool)) & JointUniversalComponent::setSpring0)
			.def("setSpring0", (void (JointUniversalComponent::*)(bool, Ogre::Real, Ogre::Real, Ogre::Real)) & JointUniversalComponent::setSpring0)
			.def("setSpring1", (void (JointUniversalComponent::*)(bool)) & JointUniversalComponent::setSpring1)
			.def("setSpring1", (void (JointUniversalComponent::*)(bool, Ogre::Real, Ogre::Real, Ogre::Real)) & JointUniversalComponent::setSpring1)
			.def("setFriction1", &JointUniversalComponent::setFriction1)
			.def("getFriction1", &JointUniversalComponent::getFriction1)
			// .def("setHardMiddleAxis", &JointUniversalComponent::setHardMiddleAxis)
			// .def("getHardMiddleAxis", &JointUniversalComponent::getHardMiddleAxis)
			];

		AddClassToCollection("JointUniversalComponent", "class inherits JointComponent", "Derived class from JointComponent. An object attached to a universal joint can rotate around two dimensions perpendicular to the axes it is attached to. It is to be seen like a double hinge joint.");
		AddClassToCollection("JointUniversalComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointUniversalComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointUniversalComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointUniversalComponent", "void setPin(Vector3 pin)", "Sets the pin for the axis of the joint, to rotate around one dimension prependicular to the axis.");
		AddClassToCollection("JointUniversalComponent", "Vector3 getPin()", "Gets the joint pin for axis.");
		AddClassToCollection("JointUniversalComponent", "void setLimits0Enabled(bool enabled)", "Sets whether this joint should be limited by its first rotation.");
		AddClassToCollection("JointUniversalComponent", "bool getLimits0Enabled()", "Gets whether this joint is limited by its first rotation.");
		AddClassToCollection("JointUniversalComponent", "void setMinMaxAngleLimit0(Degree minAngleLimit, Degree minAngleLimit)", "Sets the first min and max angle limit in degree. Note: This will only be applied if 'setLimitsEnabled' is set to 'true'.");
		AddClassToCollection("JointUniversalComponent", "Degree getMinAngleLimit0()", "Gets the first min angle limit.");
		AddClassToCollection("JointUniversalComponent", "Degree getMaxAngleLimit0()", "Gets the first max angle limit.");
		AddClassToCollection("JointUniversalComponent", "void setMotor0Enabled(bool enabled)", "Sets whether to enable the first motor for automatical rotation.");
		AddClassToCollection("JointUniversalComponent", "bool getMotor0Enabled()", "Gets whether the first motor for automatical rotation is enabled.");
		AddClassToCollection("JointUniversalComponent", "void setMotorSpeed0(float speed)", "Sets the first motor rotation speed. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.");
		AddClassToCollection("JointUniversalComponent", "float getMotorSpeed0()", "Gets the first motor rotation speed.");
		AddClassToCollection("JointUniversalComponent", "void setFriction0(float friction)", "Sets the friction (max. torque) for torque. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.");
		AddClassToCollection("JointUniversalComponent", "float getFriction0()", "Gets the first motor rotation friction.");
		AddClassToCollection("JointUniversalComponent", "void setLimits1Enabled(bool enabled)", "Sets whether this joint should be limited by its second rotation.");
		AddClassToCollection("JointUniversalComponent", "bool getLimits1Enabled()", "Gets whether this joint is limited by its second rotation.");
		AddClassToCollection("JointUniversalComponent", "void setMinMaxAngleLimit1(Degree minAngleLimit, Degree minAngleLimit)", "Sets the second min and max angle limit in degree. Note: This will only be applied if 'setLimitsEnabled' is set to 'true'.");
		AddClassToCollection("JointUniversalComponent", "Degree getMinAngleLimit1()", "Gets the second min angle limit.");
		AddClassToCollection("JointUniversalComponent", "Degree getMaxAngleLimit1()", "Gets the second max angle limit.");
		AddClassToCollection("JointUniversalComponent", "void setMotor1Enabled(bool enabled)", "Sets whether to enable the second motor for automatical rotation.");
		AddClassToCollection("JointUniversalComponent", "bool getMotor1Enabled()", "Gets whether the second motor for automatical rotation is enabled.");
		AddClassToCollection("JointUniversalComponent", "void setMotorSpeed1(float speed)", "Sets the second motor rotation speed. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.");
		AddClassToCollection("JointUniversalComponent", "float getMotorSpeed1()", "Gets the second motor rotation speed.");
		AddClassToCollection("JointUniversalComponent", "void setSpring0(bool asSpringDamper)", "Activates the first spring with currently set spring values");
		AddClassToCollection("JointUniversalComponent", "void setSpring0(bool asSpringDamper, float springDamperRelaxation, float springK, float springD)", "Sets the first spring values for this joint. Note: When 'asSpringDamper' is activated the joint will use spring values for rotation.");
		AddClassToCollection("JointUniversalComponent", "void setSpring1(bool asSpringDamper)", "Activates the second spring with currently set spring values");
		AddClassToCollection("JointUniversalComponent", "void setSpring1(bool asSpringDamper, float springDamperRelaxation, float springK, float springD)", "Sets the second spring values for this joint. Note: When 'asSpringDamper' is activated the joint will use spring values for rotation.");
		AddClassToCollection("JointUniversalComponent", "void setFriction1(float friction)", "Sets the friction (max. torque) for torque. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.");
		AddClassToCollection("JointUniversalComponent", "float getFriction1()", "Gets the second motor rotation friction.");

		module(lua)
			[
				class_<JointUniversalActuatorComponent, JointComponent>("JointUniversalActuatorComponent")
				// // .def("clone", &JointUniversalActuatorComponent::clone)
			.def("setAnchorPosition", &JointUniversalActuatorComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointUniversalActuatorComponent::getAnchorPosition)
			.def("setTargetAngle0", &JointUniversalActuatorComponent::setTargetAngle0)
			.def("getTargetAngle0", &JointUniversalActuatorComponent::getTargetAngle0)
			.def("setAngleRate0", &JointUniversalActuatorComponent::setAngleRate0)
			.def("getAngleRate0", &JointUniversalActuatorComponent::getAngleRate0)
			.def("setMinAngleLimit0", &JointUniversalActuatorComponent::setMinAngleLimit0)
			.def("getMinAngleLimit0", &JointUniversalActuatorComponent::getMinAngleLimit0)
			.def("setMaxAngleLimit0", &JointUniversalActuatorComponent::setMaxAngleLimit0)
			.def("getMaxAngleLimit0", &JointUniversalActuatorComponent::getMaxAngleLimit0)
			.def("setMaxTorque0", &JointUniversalActuatorComponent::setMaxTorque0)
			.def("getMaxTorque0", &JointUniversalActuatorComponent::getMaxTorque0)
			.def("setTargetAngle1", &JointUniversalActuatorComponent::setTargetAngle1)
			.def("getTargetAngle1", &JointUniversalActuatorComponent::getTargetAngle1)
			.def("setAngleRate1", &JointUniversalActuatorComponent::setAngleRate1)
			.def("getAngleRate1", &JointUniversalActuatorComponent::getAngleRate1)
			.def("setMinAngleLimit1", &JointUniversalActuatorComponent::setMinAngleLimit1)
			.def("getMinAngleLimit1", &JointUniversalActuatorComponent::getMinAngleLimit1)
			.def("setMaxAngleLimit1", &JointUniversalActuatorComponent::setMaxAngleLimit1)
			.def("getMaxAngleLimit1", &JointUniversalActuatorComponent::getMaxAngleLimit1)
			.def("setMaxTorque1", &JointUniversalActuatorComponent::setMaxTorque1)
			.def("getMaxTorque1", &JointUniversalActuatorComponent::getMaxTorque1)
			];

		AddClassToCollection("JointHingeActuatorComponent", "class inherits JointComponent", "Derived class from JointComponent. An object attached to a universal actuator joint can rotate around two dimensions perpendicular to the axes it is attached to. "
			"It will rotate automatically to the given target angle, or when direction change is activated from min- to max angle limit and vice versa. It can be seen like a double hinge joint.");
		AddClassToCollection("JointHingeActuatorComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointHingeActuatorComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointHingeActuatorComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointHingeActuatorComponent", "void setAngleRate0(float angularRate)", "Sets the first angle rate (rotation speed).");
		AddClassToCollection("JointHingeActuatorComponent", "float getAngleRate0()", "Gets the first angle rate (rotation speed) for this joint.");
		AddClassToCollection("JointHingeActuatorComponent", "void setTargetAngle0(Degree targetAngle)", "Sets first target angle, the game object will be rotated to. Note: This does only work, if 'setDirectionChange0' is off.");
		AddClassToCollection("JointHingeActuatorComponent", "Degree getTargetAngle0()", "Gets first target angle.");
		AddClassToCollection("JointHingeActuatorComponent", "void setMinAngleLimit0(Degree minAngleLimit)", "Sets the first min angle limit in degree. "
			"Note: If 'setDirectionChange0' is enabled, this is the angle at which the rotation direction will be changed.");
		AddClassToCollection("JointHingeActuatorComponent", "Degree getMinAngleLimit0()", "Gets the first min angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "void setMaxAngleLimit0(Degree maxAngleLimit)", "Sets the first max angle limit in degree. "
			"Note: If 'setDirectionChange0' is enabled, this is the angle at which the rotation direction will be changed.");
		AddClassToCollection("JointHingeActuatorComponent", "Degree getMaxAngleLimit0()", "Gets the first max angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "void setMaxTorque0(float maxTorque)", "Sets the first max torque during rotation. Note: This will affect the rotation rate. "
			"Note: A high value will cause that the rotation will start slow and will be faster and at the end slow again.");
		AddClassToCollection("JointHingeActuatorComponent", "float getMaxTorque0()", "Gets the first max torque that is used during rotation.");
		AddClassToCollection("JointHingeActuatorComponent", "void setDirectionChange0(bool directionChange)", "Sets whether the direction should be changed, when the first rotation reaches the min or max angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "bool getDirectionChange0()", "Gets whether the direction is changed, when the first rotation reaches the min or max angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "void setRepeat0(bool repeat)", "Sets whether to repeat the first direction changed rotation. Note: If set to false the rotation will only go from min angle limit and back to max angular limit once.");
		AddClassToCollection("JointHingeActuatorComponent", "bool getRepeat0()", "Gets whether the second direction changed rotation is repeated.");
		AddClassToCollection("JointHingeActuatorComponent", "void setAngleRate1(float angularRate)", "Sets the second angle rate (rotation speed).");
		AddClassToCollection("JointHingeActuatorComponent", "float getAngleRate1()", "Gets the second angle rate (rotation speed) for this joint.");
		AddClassToCollection("JointHingeActuatorComponent", "void setTargetAngle1(Degree targetAngle)", "Sets second target angle, the game object will be rotated to. Note: This does only work, if 'setDirectionChange0' is off.");
		AddClassToCollection("JointHingeActuatorComponent", "Degree getTargetAngle1()", "Gets second target angle.");
		AddClassToCollection("JointHingeActuatorComponent", "void setMinAngleLimit1(Degree minAngleLimit)", "Sets the second min angle limit in degree. "
			"Note: If 'setDirectionChange1' is enabled, this is the angle at which the rotation direction will be changed.");
		AddClassToCollection("JointHingeActuatorComponent", "Degree getMinAngleLimit1()", "Gets the second min angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "void setMaxAngleLimit1(Degree maxAngleLimit)", "Sets the second max angle limit in degree. "
			"Note: If 'setDirectionChange1' is enabled, this is the angle at which the rotation direction will be changed.");
		AddClassToCollection("JointHingeActuatorComponent", "Degree getMaxAngleLimit1()", "Gets the second max angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "void setMaxTorque1(float maxTorque)", "Sets the second max torque during rotation. Note: This will affect the rotation rate. "
			"Note: A high value will cause that the rotation will start slow and will be faster and at the end slow again.");
		AddClassToCollection("JointHingeActuatorComponent", "float getMaxTorque1()", "Gets the second max torque that is used during rotation.");
		AddClassToCollection("JointHingeActuatorComponent", "void setDirectionChange1(bool directionChange)", "Sets whether the direction should be changed, when the second rotation reaches the min or max angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "bool getDirectionChange1()", "Gets whether the direction is changed, when the second rotation reaches the min or max angle limit.");
		AddClassToCollection("JointHingeActuatorComponent", "void setRepeat1(bool repeat)", "Sets whether to repeat the second direction changed rotation. Note: If set to false the rotation will only go from min angle limit and back to max angular limit once.");
		AddClassToCollection("JointHingeActuatorComponent", "bool getRepeat1()", "Gets whether the second direction changed rotation is repeated.");

		module(lua)
			[
				class_<Joint6DofComponent, JointComponent>("Joint6DofComponent")
				// // .def("clone", &Joint6DofComponent::clone)
			.def("setAnchorPosition0", &Joint6DofComponent::setAnchorPosition0)
			.def("getAnchorPosition0", &Joint6DofComponent::getAnchorPosition0)
			.def("setAnchorPosition1", &Joint6DofComponent::setAnchorPosition1)
			.def("getAnchorPosition1", &Joint6DofComponent::getAnchorPosition1)
			.def("setMinMaxStopDistance", &Joint6DofComponent::setMinMaxStopDistance)
			.def("getMinStopDistance", &Joint6DofComponent::getMinStopDistance)
			.def("getMaxStopDistance", &Joint6DofComponent::getMaxStopDistance)
			.def("setMinMaxYawAngleLimit", &Joint6DofComponent::setMinMaxYawAngleLimit)
			.def("getMinYawAngleLimit", &Joint6DofComponent::getMinYawAngleLimit)
			.def("getMaxYawAngleLimit", &Joint6DofComponent::getMaxYawAngleLimit)
			.def("setMinMaxPitchAngleLimit", &Joint6DofComponent::setMinMaxPitchAngleLimit)
			.def("getMinPitchAngleLimit", &Joint6DofComponent::getMinPitchAngleLimit)
			.def("getMaxPitchAngleLimit", &Joint6DofComponent::getMaxPitchAngleLimit)
			.def("setMinMaxRollAngleLimit", &Joint6DofComponent::setMinMaxRollAngleLimit)
			.def("getMinRollAngleLimit", &Joint6DofComponent::getMinRollAngleLimit)
			.def("getMaxRollAngleLimit", &Joint6DofComponent::getMaxRollAngleLimit)
			];
		AddClassToCollection("Joint6DofComponent", "class inherits JointComponent", "Derived class from JointComponent. Joint with 6 degree of freedom.");
		AddClassToCollection("Joint6DofComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("Joint6DofComponent", "void setAnchorPosition0(Vector3 anchorPosition)", "Sets the first anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("Joint6DofComponent", "Vector3 getAnchorPosition0()", "Gets the first joint anchor position.");
		AddClassToCollection("Joint6DofComponent", "void setAnchorPosition1(Vector3 anchorPosition)", "Sets the second anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("Joint6DofComponent", "Vector3 getAnchorPosition1()", "Gets the second joint anchor position.");
		AddClassToCollection("Joint6DofComponent", "void setMinMaxStopDistance(float minStopDistance, float maxStopDistance)", "Sets the min and max stop distance.");
		AddClassToCollection("Joint6DofComponent", "float getMinStopDistance()", "Gets min stop distance.");
		AddClassToCollection("Joint6DofComponent", "float getMaxStopDistance()", "Gets max stop distance.");
		AddClassToCollection("Joint6DofComponent", "void setMinMaxYawAngleLimit(Degree minYawAngleLimit, Degree maxYawAngleLimit)", "Sets the min max yaw angle limit in degree.");
		AddClassToCollection("Joint6DofComponent", "Degree getMinYawAngleLimit()", "Gets the min yaw angle limit.");
		AddClassToCollection("Joint6DofComponent", "void setMinMaxPitchAngleLimit(Degree minPitchAngleLimit, Degree maxPitchAngleLimit)", "Sets the min max pitch angle limit in degree.");
		AddClassToCollection("Joint6DofComponent", "Degree getMinPitchAngleLimit()", "Gets the min pitch angle limit.");
		AddClassToCollection("Joint6DofComponent", "Degree getMaxPitchAngleLimit()", "Gets the max pitch angle limit.");
		AddClassToCollection("Joint6DofComponent", "void setMinMaxRollAngleLimit(Degree minRollAngleLimit, Degree maxRollAngleLimit)", "Sets the min max roll angle limit in degree.");
		AddClassToCollection("Joint6DofComponent", "Degree getMinRollAngleLimit()", "Gets the min roll angle limit.");
		AddClassToCollection("Joint6DofComponent", "Degree getMaxRollAngleLimit()", "Gets the max roll angle limit.");

		module(lua)
			[
				class_<JointMotorComponent, JointComponent>("JointMotorComponent")
				.def("setPin0", &JointMotorComponent::setPin0)
			.def("getPin0", &JointMotorComponent::getPin0)
			.def("setPin1", &JointMotorComponent::setPin1)
			.def("getPin1", &JointMotorComponent::getPin1)
			.def("setSpeed0", &JointMotorComponent::setSpeed0)
			.def("getSpeed0", &JointMotorComponent::getSpeed0)
			.def("setSpeed1", &JointMotorComponent::setSpeed1)
			.def("getSpeed1", &JointMotorComponent::getSpeed1)
			.def("setTorgue0", &JointMotorComponent::setTorgue0)
			.def("getTorgue0", &JointMotorComponent::getTorgue0)
			.def("setTorgue1", &JointMotorComponent::setTorgue1)
			.def("getTorgue1", &JointMotorComponent::getTorgue1)
			];

		AddClassToCollection("JointMotorComponent", "class inherits JointComponent", "Derived class from JointComponent. A motor joint, which needs another joint component as reference (predecessor), which should be rotated. Its also possible to use a second joint component (target) and pin for a different rotation.");
		AddClassToCollection("JointMotorComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointMotorComponent", "void setPin0(Vector3 pin0)", "Sets pin0 axis for the first reference body, to rotate around one dimension prependicular to the axis.");
		AddClassToCollection("JointMotorComponent", "Vector3 getPin0()", "Gets first reference body pin0 axis.");
		AddClassToCollection("JointMotorComponent", "void setPin1(Vector3 pin1)", "Sets pin1 axis for the second reference body, to rotate around one dimension prependicular to the axis. Note: This is optional.");
		AddClassToCollection("JointMotorComponent", "Vector3 getPin1()", "Gets second reference body pin1 axis.");
		AddClassToCollection("JointMotorComponent", "void setSpeed0(float speed0)", "Sets the rotating speed 0.");
		AddClassToCollection("JointMotorComponent", "float getSpeed0()", "Gets the rotating speed 0.");
		AddClassToCollection("JointMotorComponent", "void setSpeed1(float speed1)", "Sets the rotating speed 1.");
		AddClassToCollection("JointMotorComponent", "float getSpeed1()", "Gets the rotating speed 1.");
		AddClassToCollection("JointMotorComponent", "void setTorgue0(float torque0)", "Sets the torque rotation 0 for this joint.");
		AddClassToCollection("JointMotorComponent", "float getTorgue0()", "Gets the torque rotation 0 for this joint.");
		AddClassToCollection("JointMotorComponent", "void setTorgue1(float torque1)", "Sets the torque rotation 1 for this joint. Note: This is optional.");
		AddClassToCollection("JointMotorComponent", "float getTorgue1()", "Gets the torque rotation 1 for this joint.");

		module(lua)
			[
				class_<JointWheelComponent, JointComponent>("JointWheelComponent")
				.def("setAnchorPosition", &JointWheelComponent::setAnchorPosition)
			.def("getAnchorPosition", &JointWheelComponent::getAnchorPosition)
			.def("setPin", &JointWheelComponent::setPin)
			.def("getPin", &JointWheelComponent::getPin)
			.def("getSteeringAngle", &JointWheelComponent::getSteeringAngle)
			.def("setTargetSteeringAngle", &JointWheelComponent::setTargetSteeringAngle)
			.def("getTargetSteeringAngle", &JointWheelComponent::getTargetSteeringAngle)
			.def("setSteeringRate", &JointWheelComponent::setSteeringRate)
			.def("getSteeringRate", &JointWheelComponent::getSteeringRate)
			];

		AddClassToCollection("JointWheelComponent", "class inherits JointComponent", "Derived class from JointComponent. An object attached to a wheel joint can only rotate around one dimension perpendicular to the axis it is attached to and also steer.");
		AddClassToCollection("JointWheelComponent", "String getId()", "Gets the id of this joint.");
		AddClassToCollection("JointWheelComponent", "void setAnchorPosition(Vector3 anchorPosition)", "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.");
		AddClassToCollection("JointWheelComponent", "Vector3 getAnchorPosition()", "Gets joint anchor position.");
		AddClassToCollection("JointWheelComponent", "void setPin(Vector3 pin)", "Sets pin axis for the joint, to rotate around one dimension prependicular to the axis.");
		AddClassToCollection("JointWheelComponent", "Vector3 getPin()", "Gets joint pin axis.");

		AddClassToCollection("JointWheelComponent", "float getSteeringAngle()", "Gets current steering angle in degree.");

		AddClassToCollection("JointWheelComponent", "void setTargetSteeringAngle(float targetSteeringAngle)", "Sets the target steering angle in degree.");
		AddClassToCollection("JointWheelComponent", "float getTargetSteeringAngle()", "Gets the target steering angle in degree.");

		AddClassToCollection("JointWheelComponent", "void setSteeringRate(float steeringRate)", "Sets the steering rate.");
		AddClassToCollection("JointWheelComponent", "float getSteeringRate()", "Gets the steering rate.");
	}

	void bindLightComponents(lua_State* lua)
	{
		module(lua)
			[
				class_<LightDirectionalComponent, GameObjectComponent>("LightDirectionalComponent")
				// .def("getClassName", &LightDirectionalComponent::getClassName)
			.def("getParentClassName", &LightDirectionalComponent::getParentClassName)
			.def("setActivated", &LightDirectionalComponent::setActivated)
			.def("isActivated", &LightDirectionalComponent::isActivated)
			.def("setDiffuseColor", &LightDirectionalComponent::setDiffuseColor)
			.def("getDiffuseColor", &LightDirectionalComponent::getDiffuseColor)
			.def("setSpecularColor", &LightDirectionalComponent::setSpecularColor)
			.def("getSpecularColor", &LightDirectionalComponent::getSpecularColor)
			.def("setPowerScale", &LightDirectionalComponent::setPowerScale)
			.def("getPowerScale", &LightDirectionalComponent::getPowerScale)
			.def("setDirection", &LightDirectionalComponent::setDirection)
			.def("getDirection", &LightDirectionalComponent::getDirection)
			// .def("setAffectParentNode", &LightDirectionalComponent::setAffectParentNode)
			// .def("getAffectParentNode", &LightDirectionalComponent::getAffectParentNode)
			.def("setCastShadows", &LightDirectionalComponent::setCastShadows)
			.def("getCastShadows", &LightDirectionalComponent::getCastShadows)
			.def("setAttenuationRadius", &LightDirectionalComponent::setAttenuationRadius)
			.def("getAttenuationRadius", &LightDirectionalComponent::getAttenuationRadius)
			.def("setAttenuationLumThreshold", &LightDirectionalComponent::setAttenuationLumThreshold)
			.def("getAttenuationLumThreshold", &LightDirectionalComponent::getAttenuationLumThreshold)
			.def("setShowDummyEntity", &LightDirectionalComponent::setShowDummyEntity)
			.def("getShowDummyEntity", &LightDirectionalComponent::getShowDummyEntity)
			// .def("getOgreLight", &LightDirectionalComponent::getOgreLight)
			];

		AddClassToCollection("LightDirectionalComponent", "class inherits GameObjectComponent", "This Light simulates a huge source that is very far away - like daylight. Light hits the entire scene at the same angle everywhere.");
		// AddClassToCollection("LightDirectionalComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("LightDirectionalComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("LightDirectionalComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("LightDirectionalComponent", "void setActivated(bool activated)", "Sets whether this light type is visible.");
		AddClassToCollection("LightDirectionalComponent", "bool isActivated()", "Gets whether this light type is visible.");
		AddClassToCollection("LightDirectionalComponent", "void setDiffuseColor(Vector3 diffuseColor)", "Sets the diffuse color (r, g, b) of the light.");
		AddClassToCollection("LightDirectionalComponent", "Vector3 getDiffuseColor()", "Gets the diffuse color (r, g, b) of the light.");
		AddClassToCollection("LightDirectionalComponent", "void setSpecularColor(Vector3 specularColor)", "Sets the specular color (r, g, b) of the light.");
		AddClassToCollection("LightDirectionalComponent", "Vector3 getSpecularColor()", "Gets the specular color (r, g, b) of the light.");
		AddClassToCollection("LightDirectionalComponent", "void setPowerScale(float powerScale)", "Sets the power of the light. E.g. a value of 2 will make the light two times stronger. Default value is: PI");
		AddClassToCollection("LightDirectionalComponent", "float getPowerScale()", "Gets strength of the light.");
		AddClassToCollection("LightDirectionalComponent", "void setDirection(Vector3 direction)", "Sets the direction of the light.");
		AddClassToCollection("LightDirectionalComponent", "Vector3 getDirection()", "Gets the direction of the light.");
		AddClassToCollection("LightDirectionalComponent", "void setCastShadows(bool castShadows)", "Sets whether the game objects, that are affected by this light will cast shadows.");
		AddClassToCollection("LightDirectionalComponent", "bool getCastShadows()", "Gets whether the game objects, that are affected by this light are casting shadows.");
		AddClassToCollection("LightDirectionalComponent", "void setAttenuationRadius(float attenuationRadius)", "Sets the attenuation radius. Default value is 10.");
		AddClassToCollection("LightDirectionalComponent", "float getAttenuationRadius()", "Gets the attenuation radius.");
		AddClassToCollection("LightDirectionalComponent", "void setAttenuationLumThreshold(float attenuationLumThreshold)", "Sets the attenuation lumen threshold. Default value is 0.00192.");
		AddClassToCollection("LightDirectionalComponent", "float getAttenuationLumThreshold()", "Gets the attenuation lumen threshold.");
		AddClassToCollection("LightDirectionalComponent", "void setShowDummyEntity(bool showDummyEntity)", "Sets whether to show a dummy entity, "
			"because normally a light is not visible and its hard to find out, where it actually is, so this function might help for analysis purposes.");
		AddClassToCollection("LightDirectionalComponent", "bool getShowDummyEntity()", "Gets whether dummy entity is shown.");

		module(lua)
			[
				class_<LightPointComponent, GameObjectComponent>("LightPointComponent")
				// .def("getClassName", &LightPointComponent::getClassName)
			.def("getParentClassName", &LightPointComponent::getParentClassName)
			.def("setActivated", &LightPointComponent::setActivated)
			.def("isActivated", &LightPointComponent::isActivated)
			.def("setDiffuseColor", &LightPointComponent::setDiffuseColor)
			.def("getDiffuseColor", &LightPointComponent::getDiffuseColor)
			.def("setSpecularColor", &LightPointComponent::setSpecularColor)
			.def("getSpecularColor", &LightPointComponent::getSpecularColor)
			.def("setPowerScale", &LightPointComponent::setPowerScale)
			.def("getPowerScale", &LightPointComponent::getPowerScale)
			.def("setCastShadows", &LightPointComponent::setCastShadows)
			.def("getCastShadows", &LightPointComponent::getCastShadows)
			.def("setAttenuationMode", &LightPointComponent::setAttenuationMode)
			.def("getAttenuationMode", &LightPointComponent::getAttenuationMode)
			.def("setAttenuationRange", &LightPointComponent::setAttenuationRange)
			.def("getAttenuationRange", &LightPointComponent::getAttenuationRange)
			.def("setAttenuationConstant", &LightPointComponent::setAttenuationConstant)
			.def("getAttenuationConstant", &LightPointComponent::getAttenuationConstant)
			.def("setAttenuationLinear", &LightPointComponent::setAttenuationLinear)
			.def("getAttenuationLinear", &LightPointComponent::getAttenuationLinear)
			.def("setAttenuationQuadratic", &LightPointComponent::setAttenuationQuadratic)
			.def("getAttenuationQuadratic", &LightPointComponent::getAttenuationQuadratic)
			.def("setAttenuationRadius", &LightPointComponent::setAttenuationRadius)
			.def("getAttenuationRadius", &LightPointComponent::getAttenuationRadius)
			.def("setAttenuationLumThreshold", &LightPointComponent::setAttenuationLumThreshold)
			.def("getAttenuationLumThreshold", &LightPointComponent::getAttenuationLumThreshold)
			.def("setShowDummyEntity", &LightPointComponent::setShowDummyEntity)
			.def("getShowDummyEntity", &LightPointComponent::getShowDummyEntity)
			// .def("getOgreLight", &LightPointComponent::getOgreLight)
			];
		AddClassToCollection("LightPointComponent", "class inherits GameObjectComponent inherits GameObjectComponent", "This Light speads out equally in all directions from a point.");
		// AddClassToCollection("LightPointComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("LightPointComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("LightPointComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("LightPointComponent", "void setActivated(bool activated)", "Sets whether this light type is visible.");
		AddClassToCollection("LightPointComponent", "bool isActivated()", "Gets whether this light type is visible.");
		AddClassToCollection("LightPointComponent", "void setDiffuseColor(Vector3 diffuseColor)", "Sets the diffuse color (r, g, b) of the light.");
		AddClassToCollection("LightPointComponent", "Vector3 getDiffuseColor()", "Gets the diffuse color (r, g, b) of the light.");
		AddClassToCollection("LightPointComponent", "void setSpecularColor(Vector3 specularColor)", "Sets the specular color (r, g, b) of the light.");
		AddClassToCollection("LightPointComponent", "Vector3 getSpecularColor()", "Gets the specular color (r, g, b) of the light.");
		AddClassToCollection("LightPointComponent", "void setPowerScale(float powerScale)", "Sets the power of the light. E.g. a value of 2 will make the light two times stronger. Default value is: PI");
		AddClassToCollection("LightPointComponent", "float getPowerScale()", "Gets strength of the light.");
		AddClassToCollection("LightPointComponent", "void setCastShadows(bool castShadows)", "Sets whether the game objects, that are affected by this light will cast shadows.");
		AddClassToCollection("LightPointComponent", "bool getCastShadows()", "Gets whether the game objects, that are affected by this light are casting shadows.");
		AddClassToCollection("LightPointComponent", "void setAttenuationMode(String mode)", "Sets attenuation mode. Possible values are: 'Range', 'Radius'");
		AddClassToCollection("LightPointComponent", "String getAttenuationMode()", "Gets the attenuation mode.");
		AddClassToCollection("LightPointComponent", "void setAttenuationRange(float range)", "Sets attenuation range. Default value is: 23. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.");
		AddClassToCollection("LightPointComponent", "float getAttenuationRange()", "Gets the attenuation range.");
		AddClassToCollection("LightPointComponent", "void setAttenuationConstant(float constant)", "Sets attenuation constant. Default value is 5. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.");
		AddClassToCollection("LightPointComponent", "float getAttenuationConstant()", "Gets the attenuation constant.");
		AddClassToCollection("LightPointComponent", "void setAttenuationLinear(float linear)", "Sets linear attenuation. Default value is 0. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.");
		AddClassToCollection("LightPointComponent", "float getAttenuationLinear()", "Gets the linear attenuation constant.");
		AddClassToCollection("LightPointComponent", "void setAttenuationQuadratic(float quadric)", "Sets quadric attenuation. Default value is 0.5. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.");
		AddClassToCollection("LightPointComponent", "float getAttenuationQuadratic()", "Gets the quadric attenuation.");
		AddClassToCollection("LightPointComponent", "void setAttenuationRadius(float attenuationRadius)", "Sets the attenuation radius. Default value is 10. Note: This will only be applied, if @setAttenuationMode is set to 'Radius'.");
		AddClassToCollection("LightPointComponent", "float getAttenuationRadius()", "Gets the attenuation radius.");
		AddClassToCollection("LightPointComponent", "void setAttenuationLumThreshold(float attenuationLumThreshold)", "Sets the attenuation lumen threshold. Default value is 0.00192.");
		AddClassToCollection("LightPointComponent", "float getAttenuationLumThreshold()", "Gets the attenuation lumen threshold.");
		AddClassToCollection("LightPointComponent", "void setShowDummyEntity(bool showDummyEntity)", "Sets whether to show a dummy entity, "
			"because normally a light is not visible and its hard to find out, where it actually is, so this function might help for analysis purposes.");
		AddClassToCollection("LightPointComponent", "bool getShowDummyEntity()", "Gets whether dummy entity is shown.");

		module(lua)
			[
				class_<LightSpotComponent, GameObjectComponent>("LightSpotComponent")
				// .def("getClassName", &LightSpotComponent::getClassName)
			.def("getParentClassName", &LightSpotComponent::getParentClassName)
			.def("setActivated", &LightSpotComponent::setActivated)
			.def("isActivated", &LightSpotComponent::isActivated)
			.def("setDiffuseColor", &LightSpotComponent::setDiffuseColor)
			.def("getDiffuseColor", &LightSpotComponent::getDiffuseColor)
			.def("setSpecularColor", &LightSpotComponent::setSpecularColor)
			.def("getSpecularColor", &LightSpotComponent::getSpecularColor)
			.def("setPowerScale", &LightSpotComponent::setPowerScale)
			.def("getPowerScale", &LightSpotComponent::getPowerScale)
			.def("setSize", &LightSpotComponent::setSize)
			.def("getSize", &LightSpotComponent::getSize)
			.def("setNearClipDistance", &LightSpotComponent::setNearClipDistance)
			.def("getNearClipDistance", &LightSpotComponent::getNearClipDistance)
			.def("setCastShadows", &LightSpotComponent::setCastShadows)
			.def("getCastShadows", &LightSpotComponent::getCastShadows)
			.def("setAttenuationMode", &LightSpotComponent::setAttenuationMode)
			.def("getAttenuationMode", &LightSpotComponent::getAttenuationMode)
			.def("setAttenuationRange", &LightSpotComponent::setAttenuationRange)
			.def("getAttenuationRange", &LightSpotComponent::getAttenuationRange)
			.def("setAttenuationConstant", &LightSpotComponent::setAttenuationConstant)
			.def("getAttenuationConstant", &LightSpotComponent::getAttenuationConstant)
			.def("setAttenuationLinear", &LightSpotComponent::setAttenuationLinear)
			.def("getAttenuationLinear", &LightSpotComponent::getAttenuationLinear)
			.def("setAttenuationQuadratic", &LightSpotComponent::setAttenuationQuadratic)
			.def("getAttenuationQuadratic", &LightSpotComponent::getAttenuationQuadratic)
			.def("setAttenuationRadius", &LightSpotComponent::setAttenuationRadius)
			.def("getAttenuationRadius", &LightSpotComponent::getAttenuationRadius)
			.def("setAttenuationLumThreshold", &LightSpotComponent::setAttenuationLumThreshold)
			.def("getAttenuationLumThreshold", &LightSpotComponent::getAttenuationLumThreshold)

			.def("setShowDummyEntity", &LightSpotComponent::setShowDummyEntity)
			.def("getShowDummyEntity", &LightSpotComponent::getShowDummyEntity)
			// .def("getOgreLight", &LightSpotComponent::getOgreLight)
			];
		AddClassToCollection("LightSpotComponent", "class inherits GameObjectComponent", "This Light works like a flashlight. It produces a solid cylinder of light that is brighter at the center and fades off.");
		// AddClassToCollection("SpotLightComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("SpotLightComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("SpotLightComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("SpotLightComponent", "void setActivated(bool activated)", "Sets whether this light type is visible.");
		AddClassToCollection("SpotLightComponent", "bool isActivated()", "Gets whether this light type is visible.");
		AddClassToCollection("SpotLightComponent", "void setDiffuseColor(Vector3 diffuseColor)", "Sets the diffuse color (r, g, b) of the light.");
		AddClassToCollection("SpotLightComponent", "Vector3 getDiffuseColor()", "Gets the diffuse color (r, g, b) of the light.");
		AddClassToCollection("SpotLightComponent", "void setSpecularColor(Vector3 specularColor)", "Sets the specular color (r, g, b) of the light.");
		AddClassToCollection("SpotLightComponent", "Vector3 getSpecularColor()", "Gets the specular color (r, g, b) of the light.");
		AddClassToCollection("SpotLightComponent", "void setPowerScale(float powerScale)", "Sets the power of the light. E.g. a value of 2 will make the light two times stronger. Default value is: PI");
		AddClassToCollection("SpotLightComponent", "float getPowerScale()", "Gets strength of the light.");
		AddClassToCollection("SpotLightComponent", "void setSize(Vector3 size)", "Sets the size (x, y, z) of the spot light. Default value is: Vector3(30, 40, 1)");
		AddClassToCollection("SpotLightComponent", "Vector3 getSize()", "Gets the size (x, y, z) of the spotlight.");
		AddClassToCollection("SpotLightComponent", "void setNearClipDistance(float nearClipDistance)", "Sets near clip distance. Default value is: 0.1");
		AddClassToCollection("SpotLightComponent", "float getNearClipDistance()", "Gets the near clip distance.");
		AddClassToCollection("SpotLightComponent", "void setCastShadows(bool castShadows)", "Sets whether the game objects, that are affected by this light will cast shadows.");
		AddClassToCollection("SpotLightComponent", "bool getCastShadows()", "Gets whether the game objects, that are affected by this light are casting shadows.");
		AddClassToCollection("SpotLightComponent", "void setAttenuationMode(String mode)", "Sets attenuation mode. Possible values are: 'Range', 'Radius'");
		AddClassToCollection("SpotLightComponent", "Vector3 getAttenuationMode()", "Gets the attenuation mode.");
		AddClassToCollection("SpotLightComponent", "void setAttenuationRange(float range)", "Sets attenuation range. Default value is: 23. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.");
		AddClassToCollection("SpotLightComponent", "float getAttenuationRange()", "Gets the attenuation range.");
		AddClassToCollection("SpotLightComponent", "void setAttenuationConstant(float constant)", "Sets attenuation constant. Default value is 5. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.");
		AddClassToCollection("SpotLightComponent", "float getAttenuationConstant()", "Gets the attenuation constant.");
		AddClassToCollection("SpotLightComponent", "void setAttenuationLinear(float linear)", "Sets linear attenuation. Default value is 0. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.");
		AddClassToCollection("SpotLightComponent", "float getAttenuationLinear()", "Gets the linear attenuation constant.");
		AddClassToCollection("SpotLightComponent", "void setAttenuationQuadratic(float quadric)", "Sets quadric attenuation. Default value is 0.5. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.");
		AddClassToCollection("SpotLightComponent", "float getAttenuationQuadratic()", "Gets the quadric attenuation.");
		AddClassToCollection("SpotLightComponent", "void setAttenuationRadius(float attenuationRadius)", "Sets the attenuation radius. Default value is 10. Note: This will only be applied, if @setAttenuationMode is set to 'Radius'.");
		AddClassToCollection("SpotLightComponent", "float getAttenuationRadius()", "Gets the attenuation radius.");
		AddClassToCollection("SpotLightComponent", "void setAttenuationLumThreshold(float attenuationLumThreshold)", "Sets the attenuation lumen threshold. Default value is 0.00192.");
		AddClassToCollection("SpotLightComponent", "float getAttenuationLumThreshold()", "Gets the attenuation lumen threshold.");
		AddClassToCollection("SpotLightComponent", "void setShowDummyEntity(bool showDummyEntity)", "Sets whether to show a dummy entity, "
			"because normally a light is not visible and its hard to find out, where it actually is, so this function might help for analysis purposes.");
		AddClassToCollection("SpotLightComponent", "bool getShowDummyEntity()", "Gets whether dummy entity is shown.");

		module(lua)
			[
				class_<LightAreaComponent, GameObjectComponent>("LightAreaComponent")
				// .def("getClassName", &LightAreaComponent::getClassName)
			.def("getParentClassName", &LightAreaComponent::getParentClassName)
			.def("setActivated", &LightAreaComponent::setActivated)
			.def("isActivated", &LightAreaComponent::isActivated)
			.def("setDiffuseColor", &LightAreaComponent::setDiffuseColor)
			.def("getDiffuseColor", &LightAreaComponent::getDiffuseColor)
			.def("setSpecularColor", &LightAreaComponent::setSpecularColor)
			.def("getSpecularColor", &LightAreaComponent::getSpecularColor)
			.def("setPowerScale", &LightAreaComponent::setPowerScale)
			.def("getPowerScale", &LightAreaComponent::getPowerScale)
			.def("setCastShadows", &LightAreaComponent::setCastShadows)
			.def("getCastShadows", &LightAreaComponent::getCastShadows)
			.def("setAttenuationRadius", &LightAreaComponent::setAttenuationRadius)
			.def("getAttenuationRadius", &LightAreaComponent::getAttenuationRadius)
			// .def("setRectSize", &LightAreaComponent::setRectSize)
			// .def("getRectSize", &LightAreaComponent::getRectSize)
			.def("setTextureName", &LightAreaComponent::setTextureName)
			.def("getTextureName", &LightAreaComponent::getTextureName)
			.def("setDiffuseMipStart", &LightAreaComponent::setDiffuseMipStart)
			.def("getDiffuseMipStart", &LightAreaComponent::getDiffuseMipStart)
			.def("setDoubleSided", &LightAreaComponent::setDoubleSided)
			.def("getDoubleSided", &LightAreaComponent::getDoubleSided)
			// .def("setShowDummyEntity", &LightAreaComponent::setShowDummyEntity)
			// .def("getShowDummyEntity", &LightAreaComponent::getShowDummyEntity)
			// .def("getOgreLight", &LightAreaComponent::getOgreLight)
			];
		AddClassToCollection("LightAreaComponent", "class inherits GameObjectComponent", "An Area Light is defined by a rectangle in space. Light is emitted in all directions uniformly across their surface area, but only from one side of the rectangle.");
		// AddClassToCollection("LightAreaComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("LightAreaComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("LightAreaComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("LightAreaComponent", "void setActivated(bool activated)", "Sets whether this light type is visible.");
		AddClassToCollection("LightAreaComponent", "bool isActivated()", "Gets whether this light type is visible.");
		AddClassToCollection("LightAreaComponent", "void setDiffuseColor(Vector3 diffuseColor)", "Sets the diffuse color (r, g, b) of the light.");
		AddClassToCollection("LightAreaComponent", "Vector3 getDiffuseColor()", "Gets the diffuse color (r, g, b) of the light.");
		AddClassToCollection("LightAreaComponent", "void setSpecularColor(Vector3 specularColor)", "Sets the specular color (r, g, b) of the light.");
		AddClassToCollection("LightAreaComponent", "Vector3 getSpecularColor()", "Gets the specular color (r, g, b) of the light.");
		AddClassToCollection("LightAreaComponent", "void setPowerScale(float powerScale)", "Sets the power of the light. E.g. a value of 2 will make the light two times stronger. Default value is: PI");
		AddClassToCollection("LightAreaComponent", "float getPowerScale()", "Gets strength of the light.");
		AddClassToCollection("LightAreaComponent", "void setCastShadows(bool castShadows)", "Sets whether the game objects, that are affected by this light will cast shadows.");
		AddClassToCollection("LightAreaComponent", "bool getCastShadows()", "Gets whether the game objects, that are affected by this light are casting shadows.");
		AddClassToCollection("LightAreaComponent", "void setAttenuationRadius(float attenuationRadius)", "Sets the attenuation radius. Default value is 10.");
		AddClassToCollection("LightAreaComponent", "float getAttenuationRadius()", "Gets the attenuation radius.");
		AddClassToCollection("LightAreaComponent", "void setRectSize(Vector2 size)", "Sets the size (w, h) of the area light rectangle. Default value is: Vector2(1, 1).");
		AddClassToCollection("LightAreaComponent", "Vector2 getRectSize()", "Gets the size (w, h) of the area light rectangle.");
		AddClassToCollection("LightAreaComponent", "void setTextureName(String textureName)", "Sets the texture name for the area light for the light shining pattern. "
			"Note: If no texture is set, no texture mask is used and the light will emit from complete plane. Default value is: 'lightAreaPattern1.png'.");
		AddClassToCollection("LightAreaComponent", "String getTextureName()", "Gets the used texture name for the light shining pattern.");
		AddClassToCollection("LightAreaComponent", "void setDoubleSided(bool doubleSided)", "Specifies whether the light lits in both directions (positive & negative sides of the plane) or if only towards one.");
		AddClassToCollection("LightAreaComponent", "bool getDoubleSided()", "Gets whether the light lits in both directions (positive & negative sides of the plane) or if only towards one.");
	}

	void bindFogComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<FogComponent, GameObjectComponent>("FogComponent")
				// .def("getClassName", &FogComponent::getClassName)
			.def("getParentClassName", &FogComponent::getParentClassName)
			.def("setActivated", &FogComponent::setActivated)
			.def("isActivated", &FogComponent::isActivated)
			.def("setFogMode", &FogComponent::setFogMode)
			.def("getFogMode", &FogComponent::getFogMode)
			.def("setColor", &FogComponent::setColor)
			.def("getColor", &FogComponent::getColor)
			.def("setExpDensity", &FogComponent::setExpDensity)
			.def("getExpDensity", &FogComponent::getExpDensity)
			.def("setLinearStart", &FogComponent::setLinearStart)
			.def("getLinearStart", &FogComponent::getLinearStart)
			.def("setLinearEnd", &FogComponent::setLinearEnd)
			.def("getLinearEnd", &FogComponent::getLinearEnd)
			];
		AddClassToCollection("FogComponent", "class inherits GameObjectComponent", "Creates fog for the scene.");
		// AddClassToCollection("FogComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("FogComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("FogComponent", "void setActivated(bool activated)", "Sets whether fog is active or not.");
		AddClassToCollection("FogComponent", "bool isActivated()", "Gets whether fog is active or not.");
		AddClassToCollection("FogComponent", "void setFogMode(String mode)", "Sets the fog mode. Possible values are: 'Linear', 'Exponential', 'Exponential 2'");
		AddClassToCollection("FogComponent", "String getFogMode()", "Gets the fog mode.");
		AddClassToCollection("FogComponent", "void setColor(Vector3 color)", "Sets the color (r, g, b) of the fog. Either set this to the same as your viewport background color, or to blend in with a skybox.");
		AddClassToCollection("FogComponent", "Vector3 getColor()", "Gets the diffuse color (r, g, b) of the fog.");
		AddClassToCollection("FogComponent", "void setExpDensity(float expDensity)", "The density of the fog in 'Exponential' or 'Exponential 2' mode, as a value between 0 and 1. The default is 0.001. ");
		AddClassToCollection("FogComponent", "float getExpDensity()", "Gets the density of the fog in 'Exponential' or 'Exponential 2' mode.");
		AddClassToCollection("FogComponent", "void setLinearStart(float linearStart)", "Sets the distance in world units at which linear fog starts to encroach. Only applicable if mode is 'Linear'.");
		AddClassToCollection("FogComponent", "float getLinearStart()", "Gets the distance in world units at which linear fog starts to encroach. Only applicable if mode is 'Linear'.");
		AddClassToCollection("FogComponent", "void setLinearEnd(float linearEnd)", "Sets ths distance in world units at which linear fog becomes completely opaque. Only applicable if mode is 'Linear'.");
		AddClassToCollection("FogComponent", "float getLinearEnd()", "Gets ths distance in world units at which linear fog becomes completely opaque. Only applicable if mode is 'Linear'.");
	}

	void bindFadeComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<FadeComponent, GameObjectComponent>("FadeComponent")
			.def("getParentClassName", &FadeComponent::getParentClassName)
			.def("setActivated", &FadeComponent::setActivated)
			.def("isActivated", &FadeComponent::isActivated)
			.def("setFadeMode", &FadeComponent::setFadeMode)
			.def("getFadeMode", &FadeComponent::getFadeMode)
			.def("setDurationSec", &FadeComponent::setDurationSec)
			.def("getDurationSec", &FadeComponent::getDurationSec)
			.def("reactOnFadeCompleted", &FadeComponent::reactOnFadeCompleted)
		];
		AddClassToCollection("FadeComponent", "class inherits GameObjectComponent", "Creates a fading effect for the scene.");
		AddClassToCollection("FadeComponent", "void setActivated(bool activated)", "Activates the fading effect.");
		AddClassToCollection("FadeComponent", "bool isActivated()", "Gets whether fading is active or not.");
		AddClassToCollection("FadeComponent", "void setFadeMode(String mode)", "Sets the fade mode. Possible values are: 'FadeIn', 'FadeOut'");
		AddClassToCollection("FadeComponent", "String getFogMode()", "Gets the fog mode.");
		AddClassToCollection("FadeComponent", "void setDurationSec(float durationSec)", "Sets the duration in seconds for the fading.");
		AddClassToCollection("FadeComponent", "float getDurationSec()", "Gets the duration in seconds.");
		AddClassToCollection("FadeComponent", "void reactOnFadeCompleted(func closure)", "Sets whether to react at the moment when fade has completed.");
	}

	void bindPhysicsComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<PhysicsComponent, GameObjectComponent>("PhysicsComponent")
				//.def("getFromCast", &PhysicsObject::getFromCast)
				// .def("getClassName", &PhysicsComponent::getClassName)
			.def("getParentClassName", &PhysicsComponent::getParentClassName)
			// .def("clone", &PhysicsComponent::clone)
			.def("isMovable", &PhysicsComponent::isMovable)
			.def("setDirection", &PhysicsComponent::setDirection)
			.def("setOrientation", &PhysicsComponent::setOrientation)
			.def("getOrientation", &PhysicsComponent::getOrientation)
			.def("setPosition", (void (PhysicsComponent::*)(const Vector3&)) & PhysicsComponent::setPosition)
			.def("setPosition", (void (PhysicsComponent::*)(Real, Real, Real)) & PhysicsComponent::setPosition)
			.def("getPosition", &PhysicsComponent::getPosition)
			.def("setScale", &PhysicsComponent::setScale)
			.def("getScale", &PhysicsComponent::getScale)
			.def("translate", &PhysicsComponent::translate)
			.def("rotate", &PhysicsComponent::rotate)

			// .def("getBody", &PhysicsComponent::getBody)
			// .def("getOgreNewt", &PhysicsComponent::getOgreNewt)
			// .def("reCreateCollision", &PhysicsComponent::reCreateCollision)
			// .def("destroyCollision", &PhysicsComponent::destroyCollision)
			.def("setMass", &PhysicsComponent::setMass)
			.def("getMass", &PhysicsComponent::getMass)
			.def("setVolume", &PhysicsComponent::setVolume)
			.def("getVolume", &PhysicsComponent::getVolume)
			.def("setCollidable", &PhysicsComponent::setCollidable)
			.def("getCollidable", &PhysicsComponent::getCollidable)


			// .def("setCollisionType", &PhysicsComponent::setCollisionType)
			// .def("getCollisionType", &PhysicsComponent::getCollisionType)
			.def("getInitialPosition", &PhysicsComponent::getInitialPosition)
			.def("getInitialScale", &PhysicsComponent::getInitialScale)
			.def("getInitialOrientation", &PhysicsComponent::getInitialOrientation)
			];

		AddClassToCollection("PhysicsComponent", "class inherits GameObjectComponent", "Base class for some kind of physics components.");
		// AddClassToCollection("PhysicsComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("PhysicsComponent", "GameObjectComponent clone()", "Gets a new cloned game object component from this one.");
		// AddClassToCollection("PhysicsComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("PhysicsComponent", "bool isMovable()", "Gets whether this physics component is movable.");
		AddClassToCollection("PhysicsComponent", "void setDirection(Vector3 direction, Vector3 localDirectionVector)", "Sets the direction vector of the physics component. Note: local direction vector is NEGATIVE_UNIT_Z by default. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
		AddClassToCollection("PhysicsComponent", "void setOrientation(Quaternion orientation)", "Sets the orientation of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
		AddClassToCollection("PhysicsComponent", "Quaternion setOrientation()", "Gets the orientation of the physics component.");
		AddClassToCollection("PhysicsComponent", "void setPosition(Vector3 position)", "Sets the position of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
		AddClassToCollection("PhysicsComponent", "void setPosition2(float x, float, y, float z)", "Sets the position of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
		AddClassToCollection("PhysicsComponent", "Vector3 getPosition()", "Gets the position of the physics component.");
		AddClassToCollection("PhysicsComponent", "void setScale(Vector3 scale)", "Sets the scale of the physics component. Note: When scale has changed, the collision hull will be re-created.");
		AddClassToCollection("PhysicsComponent", "Vector3 getScale()", "Gets the scale of the physics component.");
		AddClassToCollection("PhysicsComponent", "void translate(Vector3 relativePosition)", "Sets relative position of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
		AddClassToCollection("PhysicsComponent", "void rotate(Quaternion relativeOrientation)", "Sets the relative orientation of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
		AddClassToCollection("PhysicsComponent", "void setMass(Vector3 mass)", "Sets the mass of the physics component. Default value is: 10. Note: Mass only makes sense for physics active movable components.");
		AddClassToCollection("PhysicsComponent", "void setVolume(float volume)", "Sets the volume of the physics component. Note: This can be used for game object, that are influenced by buoyancy.");

		AddClassToCollection("PhysicsComponent", "Vector3 getInitialPosition()", "Gets the initial position of the physics component (position at the time, the component has been created).");
		AddClassToCollection("PhysicsComponent", "Vector3 getInitialScale()", "Gets the initial scale of the physics component (scale at the time, the component has been created).");
		AddClassToCollection("PhysicsComponent", "Vector3 getInitialOrientation()", "Gets the initial orientation of the physics component (orientation at the time, the component has been created).");
		AddClassToCollection("PhysicsComponent", "void setCollidable(bool collidable)", "Sets whether this physics component is collidable.");
		AddClassToCollection("PhysicsComponent", "bool getCollidable()", "Gets whether this physics component is collidable.");
	}

	// Problem here: A joint gets created with an anchor Position, so changing a value, implies the joint needs to be deleted and recreated!
	/*void setJointAnchorPosition(PhysicsActiveComponent* instance, const Ogre::Vector3& anchorPosition)
	{
		instance->getJointProperties().anchorPosition = anchorPosition;
	}*/

	void bindPhysicsActiveComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<PhysicsActiveComponent::ContactData>("ContactData")
				.def("getHitGameObject", &PhysicsActiveComponent::ContactData::getHitGameObject)
			.def("getHeight", &PhysicsActiveComponent::ContactData::getHeight)
			.def("getNormal", &PhysicsActiveComponent::ContactData::getNormal)
			];

		module(lua)
		[
			class_<PhysicsActiveComponent, PhysicsComponent>("PhysicsActiveComponent")
			// .def("getClassName", &PhysicsActiveComponent::getClassName)
			// .def("clone", &PhysicsActiveComponent::clone)
			// .def("getClassId", &PhysicsActiveComponent::getClassId)
			.def("getParentClassName", &PhysicsActiveComponent::getParentClassName)
			.def("setActivated", &PhysicsActiveComponent::setActivated)
			.def("setLinearDamping", &PhysicsActiveComponent::setLinearDamping)
			.def("getLinearDamping", &PhysicsActiveComponent::getLinearDamping)
			.def("setAngularDamping", &PhysicsActiveComponent::setAngularDamping)
			.def("getAngularDamping", &PhysicsActiveComponent::getAngularDamping)
			.def("translate", &PhysicsActiveComponent::translate)
			.def("addImpulse", &PhysicsActiveComponent::addImpulse)
			.def("setVelocity", &PhysicsActiveComponent::setVelocity)
			.def("setDirectionVelocity", &PhysicsActiveComponent::setDirectionVelocity)
			.def("applyDirectionForce", &PhysicsActiveComponent::applyDirectionForce)
			.def("setMass", &PhysicsActiveComponent::setMass)
			.def("getMass", &PhysicsActiveComponent::getMass)
			.def("setMassOrigin", &PhysicsActiveComponent::setMassOrigin)
			.def("getMassOrigin", &PhysicsActiveComponent::getMassOrigin)
			.def("setSpeed", &PhysicsActiveComponent::setSpeed)
			.def("getSpeed", &PhysicsActiveComponent::getSpeed)
			.def("setMaxSpeed", &PhysicsActiveComponent::setMaxSpeed)
			.def("getMaxSpeed", &PhysicsActiveComponent::getMaxSpeed)
			.def("getMinSpeed", &PhysicsActiveComponent::getMinSpeed)
			.def("setOmegaVelocity", &PhysicsActiveComponent::setOmegaVelocity)
			.def("applyOmegaForce", &PhysicsActiveComponent::applyOmegaForce)
			.def("applyOmegaForceRotateTo", &PhysicsActiveComponent::applyOmegaForceRotateTo)
			.def("getOmegaVelocity", &PhysicsActiveComponent::getOmegaVelocity)
			.def("applyForce", &PhysicsActiveComponent::applyForce)
			.def("applyRequiredForceForVelocity", &PhysicsActiveComponent::applyRequiredForceForVelocity)
			.def("getForce", &PhysicsActiveComponent::getForce)
			.def("setGyroscopicTorqueEnabled", &PhysicsActiveComponent::setGyroscopicTorqueEnabled)
			.def("setContactSolvingEnabled", &PhysicsActiveComponent::setContactSolvingEnabled)
			.def("setGravity", &PhysicsActiveComponent::setGravity)
			.def("getGravity", &PhysicsActiveComponent::getGravity)
			.def("setCollisionDirection", &PhysicsActiveComponent::setCollisionDirection)
			.def("getCollisionDirection", &PhysicsActiveComponent::getCollisionDirection)
			.def("setCollisionPosition", &PhysicsActiveComponent::setCollisionPosition)
			.def("getCollisionPosition", &PhysicsActiveComponent::getCollisionPosition)
			.def("setCollisionSize", &PhysicsActiveComponent::setCollisionSize)
			.def("getCollisionSize", &PhysicsActiveComponent::getCollisionSize)
			.def("setConstraintAxis", &PhysicsActiveComponent::setConstraintAxis)
			.def("releaseConstraintAxis", &PhysicsActiveComponent::releaseConstraintAxis)
			.def("releaseConstraintAxisPin", &PhysicsActiveComponent::releaseConstraintAxisPin)
			.def("getConstraintAxis", &PhysicsActiveComponent::getConstraintAxis)
			.def("setConstraintDirection", &PhysicsActiveComponent::setConstraintDirection)
			.def("releaseConstraintDirection", &PhysicsActiveComponent::releaseConstraintDirection)
			.def("setGravitySourceCategory", &PhysicsActiveComponent::setGravitySourceCategory)
			.def("getGravitySourceCategory", &PhysicsActiveComponent::getGravitySourceCategory)
			.def("setContinuousCollision", &PhysicsActiveComponent::setContinuousCollision)
			.def("getContinuousCollision", &PhysicsActiveComponent::getContinuousCollision)

			// .def("getContactAhead", (OgreNewt::BasicRaycast::BasicRaycastInfo (PhysicsActiveComponent::*)(Ogre::Vector3 , Ogre::Real )) &PhysicsActiveComponent::getContactAhead)
			.def("getContactAhead", &PhysicsActiveComponent::getContactAhead)
			.def("getContactToDirection", &PhysicsActiveComponent::getContactToDirection)
			.def("getContactBelow", &PhysicsActiveComponent::getContactBelow)
			.def("setBounds", &PhysicsActiveComponent::setBounds)
		];

		AddClassToCollection("ContactData", "class", "Class, that holds contact data after a physics ray cast. See @getContactToDirection, @getContactBelow.");
		AddClassToCollection("ContactData", "GameObject getHitGameObject()", "Gets the hit game object, if nothing has been hit, null will be returned.");
		AddClassToCollection("ContactData", "float getHeight()", "Gets the height, at which the ray hit another game object. Note: This is usefull to check if a player is on ground.");
		AddClassToCollection("ContactData", "float getNormal()", "Gets normal of the hit game object. Note: This can be used to check if a ground is to steep.");

		AddClassToCollection("PhysicsActiveComponent", "class inherits PhysicsComponent", "Derived class of PhysicsComponent. It can be used for physically active game objects (movable).");
		// TODO: Check if its necessary, that each derived class must also set all functions, even they are already set in base class??
				/*AddClassToCollection("PhysicsActiveComponent", "void setDirection(Vector3 direction, Vector3 localDirectionVector)", "Sets the direction vector of the physics component. Note: local direction vector is NEGATIVE_UNIT_Z by default. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
				AddClassToCollection("PhysicsActiveComponent", "void setOrientation(Quaternion orientation)", "Sets the orientation of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
				AddClassToCollection("PhysicsActiveComponent", "Quaternion setOrientation()", "Gets the orientation of the physics component.");
				AddClassToCollection("PhysicsActiveComponent", "void setPosition(Vector3 position)", "Sets the position of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
				AddClassToCollection("PhysicsActiveComponent", "void setPosition2(float x, float, y, float z)", "Sets the position of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
				AddClassToCollection("PhysicsActiveComponent", "Vector3 getPosition()", "Gets the position of the physics component.");
				AddClassToCollection("PhysicsActiveComponent", "void setScale(Vector3 scale)", "Sets the scale of the physics component. Note: When scale has changed, the collision hull will be re-created.");
				AddClassToCollection("PhysicsActiveComponent", "Vector3 getScale()", "Gets the scale of the physics component.");
				AddClassToCollection("PhysicsActiveComponent", "void translate(Vector3 relativePosition)", "Sets relative position of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
				AddClassToCollection("PhysicsActiveComponent", "void rotate(Quaternion relativeOrientation)", "Sets the relative orientation of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
				*/
		AddClassToCollection("PhysicsActiveComponent", "void setLinearDamping(float linearDamping)", "Sets the linear damping. Range: [0, 1]");
		AddClassToCollection("PhysicsActiveComponent", "float getLinearDamping()", "Gets the linear damping.");
		AddClassToCollection("PhysicsActiveComponent", "void setAngularDamping(Vector3 angularDamping)", "Sets the angular damping. Range: [0, 1]");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getAngularDamping()", "Gets the angular damping.");
		AddClassToCollection("PhysicsActiveComponent", "void translate(Vector3 relativePosition)", "Sets relative position of the physics component.");
		AddClassToCollection("PhysicsActiveComponent", "void addImpulse(Vector3 deltaVector, Vector3 offsetPosition)", "Adds an impulse at the given offset position away from the origin of the physics body.");
		AddClassToCollection("PhysicsActiveComponent", "void setVelocity(Vector3 velocity)", "Sets the global linear velocity on the physics body. Note: This should only be used for initzialisation. Use @applyRequiredForceForVelocity in simualtion instead. Or it may be called if its a physics active kinematic body.");
		AddClassToCollection("PhysicsActiveComponent", "void setVelocityCurrentDirection(float speed)", "Sets the velocity speed for the current direction. Note: The default direction axis is applied. See: @GameObject::getDefaultDirection(). Note: This should only be set for initialization and not during simulation, as It could break physics calculation, or it may be called if its a physics active kinematic body.");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getVelocity()", "Gets currently acting velocity on the body.");
		AddClassToCollection("PhysicsActiveComponent", "void setMass(float mass)", "Sets mass for the body. Note: Internally the inertial values are adapted.");
		AddClassToCollection("PhysicsActiveComponent", "float getMass()", "Gets the mass of the body.");
		AddClassToCollection("PhysicsActiveComponent", "void setMassOrigin(Vector3 massOrigin)", "Sets mass origin for the body. Note: The mass origin is calculated automatically, but there are cases, in which the user wants"
			" to specify its own origin, e.g. balancing bird.");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getMassOrigin()", "Gets the manually set or calculated mass origin of the body.");

		AddClassToCollection("PhysicsActiveComponent", "void setSpeed(float speed)", "Sets the speed for this game object. Note: Speed is used in conjunction with other components like the AiMoveComponent etc.");
		AddClassToCollection("PhysicsActiveComponent", "float getSpeed()", "Gets the speed.");
		AddClassToCollection("PhysicsActiveComponent", "void setMaxSpeed(float maxSpeed)", "Sets the max speed for this game object. Note: Max speed is used in conjunction with other components like the AiMoveComponent etc.");
		AddClassToCollection("PhysicsActiveComponent", "float getMaxSpeed()", "Gets the max speed.");
		// Attention: Why can min speed not be set, and is set automatically when speed is set??
		AddClassToCollection("PhysicsActiveComponent", "float getMinSpeed()", "Gets the min speed.");
		AddClassToCollection("PhysicsActiveComponent", "void applyOmegaForce(Vector3 omegaForce)", "Applies omega force vector of the physics body. Note: This should be used during simulation instead of @setOmegaVelocity.");
		AddClassToCollection("PhysicsActiveComponent", "void applyOmegaForceRotateTo(Quaternion resultOrientation, Vector3 axes, Ogre::Real strength)", "Applies omega force in order to rotate the game object to the given orientation. "
			"The axes at which the rotation should occur (Vector3::UNIT_Y for y, Vector3::UNIT_SCALE for all axes, or just Vector3(1, 1, 0) for x,y axis etc.). "
			"The strength at which the rotation should occur. "
			"Note: This should be used during simulation instead of @setOmegaVelocity.");

		AddClassToCollection("PhysicsActiveComponent", "Vector3 setOmegaVelocity(Vector3 rotation)", "Set the omega velocity (rotation). Note: This should only be set for initialization and not during simulation, as It could break physics calculation. Use @applyAngularVelocity instead. Or it may be called if its a physics active kinematic body.");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getOmega()", "Gets the global angular velocity vector to the physics body. Note: This should be used in order to rotate a game object savily.");
		AddClassToCollection("PhysicsActiveComponent", "void applyOmegaForce(Vector3 omegaForce)", "Sets the omega force to the physics body. Similiar to @setForce(...) but for rotation.");
		AddClassToCollection("PhysicsActiveComponent", "void applyForce(Vector3 force)", "Sets the force to the body.");
		AddClassToCollection("PhysicsActiveComponent", "void applyRequiredForceForVelocity(Vector3 velocity)", "Applies the required force for the given velocity. Note: This should be used instead of setVelocity in simulation, for realistic physics behavior.");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getForce()", "Gets the current force acting on the game object.");

		AddClassToCollection("PhysicsActiveComponent", "void setGravity(Vector3 gravity)", "Sets the acting gravity for the physics body. Default value is Vector3(0, -19.8, 0).");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getGravity()", "Gets the acting gravity for the physics body.");
		AddClassToCollection("PhysicsActiveComponent", "void setGyroscopicTorqueEnabled(bool enable)", "Sets whether to enable gyroscopic precision for torque. See: https://www.youtube.com/watch?v=BCVQFoPO5qQ, https://www.youtube.com/watch?v=UlErvZoU7Q0.");
		AddClassToCollection("PhysicsActiveComponent", "void setContactSolvingEnabled(bool enable)", "Enables contact solving, so that in a lua script onContactSolving(otherPhysicsComponent) will be called, when two bodies are colliding.");

		AddClassToCollection("PhysicsActiveComponent", "void setCollisionDirection(Vector3 collisionOffsetDirection)", "Sets an offset collision direction for the collision hull. "
			"The collision hull will be rotated according to the given direction. Default is Vector3(0, 0, 0). Note: For convex hull, this cannot be used.");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getCollisionDirection()", "Gets the offset collision direction for the collision hull.");
		AddClassToCollection("PhysicsActiveComponent", "void setCollisionDirection(Vector3 collisionOffsetPosition)", "Sets an offset to the collision position of the collision hull. "
			"Note: This may be necessary when e.g. capsule is used and the origin point of the mesh is not in the middle of geometry. Default is Vector3(0, 0, 0). Note: For convex hull, this cannot be used.");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getCollisionPosition()", "Gets the offset to the collision position of the collision hull.");

		AddClassToCollection("PhysicsActiveComponent", "void setCollisionSize(Vector3 collisionSize)", "Sets collision size for a hull. Note: If convex hull is used, this cannot be used.");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getCollisionSize()", "Gets collision size.");
		AddClassToCollection("PhysicsActiveComponent", "void setConstraintAxis(Vector3 constraintAxis)", "Sets the constraint axis. The axis in the vector, that is set to '1' will be no more available for movement. "
			"E.g. Vector3(0, 0, 1), the physics body will only move on x, y-axis. Note: This can be used e.g. for a Jump 'n' Run game.");
		AddClassToCollection("PhysicsActiveComponent", "void releaseConstraintAxis()", "Releases the constraint axis, so that the game object is no more clipped by a plane.");
		AddClassToCollection("PhysicsActiveComponent", "void releaseConstraintAxisPin()", "Releases the pin of the constraint axis, so that the game object is still clipped on a plane but can rotate freely.");


		AddClassToCollection("PhysicsActiveComponent", "Vector3 getConstraintAxis()", "Gets the constraint axis.");
		AddClassToCollection("PhysicsActiveComponent", "void applyConstraintDirection(Vector3 constraintDirection)", "Sets the constraint direction. The physics body can only be rotated on that axis. "
			"E.g. Vector3(0, 1, 0) the physics body will always stand up. Note: This is useful when creating a character, that should move and not fall.");
		AddClassToCollection("PhysicsActiveComponent", "void releaseConstraintDirection()", "Releases the set constraint direction. @See setConstraintDirection(...) for further details.");

		AddClassToCollection("PhysicsActiveComponent", "void setGravitySourceCategory(String gravitySourceCategory)", "Sets the gravity source category. If there is a gravity source game object, gravity in that direction of the source will be calculated, "
			"but only work with the nearest object, else the force will mess up. Note: This is good for movement on a planet.");
		AddClassToCollection("PhysicsActiveComponent", "String getGravitySourceCategory()", "Gets the gravity source category.");

		// Is now: setActivated
		// AddClassToCollection("PhysicsActiveComponent", "void applySleep(bool sleep)", "Sets whether the physics body should sleep. It will be removed from the physics calculation for performance reasons. "
		// 												"Note: The sleep state will be changed, when another physics body does touch this sleeping physics body.");

		AddClassToCollection("PhysicsActiveComponent", "void setContinuousCollision(bool continuousCollision)", "Sets whether to use continuous collision, if set to true, "
			"the collision hull will be slightly resized, so that a fast moving physics body will still collide with other bodies. Note: This comes with a performance impact.");
		AddClassToCollection("PhysicsActiveComponent", "bool getContinuousCollision()", "Gets whether continuous collision mode is used.");
		AddClassToCollection("PhysicsActiveComponent", "void setAngularDamping(Vector3 angularDamping)", "Sets the angular damping. Range: [0, 1]");
		AddClassToCollection("PhysicsActiveComponent", "Vector3 getAngularDamping()", "Gets the angular damping.");

		AddClassToCollection("PhysicsActiveComponent", "ContactData getContactAhead(Vector3 offset, float length, bool forceDrawLine)", "Gets a contact data if there was a physics ray contact a head of the physics body. "
			"The direction is for the ray direction to shoot and the offset, at which offset position away from the physics component. "
			"The length of the ray in meters. If 'drawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false.");

		AddClassToCollection("PhysicsActiveComponent", "ContactData getContactToDirection(Vector3 direction, Vector3 offset, float from, float to, bool forceDrawLine)", "Gets a contact data if there was a physics ray contact with a physics body. "
			"The direction is for the ray direction to shoot and the offset, at which offset position away from the physics component. "
			"The value 'from' is from which distance to shoot the ray and the value 'to' is the length of the ray in meters. If 'drawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false.");

		AddClassToCollection("PhysicsActiveComponent", "ContactData getContactBelow(Vector3 offset, bool forceDrawLine)", "Gets a contact data below the physics body. "
			"The offset is used to set the offset position away from the physics component. "
			"The value 'from' is from which distance to shoot the ray and the value 'to' is the length of the ray in meters. If 'drawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false.");
		AddClassToCollection("PhysicsActiveComponent", "void setBounds(Vector3 minBounds, Vector3 maxBounds)", "Sets the min max bounds, until which this body can be moved.");
     }

	void bindPhysicsActiveCompoundComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<PhysicsActiveCompoundComponent, PhysicsActiveComponent>("PhysicsActiveCompoundComponent")
				// .def("getClassName", &PhysicsActiveCompoundComponent::getClassName)
			.def("getParentClassName", &PhysicsActiveCompoundComponent::getParentClassName)
			// .def("clone", &PhysicsActiveCompoundComponent::clone)
			// .def("getClassId", &PhysicsActiveCompoundComponent::getClassId)
			.def("setMeshCompoundConfigFile", &PhysicsActiveCompoundComponent::setMeshCompoundConfigFile)
			];
		AddClassToCollection("PhysicsActiveCompoundComponent", "class inherits PhysicsActiveComponent", "Derived class of PhysicsActiveComponent. It can be used for more complex compound collision hulls for physically active game objects (movable).");
		AddClassToCollection("PhysicsActiveCompoundComponent", "void setMeshCompoundConfigFile(String collisionHullFileXML)", "Sets the collision hull XML file name, which describes the sub-collision hull, that are forming the compound. "
			"E.g. torus.xml. Note: Normally only simple collision hull creation is only possible like a cylinder, etc. But when e.g. a torus should be created, its a complex collision hull, because even "
			"a convex hull cannot describe the torus, because there would be no hole anymore. But a torus can be formed of several curved pipes. Even a chain could be created that way.");
	}

	void bindPhysicsActiveDestructableComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<PhysicsActiveDestructableComponent, PhysicsActiveComponent>("PhysicsActiveDestructableComponent")
				// .def("getClassName", &PhysicsActiveDestructableComponent::getClassName)
			.def("getParentClassName", &PhysicsActiveDestructableComponent::getParentClassName)
			// .def("clone", &PhysicsActiveDestructableComponent::clone)
			// .def("getClassId", &PhysicsActiveDestructableComponent::getClassId)
			.def("setOrientation", &PhysicsActiveDestructableComponent::setOrientation)
			.def("getOrientation", &PhysicsActiveDestructableComponent::getOrientation)
			.def("setPosition", (void (PhysicsActiveDestructableComponent::*)(const Vector3&)) & PhysicsActiveDestructableComponent::setPosition)
			.def("setPosition", (void (PhysicsActiveDestructableComponent::*)(Real, Real, Real)) & PhysicsActiveDestructableComponent::setPosition)
			.def("getPosition", &PhysicsActiveDestructableComponent::getPosition)
			.def("addImpulse", &PhysicsActiveDestructableComponent::addImpulse)
			.def("translate", &PhysicsActiveDestructableComponent::translate)
			.def("setVelocity", &PhysicsActiveDestructableComponent::setVelocity)
			.def("setBreakForce", &PhysicsActiveDestructableComponent::setBreakForce)
			.def("setBreakTorque", &PhysicsActiveDestructableComponent::setBreakTorque)
			.def("setDensity", &PhysicsActiveDestructableComponent::setDensity)
			.def("setMotionLess", &PhysicsActiveDestructableComponent::setMotionLess)
			];

		AddClassToCollection("PhysicsActiveDestructableComponent", "class inherits PhysicsActiveComponent", "Derived class of PhysicsActiveComponent. It can be used to created several random pieces for a collision hull and tied together for desctruction. "
			"Note: When this component is used, several peaces of the mesh will be created automatically. This should not be used for complex collisions, as it may crash, or no pieces can be created.");
		AddClassToCollection("PhysicsActiveDestructableComponent", "void setBreakForce(float breakForce)", "Sets the break force, that is required to tear this physics body into peaces. Default value is 85.");
		AddClassToCollection("PhysicsActiveDestructableComponent", "void setBreakTorque(float breakTorque)", "Sets the break torque, that is required to tear this physics body into peaces. Default value is 85.");
		AddClassToCollection("PhysicsActiveDestructableComponent", "void setDensity(float density)", "Sets the density of physics body, for more stability. Default value is 60.");
		AddClassToCollection("PhysicsActiveDestructableComponent", "void setMotionLess(bool motionless)", "If the physics body is configured as motionless. It will behave like a normal artifact physics body and cannot be moved.");
		AddClassToCollection("PhysicsActiveDestructableComponent", "void setVelocity(Vector3 velocity)", "Sets the global linear velocity on the physics body. Note: This should only be used for initzialisation. Use @applyRequiredForceForVelocity in simualtion instead. Or it may be called if its a physics active kinematic body.");
		AddClassToCollection("PhysicsActiveDestructableComponent", "Vector3 getVelocity()", "Gets currently acting velocity on the body.");
	}

	void bindPhysicsActiveKinematicComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<PhysicsActiveKinematicComponent, PhysicsActiveComponent>("PhysicsActiveKinematicComponent")
			// .def("getClassName", &PhysicsActiveKinematicComponent::getClassName)
			.def("getParentClassName", &PhysicsActiveKinematicComponent::getParentClassName)
			// .def("clone", &PhysicsActiveKinematicComponent::clone)
			// .def("getClassId", &PhysicsActiveKinematicComponent::getClassId)
			.def("setCollidable", &PhysicsActiveKinematicComponent::setCollidable)
			.def("getCollidable", &PhysicsActiveKinematicComponent::getCollidable)
			.def("setOmegaVelocityRotateTo", &PhysicsActiveKinematicComponent::setOmegaVelocityRotateTo)
		];

		AddClassToCollection("PhysicsActiveKinematicComponent", "class inherits PhysicsActiveComponent", "Derived class of PhysicsActiveComponent. It is a special kind of physics component. It does not react on forces but just on velocity and collision system.");
		AddClassToCollection("PhysicsActiveKinematicComponent", "void setCollidable(bool collidable)", "Sets whether this body is collidable with other physics bodies.");
		AddClassToCollection("PhysicsActiveKinematicComponent", "bool getCollidable()", "Gets whether this body is collidable with other phyisics bodies.");
		AddClassToCollection("PhysicsActiveKinematicComponent", "void setOmegaVelocityRotateTo(Quaternion resultOrientation, Vector3 axes, Ogre::Real strength)", "Sets the omega velocity to rotate the game object to the given result orientation. "
			"The axes at which the rotation should occur (Vector3::UNIT_Y for y, Vector3::UNIT_SCALE for all axes, or just Vector3(1, 1, 0) for x,y axis etc.). "
			"The strength at which the rotation should occur.");
	}

	void bindPhysicsArtifactComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<PhysicsArtifactComponent, PhysicsComponent>("PhysicsArtifactComponent")
				// .def("getClassName", &PhysicsArtifactComponent::getClassName)
			.def("getParentClassName", &PhysicsArtifactComponent::getParentClassName)
			// .def("clone", &PhysicsArtifactComponent::clone)
			// .def("getClassId", &PhysicsArtifactComponent::getClassId)
			// .def("reCreateCollision", &PhysicsArtifactComponent::reCreateCollision)
			// .def("setSerialize", &PhysicsArtifactComponent::setSerialize)
			// .def("getSerialize", &PhysicsArtifactComponent::getSerialize)
			];
		AddClassToCollection("PhysicsArtifactComponent", "class inherits PhysicsComponent", "Derived class of PhysicsActiveComponent. " + PhysicsArtifactComponent::getStaticInfoText());
	}

	void bindPhysicsCompoundConnectionComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<PhysicsCompoundConnectionComponent, PhysicsActiveComponent>("PhysicsCompoundConnectionComponent")
				// .def("getClassName", &PhysicsCompoundConnectionComponent::getClassName)
			.def("getParentClassName", &PhysicsCompoundConnectionComponent::getParentClassName)
			// .def("clone", &PhysicsCompoundConnectionComponent::clone)
			// .def("getClassId", &PhysicsCompoundConnectionComponent::getClassId)
			// .def("inheritVelOmega", &PhysicsCompoundConnectionComponent::inheritVelOmega)
			.def("setActivated", &PhysicsCompoundConnectionComponent::setActivated)
			// .def("isActive", &PhysicsCompoundConnectionComponent::isActivated)
			.def("createCompoundCollision", &PhysicsCompoundConnectionComponent::createCompoundCollision)
			.def("destroyCompoundCollision", &PhysicsCompoundConnectionComponent::destroyCompoundCollision)
			.def("getId", &PhysicsCompoundConnectionComponent::getId)
			.def("setRootId", &PhysicsCompoundConnectionComponent::setRootId)
			.def("getRootId", &PhysicsCompoundConnectionComponent::getRootId)
			.def("getPriorId", &PhysicsCompoundConnectionComponent::getPriorId)
			// .def("addPhysicsActiveComponent", &PhysicsCompoundConnectionComponent::addPhysicsActiveComponent)
			];
		// No lua api?
	}

	void bindPhysicsExplosionComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<PhysicsExplosionComponent, GameObjectComponent>("PhysicsExplosionComponent")
				// .def("getClassName", &PhysicsExplosionComponent::getClassName)
			.def("getParentClassName", &PhysicsExplosionComponent::getParentClassName)
			// .def("clone", &PhysicsExplosionComponent::clone)
			// .def("getClassId", &PhysicsExplosionComponent::getClassId)
			.def("setActivated", &PhysicsExplosionComponent::setActivated)
			.def("isActivated", &PhysicsExplosionComponent::isActivated)
			.def("setAffectedCategories", &PhysicsExplosionComponent::setAffectedCategories)
			.def("getAffectedCategories", &PhysicsExplosionComponent::getAffectedCategories)
			.def("setExplosionCountDownSec", &PhysicsExplosionComponent::setExplosionCountDownSec)
			.def("getExplosionCountDownSec", &PhysicsExplosionComponent::getExplosionCountDownSec)
			.def("setExplosionRadius", &PhysicsExplosionComponent::setExplosionRadius)
			.def("getExplosionRadius", &PhysicsExplosionComponent::getExplosionRadius)
			.def("setExplosionStrengthN", &PhysicsExplosionComponent::setExplosionStrengthN)
			.def("getExplosionStrengthN", &PhysicsExplosionComponent::getExplosionStrengthN)
			];

		AddClassToCollection("PhysicsExplosionComponent", "class inherits GameObjectComponent", PhysicsExplosionComponent::getStaticInfoText());
		// AddClassToCollection("PhysicsExplosionComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("PhysicsExplosionComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("PhysicsExplosionComponent", "void setActivated(bool activated)", "Activates the components behaviour, so that explosion will be controller by the explosion timer.");
		AddClassToCollection("PhysicsExplosionComponent", "bool isActivated()", "Gets whether explosion has been started.");
		AddClassToCollection("PhysicsExplosionComponent", "void setAffectedCategories(String categories)", "Sets affected categories. Note: This function can be used e.g. to exclude some game object, even if there were at range when the detonation occurred.");
		AddClassToCollection("PhysicsExplosionComponent", "String getAffectedCategories()", "Gets the affected categories.");
		AddClassToCollection("PhysicsExplosionComponent", "void setExplosionCountDownSec(float countDown)", "Sets the explosion timer in seconds. The timer starts to count down, when the component is activated.");
		AddClassToCollection("PhysicsExplosionComponent", "float getExplosionCountDownSec()", "Gets the spawn interval in seconds.");
		AddClassToCollection("PhysicsExplosionComponent", "void setExplosionRadius(float radius)", "Sets the explosion radius in meters.");
		AddClassToCollection("PhysicsExplosionComponent", "float getExplosionRadius()", "Gets the explosion radius in meters.");
		AddClassToCollection("PhysicsExplosionComponent", "void setExplosionStrengthN(float strengthN)", "Sets the explosion strength in newton. Note: The given explosion strength is a maximal value. The far away an affected game object is away from the explosion center the weaker the detonation.");
		AddClassToCollection("PhysicsExplosionComponent", "float getExplosionStrengthN()", "Gets the explosion strength in newton.");
	}

	void bindPhysicsPlayerControllerComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<PhysicsPlayerControllerComponent, PhysicsActiveComponent>("PhysicsPlayerControllerComponent")
			// .def("getClassName", &PhysicsPlayerControllerComponent::getClassName)
			.def("getParentClassName", &PhysicsPlayerControllerComponent::getParentClassName)
			// .def("clone", &PhysicsPlayerControllerComponent::clone)
			// .def("getClassId", &PhysicsPlayerControllerComponent::getClassId)
			.def("move", &PhysicsPlayerControllerComponent::move)
			.def("toggleCrouch", &PhysicsPlayerControllerComponent::toggleCrouch)
			.def("jump", &PhysicsPlayerControllerComponent::jump)
			.def("getFrame", &PhysicsPlayerControllerComponent::getFrame)
			.def("setRadius", &PhysicsPlayerControllerComponent::setRadius)
			.def("getRadius", &PhysicsPlayerControllerComponent::getRadius)
			.def("setHeight", &PhysicsPlayerControllerComponent::setHeight)
			.def("getHeight", &PhysicsPlayerControllerComponent::getHeight)
			.def("setStepHeight", &PhysicsPlayerControllerComponent::setStepHeight)
			.def("getStepHeight", &PhysicsPlayerControllerComponent::getStepHeight)
			.def("getUpDirection", &PhysicsPlayerControllerComponent::getUpDirection)
			.def("isInFreeFall", &PhysicsPlayerControllerComponent::isInFreeFall)
			.def("isOnFloor", &PhysicsPlayerControllerComponent::isOnFloor)
			.def("isCrouching", &PhysicsPlayerControllerComponent::isCrouching)
			.def("setJumpSpeed", &PhysicsPlayerControllerComponent::setJumpSpeed)
			.def("getJumpSpeed", &PhysicsPlayerControllerComponent::getJumpSpeed)
			.def("getForwardSpeed", &PhysicsPlayerControllerComponent::getForwardSpeed)
			.def("getSideSpeed", &PhysicsPlayerControllerComponent::getSideSpeed)
			.def("getHeading", &PhysicsPlayerControllerComponent::getHeading)
			.def("getVelocity", &PhysicsPlayerControllerComponent::getVelocity)
		];
		AddClassToCollection("PhysicsPlayerControllerComponent", "class inherits PhysicsActiveComponent", "Derived class of PhysicsActiveComponent. It can be used to control a player with jump/stairs up going functionality.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "void move(float forwardSpeed, float sideSpeed, Radian headingAngleRad)", "Moves the the player according to the forward, sidespeed and heading angle in radian");
		AddClassToCollection("PhysicsPlayerControllerComponent", "void toggleCrouch()", "Toggles whether the player is crouching or not. Note: When crouching, the corresponding animation should be changed.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "void jump()", "Applies a player jump according to the given Jump speed. Note: developer should take care whether player can jump by using @isOnFloor function.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "Quaternion getFrame()", "Gets the current local orientation.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "void setRadius(float radius)", "Sets the player collision hull radius.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "float getRadius()", "Gets the player collision hull radius.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "void setHeight(float height)", "Sets the player collision hull height.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "float getHeight()", "Gets the player collision hull height.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "void setStepHeight(float stepHeight)", "Sets the max. step height, the player is able to walk up.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "float getStepHeight()", "Gets the max. step height, the player is able to walk up.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "Vector3 getupDirection()", "Gets the up direction of the player.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "bool isInFreeFall()", "Gets whether the player is in free fall.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "bool isOnFloor()", "Gets whether the player is on floor.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "bool isCrouching()", "Gets whether the player is crouching.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "void setJumpSpeed(float jumpSpeed)", "Sets the player jump speed. Default value is 20.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "float getJumpSpeed()", "Gets the player collision jump speed.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "float getForwardSpeed()", "Gets the player current forward speed when moving.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "float getSideSpeed()", "Gets the player current side speed when moving.");
		AddClassToCollection("PhysicsPlayerControllerComponent", "float getHeading()", "Gets the player current heading in degrees value.");

		module(lua)
		[
			class_<PlayerContact>("PlayerContact")
			.def("getPosition", &PlayerContact::getPosition)
			.def("getNormal", &PlayerContact::getNormal)
			.def("getPenetration", &PlayerContact::getPenetration)
			.def("setResultFriction", &PlayerContact::setResultFriction)
			.def("getResultFriction", &PlayerContact::getResultFriction)
		];
		AddClassToCollection("PlayerContact", "PlayerContact", "It can be used in the player controller component contact callback to get collision data and set the result friction.");
		AddClassToCollection("PlayerContact", "Vector3 getPosition()", "Gets the collision position.");
		AddClassToCollection("PlayerContact", "Vector3 getNormal()", "Gets the collision normal.");
		AddClassToCollection("PlayerContact", "float getPenetration()", "Gets the penetration between the two collided game objects.");
		AddClassToCollection("PlayerContact", "void setResultFriction(float resultFriction)", "Sets the result friction. With that its possible to control how much friction the player will get on the ground.");
	}

	luabind::object getPositionAndNormal(OgreNewt::Contact* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		Ogre::Vector3 position = Ogre::Vector3::ZERO;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		instance->getPositionAndNormal(position, normal, instance->getBody0());

		obj[0] = position;
		obj[1] = normal;

		return obj;
	}

	luabind::object getTangentDirections(OgreNewt::Contact* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		Ogre::Vector3 direction1 = Ogre::Vector3::ZERO;
		Ogre::Vector3 direction2 = Ogre::Vector3::ZERO;

		instance->getContactTangentDirections(instance->getBody0(), direction1, direction2);

		obj[0] = direction1;
		obj[1] = direction2;

		return obj;
	}

	void bindPhysicsMaterialComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<OgreNewt::Contact>("Contact")
				.def("getNormalSpeed", &OgreNewt::Contact::getNormalSpeed)
			// .def("getForce", &OgreNewt::Contact::getForce)
			.def("setContactPosition", &OgreNewt::Contact::setContactPosition)
			.def("getPositionAndNormal", &getPositionAndNormal)
			.def("getContactTangentDirections", &getTangentDirections)
			.def("getTangentSpeed", &OgreNewt::Contact::getTangentSpeed)
			.def("setSoftness", &OgreNewt::Contact::setSoftness)
			.def("setElasticity", &OgreNewt::Contact::setElasticity)
			.def("setFrictionState", &OgreNewt::Contact::setFrictionState)
			.def("setFrictionCoefficient", &OgreNewt::Contact::setFrictionCoef)
			.def("setTangentAcceleration", &OgreNewt::Contact::setTangentAcceleration)
			.def("setNormalDirection", &OgreNewt::Contact::setNormalDirection)
			.def("setNormalAcceleration", &OgreNewt::Contact::setNormalAcceleration)
			.def("setRotateTangentDirections", &OgreNewt::Contact::setRotateTangentDirections)
			.def("getContactForce", &OgreNewt::Contact::getContactForce)
			.def("getContactMaxNormalImpact", &OgreNewt::Contact::getContactMaxNormalImpact)
			.def("setContactTangentFriction", &OgreNewt::Contact::setContactTangentFriction)
			.def("getContactMaxTangentImpact", &OgreNewt::Contact::getContactMaxTangentImpact)
			.def("setContactPruningTolerance", &OgreNewt::Contact::setContactPruningTolerance)
			.def("getContactPruningTolerance", &OgreNewt::Contact::getContactPruningTolerance)
			.def("getContactPenetration", &OgreNewt::Contact::getContactPenetration)
			.def("getClosestDistance", &OgreNewt::Contact::getClosestDistance)
			];

		AddClassToCollection("Contact", "class", "Contact can be used, to get details information when a collision of two bodies occured and to control, what should happen with them.");
		AddClassToCollection("Contact", "float getNormalSpeed()", "Gets the speed at the contact normal.");
		AddClassToCollection("Contact", "void setContactPosition(Vector3 contactPosition)", "Sets the contact position.");
		AddClassToCollection("Contact", "table[position][normal] getPositionAndNormal()", "Gets the contact position and normal.");
		AddClassToCollection("Contact", "table[direction1][direction2] getTangentDirections()", "Gets the contact tangent vector. Returns the contact primary tangent vector and the contact secondary tangent vector.");
		AddClassToCollection("Contact", "float getTangentSpeed()", "Gets the speed of this contact along the tangent vector of the contact.");
		AddClassToCollection("Contact", "void setSoftness(float softness)", "Overrides the default softness for this contact.");
		AddClassToCollection("Contact", "void setElasticity(float elasticity)", "Overrides the default elasticity for this contact.");
		AddClassToCollection("Contact", "void setFrictionState(int state, int index)", "Enables or disables friction calculation for this contact. Note: State 0 makes the contact frictionless along the index tangent vector. The index 0 is for primary tangent vector or 1 for the secondary tangent.");
		AddClassToCollection("Contact", "void setFrictionCoefficient(float static, float kinetic, int index)", "Overrides the default value of the kinetic and static coefficient of friction for this contact. Note: Static friction coefficient must be positive. Kinetic friction coeeficient must be positive. The index 0 is for primary tangent vector or 1 for the secondary tangent.");
		AddClassToCollection("Contact", "void setTangentAcceleration(float acceleration)", "Forces the contact point to have a non-zero acceleration along the surface plane. Note: The index 0 is for primary tangent vector or 1 for the secondary tangent.");
		AddClassToCollection("Contact", "void setRotateTangentDirections(Vector3 direction)", "Rotates the tangent direction of the contacts until the primary direction is aligned with the align Vector. Details: This function rotates the tangent vectors of the "
			"contact point until the primary tangent vector and the align vector are perpendicular (ex. when the dot product between the primary tangent vector and the alignVector is 1.0). This function can be used in conjunction with @setTangentAcceleration in order to create special effects. For example, conveyor belts, cheap low LOD vehicles, slippery surfaces, etc.");
		AddClassToCollection("Contact", "void setNormalDirection(Vector3 direction)", "Sets the new direction of the for this contact point. Note: This function changes the basis of the contact point to one where the contact normal is aligned to the new direction vector and the tangent direction are recalculated to be perpendicular to the new contact normal.");
		AddClassToCollection("Contact", "void setNormalAcceleration(float acceleration)", "Forces the contact point to have a non-zero acceleration aligned this the contact normal. Note: This function can be used for special effects like implementing jump, of explosive contact in a call back.");
		AddClassToCollection("Contact", "float getContactForce()", "Gets the contact force vector in global space. Note: The contact force value is only valid when calculating resting contacts. This means if two bodies collide with non zero relative velocity, the reaction force will be an impulse, which is not a reaction force, this will return zero vector. "
			" This function will only return meaningful values when the colliding bodies are at rest.");
		AddClassToCollection("Contact", "float getContactMaxNormalImpact()", "Gets the maximum normal impact of the collision.");
		AddClassToCollection("Contact", "void setContactTangentFriction(float friction, int index)", "Sets the contact tangent friction along the surface plane. Note: State 0 makes the contact frictionless along the index tangent vector. The index 0 is for primary tangent vector or 1 for the secondary tangent.");
		AddClassToCollection("Contact", "float getContactMaxTangentImpact()", "Gets the maximum tangent impact of the collision.");
		AddClassToCollection("Contact", "void setContactPruningTolerance(float tolerance)", "Sets the pruning tolerance for this contact.");
		AddClassToCollection("Contact", "float getContactPruningTolerance()", "Gets the pruning tolerance for this contact.");
		AddClassToCollection("Contact", "float getContactPenetration()", "Gets the contact penetration.");
		AddClassToCollection("Contact", "float getClosestDistance()", "Gets the closest distance between the two GameObjects at this contact.");


		module(lua)
			[
				class_<PhysicsMaterialComponent, GameObjectComponent>("PhysicsMaterialComponent")
				// .def("getClassName", &PhysicsMaterialComponent::getClassName)
			.def("getParentClassName", &PhysicsMaterialComponent::getParentClassName)
			// .def("clone", &PhysicsMaterialComponent::clone)
			// .def("getClassId", &PhysicsMaterialComponent::getClassId)
			.def("setCategory1", &PhysicsMaterialComponent::setCategory1)
			.def("getCategory1", &PhysicsMaterialComponent::getCategory1)
			.def("setCategory2", &PhysicsMaterialComponent::setCategory2)
			.def("getCategory2", &PhysicsMaterialComponent::getCategory2)
			.def("setFriction", &PhysicsMaterialComponent::setFriction)
			.def("getFriction", &PhysicsMaterialComponent::getFriction)
			.def("setSoftness", &PhysicsMaterialComponent::setSoftness)
			.def("getSoftness", &PhysicsMaterialComponent::getSoftness)
			.def("setElasticity", &PhysicsMaterialComponent::setElasticity)
			.def("getElasticity", &PhysicsMaterialComponent::getElasticity)
			.def("setSurfaceThickness", &PhysicsMaterialComponent::setSurfaceThickness)
			.def("getSurfaceThickness", &PhysicsMaterialComponent::getSurfaceThickness)
			.def("setCollideable", &PhysicsMaterialComponent::setCollideable)
			.def("getCollideable", &PhysicsMaterialComponent::getCollideable)
			.def("setContactBehavior", &PhysicsMaterialComponent::setContactBehavior)
			.def("getContactBehavior", &PhysicsMaterialComponent::getContactBehavior)
			.def("setContactSpeed", &PhysicsMaterialComponent::setContactSpeed)
			.def("getContactSpeed", &PhysicsMaterialComponent::getContactSpeed)
			.def("setContactDirection", &PhysicsMaterialComponent::setContactDirection)
			.def("getContactDirection", &PhysicsMaterialComponent::getContactDirection)
			];

		AddClassToCollection("PhysicsMaterialComponent", "class inherits GameObjectComponent", PhysicsMaterialComponent::getStaticInfoText());
		// AddClassToCollection("PhysicsMaterialComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("PhysicsMaterialComponent", "void setFriction(Vector2 frictionData)", "Overrides the friction for this two material categories. Note: Friction data x value is the static friction and y the kinetic friction. "
			"Note: static friction and kinetic friction must be positive values. Kinetic friction must be lower than static friction. It is recommended that static friction and kinetic friction be set to a value lower or equal to 1.0, however because some synthetic materials can have higher than one coefficient of friction, Newton allows for the coefficient of friction to be as high as 2.0.");
		AddClassToCollection("PhysicsMaterialComponent", "Vector2 getFriction()", "Gets the friction for this two material categories. Note: Friction data x value is the static friction and y the kinetic friction.");
		AddClassToCollection("PhysicsMaterialComponent", "void setSoftness(float softness)", "Overrides the softness for this two material categories. Details: A typical value for softness coefficient is 0.15, default value is 0.1. "
			"Note: Must be a positive value.");
		AddClassToCollection("PhysicsMaterialComponent", "float getSoftness()", "Gets the softness for this two material categories.");
		AddClassToCollection("PhysicsMaterialComponent", "void setElasticity(float elasticity)", "Overrides the default coefficients of restitution (elasticity) for the material interaction between two physics material categories. Note: Must be a positive value. It is recommended that elastic coefficient be set to a value lower or equal to 1.0.");
		AddClassToCollection("PhysicsMaterialComponent", "float getElasticity()", "Gets the elasticity this two material categories.");
		AddClassToCollection("PhysicsMaterialComponent", "void setSurfaceThickness(float surfaceThickness)", "Overrides the surface thickness for this two material categories. "
			"Details: Set an imaginary thickness between the collision geometry of two colliding bodies. Note: Surfaces thickness can improve the behaviors of rolling objects on flat surfaces.");
		AddClassToCollection("PhysicsMaterialComponent", "float getSurfaceThickness()", "Gets the surface thickness for this two material categories.");
		AddClassToCollection("PhysicsMaterialComponent", "void setCollideable(bool collideable)", "Overrides the collidable state for this two material categories. Note: False means that those two material groups will not collide against each other.");
		AddClassToCollection("PhysicsMaterialComponent", "float getCollideable()", "Gets the softness for this material pair.");

		AddClassToCollection("PhysicsMaterialComponent", "void setContactSpeed(float speed)", "Sets the contact speed for a specified contact behavior.");
		AddClassToCollection("PhysicsMaterialComponent", "float getContactSpeed()", "Gets the contact speed for a specified contact behavior.");
		AddClassToCollection("PhysicsMaterialComponent", "void setContactDirection(Vector3 direction)", "Sets the contact direction for a specified contact behavior.");
		AddClassToCollection("PhysicsMaterialComponent", "Vector3 getContactDirection()", "Gets the contact direction for a specified contact behavior.");
	}

	void bindPlaneComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<PlaneComponent, GameObjectComponent>("PlaneComponent")
				// .def("getClassName", &PlaneComponent::getClassName)
			.def("getParentClassName", &PlaneComponent::getParentClassName)
			// .def("clone", &PlaneComponent::clone)
			// .def("getClassId", &PlaneComponent::getClassId)
			// .def("setDistance", &PlaneComponent::setDistance)
			// .def("getDistance", &PlaneComponent::getDistance)
			.def("setWidth", &PlaneComponent::setWidth)
			.def("getWidth", &PlaneComponent::getWidth)
			.def("setHeight", &PlaneComponent::setHeight)
			.def("getHeight", &PlaneComponent::getHeight)
			.def("setXSegments", &PlaneComponent::setXSegments)
			.def("getXSegments", &PlaneComponent::getXSegments)
			.def("setYSegments", &PlaneComponent::setYSegments)
			.def("getYSegments", &PlaneComponent::getYSegments)
			.def("setNumTexCoordSets", &PlaneComponent::setNumTexCoordSets)
			.def("getNumTexCoordSets", &PlaneComponent::getNumTexCoordSets)
			.def("setUTile", &PlaneComponent::setUTile)
			.def("getUTile", &PlaneComponent::getUTile)
			.def("setVTile", &PlaneComponent::setVTile)
			.def("getVTile", &PlaneComponent::getVTile)
			// .def("setNormal", &PlaneComponent::setNormal)
			// .def("getNormal", &PlaneComponent::getNormal)
			// .def("setUp", &PlaneComponent::setUp)
			// .def("getUp", &PlaneComponent::getUp)
			];

		AddClassToCollection("PlaneComponent", "class inherits GameObjectComponent", PlaneComponent::getStaticInfoText());
		// AddClassToCollection("PlaneComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("PlaneComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("PlaneComponent", "void setWidth(float width)", "Sets the width of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.");
		AddClassToCollection("PlaneComponent", "float getWidth()", "Gets the width of the plane.");
		AddClassToCollection("PlaneComponent", "void setHeight(float height)", "Sets the height of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.");
		AddClassToCollection("PlaneComponent", "float getHeight()", "Gets the height of the plane.");
		AddClassToCollection("PlaneComponent", "void setHeight(float height)", "Sets the height of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.");
		AddClassToCollection("PlaneComponent", "float getHeight()", "Gets the height of the plane.");
		AddClassToCollection("PlaneComponent", "void setXSegments(float xSegments)", "Sets number of x-segments of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.");
		AddClassToCollection("PlaneComponent", "float getXSegments()", "Gets number of x-segments of the plane.");
		AddClassToCollection("PlaneComponent", "void setYSegments(float ySegments)", "Sets number of y-segments of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.");
		AddClassToCollection("PlaneComponent", "float getYSegments()", "Gets number of y-segments of the plane.");
		AddClassToCollection("PlaneComponent", "void setNumTexCoordSets(float numTexCoordSets)", "Sets number of texture coordinate sets of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.");
		AddClassToCollection("PlaneComponent", "float getNumTexCoordSets()", "Gets number of texture coordinate sets of the plane.");
		AddClassToCollection("PlaneComponent", "void setUTile(float uTile)", "Sets number of u-tiles of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.");
		AddClassToCollection("PlaneComponent", "float getUTile()", "Gets number of u-tiles of the plane.");
		AddClassToCollection("PlaneComponent", "void setUTile(float vTile)", "Sets number of v-tiles of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.");
		AddClassToCollection("PlaneComponent", "float getUTile()", "Gets number of v-tiles of the plane.");
	}

	luabind::object getRagDataList(PhysicsRagDollComponent* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		for (size_t i = 0; i < instance->getRagDataList().size(); i++)
		{
			obj[i] = instance->getRagDataList()[i];
		}

		return obj;
	}

	void bindPhysicsRagDollComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<PhysicsRagDollComponent, PhysicsActiveComponent>("PhysicsRagDollComponent")
				// .def("getClassName", &PhysicsRagDollComponent::getClassName)
				// .def("clone", &PhysicsRagDollComponent::clone)
				// .def("getClassId", &PhysicsRagDollComponent::getClassId)
			.def("inheritVelOmega", &PhysicsRagDollComponent::inheritVelOmega)
			.def("setActivated", &PhysicsRagDollComponent::setActivated)
			.def("setState", &PhysicsRagDollComponent::setState)
			// .def("isActive", &PhysicsRagDollComponent::isActivated)
			.def("setVelocity", &PhysicsRagDollComponent::setVelocity)
			.def("getVelocity", &PhysicsRagDollComponent::getVelocity)
			.def("getPosition", &PhysicsRagDollComponent::getPosition)
			.def("setOrientation", &PhysicsRagDollComponent::setOrientation)
			.def("getOrientation", &PhysicsRagDollComponent::getOrientation)
			.def("setInitialState", &PhysicsRagDollComponent::setInitialState)
			.def("setAnimationEnabled", &PhysicsRagDollComponent::setAnimationEnabled)
			.def("isAnimationEnabled", &PhysicsRagDollComponent::isAnimationEnabled)
			// .def("applyModelStateToRagdoll", &PhysicsRagDollComponent::applyModelStateToRagdoll)
			// .def("applyRagdollStateToModel", &PhysicsRagDollComponent::applyRagdollStateToModel)
			// .def("startRagdolling", &PhysicsRagDollComponent::startRagdolling)
			.def("setBoneConfigFile", &PhysicsRagDollComponent::setBoneConfigFile)
			.def("getBoneConfigFile", &PhysicsRagDollComponent::getBoneConfigFile)
			.def("getRagDataList", &getRagDataList)
			.def("getRagBone", &PhysicsRagDollComponent::getRagBone)
			.def("setBoneRotation", &PhysicsRagDollComponent::setBoneRotation)
			.scope
			[
				class_<PhysicsRagDollComponent::RagBone>("RagBone")
				.def("getName", &PhysicsRagDollComponent::RagBone::getName)
			.def("getPosition", &PhysicsRagDollComponent::RagBone::getPosition)
			.def("setOrientation", &PhysicsRagDollComponent::RagBone::setOrientation)
			.def("getOrientation", &PhysicsRagDollComponent::RagBone::getOrientation)
			// .def("rotate", &PhysicsRagDollComponent::RagBone::rotate)
			.def("setInitialState", &PhysicsRagDollComponent::RagBone::setInitialState)
			// .def("getBody", &PhysicsRagDollComponent::RagBone::getBody)
			.def("getOgreBone", &PhysicsRagDollComponent::RagBone::getOgreBone)
			.def("getParentRagBone", &PhysicsRagDollComponent::RagBone::getParentRagBone)
			.def("getInitialBonePosition", &PhysicsRagDollComponent::RagBone::getInitialBonePosition)
			.def("getInitialBoneOrientation", &PhysicsRagDollComponent::RagBone::getInitialBoneOrientation)
			// .def("getInitialBodyPosition", &PhysicsRagDollComponent::RagBone::getInitialBodyPosition)
			// .def("getInitialBodyOrientation", &PhysicsRagDollComponent::RagBone::getInitialBodyOrientation)
			// .def("getPositionCorrection", &PhysicsRagDollComponent::RagBone::getPositionCorrection)
			// .def("getOrientationCorrection", &PhysicsRagDollComponent::RagBone::getOrientationCorrection)
			// .def("setInitialPose", &PhysicsRagDollComponent::RagBone::setInitialPose)
			.def("getPhysicsRagDollComponent", &PhysicsRagDollComponent::RagBone::getPhysicsRagDollComponent)
			.def("getRagPose", &PhysicsRagDollComponent::RagBone::getRagPose)
			.def("applyPose", &PhysicsRagDollComponent::RagBone::applyPose)
			// .def("getSceneNode", &PhysicsRagDollComponent::RagBone::getSceneNode)
			.def("getJointComponent", &getRagJointComponent)
			.def("getJointHingeComponent", &getRagJointHingeComponent)
			.def("getJointUniversalComponent", &getRagJointUniversalComponent)
			.def("getJointBallAndSocketComponent", &getRagJointBallAndSocketComponent)
			.def("getJointHingeActuatorComponent", &getRagJointHingeActuatorComponent)
			.def("getJointUniversalActuatorComponent", &getRagJointUniversalActuatorComponent)
			.def("applyRequiredForceForVelocity", &PhysicsRagDollComponent::RagBone::applyRequiredForceForVelocity)
			.def("applyOmegaForce", &PhysicsRagDollComponent::RagBone::applyOmegaForce)
			.def("applyOmegaForceRotateTo", &PhysicsRagDollComponent::RagBone::applyOmegaForceRotateTo)
			]
			];
		AddClassToCollection("PhysicsRagDollComponent", "class inherits PhysicsActiveComponent", PhysicsRagDollComponent::getStaticInfoText());
		// AddClassToCollection("PhysicsRagDollComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("PhysicsRagDollComponent", "number getClassId()", "Gets the class id of this component.");
		AddClassToCollection("PhysicsRagDollComponent", "void setVelocity(Vector3 velocity)", "Sets the global linear velocity on the physics body. Note: This should only be used for initzialisation. Use @applyRequiredForceForVelocity in simualtion instead. Or it may be called if its a physics active kinematic body.");
		AddClassToCollection("PhysicsRagDollComponent", "Vector3 getVelocity()", "Gets currently acting velocity on the body.");
		AddClassToCollection("PhysicsRagDollComponent", "Vector3 getPosition()", "Gets the position of the physics component.");
		AddClassToCollection("PhysicsRagDollComponent", "void setOrientation(Quaternion orientation)", "Sets the orientation of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
		AddClassToCollection("PhysicsRagDollComponent", "Quaternion getOrientation()", "Gets the orientation of the physics component.");
		AddClassToCollection("PhysicsRagDollComponent", "void setInitialState()", "If in ragdoll state, resets all bones ot its initial position and orientation.");
		AddClassToCollection("PhysicsRagDollComponent", "void setAnimationEnabled(bool enabled)", "Enables animation for the ragdoll. That is, the bones are no more controlled manually, but transform comes from animation state.");
		AddClassToCollection("PhysicsRagDollComponent", "bool isAnimationEnabled()", "Gets whether the ragdoll is being animated.");
		AddClassToCollection("PhysicsRagDollComponent", "void setBoneConfigFile(String boneConfigFile)", "Sets the bone configuration file. Which describes in XML, how the ragdoll is configure. The file must be placed in the same folder as the mesh and skeleton file. Note: The file can be exchanged at runtime, if a different ragdoll configuration is desire.");
		AddClassToCollection("PhysicsRagDollComponent", "String getBoneConfigFile()", "Gets the currently applied bone config file.");
		AddClassToCollection("PhysicsRagDollComponent", "table[RagBone] getRagDataList()", "Gets List of all configured rag bones.");
		AddClassToCollection("PhysicsRagDollComponent", "RagBone getRagBone(String ragboneName)", "Gets RagBone from the given name or nil, if it does not exist.");
		AddClassToCollection("PhysicsRagDollComponent", "void setBoneRotation(String ragboneName, Vector3 axis, float degree)", "Rotates the given RagBone around the given axis by degree amount.");

		AddClassToCollection("RagBone", "class", "The inner class RagBone represents one physically controlled rag bone.");
		AddClassToCollection("RagBone", "String getName()", "Gets name of this bone, that has been specified in the bone config file.");
		AddClassToCollection("RagBone", "Vector3 getPosition()", "Gets the position of this rag bone in global space.");
		AddClassToCollection("RagBone", "void setOrientation(Quaternion orientation)", "Sets the orientation of this rag bone in global space. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!");
		AddClassToCollection("RagBone", "Quaternion getOrientation()", "Gets the orientation of this rag bone in global space.");
		AddClassToCollection("RagBone", "void setInitialState()", "If in ragdoll state, resets this rag bone ot its initial position and orientation.");
		AddClassToCollection("RagBone", "OldBone getOgreBone()", "Gets the Ogre v1 old bone.");
		AddClassToCollection("RagBone", "RagBone getParentRagBone()", "Gets the parent rag bone or nil, if its the root.");
		AddClassToCollection("RagBone", "Vector3 getInitialBonePosition()", "Gets the initial position of this rag bone in global space.");
		AddClassToCollection("RagBone", "Quaternion getInitialBoneOrientation()", "Gets the initial orientation of this rag bone in global space.");
		AddClassToCollection("RagBone", "PhysicsRagDollComponent getPhysicsRagDollComponent()", "Gets PhysicsRagDollComponent outer class object from this rag bone.");
		AddClassToCollection("RagBone", "Vector3 getPose()", "Gets the pose of this rag bone. Note: This is a vector if not ZERO, that is used to constraint a specific axis, so that the rag bone can not be moved along that axis.");
		AddClassToCollection("RagBone", "void applyPose(Vector3 pose)", "Applies a the pose to this rag bone. Note: This is a vector if not ZERO, that is used to constraint a specific axis, so that the rag bone can not be moved along that axis.");
		// AddClassToCollection("RagBone", "SceneNode getSceneNode()", "Gets the Ogre scene node of this rag bone for further manipulations, like attaching another object to it.");
		AddClassToCollection("RagBone", "JointComponent getJointComponent()", "Gets the base joint component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.");
		AddClassToCollection("RagBone", "JointHingeComponent getJointHingeComponent()", "Gets the joint hinge component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.");
		AddClassToCollection("RagBone", "JointComponent getJointUniversalComponent()", "Gets the joint universal (double hinge) component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.");
		AddClassToCollection("RagBone", "JointComponent getJointBallAndSocketComponent()", "Gets the joint ball and socket component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.");
		AddClassToCollection("RagBone", "JointComponent getJointHingeActuatorComponent()", "Gets the joint hinge actuator component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.");
		AddClassToCollection("RagBone", "JointComponent getJointUniversalActuatorComponent()", "Gets the joint universal actuator (double hinge actuator) component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.");
		AddClassToCollection("RagBone", "void applyRequiredForceForVelocity(Vector3 velocity)", "Applies internally as many force, to satisfy the given velocity and move the bone by that force.");
		AddClassToCollection("RagBone", "void applyOmegaForce(Vector3 omega)", "Applies omega force in order to rotate the rag bone.");
		AddClassToCollection("RagBone", "void applyOmegaForceRotateTo(Quaternion resultOrientation, Vector3 axes, Ogre::Real strength)", "Applies omega force in order to rotate the game object to the given orientation. "
							 "The axes at which the rotation should occur (Vector3::UNIT_Y for y, Vector3::UNIT_SCALE for all axes, or just Vector3(1, 1, 0) for x,y axis etc.). "
							 "The strength at which the rotation should occur. "
							 "Note: This should be used during simulation instead of @setOmegaVelocity.");
	}

	/*void bindPhysicsMathSliderComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<PhysicsMathSliderComponent, PhysicsActiveComponent>("PhysicsMathSliderComponent")
			// .def("getClassName", &PhysicsMathSliderComponent::getClassName)
			// .def("clone", &PhysicsMathSliderComponent::clone)
			// .def("getClassId", &PhysicsMathSliderComponent::getClassId)
			.def("setXFunction", &PhysicsMathSliderComponent::setXFunction)
			.def("setYFunction", &PhysicsMathSliderComponent::setYFunction)
			.def("setZFunction", &PhysicsMathSliderComponent::setZFunction)
		];
	}*/

	void bindPhysicsTerrainComponent(lua_State* lua)
	{

	}

	void bindSimpleSoundComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<SimpleSoundComponent, GameObjectComponent>("SimpleSoundComponent")
				// .def("getClassName", &SimpleSoundComponent::getClassName)
				// .def("clone", &SimpleSoundComponent::clone)
				// .def("getClassId", &SimpleSoundComponent::getClassId)
			.def("setSoundName", &SimpleSoundComponent::setSoundName)
			.def("getSoundName", &SimpleSoundComponent::getSoundName)
			.def("setLoop", &SimpleSoundComponent::setLoop)
			.def("isLooped", &SimpleSoundComponent::isLooped)
			.def("setVolume", &SimpleSoundComponent::setVolume)
			.def("getVolume", &SimpleSoundComponent::getVolume)
			.def("setStream", &SimpleSoundComponent::setStream)
			.def("isStreamed", &SimpleSoundComponent::isStreamed)
			.def("setRelativeToListener", &SimpleSoundComponent::setRelativeToListener)
			.def("isRelativeToListener", &SimpleSoundComponent::isRelativeToListener)
			.def("setActivated", &SimpleSoundComponent::setActivated)
			.def("isActivated", &SimpleSoundComponent::isActivated)

			.def("setFadeInOutTime", &SimpleSoundComponent::setFadeInOutTime)
			.def("setInnerOuterConeAngle", &SimpleSoundComponent::setInnerOuterConeAngle)
			.def("setOuterConeGain", &SimpleSoundComponent::setOuterConeGain)
			.def("getOuterConeGain", &SimpleSoundComponent::getOuterConeGain)
			.def("setPitch", &SimpleSoundComponent::setPitch)
			.def("getPitch", &SimpleSoundComponent::getPitch)
			// .def("setPriority", &SimpleSoundComponent::setPriority)
			// .def("getPriority", &SimpleSoundComponent::getPriority)
			.def("setDistanceValues", &SimpleSoundComponent::setDistanceValues)
			.def("setSecondOffset", &SimpleSoundComponent::setSecondOffset)
			.def("getSecondOffset", &SimpleSoundComponent::getSecondOffset)
			.def("setVelocity", &SimpleSoundComponent::setVelocity)
			.def("getVelocity", &SimpleSoundComponent::getVelocity)
			.def("setDirection", &SimpleSoundComponent::setDirection)
			.def("getDirection", &SimpleSoundComponent::getDirection)

			// .def("getSound", &SimpleSoundComponent::getSound)

			.def("enableSpectrumAnalysis", &SimpleSoundComponent::enableSpectrumAnalysis)
			.def("getVUPointsData", &getVUPointsData)
			.def("getAmplitudeData", &getAmplitudeData)
			.def("getLevelData", &getLevelData)
			.def("getFrequencyData", &getFrequencyData)
			.def("getPhaseData", &getPhaseData)
			.def("getActualSpectrumSize", &SimpleSoundComponent::getActualSpectrumSize)
			.def("getCurrentSpectrumTimePosSec", &SimpleSoundComponent::getCurrentSpectrumTimePosSec)
			.def("getFrequency", &SimpleSoundComponent::getFrequency)
			.def("isSpectrumArea", &SimpleSoundComponent::isSpectrumArea)
			.def("isInstrument", &SimpleSoundComponent::isInstrument)
			];

		AddClassToCollection("SimpleSoundComponent", "class inherits GameObjectComponent", "With this component a dolby surround is created and calibrated. It can be used for music, sound and effects like spectrum analysis, doppler effect etc.");
		// AddClassToCollection("SimpleSoundComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("SimpleSoundComponent", "float getClassId()", "Gets the class id of this component.");
		AddClassToCollection("SimpleSoundComponent", "void setSoundName(String soundName)", "Sets the sound name to be played. Note: The sound name should be set without path. It will be searched automatically in the Audio Resource group.");
		AddClassToCollection("SimpleSoundComponent", "String getSoundName()", "Gets the sound name.");
		AddClassToCollection("SimpleSoundComponent", "void setLoop(bool loop)", "Sets whether to loop the sound, if its played completely.");
		AddClassToCollection("SimpleSoundComponent", "bool isLooped()", "Gets whether this sound is played in loop.");
		AddClassToCollection("SimpleSoundComponent", "void setVolume(float volume)", "Sets the volume (gain) of the sound. Note: 0 means extremely quiet and 100 is maximum.");
		AddClassToCollection("SimpleSoundComponent", "float getVolume()", "Gets the volume (gain) of the sound.");
		AddClassToCollection("SimpleSoundComponent", "void setStream(bool stream)", "Sets whether this sound should be streamed instead of fully loaded to buffer. "
			"Note: The sound gets streamed during runtime. If set to false, the music gets loaded into a buffer offline."
			"Buffering the music file offline, is more stable for big files. Streaming is useful for small files and it is more performant. "
			"But using streaming for bigger files can cause interrupting the music playback and stops it");
		AddClassToCollection("SimpleSoundComponent", "bool isStreamed()", "Gets whether is sound is streamed instead of fully loaded to buffer.");
		AddClassToCollection("SimpleSoundComponent", "void setRelativeToListener(bool relativeToListener)", "Sets whether this should should be played relative to the listener. Note: "
			"When set to true, the sound may come out of another dolby surround box depending where the camera is located creating nice and maybe creapy sound effects. "
			"When set to false, it does not matter where the camera is, the sound will always have the same values.");
		AddClassToCollection("SimpleSoundComponent", "bool isRelativeToListener()", "Gets whether is sound is player relative to the listener.");
		AddClassToCollection("SimpleSoundComponent", "void setActivated(bool activated)", "Activates the sound. Note: If set to true, the will be played, else the sound stops.");
		AddClassToCollection("SimpleSoundComponent", "bool isActivated()", "Gets whether this sound is activated.");
		AddClassToCollection("SimpleSoundComponent", "void setFadeInOutTime(Vector2 fadeInFadeOutTime)", "Setting fadeInFadeOutTime.x, the sound starts playing while fading in. Setting fadeInFadeOutTime.y fades out, but keeps playing at volume 0, so it can be faded in again.");
		AddClassToCollection("SimpleSoundComponent", "void setInnerOuterConeAngle(Vector2 innerOuterConeAngle)", "Setting innerOuterConeAngle.x sets the inner angle of the sound cone for a directional sound. Valid values are [0 - 360]. "
			"Setting innerOuterConeAngle.y sets the outer angle of the sound cone for a directional sound. Valid values are [0 - 360]. Note: "
			"Each directional source has three zones: The inner zone as defined by the innerOuterConeAngle.x where the gain is constant and is set by 'setVolume'. "
			"The outer zone which is set by innerOuterConeAngle.y and the gain is a linear. Transition between the gain and the outerConeGain, outside of the sound cone. The gain in this zone is set by 'setOuterConeGain'.");
		AddClassToCollection("SimpleSoundComponent", "void setOuterConeGain(float outerConeGain)", "Sets the gain outside the sound cone of a directional sound. Note: "
			"Each directional source has three zones:<ol><li>The inner zone as defined by the innerOuterConeAngle.x where the gain is constant and is set by 'setVolume'. "
			"The outer zone which is set by innerOuterConeAngle.y and the gain is a linear transition between the gain and the outerConeGain, outside of the sound cone. "
			"The gain in this zone is set by setOuterConeGain. Valid range is [0.0 - 1.0] all other values will be ignored");
		AddClassToCollection("SimpleSoundComponent", "float getOuterConeGain()", "Gets the outer cone gain.");
		AddClassToCollection("SimpleSoundComponent", "void setPitch(float pitch)", "Sets the pitch multiplier. Note: Pitch must always be positive non-zero, all other values will be ignored.");
		AddClassToCollection("SimpleSoundComponent", "float getPitch()", "Gets the pitch multiplier.");
		AddClassToCollection("SimpleSoundComponent", "void setDistanceValues(Vector3 distanceValues)", "Sets the variables used in the distance attenuation calculation. "
			"Setting distanceValues.x, sets the max distance used in the Inverse Clamped Distance Model. Setting distanceValues.y, "
			"sets the rolloff rate for the source. Setting distanceValues.z, sets the reference distance used in attenuation calculations.");
		AddClassToCollection("SimpleSoundComponent", "void setSecondOffset(float secondOffset)", "Sets the offset within the audio stream in seconds. Note: Negative values will be ignored.");
		AddClassToCollection("SimpleSoundComponent", "float getSecondOffset()", "Gets the current offset within the audio stream in seconds.");
		AddClassToCollection("SimpleSoundComponent", "void setVelocity(Vector3 velocity)", "Sets the velocity of the sound.");
		AddClassToCollection("SimpleSoundComponent", "Vector3 getVelocity()", "Gets the velocity of the sound.");
		AddClassToCollection("SimpleSoundComponent", "void setDirection(Vector3 direction)", "Sets the direction of the sound. Note: In the case that this sound is attached to a SceneNode this directions becomes relative to the parent's direction. Default value is NEGATIVE_UNIT_Z.");
		AddClassToCollection("SimpleSoundComponent", "Vector3 getDirection()", "Gets the direction of the sound.");
		// AddClassToCollection("SimpleSoundComponent", "Sound getSound()", "Gets the sound instance directly.");

		AddClassToCollection("SimpleSoundComponent", "void enableSpectrumAnalysis(bool enable, int processingSize, int numberOfBands, WindowType windowType, SpectrumPreparationType spectrumPreparationType, float smoothFactor)", "Enables spectrum analysis."
			"Sets the spectrum processing size. Note: addSoundSpectrumHandler must be used and stream must be set to true and spectrumProcessingSize must always be higher as mSpectrumNumberOfBands and divisible by 2. Default value is 1024. It must not go below 1024"
			"Sets the spectrum number of bands. Note: addSoundSpectrumHandler must be used and stream must be set to true. Default value is 20"
			"Sets the math window type for more harmonic visualization: rectangle, blackman harris, blackman, hanning, hamming, tukey"
			"Sets the spectrum preparation type: raw (for own visualization), linear, logarithmic"
			"Sets the spectrum motion smooth factor (default 0 disabled, max 1)");

		AddClassToCollection("SimpleSoundComponent", "table[number] getVUPointsData()", "Gets the spectrum VU points data list.");
		AddClassToCollection("SimpleSoundComponent", "table[number] getAmplitudeData()", "Gets the amplitude data list.");
		AddClassToCollection("SimpleSoundComponent", "table[number] getLevelData()", "Gets the level data list.");
		AddClassToCollection("SimpleSoundComponent", "table[number] getFrequencyData()", "Gets the frequency data list.");
		AddClassToCollection("SimpleSoundComponent", "table[number] getPhaseData()", "Gets the phase list (Which phase in degrees is present at the spectrum processing point in time.");
		AddClassToCollection("SimpleSoundComponent", "float getActualSpectrumSize()", "Gets the actual spectrum size.");
		AddClassToCollection("SimpleSoundComponent", "float getCurrentSpectrumTimePosSec()", "Gets the current spectrum time position in seconds when the sound is being played.");
		AddClassToCollection("SimpleSoundComponent", "float getFrequency()", "Gets the frequency.");
		AddClassToCollection("SimpleSoundComponent", "bool isSpectrumArea(SpectrumArea spectrumArea)", "During spectrum analysis, gets whether a specific spectrum area has been recognized when being played.");
		AddClassToCollection("SimpleSoundComponent", "bool isInstrument(Instrument instrument)", "During spectrum analysis, gets whether a specific instrument has been recognized when being played.");
	}

	void bindSoundComponent(lua_State* lua)
	{
		// not required anymore
	}

	void setSpawnTargetId(SpawnComponent* instance, const Ogre::String& id)
	{
		instance->setSpawnTargetId(Ogre::StringConverter::parseUnsignedLong(id));
	}

	Ogre::String getSpawnTargetId(SpawnComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getSpawnTargetId());
	}

	void bindSpawnComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<SpawnComponent, GameObjectComponent>("SpawnComponent")
			// .def("getClassName", &SpawnComponent::getClassName)
			// .def("clone", &SpawnComponent::clone)
			// .def("getClassId", &SpawnComponent::getClassId)
			.def("setActivated", &SpawnComponent::setActivated)
			.def("isActivated", &SpawnComponent::isActivated)
			.def("setIntervalMS", &SpawnComponent::setIntervalMS)
			.def("getIntervalMS", &SpawnComponent::getIntervalMS)
			.def("setCount", &SpawnComponent::setCount)
			.def("getCount", &SpawnComponent::getCount)
			.def("setLifeTimeMS", &SpawnComponent::setLifeTimeMS)
			.def("getLifeTimeMS", &SpawnComponent::getLifeTimeMS)
			.def("setOffsetPosition", &SpawnComponent::setOffsetPosition)
			.def("getOffsetPosition", &SpawnComponent::getOffsetPosition)
			.def("setOffsetOrientation", &SpawnComponent::setOffsetOrientation)
			.def("getOffsetOrientation", &SpawnComponent::getOffsetOrientation)
			// .def("setInitPosition", &SpawnComponent::setInitPosition)
			// .def("getInitPosition", &SpawnComponent::getInitPosition)
			// .def("setInitOrientation", &SpawnComponent::setInitOrientation)
			// .def("getInitOrienation", &SpawnComponent::getInitOrienation)
			.def("setSpawnAtOrigin", &SpawnComponent::setSpawnAtOrigin)
			.def("isSpawnedAtOrigin", &SpawnComponent::isSpawnedAtOrigin)
			.def("setSpawnTargetId", &setSpawnTargetId)
			.def("getSpawnTargetId", &getSpawnTargetId)
			.def("setKeepAliveSpawnedGameObjects", &SpawnComponent::setKeepAliveSpawnedGameObjects)
			.def("reactOnSpawn", &SpawnComponent::reactOnSpawn)
			.def("reactOnVanish", &SpawnComponent::reactOnVanish)
		];

		AddClassToCollection("SpawnComponent", "class inherits GameObjectComponent", SpawnComponent::getStaticInfoText());
		// AddClassToCollection("SpawnComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("SpawnComponent", "int getClassId()", "Gets the class id of this component.");
		AddClassToCollection("SpawnComponent", "void setActivated(bool activated)", "Activates the components behaviour, so that the spawning will begin.");
		AddClassToCollection("SpawnComponent", "bool isActivated()", "Gets whether the component behaviour is activated or not.");
		AddClassToCollection("SpawnComponent", "void setIntervalMS(int intervalMS)", "Sets the spawn interval in milliseconds. Details: E.g. an interval of 10000 would spawn a game object each 10 seconds.");
		AddClassToCollection("SpawnComponent", "int getIntervalMS()", "Gets the spawn interval in milliseconds.");
		AddClassToCollection("SpawnComponent", "void setCount(int count)", "Sets the spawn count. Details: E.g. a count of 100 would spawn 100 game objects of this type. When the count is set to 0, the spawning will never stop.");
		AddClassToCollection("SpawnComponent", "int getCount()", "Gets the Gets the spawn count.");
		AddClassToCollection("SpawnComponent", "void setLifeTimeMS(int lifeTimeMS)", "Sets the spawned game object life time in milliseconds. When the life time is set to 0, the spawned gameobject will live forever. Details: E.g. a life time of 10000 would let live the spawned game object for 10 seconds.");
		AddClassToCollection("SpawnComponent", "int getLifeTimeMS()", "Gets the spawned game object life time in milliseconds.");
		// AddClassToCollection("SpawnComponent", "void setInitPosition(Vector3 spawnInitPosition)", "Sets a new initial position, at which game objects should be spawned.");
		// AddClassToCollection("SpawnComponent", "Vector3 getInitPosition()", "Gets the initial spawn position.");
		// AddClassToCollection("SpawnComponent", "void setInitOrientation(Quaternion spawnInitOrientation)", "Sets a new initial orientation, at which game objects should be spawned.");
		// AddClassToCollection("SpawnComponent", "Quaternion getInitOrienation()", "Gets the initial spawn orientation.");
		AddClassToCollection("SpawnComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets a new initial position, at which game objects should be spawned.");
		AddClassToCollection("SpawnComponent", "Vector3 getOffsetPosition()", "Gets the offset position of the to be spawned game objects.");
		AddClassToCollection("SpawnComponent", "void setOffsetOrientation(Quaternion offsetOrientation)", "Sets an offset orientation at which the spawned game objects will occur relative to the origin game object. Note: The offset orientation to set (degreeX, degreeY, degreeZ).");
		AddClassToCollection("SpawnComponent", "Quaternion getOffsetOrientation()", "Gets an offset orientation at which the spawned game objects will occur relative to the origin game object.");
		AddClassToCollection("SpawnComponent", "void setSpawnAtOrigin(bool spawnAtOrigin)", "Sets whether the spawn new game objects always at the init position or the current position of the original game object. Note: "
			"If spawn at origin is set to false. New game objects will be spawned at the current position of the original game object. Thus complex scenarios are possible! "
			"E.g. pin the original active physical game object at a rotating block. When spawning a new game object implement the spawn callback and add for the spawned game object "
			"an impulse at the current direction. Spawned game objects will be catapultated in all directions while the original game object is being rotated!");
		AddClassToCollection("SpawnComponent", "bool isSpawnedAtOrigin()", "Gets whether the game objects are spawned at initial position and orientation of the original game object.");
		AddClassToCollection("SpawnComponent", "void setSpawnTargetId(String spawnTargetId)", "Sets spawn target game object id. Note: "
			"Its possible to specify another game object which should be spawned "
			" at the position of this game object. E.g. a rotating case shall spawn coins, so the case has the spawn component and as spawn target the coin scene node name.");
		AddClassToCollection("SpawnComponent", "String getSpawnTargetId()", "Gets the spawn target game object id.");
		AddClassToCollection("SpawnComponent", "void setKeepAliveSpawnedGameObjects(bool keepAlive)", "Sets whether keep alive spawned game objects, when the component has ben de-activated and re-activated again.");
		AddClassToCollection("MyGUIComponent", "void reactOnSpawn(func closure, spawnedGameObject, originGameObject)",
							 "Sets whether to react at the moment when a game object is spawned.");
		AddClassToCollection("MyGUIComponent", "void reactOnVanish(func closure, spawnedGameObject, originGameObject)",
							 "Sets whether to react at the moment when a game object is vanished.");
	}

	void bindVehicleComponent(lua_State* lua)
	{
		module(lua)
			[
				class_<VehicleComponent, GameObjectComponent>("VehicleComponent")
				// .def("getClassName", &VehicleComponent::getClassName)
				// .def("clone", &VehicleComponent::clone)
				// .def("getClassId", &VehicleComponent::getClassId)
			.def("setThrottle", &VehicleComponent::setThrottle)
			.def("setClutchPedal", &VehicleComponent::setClutchPedal)
			.def("setSteeringValue", &VehicleComponent::setSteeringValue)
			.def("setBrakePedal", &VehicleComponent::setBrakePedal)
			.def("setIgnitionKey", &VehicleComponent::setIgnitionKey)
			.def("setHandBrakeValue", &VehicleComponent::setHandBrakeValue)
			.def("setGear", &VehicleComponent::setGear)
			.def("setManualTransmission", &VehicleComponent::setManualTransmission)
			.def("setLockDifferential", &VehicleComponent::setLockDifferential)
			];

		AddClassToCollection("VehicleComponent", "class inherits GameObjectComponent", VehicleComponent::getStaticInfoText());
		// TODO: Unfinised
	}

	Ogre::Vector3 getCurrentWayPoint(KI::Path* instance)
	{
		auto& result = instance->getCurrentWaypoint();
		if (result.first)
		{
			return result.second;
		}
		else
		{
			return Ogre::Vector3::ZERO;
		}
	}

	luabind::object getWayPoints(KI::Path* instance)
	{
		luabind::object obj = luabind::newtable(LuaScriptApi::getInstance()->getLua());

		for (size_t i = 0; i < instance->getWayPoints().size(); i++)
		{
			obj[i] = instance->getWayPoints()[i];
		}

		return obj;
	}

	void setTargetAgentId(KI::MovingBehavior* instance, const Ogre::String& id)
	{
		instance->setTargetAgentId(Ogre::StringConverter::parseUnsignedLong(id));
	}

	void setTargetAgentId2(KI::MovingBehavior* instance, const Ogre::String& id2)
	{
		instance->setTargetAgentId2(Ogre::StringConverter::parseUnsignedLong(id2));
	}

	Ogre::String getAgentId(KI::MovingBehavior* instance)
	{
		return Ogre::StringConverter::toString(instance->getAgentId());
	}

	void bindAiLuaComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<KI::Path>("Path")
			.def("createRandomPath", &KI::Path::createRandomPath, return_stl_iterator)
			.def("isFinished", &KI::Path::isFinished)
			.def("getCurrentWayPoint", &getCurrentWayPoint)
			.def("setRepeat", &KI::Path::setRepeat)
			.def("getRepeat", &KI::Path::getRepeat)
			.def("setDirectionChange", &KI::Path::setDirectionChange)
			.def("getDirectionChange", &KI::Path::getDirectionChange)
			.def("addWayPoint", &KI::Path::addWayPoint)
			.def("setWayPoint", &KI::Path::setWayPoint)
			.def("clear", &KI::Path::clear)
			// .def("getWayPoints", &KI::Path::getWayPoints, return_stl_iterator)
			.def("getWayPoints", &getWayPoints)
			// .def("setNextWaypoint", &KI::Path::setNextWaypoint)
		];

		AddClassToCollection("Path", "class", "Creates a path for ai path follow component.");
		AddClassToCollection("Path", "bool isFinished()", "Gets whether the end of the path list has been reached by the agent.");
		AddClassToCollection("Path", "Vector3 getCurrentWayPoint()", "Gets the current way point position.");
		AddClassToCollection("Path", "void setRepeat(bool repeat)", "Sets whether the path traversal should be repeated, if the path is finished.");
		AddClassToCollection("Path", "bool getRepeat()", "Gets whether the path traversal should be repeated, if the path is finished.");
		AddClassToCollection("Path", "void setDirectionChange(bool directionChange)", "Sets whether the traversal direction should be changed, if path is finished.");
		AddClassToCollection("Path", "bool getDirectionChange()", "Gets whether the traversal direction should be changed, if path is finished.");
		AddClassToCollection("Path", "void addWayPoint(Vector3 newPoint)", "Adds a new way point a the end of the list.");
		AddClassToCollection("Path", "void setWayPoint(Vector3 newPoint)", "Clears the list and sets a new way point.");
		AddClassToCollection("Path", "void clear()", "Clears the way point list.");
		AddClassToCollection("Path", "table[Vector3] getWayPoints()", "Gets the list of all waypoints.");
		// AddClassToCollection("Path", "void setNextWaypoint(Vector3 nextPoint)", "Moves the internal iterator to the next waypoint.");

		module(lua)
		[
			class_<KI::MovingBehavior>("MovingBehavior")
			.enum_("BehaviorType")
			[
				value("STOP", KI::MovingBehavior::BehaviorType::STOP),
				value("NONE", KI::MovingBehavior::BehaviorType::NONE),
				value("MOVE", KI::MovingBehavior::BehaviorType::MOVE),
				value("MOVE_RANDOMLY", KI::MovingBehavior::BehaviorType::MOVE_RANDOMLY),
				value("SEEK", KI::MovingBehavior::BehaviorType::SEEK),
				value("FLEE", KI::MovingBehavior::BehaviorType::FLEE),
				value("ARRIVE", KI::MovingBehavior::BehaviorType::ARRIVE),
				value("WANDER", KI::MovingBehavior::BehaviorType::WANDER),
				value("PATH_FINDING_WANDER", KI::MovingBehavior::BehaviorType::PATH_FINDING_WANDER),
				value("PURSUIT", KI::MovingBehavior::BehaviorType::PURSUIT),
				value("EVADE", KI::MovingBehavior::BehaviorType::EVADE),
				value("HIDE", KI::MovingBehavior::BehaviorType::HIDE),
				value("FOLLOW_PATH", KI::MovingBehavior::BehaviorType::FOLLOW_PATH),
				value("OBSTACLE_AVOIDANCE", KI::MovingBehavior::BehaviorType::OBSTACLE_AVOIDANCE),
				value("FLOCKING_COHESION", KI::MovingBehavior::BehaviorType::FLOCKING_COHESION),
				value("FLOCKING_SEPARATION", KI::MovingBehavior::BehaviorType::FLOCKING_SEPARATION),
				value("FLOCKING_ALIGNMENT", KI::MovingBehavior::BehaviorType::FLOCKING_ALIGNMENT),
				value("FLOCKING_OBSTACLE_AVOIDANCE", KI::MovingBehavior::BehaviorType::FLOCKING_OBSTACLE_AVOIDANCE),
				value("FLOCKING_FLEE", KI::MovingBehavior::BehaviorType::FLOCKING_FLEE),
				value("FLOCKING_SEEK", KI::MovingBehavior::BehaviorType::FLOCKING_SEEK),
				value("FLOCKING", KI::MovingBehavior::BehaviorType::FLOCKING),
				value("SEEK_2D", KI::MovingBehavior::BehaviorType::SEEK_2D),
				value("FLEE_2D", KI::MovingBehavior::BehaviorType::FLEE_2D),
				value("ARRIVE_2D", KI::MovingBehavior::BehaviorType::ARRIVE_2D),
				value("PATROL_2D", KI::MovingBehavior::BehaviorType::PATROL_2D),
				value("WANDER_2D", KI::MovingBehavior::BehaviorType::WANDER_2D),
				value("FOLLOW_PATH_2D", KI::MovingBehavior::BehaviorType::FOLLOW_PATH_2D)
			]
		    .def("getPath", &KI::MovingBehavior::getPath)
			.def("setRotationSpeed", &KI::MovingBehavior::setRotationSpeed)
			.def("isSwitchOn", &KI::MovingBehavior::isSwitchOn)
			.def("setDefaultDirection", &KI::MovingBehavior::setDefaultDirection)
			.def("getTargetAgent", &KI::MovingBehavior::getTargetAgent)
			.def("getTargetAgent2", &KI::MovingBehavior::getTargetAgent2)
			.def("setTargetAgentId", &setTargetAgentId)
			.def("setTargetAgentId2", &setTargetAgentId2)
			// .def("setDeceleration", &KI::MovingBehavior::setDeceleration)
			.def("setWanderJitter", &KI::MovingBehavior::setWanderJitter)
			.def("setWanderRadius", &KI::MovingBehavior::setWanderRadius)
			.def("setWanderDistance", &KI::MovingBehavior::setWanderDistance)
			.def("setObstacleHideData", &KI::MovingBehavior::setObstacleHideData)
			.def("setObstacleAvoidanceData", &KI::MovingBehavior::setObstacleAvoidanceData)
			.def("setGoalRadius", &KI::MovingBehavior::setGoalRadius)
			.def("getGoalRadius", &KI::MovingBehavior::getGoalRadius)
			.def("setActualizePathDelaySec", &KI::MovingBehavior::setActualizePathDelaySec)
			.def("getActualizePathDelaySec", &KI::MovingBehavior::getActualizePathDelaySec)
			.def("setPathFindData", &KI::MovingBehavior::setPathFindData)
			// .def("setPathSlot", &KI::MovingBehavior::setPathSlot)
			.def("getPathSlot", &KI::MovingBehavior::getPathSlot)
			// .def("setPathTargetSlot", &KI::MovingBehavior::setPathTargetSlot)
			.def("getPathTargetSlot", &KI::MovingBehavior::getPathTargetSlot)
			.def("findPath", &KI::MovingBehavior::findPath)
			// .def("setDrawPath", &KI::MovingBehavior::setDrawPath)
			.def("setFlyMode", &KI::MovingBehavior::setFlyMode)
			.def("isInFlyMode", &KI::MovingBehavior::isInFlyMode)
			.def("setNeighborDistance", &KI::MovingBehavior::setNeighborDistance)
			.def("getNeighborDistance", &KI::MovingBehavior::getNeighborDistance)
			.def("setBehavior", (void (KI::MovingBehavior::*)(KI::MovingBehavior::BehaviorType)) & KI::MovingBehavior::setBehavior)
			.def("setBehavior", (void (KI::MovingBehavior::*)(const Ogre::String&)) & KI::MovingBehavior::setBehavior)
			.def("addBehavior", (void (KI::MovingBehavior::*)(KI::MovingBehavior::BehaviorType)) & KI::MovingBehavior::addBehavior)
			.def("addBehavior", (void (KI::MovingBehavior::*)(const Ogre::String&)) & KI::MovingBehavior::addBehavior)
			.def("removeBehavior", (void (KI::MovingBehavior::*)(KI::MovingBehavior::BehaviorType)) & KI::MovingBehavior::removeBehavior)
			.def("removeBehavior", (void (KI::MovingBehavior::*)(const Ogre::String&)) & KI::MovingBehavior::removeBehavior)
			.def("getCurrentBehavior", &KI::MovingBehavior::getCurrentBehavior)
			.def("reset", &KI::MovingBehavior::reset)
			// .def("getAgentId", &KI::MovingBehavior::getAgentId)
			.def("getAgentId", &getAgentId)
			.def("setWeightSeparation", &KI::MovingBehavior::setWeightSeparation)
			.def("getWeightSeparation", &KI::MovingBehavior::getWeightSeparation)
			.def("setWeightCohesion", &KI::MovingBehavior::setWeightCohesion)
			.def("getWeightCohesion", &KI::MovingBehavior::getWeightCohesion)
			.def("setWeightAlignment", &KI::MovingBehavior::setWeightAlignment)
			.def("getWeightAlignment", &KI::MovingBehavior::getWeightAlignment)
			.def("setWeightWander", &KI::MovingBehavior::setWeightWander)
			.def("getWeightWander", &KI::MovingBehavior::getWeightWander)
			.def("setWeightObstacleAvoidance", &KI::MovingBehavior::setWeightObstacleAvoidance)
			.def("getWeightObstacleAvoidance", &KI::MovingBehavior::getWeightObstacleAvoidance)
			.def("setWeightSeek", &KI::MovingBehavior::setWeightSeek)
			.def("getWeightSeek", &KI::MovingBehavior::getWeightSeek)
			.def("setWeightFlee", &KI::MovingBehavior::setWeightFlee)
			.def("getWeightFlee", &KI::MovingBehavior::getWeightFlee)
			.def("setWeightArrive", &KI::MovingBehavior::setWeightArrive)
			.def("getWeightArrive", &KI::MovingBehavior::getWeightArrive)
			.def("setWeightPursuit", &KI::MovingBehavior::setWeightPursuit)
			.def("getWeightPursuit", &KI::MovingBehavior::getWeightPursuit)
			.def("setWeightOffsetPursuit", &KI::MovingBehavior::setWeightOffsetPursuit)
			.def("getWeightOffsetPursuit", &KI::MovingBehavior::getWeightOffsetPursuit)
			.def("setWeightHide", &KI::MovingBehavior::setWeightHide)
			.def("getWeightHide", &KI::MovingBehavior::getWeightHide)
			.def("setWeightEvade", &KI::MovingBehavior::setWeightEvade)
			.def("getWeightEvade", &KI::MovingBehavior::getWeightEvade)
			.def("setWeightFollowPath", &KI::MovingBehavior::setWeightFollowPath)
			.def("getWeightFollowPath", &KI::MovingBehavior::getWeightFollowPath)
			.def("setWeightInterpose", &KI::MovingBehavior::setWeightInterpose)
			.def("getWeightInterpose", &KI::MovingBehavior::getWeightInterpose)
			.def("getIsStuck", &KI::MovingBehavior::getIsStuck)
		    .def("setStuckCheckTime", &KI::MovingBehavior::setStuckCheckTime)
			.def("getMotionDistanceChange", &KI::MovingBehavior::getMotionDistanceChange)
			.def("setAutoOrientation", &KI::MovingBehavior::setAutoOrientation)
			.def("setAutoAnimation", &KI::MovingBehavior::setAutoAnimation)
			.def("setOffsetPosition", &KI::MovingBehavior::setOffsetPosition)
		];

		AddClassToCollection("MovingBehavior", "class", "Moving behavior controls one or several game objects (agents) in a artificial intelligent manner. Note: Its possible to combine behaviors.");
		AddClassToCollection("MovingBehavior", "Path getPath()", "Gets path for waypoints manipulation.");
		AddClassToCollection("MovingBehavior", "void setRotationSpeed(float speed)", "Sets the agent rotation speed. Default value is 0.1.");
		AddClassToCollection("MovingBehavior", "bool isSwitchOn(BehaviorType behaviorType)", "Gets whether a specifig behavior is switched on.");
		AddClassToCollection("MovingBehavior", "void setDefaultDirection(Vector3 direction)", "Sets the direction the player is modelled.");
		AddClassToCollection("MovingBehavior", "GameObject getTargetAgent()", "Gets the target agent or nil, if does not exist.");
		AddClassToCollection("MovingBehavior", "GameObject getTargetAgent2()", "Gets the target agent 2 or nil, if does not exist.");
		// AddClassToCollection("MovingBehavior", "String getTargetAgentId()", "Gets the target agent id.");
		// AddClassToCollection("MovingBehavior", "String getTargetAgentId2()", "Gets the target agent 2 id.");
		AddClassToCollection("MovingBehavior", "void setTargetAgentId(String id)", "Sets or changes the target agent id.");
		AddClassToCollection("MovingBehavior", "void setTargetAgentId2(String id2)", "Sets or changes the target agent id 2 for interpose behavior.");
		AddClassToCollection("MovingBehavior", "void setWanderJitter(float jitter)", "Sets the wander jitter. Default value is 1.");
		AddClassToCollection("MovingBehavior", "void setWanderRadius(float radius)", "Sets the wander radius. Default value is 1.2.");
		AddClassToCollection("MovingBehavior", "void setWanderDistance(float distance)", "Sets the wander distance. Default value is 20.");
		AddClassToCollection("MovingBehavior", "void setObstacleHideData(String obstaclesHideCategories, float obstacleRangeRadius)", "Sets the categories of game objects which do count as obstacle and a range radius, "
			"at which the agent will hide, if 'HIDE' behavior is set. Note: Combined categories can be used like 'ALL-Player' etc.");
		AddClassToCollection("MovingBehavior", "void setObstacleAvoidanceData(String obstaclesHideCategories, float obstacleRangeRadius)", "Sets the categories of game objects which do count as obstacle and a range radius, "
			"at which the agent will start turning from the obstacle. Note: Combined categories can be used like 'ALL-Player' etc.");
		AddClassToCollection("MovingBehavior", "void setGoalRadius(float radius)", "Sets the tolerance radius, at which the agent is considered to be at the goal. Note: If the radius is to small, It may happen, that the agent never reaches the goal. Default value is 0.2.");
		AddClassToCollection("MovingBehavior", "float getGoalRadius()", "Gets the tolerance radius, at which the agent is considered to be at the goal.");
		AddClassToCollection("MovingBehavior", "void setActualizePathDelaySec(float delay)", "Sets the interval in seconds in which a changed path will be re-created.");
		AddClassToCollection("MovingBehavior", "float getActualizePathDelaySec()", "Gets the interval in seconds in which a changed path will be re-created.");
		AddClassToCollection("MovingBehavior", "void setPathFindData(int pathSlot, int targetSlot, bool drawPath)", "Sets the path find data, if recast navigation is enabled. "
			"Note: Since its possible to have several paths running concurrently each path search must have an unique slot. The pathSlot identifies the target the path leads to. "
			"The targetSlot specifies the slot in which the found path is to be stored. The drawPath variable specifies whether to draw the path or not for debug purposes. "
			"The border values for slots are: [0, 128].");
		AddClassToCollection("MovingBehavior", "int getPathSlot()", "Gets the used path slot. Note: Recast navigation must be enabled.");
		AddClassToCollection("MovingBehavior", "int getPathTargetSlot()", "Gets the used path target slot. Note: Recast navigation must be enabled.");
		AddClassToCollection("MovingBehavior", "void findPath()", "Tries to find an optimal path. It takes a start point and an end point and, if possible, generates a list of lines in a path. "
			"It might fail if the start or end points aren't near any navigation mesh polygons, or if the path is too long, or it can't make a path, or various other reasons.So far I've not had problems though. Note: Recast navigation must be enabled.");
		AddClassToCollection("MovingBehavior", "void setFlyMode(bool flyMode)", "Sets whether to use fly mode (taking y-axis into account). Note: this can be used e.g. for birds flying around.");
		AddClassToCollection("MovingBehavior", "bool isInFlyMode()", "Gets whether the agent is in flying mode (taking y-axis into account).");
		AddClassToCollection("MovingBehavior", "void setNeighborDistance(float distance)", "Sets the neighbor agent distance in a flocking scenario. Note: 'FLOCKING' behavior must be switched on. Default value is 1.6 meters.");
		AddClassToCollection("MovingBehavior", "float getNeighborDistance()", "Gets the neighbor agent distance in a flocking scenario.");
		AddClassToCollection("MovingBehavior", "void setBehavior(BehaviorType behaviorType)", "Clears all behaviors and set the given behavior.");
		AddClassToCollection("MovingBehavior", "void setBehavior2(Strimg behaviorType)", "Clears all behaviors and set the given behavior by string.");
		AddClassToCollection("MovingBehavior", "void addBehavior(BehaviorType behaviorType)", "Adds the given behavior to already existing behaviors.");
		AddClassToCollection("MovingBehavior", "void addBehavior2(Strimg behaviorType)", "Adds the given behavior to already existing behaviors by string.");
		AddClassToCollection("MovingBehavior", "void removeBehavior(BehaviorType behaviorType)", "Removes the given behavior from already existing behaviors.");
		AddClassToCollection("MovingBehavior", "void removeBehavior2(Strimg behaviorType)", "Removes the given behavior from already existing behaviors by string.");
		AddClassToCollection("MovingBehavior", "String getCurrentBehavior()", "Gets the current behavior constellation as composed string in the form 'SEEK ARRIVE' etc.");
		AddClassToCollection("MovingBehavior", "void reset()", "Resets all behaviors, clears paths etc. and sets the default behavior 'NONE'.");
		AddClassToCollection("MovingBehavior", "String getAgentId()", "Gets the agent (game object) id.");

		AddClassToCollection("MovingBehavior", "void setWeightSeparation(float weight)", "Sets the weight for separation rule in a flocking scenario. Note: 'FLOCKING' behavior must be switched on. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightSeparation()", "Gets the weight for separation rule in a flocking scenario.");
		AddClassToCollection("MovingBehavior", "void setWeightCohesion(float weight)", "Sets the weight for cohesion rule in a flocking scenario. Note: 'FLOCKING' behavior must be switched on. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightCohesion()", "Gets the weight for cohesion rule in a flocking scenario.");
		AddClassToCollection("MovingBehavior", "void setWeightAlignment(float weight)", "Sets the weight for alignment rule in a flocking scenario. Note: 'FLOCKING' behavior must be switched on. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightAlignment()", "Gets the weight for alignment rule in a flocking scenario.");
		AddClassToCollection("MovingBehavior", "void setWeightWander(float weight)", "Sets the weight for wander behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightWander()", "Gets the weight for wander behavior.");
		AddClassToCollection("MovingBehavior", "void setWeightObstacleAvoidance(float weight)", "Sets the weight for obstacle avoidance behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightObstacleAvoidance()", "Gets the weight for obstacle avoidance behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "void setWeightSeek(float weight)", "Sets the weight for seek behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightSeek()", "Gets the weight for seek behavior.");
		AddClassToCollection("MovingBehavior", "void setWeightFlee(float weight)", "Sets the weight for flee behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightFlee()", "Gets the weight for flee behavior.");
		AddClassToCollection("MovingBehavior", "void setWeightArrive(float weight)", "Sets the weight for arrive behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightArrive()", "Gets the weight for arrive behavior.");
		AddClassToCollection("MovingBehavior", "void setWeightPursuit(float weight)", "Sets the weight for pursuit behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightPursuit(float weight)", "Sets the weight for pursuit behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "void setWeightOffsetPursuit(float weight)", "Sets the weight for offset pursuit behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightOffsetPursuit(float weight)", "Sets the weight for offset pursuit behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "void setWeightHide(float weight)", "Sets the weight for hide behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightHide()", "Gets the weight for hide behavior.");
		AddClassToCollection("MovingBehavior", "void setWeightEvade(float weight)", "Sets the weight for evade behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightEvade()", "Gets the weight for evade behavior.");
		AddClassToCollection("MovingBehavior", "void setWeightFollowPath(float weight)", "Sets the weight for follow path behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightFollowPath()", "Gets the weight for follow path behavior.");
		AddClassToCollection("MovingBehavior", "void setWeightInterpose(float weight)", "Sets the weight for interpose behavior. Default value is 1.");
		AddClassToCollection("MovingBehavior", "float getWeightInterpose()", "Gets the weight for interpose behavior.");
		AddClassToCollection("MovingBehavior", "bool getIsStuck()", "Gets whether the agent is stuck (can not reach the goal).");
		AddClassToCollection("MovingBehavior", "void setStuckCheckTime(float timeSec)", "The time in seconds the agent is stuck, until the current behavior will be removed. 0 disables the stuck check.");
		AddClassToCollection("MovingBehavior", "float getMotionDistanceChange()", "Gets motion distance change of the agent. The value is clamped from 0 to 1. 0 means the agent is not moving and 1 the agent is moving at current speed.");
		AddClassToCollection("MovingBehavior", "void setAutoOrientation(bool autoOrientation)", "Sets whether the agent should be auto orientated during ai movement.");
		AddClassToCollection("MovingBehavior", "void setAutoAnimation(bool autoAnimation)", "Sets whether to use auto animation during ai movement. That is, the animation speed is adapted dynamically depending the velocity, which will create a much more realistic effect. Note: The game object must have a proper configured animation component.");
		AddClassToCollection("MovingBehavior", "void setOffsetPosition(Vector3 offsetPosition)", "Sets the offset position for offset pursuit behavior.");

		AddClassToCollection("BehaviorType", "STOP", "Resets the behavior, but the agent will remain on its last valid ai position.");
		AddClassToCollection("BehaviorType", "NONE", "Resets the behavior, so that no ai is taking place anymore and the agent is moved via physics again.");
		AddClassToCollection("BehaviorType", "MOVE", "Sets a stupid move behavior, so that the agent will be moved in its current direction.");
		AddClassToCollection("BehaviorType", "MOVE_RANDOMLY", "The agent will be moved randomly.");
		AddClassToCollection("BehaviorType", "SEEK", "The agent will seek another target agent.");
		AddClassToCollection("BehaviorType", "FLEE", "The agent will flee from another target agent.");
		AddClassToCollection("BehaviorType", "ARRIVE", "The agent will arrive at the target position.");
		AddClassToCollection("BehaviorType", "WANDER", "The agent will wander randomly.");
		AddClassToCollection("BehaviorType", "PATH_FINDING_WANDER", "The agent will wander randomly by target points generated from @NavMeshComponents. Note: Recast scenario must be activated.");
		AddClassToCollection("BehaviorType", "PURSUIT", "The agent will pursuit the target agent.");
		AddClassToCollection("BehaviorType", "EVADE", "The agent will evade from the target agent.");
		AddClassToCollection("BehaviorType", "HIDE", "The agent will hide from the target agent.");
		AddClassToCollection("BehaviorType", "FOLLOW_PATH", "The agent will follow a path. Note: If recast is activated it may be a navigation mesh path.");
		AddClassToCollection("BehaviorType", "OBSTACLE_AVOIDANCE", "The agent will avoid obstacles.");
		AddClassToCollection("BehaviorType", "FLOCKING_COHESION", "The agent category will add the flocking cohesion behavior.");
		AddClassToCollection("BehaviorType", "FLOCKING_SEPARATION", "The agent category will add the flocking separation behavior.");
		AddClassToCollection("BehaviorType", "FLOCKING_ALIGNMENT", "The agent category will add the flocking alignment behavior.");
		AddClassToCollection("BehaviorType", "FLOCKING_OBSTACLE_AVOIDANCE", "The agent category will avoid obstacles.");
		AddClassToCollection("BehaviorType", "FLOCKING_FLEE", "The agent category will flee from the target agent.");
		AddClassToCollection("BehaviorType", "FLOCKING_SEEK", "The agent category will seek the target agent.");
		AddClassToCollection("BehaviorType", "FLOCKING", "This behavior will activate flocking behavior, by the given category group.");
		AddClassToCollection("BehaviorType", "SEEK_2D", "The agent will seek another target agent in a 2D scenario.");
		AddClassToCollection("BehaviorType", "FLEE_2D", "The agent will flee from another target agent in a 2D scenario.");
		AddClassToCollection("BehaviorType", "ARRIVE_2D", "The agent will arrive at the target position in a 2D scenario.");
		AddClassToCollection("BehaviorType", "PATROL_2D", "The agent will patrol in a 2D scenario.");
		AddClassToCollection("BehaviorType", "WANDER_2D", "The agent will wander randomly in a 2D scenario.");
		AddClassToCollection("BehaviorType", "FOLLOW_PATH_2D", "The agent will follow a path in a 2D scenario.");

		module(lua)
		[
			class_<LuaStateMachine<GameObject> >("LuaStateMachine")
			// .def("setCurrentState", &LuaStateMachine<GameObject>::setCurrentState)
			.def("setGlobalState", &LuaStateMachine<GameObject>::setGlobalState)
			.def("exitGlobalState", &LuaStateMachine<GameObject>::exitGlobalState)
			.def("getCurrentState", &LuaStateMachine<GameObject>::getCurrentState)
			.def("getPreviousState", &LuaStateMachine<GameObject>::getPreviousState)
			.def("getGlobalState", &LuaStateMachine<GameObject>::getGlobalState)
			.def("changeState", &LuaStateMachine<GameObject>::changeState)
			.def("revertToPreviousState", &LuaStateMachine<GameObject>::revertToPreviousState)
		];

		module(lua)
		[
			class_<AiLuaComponent, GameObjectComponent>("AiLuaComponent")
			// .def("getClassName", &AiLuaComponent::getClassName)
			// .def("clone", &AiLuaComponent::clone)
			// .def("getClassId", &AiLuaComponent::getClassId)
			.def("setActivated", &AiLuaComponent::setActivated)
			.def("isActivated", &AiLuaComponent::isActivated)
			.def("setRotationSpeed", &AiLuaComponent::setRotationSpeed)
			.def("getRotationSpeed", &AiLuaComponent::getRotationSpeed)
			.def("setFlyMode", &AiLuaComponent::setFlyMode)
			.def("getIsInFlyMode", &AiLuaComponent::getIsInFlyMode)
			.def("setStartStateName", &AiLuaComponent::setStartStateName)
			.def("getStartStateName", &AiLuaComponent::getStartStateName)
			// No: Is done via ailuacomponent, because of some sanity checks!
			// .def("getStateMachine", &AiLuaComponent::getStateMachine)
			.def("getMovingBehavior", &AiLuaComponent::getMovingBehavior)
			.def("setGlobalState", &AiLuaComponent::setGlobalState)
			.def("exitGlobalState", &AiLuaComponent::exitGlobalState)
			.def("getCurrentState", &AiLuaComponent::getCurrentState)
			.def("getPreviousState", &AiLuaComponent::getPreviousState)
			.def("getGlobalState", &AiLuaComponent::getGlobalState)
			.def("changeState", &AiLuaComponent::changeState)
			.def("revertToPreviousState", &AiLuaComponent::revertToPreviousState)
			.def("reactOnPathGoalReached", &AiLuaComponent::reactOnPathGoalReached)
			.def("reactOnAgentStuck", &AiLuaComponent::reactOnAgentStuck)
		];

		AddClassToCollection("AiLuaComponent", "class inherits GameObjectComponent", AiLuaComponent::getStaticInfoText());
		// AddClassToCollection("AiLuaComponent", "String getClassName()", "Gets the class name of this component as string.");
		// AddClassToCollection("AiLuaComponent", "int getClassId()", "Gets the class id of this component.");
		AddClassToCollection("AiLuaComponent", "void setActivated(bool activated)", "Activates the components behaviour, so that the lua script will be executed.");
		AddClassToCollection("AiLuaComponent", "bool isActivated()", "Gets whether the component behaviour is activated or not.");
		AddClassToCollection("AiLuaComponent", "void setRotationSpeed(float speed)", "Sets the agent rotation speed. Default value is 0.1.");
		AddClassToCollection("AiLuaComponent", "float getRotationSpeed()", "Gets the agent rotation speed. Default value is 0.1.");
		AddClassToCollection("AiLuaComponent", "void setFlyMode(bool flyMode)", "Sets whether to use fly mode (taking y-axis into account). Note: this can be used e.g. for birds flying around.");
		AddClassToCollection("AiLuaComponent", "bool isInFlyMode()", "Gets whether the agent is in flying mode (taking y-axis into account).");
		AddClassToCollection("AiLuaComponent", "void setStartStateName(String stateName)", "Sets the start state name, which will be loaded in lua script and executed.");
		AddClassToCollection("AiLuaComponent", "String getStartStateName()", "Gets the start state name, which is loaded in lua script and executed.");
		// AddClassToCollection("AiLuaComponent", "LuaStateMachine getStateMachine()", "Gets the lua state machine for manipulating states.");
		AddClassToCollection("AiLuaComponent", "MovingBehavior getMovingBehavior()", "Gets the moving behavior instance for this agent.");

		AddClassToCollection("AiLuaComponent", "void setGlobalState(table stateName)", "Sets the global state name, which will be loaded in lua script and executed besides the current state. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		AddClassToCollection("AiLuaComponent", "void exitGlobalState()", "Exits the global state, if it does exist.");
		AddClassToCollection("AiLuaComponent", "String getCurrentState()", "Gets the current state name.");
		AddClassToCollection("AiLuaComponent", "String getPreviousState()", "Gets the previous state name, that has been loaded bevor the current state name. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		AddClassToCollection("AiLuaComponent", "String getGlobalState()", "Gets the global state name, if existing, else empty string. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		AddClassToCollection("AiLuaComponent", "void changeState(table newStateName)", "Changes the current state name to a new one. Calles 'exit' function on the current state and 'enter' on the new state in lua script. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		AddClassToCollection("AiLuaComponent", "void revertToPreviousState()", "Changes the current state name to the previous one. Calles 'exit' function on the current state and 'enter' on the previous state in lua script. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		AddClassToCollection("AiLuaComponent", "void reactOnPathGoalReached(func closure, GameObject targetGameObject, string functionName, GameObject gameObject)",
			"Sets whether to react the agent reached the goal.");
		AddClassToCollection("AiLuaComponent", "void reactOnAgentStuck(func closure, GameObject targetGameObject, string functionName, GameObject gameObject)",
			"Sets whether to react the agent got stuck.");
	}

	void bindMeshDecalComponent(lua_State* lua)
	{
		/*module(lua)
		[
			class_<MeshDecalComponent, GameObjectComponent>("MeshDecalComponent")
			// .def("getClassName", &MeshDecalComponent::getClassName)
			// .def("clone", &MeshDecalComponent::clone)
			// .def("getClassId", &MeshDecalComponent::getClassId)
			.def("setDatablockName", &MeshDecalComponent::setDatablockName)
			.def("getDatablockName", &MeshDecalComponent::getDatablockName)
			.def("setNumPartitions", &MeshDecalComponent::setNumPartitions)
			.def("getNumPartitions", &MeshDecalComponent::getNumPartitions)
			.def("setUpdateFrequency", &MeshDecalComponent::setUpdateFrequency)
			.def("getUpdateFrequency", &MeshDecalComponent::getUpdateFrequency)
			.def("setCategoryNames", &MeshDecalComponent::setCategoryNames)
			.def("getCategoryNames", &MeshDecalComponent::getCategoryNames)
			.def("setPosition", &MeshDecalComponent::setPosition)
			.def("setMousePosition", &MeshDecalComponent::setMousePosition)
			.def("setActivated", &MeshDecalComponent::setActivated)
			.def("isActivated", &MeshDecalComponent::isActivated)
			.def("getMeshDecal", &MeshDecalComponent::getMeshDecal)
		];*/
	}

	Ogre::String getWidgetId(MyGUIComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getId());
	}

	void setWidgetParentId(MyGUIComponent* instance, const Ogre::String& id)
	{
		instance->setParentId(Ogre::StringConverter::parseUnsignedLong(id));
	}

	Ogre::String getWidgetParentId(MyGUIComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getParentId());
	}

	void setSourceIdMyGUIController(MyGUIControllerComponent* instance, const Ogre::String& id)
	{
		instance->setSourceId(Ogre::StringConverter::parseUnsignedLong(id));
	}

	Ogre::String getSourceIdMyGUIController(MyGUIControllerComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getSourceId());
	}

	void addQuantityToInventory(InventoryItemComponent* instance, const Ogre::String& id, int quantity, bool once)
	{
		instance->addQuantityToInventory(Ogre::StringConverter::parseUnsignedLong(id), "", quantity, once);
	}

	void removeQuantityFromInventory(InventoryItemComponent* instance, const Ogre::String& id, int quantity, bool once)
	{
		instance->removeQuantityFromInventory(Ogre::StringConverter::parseUnsignedLong(id), "", quantity, once);
	}

	void addQuantityToInventory2(InventoryItemComponent* instance, const Ogre::String& id, const Ogre::String& componentName, int quantity, bool once)
	{
		instance->addQuantityToInventory(Ogre::StringConverter::parseUnsignedLong(id), componentName, quantity, once);
	}

	void removeQuantityFromInventory2(InventoryItemComponent* instance, const Ogre::String& id, const Ogre::String& componentName, int quantity, bool once)
	{
		instance->removeQuantityFromInventory(Ogre::StringConverter::parseUnsignedLong(id), componentName, quantity, once);
	}

	Ogre::String getSenderInventoryId(DragDropData* instance)
	{
		return Ogre::StringConverter::toString(instance->getSenderInventoryId());
	}

	void bindMyGUIComponents(lua_State* lua)
	{
		module(lua)
		[
			class_<MyGUIComponent, GameObjectComponent>("MyGUIComponent")
			// .def("getClassName", &MyGUIComponent::getClassName)
			// .def("getClassId", &MyGUIComponent::getClassId)
			.def("setActivated", &MyGUIComponent::setActivated)
			.def("isActivated", &MyGUIComponent::isActivated)
			.def("setRealPosition", &MyGUIComponent::setRealPosition)
			.def("getRealPosition", &MyGUIComponent::getRealPosition)
			.def("setRealSize", &MyGUIComponent::setRealSize)
			.def("getRealSize", &MyGUIComponent::getRealSize)
			.def("setAlign", &MyGUIComponent::setAlign)
			.def("getAlign", &MyGUIComponent::getAlign)
			.def("setLayer", &MyGUIComponent::setLayer)
			.def("getLayer", &MyGUIComponent::getLayer)
			.def("setColor", &MyGUIComponent::setColor)
			.def("getColor", &MyGUIComponent::getColor)
			.def("setEnabled", &MyGUIComponent::setEnabled)
			.def("getEnabled", &MyGUIComponent::getEnabled)
			// .def("setParentId", &MyGUIComponent::setParentId)
			// .def("getParentId", &MyGUIComponent::getParentId)
			.def("getId", &getWidgetId)
			.def("setParentId", &setWidgetParentId)
			.def("getParentId", &getWidgetParentId)
			// .def("getWidget", &MyGUIComponent::getWidget)
			.def("reactOnMouseButtonClick", &MyGUIComponent::reactOnMouseButtonClick)
			.def("reactOnMouseEnter", &MyGUIComponent::reactOnMouseEnter)
			.def("reactOnMouseLeave", &MyGUIComponent::reactOnMouseLeave)
		];

		AddClassToCollection("MyGUIComponent", "class inherits GameObjectComponent", MyGUIComponent::getStaticInfoText());
		AddClassToCollection("MyGUIComponent", "void setActivated(bool activated)", "Controls the visibility of the widget.");
		AddClassToCollection("MyGUIComponent", "bool isActivated()", "Gets whether this widget is visible or not.");
		AddClassToCollection("MyGUIComponent", "void setRealPosition(Vector2 position)", "Sets the relative position of the widget. Note: Relative means: 0,0 is left top corner and 1,1 is right bottom corner of the whole screen.");
		AddClassToCollection("MyGUIComponent", "Vector2 getRealPosition()", "Gets the relative position of the widget.");
		AddClassToCollection("MyGUIComponent", "void setRealSize(Vector2 size)", "Sets the relative size of the widget. Note: Relative means, a size of 1,1 the widget would fill the whole screen.");
		AddClassToCollection("MyGUIComponent", "Vector2 getRealSize()", "Gets the relative size of the widget.");
		AddClassToCollection("MyGUIComponent", "void setAlign(String align)", "Sets the widget alignment. Possible values are: 'HCenter', 'Center', 'Left', 'Right', 'HStretch', 'Top', 'Bottom', 'VStretch', 'Stretch', 'Default' (LEFT|TOP).");
		AddClassToCollection("MyGUIComponent", "String getAlign()", "Gets the widget alignment.");
		AddClassToCollection("MyGUIComponent", "void setLayer(String layer)", "Sets the widget's layer (z-order). Possible values are: 'Wallpaper', 'ToolTip', 'Info', 'FadeMiddle', 'Popup', 'Main', 'Modal', 'Middle', 'Overlapped', 'Back', 'DragAndDrop', 'FadeBusy', 'Pointer', 'Fade', 'Statistic'. Default value is 'POPUP'");
		AddClassToCollection("MyGUIComponent", "String getLayer()", "Gets the widget's layer (z-order).");
		AddClassToCollection("MyGUIComponent", "void setColor(Vector4 color)", "Sets the widget's background color (r, g, b, a).");
		AddClassToCollection("MyGUIComponent", "Vector4 getColor()", "Gets the widget's background color (r, g, b, a).");
		AddClassToCollection("MyGUIComponent", "void setEnabled(bool loop)", "Sets whether the widget is enabled.");
		AddClassToCollection("MyGUIComponent", "bool getEnabled()", "Gets whether whether the widget is enabled.");
		AddClassToCollection("MyGUIComponent", "String getId()", "Gets the id of this widget. Note: Widgets can be attached as child of other widgets and be transformed relatively to their parent widget. Hence each widget has an id and its parent id can be filled with the id of another widget component in the same game object..");
		AddClassToCollection("MyGUIComponent", "void setParentId(String parentId)", "Sets the parent widget id. Note: Widgets can be attached as child of other widgets and be transformed relatively to their parent widget. Hence each widget has an id and its parent id can be filled with the id of another widget component in the same game object..");
		AddClassToCollection("MyGUIComponent", "bool getParentId()", "Gets the parent id of this widget.");
		AddClassToCollection("MyGUIComponent", "void reactOnMouseButtonClick(func closureFunction)",
														  "Sets whether to react if a mouse button has been clicked on the given widget.");
		AddClassToCollection("MyGUIComponent", "void reactOnMouseEnter(func closureFunction)",
														  "Sets whether to react if mouse has entered the given widget (hover start).");
		AddClassToCollection("MyGUIComponent", "void reactOnMouseEnter(func closureFunction)",
														  "Sets whether to react if mouse has left the given widget (hover end).");

		module(lua)
			[
				class_<MyGUIWindowComponent, MyGUIComponent>("MyGUIWindowComponent")
				// .def("getClassName", &MyGUIWindowComponent::getClassName)
				// .def("getClassId", &MyGUIWindowComponent::getClassId)
			.def("setSkin", &MyGUIWindowComponent::setSkin)
			.def("getSkin", &MyGUIWindowComponent::getSkin)
			.def("setMovable", &MyGUIWindowComponent::setMovable)
			.def("getMovable", &MyGUIWindowComponent::getMovable)
			.def("setWindowCaption", &MyGUIWindowComponent::setWindowCaption)
			.def("getWindowCaption", &MyGUIWindowComponent::getWindowCaption)
			];

		AddClassToCollection("MyGUIWindowComponent", "class inherits MyGUIComponent", MyGUIWindowComponent::getStaticInfoText());
		AddClassToCollection("MyGUIWindowComponent", "void setSkin(String skin)", "Sets the widgets skin. Possible values are: 'WindowCSX', 'Window', 'WindowC', 'WindowCX', 'WindowCS', 'WindowCX2_Dark', 'WindowCXS2_Dark', 'Back1Skin_Dark', 'PanelSkin'.");
		AddClassToCollection("MyGUIWindowComponent", "String getSkin()", "Gets the widgets skin.");
		AddClassToCollection("MyGUIWindowComponent", "void setMovable(bool movable)", "Sets whether this widget can be moved manually. Default value is false.");
		AddClassToCollection("MyGUIWindowComponent", "bool getMovable()", "Gets whether this widget can be moved manually.");
		AddClassToCollection("MyGUIWindowComponent", "void setWindowCaption(String caption)", "Sets the title caption for this widget.");
		AddClassToCollection("MyGUIWindowComponent", "String getWindowCaption()", "Gets the title caption of this widget.");
		// TODO: Is it required for lua api, that each derived widget must have all properties of the base widget??

		module(lua)
		[
			class_<MyGUIItemBoxComponent, MyGUIWindowComponent>("MyGUIItemBoxComponent")
			// .def("getClassName", &MyGUIItemBoxComponent::getClassName)
			// .def("getClassId", &MyGUIItemBoxComponent::getClassId)
			.def("getItemCount", &MyGUIItemBoxComponent::getItemCount)
			.def("getResourceName", &MyGUIItemBoxComponent::getResourceName)
			.def("setQuantity2", (void (MyGUIItemBoxComponent::*)(unsigned int, unsigned int)) & MyGUIItemBoxComponent::setQuantity)
			.def("setQuantity", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, unsigned int)) & MyGUIItemBoxComponent::setQuantity)
			.def("getQuantity2", (unsigned int (MyGUIItemBoxComponent::*)(unsigned int)) & MyGUIItemBoxComponent::getQuantity)
			.def("getQuantity", (unsigned int (MyGUIItemBoxComponent::*)(const Ogre::String&)) & MyGUIItemBoxComponent::getQuantity)
			.def("addQuantity", &MyGUIItemBoxComponent::addQuantity)
			.def("increaseQuantity", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, unsigned int)) & MyGUIItemBoxComponent::increaseQuantity)
			.def("decreaseQuantity", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, unsigned int)) & MyGUIItemBoxComponent::decreaseQuantity)
			.def("removeQuantity", &MyGUIItemBoxComponent::removeQuantity)

			.def("setSellValue2", (void (MyGUIItemBoxComponent::*)(unsigned int, Ogre::Real)) & MyGUIItemBoxComponent::setSellValue)
			.def("setSellValue", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, Ogre::Real)) & MyGUIItemBoxComponent::setSellValue)
			.def("getSellValue2", (Ogre::Real (MyGUIItemBoxComponent::*)(unsigned int)) & MyGUIItemBoxComponent::getSellValue)
			.def("getSellValue", (Ogre::Real (MyGUIItemBoxComponent::*)(const Ogre::String&)) & MyGUIItemBoxComponent::getSellValue)

			.def("setBuyValue2", (void (MyGUIItemBoxComponent::*)(unsigned int, Ogre::Real)) & MyGUIItemBoxComponent::setBuyValue)
			.def("setBuyValue", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, Ogre::Real)) & MyGUIItemBoxComponent::setBuyValue)
			.def("getBuyValue2", (Ogre::Real(MyGUIItemBoxComponent::*)(unsigned int)) & MyGUIItemBoxComponent::getBuyValue)
			.def("getBuyValue", (Ogre::Real(MyGUIItemBoxComponent::*)(const Ogre::String&)) & MyGUIItemBoxComponent::getBuyValue)

			.def("clearItems", &MyGUIItemBoxComponent::clearItems)
			.def("reactOnDropItem", &MyGUIItemBoxComponent::reactOnDropItem)
			.def("reactOnMouseButtonClick", &MyGUIItemBoxComponent::reactOnMouseButtonClick)
		];

		AddClassToCollection("MyGUIItemBoxComponent", "class inherits MyGUIWindowComponent", MyGUIItemBoxComponent::getStaticInfoText());
		AddClassToCollection("MyGUIItemBoxComponent", "int getItemCount()", "Gets the max inventory item count.");
		AddClassToCollection("MyGUIItemBoxComponent", "String getResourceName(int index)", "Gets resource name for the given index. E.g. if 'energy' resource is placed in inventory at index 1, getResourceName(1) would deliver the energy resource. "
			"Note: This function can be used in a loop in conjunction with @getItemCount to iterate over all items in inventory and dump all resources.");
		AddClassToCollection("MyGUIItemBoxComponent", "void setQuantity2(int index, int quantity)", "Sets the quantity of the resource for the given index.");
		AddClassToCollection("MyGUIItemBoxComponent", "void setQuantity(int index, String resourceName)", "Sets the quantity of the resource.");
		AddClassToCollection("MyGUIItemBoxComponent", "int getQuantity2(int index)", "Gets the quantity of the resource for the given index.");
		AddClassToCollection("MyGUIItemBoxComponent", "int getQuantity(String resourceName)", "Gets the quantity of the resource.");
		AddClassToCollection("MyGUIItemBoxComponent", "void increaseQuantity(String resourceName, int quantity)", "Gets the current quantity and increases by the given quantity value.");
		AddClassToCollection("MyGUIItemBoxComponent", "void decreaseQuantity(String resourceName, int quantity)", "Gets the current quantity and decreases by the given quantity value.");
		AddClassToCollection("MyGUIItemBoxComponent", "float getSellValue2(int index)", "Gets the sell value of the resource for the given index.");
		AddClassToCollection("MyGUIItemBoxComponent", "float getSellValue(String resourceName)", "Gets the sell value of the resource.");
		AddClassToCollection("MyGUIItemBoxComponent", "float getBuyValue2(int index)", "Gets the buy value of the resource for the given index.");
		AddClassToCollection("MyGUIItemBoxComponent", "float getBuyValue(String resourceName)", "Gets the buy value of the resource.");
		AddClassToCollection("MyGUIItemBoxComponent", "void addQuantity(String resourceName, int quantity)", "Adds the quantity of the resource, if does not exist, creates a new slot in inventory.");
		AddClassToCollection("MyGUIItemBoxComponent", "void removeQuantity(String resourceName, int quantity)", "Removes the quantity from the resource.");
		AddClassToCollection("MyGUIItemBoxComponent", "void clearItems()", "Cleares the whole inventory.");
		AddClassToCollection("MyGUIItemBoxComponent", "void reactOnDropItem(func closureFunction, DragDropData dragDropData)",
			"Sets whether to react if an item is requested to be drag and dropped to another inventory. A return value also can be set to prohibit the operation. E.g. getMyGUIItemBoxComponent():reactOnDropItem(function(dragDropData) ... end");
		AddClassToCollection("MyGUIComponent", "void reactOnMouseButtonClick(func closureFunction, string resourceName, int buttonId)",
														  "Sets whether to react if a mouse button has been clicked on the inventory. The clicked resource name will be received and the clicked mouse button id.");

		module(lua)
		[
			class_<DragDropData>("DragDropData")
			.def("getResourceName", &DragDropData::getResourceName)
			.def("getQuantity", &DragDropData::getQuantity)
			.def("getSellValue", &DragDropData::getSellValue)
			.def("getBuyValue", &DragDropData::getBuyValue)
			.def("getSenderReceiverIsSame", &DragDropData::getSenderReceiverIsSame)
			.def("getSenderInventoryId", &getSenderInventoryId)
			.def("setCanDrop", &DragDropData::setCanDrop)
		];
		AddClassToCollection("DragDropData", "DragDropData", "It can be used when an item is dragged from one inventory to another to get some data and control if it may be dropped.");
		AddClassToCollection("DragDropData", "String getResourceName()", "Gets the resource name.");
		AddClassToCollection("DragDropData", "int getQuantity()", "Gets resource quantity.");
		AddClassToCollection("DragDropData", "float getSellValue()", "Gets the sell value of the resource.");
		AddClassToCollection("DragDropData", "float getBuyValue()", "Gets the buy value of the resource.");
		AddClassToCollection("DragDropData", "bool getSenderReceiverIsSame()", "Gets whether the inventory sender and receiver is the same. E.g. moving the item within the same inventory.");
		AddClassToCollection("DragDropData", "string getSenderInventoryId()", "Gets the sender inventory id.");
		AddClassToCollection("DragDropData", "void setCanDrop(bool canDrop)", "Sets whether the item can be dropped.");

		module(lua)
			[
				class_<InventoryItemComponent, GameObjectComponent>("InventoryItemComponent")
				// .def("getClassName", &InventoryItemComponent::getClassName)
				// .def("getClassId", &InventoryItemComponent::getClassId)
			.def("setResourceName", &InventoryItemComponent::setResourceName)
			.def("getResourceName", &InventoryItemComponent::getResourceName)
			// .def("setQuantity", &InventoryItemComponent::setQuantity)
			// .def("getQuantity", &InventoryItemComponent::getQuantity)
			// .def("addQuantityToInventory", &InventoryItemComponent::addQuantityToInventory)
			// .def("removeQuantityFromInventory", &InventoryItemComponent::removeQuantityFromInventory)
			.def("addQuantityToInventory", &addQuantityToInventory)
			.def("removeQuantityFromInventory", &removeQuantityFromInventory)
			.def("addQuantityToInventory2", &addQuantityToInventory2)
			.def("removeQuantityFromInventory2", &removeQuantityFromInventory2)
			];

		AddClassToCollection("InventoryItemComponent", "class inherits GameObjectComponent", InventoryItemComponent::getStaticInfoText());
		AddClassToCollection("InventoryItemComponent", "void setResourceName(String resourceName)", "Sets the used resource name.");
		AddClassToCollection("InventoryItemComponent", "String getResourceName()", "Gets the used resource name.");
		AddClassToCollection("InventoryItemComponent", "void addQuantityToInventory(String inventoryIdGameObject, int quantity, bool once)", "Increases the quantity of this inventory item in the inventory game object. "
			"E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:addQuantityToInventory(MAIN_GAMEOBJECT_ID, 1, true)'");
		AddClassToCollection("InventoryItemComponent", "void removeQuantityFromInventory(String inventoryIdGameObject, int quantity, bool once)", "Decreases the quantity of this inventory item in the inventory game object. "
			"E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:removeQuantityFromInventory(MAIN_GAMEOBJECT_ID, 1, true)'");
		AddClassToCollection("InventoryItemComponent", "void addQuantityToInventory(String inventoryIdGameObject, String componentName, int quantity, bool once)", "Increases the quantity of this inventory item in the inventory game object. "
			"E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:addQuantityToInventory(MAIN_GAMEOBJECT_ID, 'Player1InventoryComponent', 1, true)'");
		AddClassToCollection("InventoryItemComponent", "void removeQuantityFromInventory(String inventoryIdGameObject, String componentName, int quantity, bool once)", "Decreases the quantity of this inventory item in the inventory game object. "
			"E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:removeQuantityFromInventory(MAIN_GAMEOBJECT_ID, 'Player1InventoryComponent', 1, true)'");

		module(lua)
			[
				class_<MyGUITextComponent, MyGUIComponent>("MyGUITextComponent")
				// .def("getClassName", &MyGUITextComponent::getClassName)
				// .def("getClassId", &MyGUITextComponent::getClassId)
			.def("setCaption", &MyGUITextComponent::setCaption)
			.def("getCaption", &MyGUITextComponent::getCaption)
			.def("setFontHeight", &MyGUITextComponent::setFontHeight)
			.def("getFontHeight", &MyGUITextComponent::getFontHeight)
			.def("setTextAlign", &MyGUITextComponent::setTextAlign)
			.def("getTextAlign", &MyGUITextComponent::getTextAlign)
			.def("setTextColor", &MyGUITextComponent::setTextColor)
			.def("getTextColor", &MyGUITextComponent::getTextColor)
			.def("setReadOnly", &MyGUITextComponent::setReadOnly)
			.def("getReadOnly", &MyGUITextComponent::getReadOnly)
			];

		AddClassToCollection("MyGUITextComponent", "class inherits MyGUIComponent", MyGUITextComponent::getStaticInfoText());
		AddClassToCollection("MyGUITextComponent", "void setCaption(String caption)", "Sets the caption for this widget. Note: If multi line is enabled, backslash 'n' can be used for a line break.");
		AddClassToCollection("MyGUITextComponent", "String getCaption()", "Gets the caption of this widget.");
		AddClassToCollection("MyGUITextComponent", "void setFontHeight(int fontHeight)", "Sets the font height for this widget. Note: Minimum value is 10 and maximum value is 60. Odd font sizes will be rounded to the next even font size. E.g. 23 will become 24.");
		AddClassToCollection("MyGUITextComponent", "int getFontHeight()", "Gets the font height of this widget.");
		AddClassToCollection("MyGUITextComponent", "void setTextAlign(String align)", "Sets the widget text alignment. Possible values are: 'HCenter', 'Center', 'Left', 'Right', 'HStretch', 'Top', 'Bottom', 'VStretch', 'Stretch', 'Default' (LEFT|TOP).");
		AddClassToCollection("MyGUITextComponent", "String getTextAlign()", "Gets the widget text alignment.");
		AddClassToCollection("MyGUITextComponent", "void setTextColor(Vector4 color)", "Sets the widget's text color (r, g, b, a).");
		AddClassToCollection("MyGUITextComponent", "Vector4 getTextColor()", "Gets the widget's text color (r, g, b, a).");
		AddClassToCollection("MyGUITextComponent", "void setReadOnly(bool readonly)", "Sets whether the widget is read only (Text cannot be manipulated).");
		AddClassToCollection("MyGUITextComponent", "bool getReadOnly()", "Gets whether the widget is read only (Text cannot be manipulated).");

		module(lua)
			[
				class_<MyGUIButtonComponent, MyGUIComponent>("MyGUIButtonComponent")
				// .def("getClassName", &MyGUIButtonComponent::getClassName)
				// .def("getClassId", &MyGUIButtonComponent::getClassId)
			.def("setCaption", &MyGUIButtonComponent::setCaption)
			.def("getCaption", &MyGUIButtonComponent::getCaption)
			.def("setFontHeight", &MyGUIButtonComponent::setFontHeight)
			.def("getFontHeight", &MyGUIButtonComponent::getFontHeight)
			.def("setTextAlign", &MyGUIButtonComponent::setTextAlign)
			.def("getTextAlign", &MyGUIButtonComponent::getTextAlign)
			.def("setTextColor", &MyGUIButtonComponent::setTextColor)
			.def("getTextColor", &MyGUIButtonComponent::getTextColor)
			];

		AddClassToCollection("MyGUIButtonComponent", "class inherits MyGUIComponent", MyGUIButtonComponent::getStaticInfoText());
		AddClassToCollection("MyGUIButtonComponent", "void setCaption(String caption)", "Sets the caption for this widget.");
		AddClassToCollection("MyGUIButtonComponent", "String getCaption()", "Gets the caption of this widget.");
		AddClassToCollection("MyGUIButtonComponent", "void setFontHeight(int fontHeight)", "Sets the font height for this widget. Note: Minimum value is 10 and maximum value is 60. Odd font sizes will be rounded to the next even font size. E.g. 23 will become 24.");
		AddClassToCollection("MyGUIButtonComponent", "int getFontHeight()", "Gets the font height of this widget.");
		AddClassToCollection("MyGUIButtonComponent", "void setTextAlign(String align)", "Sets the widget text alignment. Possible values are: 'HCenter', 'Center', 'Left', 'Right', 'HStretch', 'Top', 'Bottom', 'VStretch', 'Stretch', 'Default' (LEFT|TOP).");
		AddClassToCollection("MyGUIButtonComponent", "String getTextAlign()", "Gets the widget text alignment.");
		AddClassToCollection("MyGUIButtonComponent", "void setTextColor(Vector4 color)", "Sets the widget's text color (r, g, b, a).");
		AddClassToCollection("MyGUIButtonComponent", "Vector4 getTextColor()", "Gets the widget's text color (r, g, b, a).");

		module(lua)
			[
				class_<MyGUICheckBoxComponent, MyGUIComponent>("MyGUICheckBoxComponent")
				// .def("getClassName", &MyGUICheckBoxComponent::getClassName)
				// .def("getClassId", &MyGUICheckBoxComponent::getClassId)
			.def("setCaption", &MyGUICheckBoxComponent::setCaption)
			.def("getCaption", &MyGUICheckBoxComponent::getCaption)
			.def("setFontHeight", &MyGUICheckBoxComponent::setFontHeight)
			.def("getFontHeight", &MyGUICheckBoxComponent::getFontHeight)
			.def("setTextAlign", &MyGUICheckBoxComponent::setTextAlign)
			.def("getTextAlign", &MyGUICheckBoxComponent::getTextAlign)
			.def("setTextColor", &MyGUICheckBoxComponent::setTextColor)
			.def("getTextColor", &MyGUICheckBoxComponent::getTextColor)
			];

		AddClassToCollection("MyGUICheckBoxComponent", "class inherits MyGUIComponent", MyGUICheckBoxComponent::getStaticInfoText());
		AddClassToCollection("MyGUICheckBoxComponent", "void setCaption(String caption)", "Sets the caption for this widget.");
		AddClassToCollection("MyGUICheckBoxComponent", "String getCaption()", "Gets the caption of this widget.");
		AddClassToCollection("MyGUICheckBoxComponent", "void setFontHeight(int fontHeight)", "Sets the font height for this widget. Note: Minimum value is 10 and maximum value is 60. Odd font sizes will be rounded to the next even font size. E.g. 23 will become 24.");
		AddClassToCollection("MyGUICheckBoxComponent", "int getFontHeight()", "Gets the font height of this widget.");
		AddClassToCollection("MyGUICheckBoxComponent", "void setTextAlign(String align)", "Sets the widget text alignment. Possible values are: 'HCenter', 'Center', 'Left', 'Right', 'HStretch', 'Top', 'Bottom', 'VStretch', 'Stretch', 'Default' (LEFT|TOP).");
		AddClassToCollection("MyGUICheckBoxComponent", "String getTextAlign()", "Gets the widget text alignment.");
		AddClassToCollection("MyGUICheckBoxComponent", "void setTextColor(Vector4 color)", "Sets the widget's text color (r, g, b, a).");
		AddClassToCollection("MyGUICheckBoxComponent", "Vector4 getTextColor()", "Gets the widget's text color (r, g, b, a).");

		module(lua)
			[
				class_<MyGUIImageBoxComponent, MyGUIComponent>("MyGUIImageBoxComponent")
				// .def("getClassName", &MyGUIImageBoxComponent::getClassName)
				// .def("getClassId", &MyGUIImageBoxComponent::getClassId)
			.def("setImageFileName", &MyGUIImageBoxComponent::setImageFileName)
			.def("getImageFileName", &MyGUIImageBoxComponent::getImageFileName)
			.def("setCenter", &MyGUIImageBoxComponent::setCenter)
			.def("getCenter", &MyGUIImageBoxComponent::getCenter)
			.def("setAngle", &MyGUIImageBoxComponent::setAngle)
			.def("getAngle", &MyGUIImageBoxComponent::getAngle)
			.def("setRotationSpeed", &MyGUIImageBoxComponent::setRotationSpeed)
			.def("getRotationSpeed", &MyGUIImageBoxComponent::getRotationSpeed)
			];

		AddClassToCollection("MyGUIImageBoxComponent", "class inherits MyGUIComponent", MyGUIImageBoxComponent::getStaticInfoText());
		AddClassToCollection("MyGUIImageBoxComponent", "void setImageFileName(String fileName)", "Sets the image file name. Note: Must exist in a resource group.");
		AddClassToCollection("MyGUIImageBoxComponent", "String getImageFileName()", "Gets the used image file name.");
		AddClassToCollection("MyGUIImageBoxComponent", "void setCenter(Vector2 center)", "Sets the center position of the image. Note: Since an image can be rotated, the center position is imporant. Default is 0.5, 0.5.");
		AddClassToCollection("MyGUIImageBoxComponent", "Vector2 getCenter()", "Gets the center position of the image.");
		AddClassToCollection("MyGUIImageBoxComponent", "void setAngle(float angleDegrees)", "Sets the image angle in degrees. Note: @setRotationSpeed must be 0, else the image will be rotating and overwriting the current angle.");
		AddClassToCollection("MyGUIImageBoxComponent", "float getAngle()", "Gets the image angle in degrees. Note: If @setRotationSpeed is set different from 0, the current angle continues to change.");
		AddClassToCollection("MyGUIImageBoxComponent", "void setRotationSpeed(float rotationSpeed)", "Sets the image rotation speed, if its desired, that the image should rotate.");
		AddClassToCollection("MyGUIImageBoxComponent", "float getRotationSpeed()", "Gets the image rotation speed.");

		module(lua)
			[
				class_<MyGUIProgressBarComponent, MyGUIComponent>("MyGUIProgressBarComponent")
				// .def("getClassName", &MyGUIProgressBarComponent::getClassName)
				// .def("getClassId", &MyGUIProgressBarComponent::getClassId)
			.def("setValue", &MyGUIProgressBarComponent::setValue)
			.def("getValue", &MyGUIProgressBarComponent::getValue)
			.def("setRange", &MyGUIProgressBarComponent::setRange)
			.def("getRange", &MyGUIProgressBarComponent::getRange)
			.def("setFlowDirection", &MyGUIProgressBarComponent::setFlowDirection)
			.def("getFlowDirection", &MyGUIProgressBarComponent::getFlowDirection)
			];

		AddClassToCollection("MyGUIProgressBarComponent", "class inherits MyGUIComponent", MyGUIProgressBarComponent::getStaticInfoText());
		AddClassToCollection("MyGUIProgressBarComponent", "void setValue(int value)", "Sets the progress bar current value.");
		AddClassToCollection("MyGUIProgressBarComponent", "int getValue()", "Gets the progress bar current value.");
		AddClassToCollection("MyGUIProgressBarComponent", "void setRange(int value)", "Sets the range (max value) for the progress bar.");
		AddClassToCollection("MyGUIProgressBarComponent", "int getRange()", "Gets the range (max value) for the progress bar.");
		AddClassToCollection("MyGUIProgressBarComponent", "void setFlowDirection(String flowDirection)", "Sets flow direction for the progress bar.");
		AddClassToCollection("MyGUIProgressBarComponent", "String getFlowDirection()", "Gets the flow direction for the progress bar. Possible values are: 'LeftToRight', 'RightToLeft', 'TopToBottom', 'BottomToTop'");


		module(lua)
		[
			class_<MyGUIListBoxComponent, MyGUIComponent>("MyGUIListBoxComponent")
			.def("setItemCount", &MyGUIListBoxComponent::setItemCount)
			.def("getItemCount", &MyGUIListBoxComponent::getItemCount)
			.def("setItemText", &MyGUIListBoxComponent::setItemText)
			.def("getItemText", &MyGUIListBoxComponent::getItemText)
			.def("addItem", &MyGUIListBoxComponent::addItem)
			.def("insertItem", &MyGUIListBoxComponent::insertItem)
			.def("removeItem", &MyGUIListBoxComponent::removeItem)
			.def("getSelectedIndex", &MyGUIListBoxComponent::getSelectedIndex)
			.def("reactOnSelected", &MyGUIListBoxComponent::reactOnSelected)
			.def("reactOnAccept", &MyGUIListBoxComponent::reactOnAccept)
		];

		AddClassToCollection("MyGUIListBoxComponent", "class inherits MyGUIComponent", MyGUIListBoxComponent::getStaticInfoText());
		AddClassToCollection("MyGUIListBoxComponent", "void setItemCount(unsigned int itemCount)", "Sets items count.");
		AddClassToCollection("MyGUIListBoxComponent", "unsigned int getItemCount()", "Gets the items count.");
		AddClassToCollection("MyGUIListBoxComponent", "void setItemText(unsigned int index, String itemText)", "Sets the item text at the given index.");
		AddClassToCollection("MyGUIListBoxComponent", "String getItemText(unsigned int index)", "Gets the the item at the given index.");
		AddClassToCollection("MyGUIListBoxComponent", "void addItem(String item)", "Adds an item.");
		AddClassToCollection("MyGUIListBoxComponent", "void insertItem(int index, String item)", "Inserts an item at the given index.");
		AddClassToCollection("MyGUIListBoxComponent", "void removeItem(unsigned int index)", "Removes the item at the given index.");
		AddClassToCollection("MyGUIListBoxComponent", "int getSelectedIndex()", "Gets the selected index, or -1 if nothing is selected. Note: Always check against -1!");
		AddClassToCollection("MyGUIListBoxComponent", "void reactOnSelected(func closureFunction, int index)",
							 "Sets whether to react if a list item has been selected. The clicked list item index will be received.");
		AddClassToCollection("MyGUIListBoxComponent", "void reactOnAccept(func closureFunction, int index)",
							 "Sets whether to react if a list item change has been accepted. The accepted list item index will be received.");

		module(lua)
			[
				class_<MyGUIComboBoxComponent, MyGUIComponent>("MyGUIComboBoxComponent")
				.def("setItemCount", &MyGUIComboBoxComponent::setItemCount)
			.def("getItemCount", &MyGUIComboBoxComponent::getItemCount)
			.def("setItemText", &MyGUIComboBoxComponent::setItemText)
			.def("getItemText", &MyGUIComboBoxComponent::getItemText)
			.def("addItem", &MyGUIComboBoxComponent::addItem)
			.def("insertItem", &MyGUIComboBoxComponent::insertItem)
			.def("removeItem", &MyGUIComboBoxComponent::removeItem)
			.def("getSelectedIndex", &MyGUIComboBoxComponent::getSelectedIndex)
			.def("reactOnSelected", &MyGUIComboBoxComponent::reactOnSelected)
			];

		AddClassToCollection("MyGUIComboBoxComponent", "class inherits MyGUIComponent", MyGUIComboBoxComponent::getStaticInfoText());
		AddClassToCollection("MyGUIComboBoxComponent", "void setItemCount(unsigned int itemCount)", "Sets items count.");
		AddClassToCollection("MyGUIComboBoxComponent", "unsigned int getItemCount()", "Gets the items count.");
		AddClassToCollection("MyGUIComboBoxComponent", "void setItemText(unsigned int index, String itemText)", "Sets the item text at the given index.");
		AddClassToCollection("MyGUIComboBoxComponent", "String getItemText(unsigned int index)", "Gets the the item at the given index.");
		AddClassToCollection("MyGUIComboBoxComponent", "void addItem(String item)", "Adds an item.");
		AddClassToCollection("MyGUIComboBoxComponent", "void insertItem(int index, String item)", "Inserts an item at the given index.");
		AddClassToCollection("MyGUIComboBoxComponent", "void removeItem(unsigned int index)", "Removes the item at the given index.");
		AddClassToCollection("MyGUIComboBoxComponent", "int getSelectedIndex()", "Gets the selected index, or -1 if nothing is selected. Note: Always check against -1!");
		AddClassToCollection("MyGUIComboBoxComponent", "void reactOnSelected(func closureFunction, int index)",
							 "Sets whether to react if a list item has been selected. The clicked list item index will be received.");

		module(lua)
			[
				class_<MyGUI::MessageBoxStyle>("MessageBoxStyle")
				.enum_("Enum")
			[
				value("Ok", MyGUI::MessageBoxStyle::Ok),
				value("Yes", MyGUI::MessageBoxStyle::Yes),
				value("No", MyGUI::MessageBoxStyle::No),
				value("Abort", MyGUI::MessageBoxStyle::Abort),
				value("Retry", MyGUI::MessageBoxStyle::Retry),
				value("Ignore", MyGUI::MessageBoxStyle::Ignore),
				value("Cancel", MyGUI::MessageBoxStyle::Cancel),
				value("Try", MyGUI::MessageBoxStyle::Try),
				value("Continue", MyGUI::MessageBoxStyle::Continue)
			]
			];

		AddClassToCollection("MessageBoxStyle", "class", "MessageBox style class for MyGUI Message box reaction, which button has been pressed.");
		AddClassToCollection("MessageBoxStyle", "Yes", "Yes button.");
		AddClassToCollection("MessageBoxStyle", "No", "No button.");
		AddClassToCollection("MessageBoxStyle", "Abort", "Abort button.");
		AddClassToCollection("MessageBoxStyle", "Retry", "Retry button.");
		AddClassToCollection("MessageBoxStyle", "Ignore", "Ignore button.");
		AddClassToCollection("MessageBoxStyle", "Cancel", "Cancel button.");
		AddClassToCollection("MessageBoxStyle", "Try", "Try button.");
		AddClassToCollection("MessageBoxStyle", "Continue", "Continue button.");

		module(lua)
			[
				class_<MyGUIMessageBoxComponent, GameObjectComponent>("MyGUIMessageBoxComponent")
				.def("setActivated", &MyGUIMessageBoxComponent::setActivated)
			.def("isActivated", &MyGUIMessageBoxComponent::isActivated)
			.def("setTitle", &MyGUIMessageBoxComponent::setTitle)
			.def("getTitle", &MyGUIMessageBoxComponent::getTitle)
			.def("setMessage", &MyGUIMessageBoxComponent::setMessage)
			.def("getMessage", &MyGUIMessageBoxComponent::getMessage)
			.def("setStylesCount", &MyGUIMessageBoxComponent::setStylesCount)
			.def("getStylesCount", &MyGUIMessageBoxComponent::getStylesCount)
			.def("setStyle", &MyGUIMessageBoxComponent::setStyle)
			.def("getStyle", &MyGUIMessageBoxComponent::getStyle)
			];

		AddClassToCollection("MyGUIMessageBoxComponent", "class inherits MyGUIComponent", MyGUIComboBoxComponent::getStaticInfoText());
		AddClassToCollection("MyGUIMessageBoxComponent", "void setActivated(bool activated)", "Activates this component, so that the message box will be shown.");
		AddClassToCollection("MyGUIMessageBoxComponent", "bool isActivated()", "Gets whether the message box is activated (shown).");
		AddClassToCollection("MyGUIMessageBoxComponent", "void setTitle(String title)", "Sets the title. Note: Using #{Identifier}, the text can be presented in a prior specified language.");
		AddClassToCollection("MyGUIMessageBoxComponent", "String getTitle()", "Gets the title.");
		AddClassToCollection("MyGUIMessageBoxComponent", "void setMessage(String message)", "Sets the message. Note: Using #{Identifier}, the text can be presented in a prior specified language.");
		AddClassToCollection("MyGUIMessageBoxComponent", "String getMessage()", "Gets the Message.");
		AddClassToCollection("MyGUIMessageBoxComponent", "void setStylesCount(unsigned int stylesCount)", "Sets the message box styles count.");
		AddClassToCollection("MyGUIMessageBoxComponent", "unsigned int getStylesCount()", "Gets the styles count.");
		AddClassToCollection("MyGUIMessageBoxComponent", "void setStyle(unsigned int index, String style)", "Sets the style at the given index. Possible values are: "
			"'Ok', 'Yes', 'No', 'Abord', 'Retry', 'Ignore', 'Cancel', 'Try', 'Continue', 'IconInfo', 'IconQuest', 'IconError', 'IconWarning'. Note: Several styles can be combined.");
		AddClassToCollection("MyGUIMessageBoxComponent", "String getStyle(unsigned int index)", "Gets the the style at the given index.");

		////////////////////////////////////MyGUI Controller/////////////////////////////////////////////////

		module(lua)
			[
				class_<MyGUIControllerComponent, GameObjectComponent>("MyGUIControllerComponent")
				// .def("getClassName", &MyGUIControllerComponent::getClassName)
				// .def("getClassId", &MyGUIControllerComponent::getClassId)
			.def("setActivated", &MyGUIControllerComponent::setActivated)
			.def("isActivated", &MyGUIControllerComponent::isActivated)
			// .def("setSourceId", &MyGUIControllerComponent::setSourceId)
			// .def("getSourceId", &MyGUIControllerComponent::getSourceId)
			.def("setSourceId", &setSourceIdMyGUIController)
			.def("getSourceId", &getSourceIdMyGUIController)
			];

		AddClassToCollection("MyGUIControllerComponent", "class inherits GameObjectComponent", MyGUIControllerComponent::getStaticInfoText());
		AddClassToCollection("MyGUIControllerComponent", "void setActivated(bool activated)", "Activates the controller for a target widget component.");
		AddClassToCollection("MyGUIControllerComponent", "bool isActivated()", "Gets whether the controller for a target widget component is activated.");
		AddClassToCollection("MyGUIControllerComponent", "void setSourceId(String sourceId)", "Sets the source MyGUI UI widget component id, at which this controller should be attached to.");
		AddClassToCollection("MyGUIControllerComponent", "String getSourceId()", "Gets the source MyGUI UI widget component id, at which this controller is attached.");

		module(lua)
			[
				class_<MyGUIPositionControllerComponent, MyGUIControllerComponent>("MyGUIPositionControllerComponent")
				// .def("getClassName", &MyGUIPositionControllerComponent::getClassName)
				// .def("getClassId", &MyGUIPositionControllerComponent::getClassId)
			.def("setCoordinate", &MyGUIPositionControllerComponent::setCoordinate)
			.def("getCoordinate", &MyGUIPositionControllerComponent::getCoordinate)
			.def("setFunction", &MyGUIPositionControllerComponent::setFunction)
			.def("getFunction", &MyGUIPositionControllerComponent::getFunction)
			.def("setDurationSec", &MyGUIPositionControllerComponent::setDurationSec)
			.def("getDurationSec", &MyGUIPositionControllerComponent::getDurationSec)
			];

		AddClassToCollection("MyGUIPositionControllerComponent", "class inherits MyGUIControllerComponent", MyGUIPositionControllerComponent::getStaticInfoText());
		AddClassToCollection("MyGUIPositionControllerComponent", "void setCoordinate(Vector4 coordinate)", "Sets the target transform coordinate at which the widget will be moved and resized. Note: x,y is the new relative position and z,w is the new size. If z,w is set to 0, the widget will keep its current size.");
		AddClassToCollection("MyGUIPositionControllerComponent", "Vector4 getCoordinate()", "Gets the target transform coordinate.");
		AddClassToCollection("MyGUIPositionControllerComponent", "void setFunction(String movementFunction)", "Sets the movement function. Possible values are: 'Inertional', 'Accelerated', 'Slowed', 'Jump'.");
		AddClassToCollection("MyGUIPositionControllerComponent", "String getFunction()", "Gets the used movement function.");
		AddClassToCollection("MyGUIPositionControllerComponent", "void setDurationSec(float durationSec)", "Sets the duration in seconds after which the widget should be at the target coordinate.");
		AddClassToCollection("MyGUIPositionControllerComponent", "float getDurationSec()", "Gets the duration in seconds.");

		module(lua)
			[
				class_<MyGUIFadeAlphaControllerComponent, MyGUIControllerComponent>("MyGUIFadeAlphaControllerComponent")
				// .def("getClassName", &MyGUIFadeAlphaControllerComponent::getClassName)
				// .def("getClassId", &MyGUIFadeAlphaControllerComponent::getClassId)
			.def("setAlpha", &MyGUIFadeAlphaControllerComponent::setAlpha)
			.def("getAlpha", &MyGUIFadeAlphaControllerComponent::getAlpha)
			.def("setCoefficient", &MyGUIFadeAlphaControllerComponent::setCoefficient)
			.def("getCoefficient", &MyGUIFadeAlphaControllerComponent::getCoefficient)
			];

		AddClassToCollection("MyGUIFadeAlphaControllerComponent", "class inherits MyGUIControllerComponent", MyGUIFadeAlphaControllerComponent::getStaticInfoText());
		AddClassToCollection("MyGUIFadeAlphaControllerComponent", "void setAlpha(float alpha)", "Sets the alpha that will be as result of changing.");
		AddClassToCollection("MyGUIFadeAlphaControllerComponent", "float getAlpha()", "Gets the target alpha.");
		AddClassToCollection("MyGUIFadeAlphaControllerComponent", "void setCoefficient(float coefficient)", "Sets the coefficient of alpha changing speed. Note: 1 means that alpha will change from 0 to 1 at 1 second.");
		AddClassToCollection("MyGUIFadeAlphaControllerComponent", "float getCoefficient()", "Gets the coefficient of alpha changing speed.");

		module(lua)
			[
				class_<MyGUIScrollingMessageControllerComponent, MyGUIControllerComponent>("MyGUIScrollingMessageControllerComponent")
				// .def("getClassName", &MyGUIScrollingMessageControllerComponent::getClassName)
				// .def("getClassId", &MyGUIScrollingMessageControllerComponent::getClassId)
			.def("setMessageCount", &MyGUIScrollingMessageControllerComponent::setMessageCount)
			.def("getMessageCount", &MyGUIScrollingMessageControllerComponent::getMessageCount)
			.def("setMessage", &MyGUIScrollingMessageControllerComponent::setMessage)
			.def("getMessage", &MyGUIScrollingMessageControllerComponent::getMessage)
			.def("setDurationSec", &MyGUIScrollingMessageControllerComponent::setDurationSec)
			.def("getDurationSec", &MyGUIScrollingMessageControllerComponent::getDurationSec)
			];

		AddClassToCollection("MyGUIScrollingMessageControllerComponent", "class inherits MyGUIControllerComponent", MyGUIScrollingMessageControllerComponent::getStaticInfoText());
		AddClassToCollection("MyGUIScrollingMessageControllerComponent", "void setMessageCount(int count)", "Sets scrolling message line count. Note: Each message occupies one line.");
		AddClassToCollection("MyGUIScrollingMessageControllerComponent", "int getMessageCount()", "Gets the scrolling message line count.");
		AddClassToCollection("MyGUIScrollingMessageControllerComponent", "void setMessage(int index, String message)", "Sets scrolling message at the given index.");
		AddClassToCollection("MyGUIScrollingMessageControllerComponent", "String getMessage(int index)", "Gets the scrolling message from the given index.");
		AddClassToCollection("MyGUIPositionControllerComponent", "void setDurationSec(float durationSec)", "Sets the scrolling duration in seconds.");
		AddClassToCollection("MyGUIPositionControllerComponent", "float getDurationSec()", "Gets the scrolling duration in seconds.");

		module(lua)
		[
			class_<MyGUIEdgeHideControllerComponent, MyGUIControllerComponent>("MyGUIEdgeHideControllerComponent")
			// .def("getClassName", &MyGUIEdgeHideControllerComponent::getClassName)
			// .def("getClassId", &MyGUIEdgeHideControllerComponent::getClassId)
			.def("setTimeSec", &MyGUIEdgeHideControllerComponent::setTimeSec)
			.def("getTimeSec", &MyGUIEdgeHideControllerComponent::getTimeSec)
			.def("setRemainPixels", &MyGUIEdgeHideControllerComponent::setRemainPixels)
			.def("getRemainPixels", &MyGUIEdgeHideControllerComponent::getRemainPixels)
			.def("setShadowSize", &MyGUIEdgeHideControllerComponent::setShadowSize)
			.def("getShadowSize", &MyGUIEdgeHideControllerComponent::getShadowSize)
		];

		AddClassToCollection("MyGUIEdgeHideControllerComponent", "class inherits MyGUIControllerComponent", MyGUIEdgeHideControllerComponent::getStaticInfoText());
		AddClassToCollection("MyGUIEdgeHideControllerComponent", "void setTimeSec(float timeSec)", "Sets the time in seconds in which widget will be hidden or shown.");
		AddClassToCollection("MyGUIEdgeHideControllerComponent", "float getTimeSec()", "Gets the time in seconds in which widget will be hidden or shown.");
		AddClassToCollection("MyGUIEdgeHideControllerComponent", "void setRemainPixels(int pixels)", "Sets how many pixels are visible after full hide.");
		AddClassToCollection("MyGUIEdgeHideControllerComponent", "int getRemainPixels()", "Gets how many pixels are visible after full hide.");
		AddClassToCollection("MyGUIEdgeHideControllerComponent", "void setShadowSize(int shadowSize)", "Sets the shadow size, which will be added to 'remain pixels' when hiding left or top (for example used for windows with shadows).");
		AddClassToCollection("MyGUIEdgeHideControllerComponent", "int getShadowSize()", "Gets the shadow size, which will be added to 'remain pixels' when hiding left or top.");

		module(lua)
		[
			class_<MyGUIRepeatClickControllerComponent, MyGUIControllerComponent>("MyGUIRepeatClickControllerComponent")
			// .def("getClassName", &MyGUIRepeatClickControllerComponent::getClassName)
			// .def("getClassId", &MyGUIRepeatClickControllerComponent::getClassId)
			.def("setTimeLeftSec", &MyGUIRepeatClickControllerComponent::setTimeLeftSec)
			.def("getTimeLeftSec", &MyGUIRepeatClickControllerComponent::getTimeLeftSec)
			.def("setStep", &MyGUIRepeatClickControllerComponent::setStep)
			.def("getStep", &MyGUIRepeatClickControllerComponent::getStep)
		];

		AddClassToCollection("MyGUIRepeatClickControllerComponent", "class inherits MyGUIControllerComponent", MyGUIRepeatClickControllerComponent::getStaticInfoText());
		AddClassToCollection("MyGUIRepeatClickControllerComponent", "void setTimeLeftSec(float timeLeftSec)", "Sets the delay in second before the first event will be triggered.");
		AddClassToCollection("MyGUIRepeatClickControllerComponent", "float getTimeLeftSec()", "Gets the delay in second before the first event will be triggered.");
		AddClassToCollection("MyGUIRepeatClickControllerComponent", "void setStep(float step)", "Sets the delay after each event before the next event is triggered.");
		AddClassToCollection("MyGUIRepeatClickControllerComponent", "float getStep()", "Gets the delay after each event before the next event is triggered.");

		module(lua)
		[
				class_<MyGUIMiniMapComponent, MyGUIWindowComponent>("MyGUIMiniMapComponent")
				// .def("getClassName", &MyGUIMiniMapComponent::getClassName)
				// .def("getClassId", &MyGUIMiniMapComponent::getClassId)
			.def("showMiniMap", &MyGUIMiniMapComponent::showMiniMap)
			.def("isMiniMapShown", &MyGUIMiniMapComponent::isMiniMapShown)
			.def("setStartPosition", &MyGUIMiniMapComponent::setStartPosition)
			.def("getStartPosition", &MyGUIMiniMapComponent::getStartPosition)
			// .def("setSkinName", &MyGUIMiniMapComponent::setSkinName)
			// .def("getSkinName", &MyGUIMiniMapComponent::getSkinName)
			.def("getMiniMapTilesCount", &MyGUIMiniMapComponent::getMiniMapTilesCount)
			.def("setMiniMapTileColor", &MyGUIMiniMapComponent::setMiniMapTileColor)
			.def("getMiniMapTileColor", &MyGUIMiniMapComponent::getMiniMapTileColor)
			.def("setToolTipDescription", &MyGUIMiniMapComponent::setToolTipDescription)
			.def("getToolTipDescription", &MyGUIMiniMapComponent::getToolTipDescription)
			.def("setMiniMapTileVisible", &MyGUIMiniMapComponent::setMiniMapTileVisible)
			.def("isMiniMapTileVisible", &MyGUIMiniMapComponent::isMiniMapTileVisible)
			.def("setTrackableCount", &MyGUIMiniMapComponent::setTrackableCount)
			.def("getTrackableCount", &MyGUIMiniMapComponent::getTrackableCount)
			// id is here int and correct, must not be converted to string
			.def("setTrackableId", &MyGUIMiniMapComponent::setTrackableId)
			.def("getTrackableId", &MyGUIMiniMapComponent::getTrackableId)
			.def("setTrackableImage", &MyGUIMiniMapComponent::setTrackableImage)
			.def("getTrackableImage", &MyGUIMiniMapComponent::getTrackableImage)
			.def("setTrackableImageTileSize", &MyGUIMiniMapComponent::setTrackableImageTileSize)
			.def("getTrackableImageTileSize", &MyGUIMiniMapComponent::getTrackableImageTileSize)
			.def("generateMiniMap", &MyGUIMiniMapComponent::generateMiniMap)
			.def("generateTrackables", &MyGUIMiniMapComponent::generateTrackables)
			.def("reactOnMouseButtonClick", &MyGUIMiniMapComponent::reactOnMouseButtonClick)
		];

		AddClassToCollection("MyGUIMiniMapComponent", "class inherits MyGUIWindowComponent", MyGUIMiniMapComponent::getStaticInfoText());
		AddClassToCollection("MyGUIMiniMapComponent", "void showMiniMap(bool show)", "Shows the minimap. Internally generates the minimap from the scratch and the trackables.");
		AddClassToCollection("MyGUIMiniMapComponent", "bool isMiniMapShown()", "Gets whether the minimap is shown.");
		AddClassToCollection("MyGUIMiniMapComponent", "void setStartPosition(Vector2 startPosition)", "Sets the relative start position, at which the whole minimap will be generated. Default value is 0,0 (left, top corner). E.g. setting to 0.5, 0.5 would start in the middle of the screen.");
		AddClassToCollection("MyGUIMiniMapComponent", "Vector2 getStartPosition()", "Gets the start position, at which the whole minimap will be generated.");
		AddClassToCollection("MyGUIMiniMapComponent", "int getMiniMapTilesCount()", "Gets the generated minimap tiles count.");
		AddClassToCollection("MyGUIMiniMapComponent", "void setMiniMapTileColor(int index, Vector3 color)", "Sets the color for the given minimap tile index.");
		AddClassToCollection("MyGUIMiniMapComponent", "Vector3 getMiniMapTileColor(int index)", "Gets the color for the given minimap tile index.");
		AddClassToCollection("MyGUIMiniMapComponent", "void setToolTipDescription(int index, String description)", "Sets the tooltip description for the given minimap tile index.");
		AddClassToCollection("MyGUIMiniMapComponent", "String getToolTipDescription(int index)", "Gets the tooltip description for the given minimap tile index.");
		AddClassToCollection("MyGUIMiniMapComponent", "void setTrackableCount(int count)", "Sets the count of trackables. Trackables are important points on a map, like items or save room etc.");
		AddClassToCollection("MyGUIMiniMapComponent", "int getTrackableCount()", "Gets the count of trackables.");
		AddClassToCollection("MyGUIMiniMapComponent", "void setTrackableId(int index, String id)", "Sets the trackable id for the given trackable index, that is shown on the mini map. Note: "
			"If the trackable id is in another scene, the scene name must be specified. For example 'scene3:2341435213'. "
			"Will search in scene3 for the game object with the id 2341435213 and in conjunction with the image attribute, the image will be placed correctly on the minimap."
			"If the scene name is missing, its assumed, that the id is an global one (like the player which is available for each scene) and has the same id for each scene.");
		AddClassToCollection("MyGUIMiniMapComponent", "String getTrackableId()", "Gets the count of trackables.");
		AddClassToCollection("MyGUIMiniMapComponent", "void setTrackableImage(int index, String imageName)", "Sets the image name for the given trackable index.");
		AddClassToCollection("MyGUIMiniMapComponent", "String getTrackableImage(int index)", "Gets the image name for the given trackable index.");
		AddClassToCollection("MyGUIMiniMapComponent", "void setTrackableImageTileSize(int index, Vector2 tileSize)", "Sets the image tile size for the given trackable index. Details: Image may be of size: 32x64, but tile size 32x32, so that sprite animation is done automatically switching the image tiles from 0 to 32 and 32 to 64 automatically.");
		AddClassToCollection("MyGUIMiniMapComponent", "Vector2 getTrackableImageTileSize(int index)", "Gets the image name for the given trackable index.");
		AddClassToCollection("MyGUIMiniMapComponent", "void generateMiniMap()", "Generates the minimap from the scratch manually.");
		AddClassToCollection("MyGUIMiniMapComponent", "void generateTrackables()", "Generates the trackables from the scratch manually.");

		AddClassToCollection("MyGUIComponent", "void reactOnMouseButtonClick(func closureFunction, int mapTileIndex)",
														  "Sets whether to react if a mouse button has been clicked on the minimap. The clicked map tile index will be received.");
	}

	void bindPhysicsVehicle(lua_State* lua)
	{

	}

	void bindPhysicsTire(lua_State* lua)
	{

	}

	/////////////////////Modules///////////////////////////////////////////////////////////////

	void bindCaelumModule(lua_State* lua)
	{

	}

	void bindDefferedShadingModule(lua_State* lua)
	{

	}

	void bindHydraxModule(lua_State* lua)
	{

	}

	void bindOgreALModule(lua_State* lua)
	{
		module(lua)
		[
			class_<OgreAL::AudioProcessor>("AudioProcessor")
			.enum_("SpectrumPreparationType")
			[
				value("RAW", OgreAL::AudioProcessor::SpectrumPreparationType::RAW),
				value("LINEAR", OgreAL::AudioProcessor::SpectrumPreparationType::LINEAR),
				value("LOGARITHMIC", OgreAL::AudioProcessor::SpectrumPreparationType::LOGARITHMIC)
			]
			.enum_("SpectrumArea")
			[
				value("DEEP_BASS", OgreAL::AudioProcessor::SpectrumArea::DEEP_BASS),
				value("LOW_BASS", OgreAL::AudioProcessor::SpectrumArea::LOW_BASS),
				value("MID_BASS", OgreAL::AudioProcessor::SpectrumArea::MID_BASS),
				value("UPPER_BASS", OgreAL::AudioProcessor::SpectrumArea::UPPER_BASS),
				value("LOWER_MIDRANGE", OgreAL::AudioProcessor::SpectrumArea::LOWER_MIDRANGE),
				value("MIDDLE_MIDRANGE", OgreAL::AudioProcessor::SpectrumArea::MIDDLE_MIDRANGE),
				value("UPPER_MIDRANGE", OgreAL::AudioProcessor::SpectrumArea::UPPER_MIDRANGE),
				value("PRESENCE_RANGE", OgreAL::AudioProcessor::SpectrumArea::PRESENCE_RANGE),
				value("HIGH_END", OgreAL::AudioProcessor::SpectrumArea::HIGH_END),
				value("EXTREMELY_HIGH_END", OgreAL::AudioProcessor::SpectrumArea::EXTREMELY_HIGH_END)
			]
			.enum_("Instrument")
			[
				value("KICK_DRUM", OgreAL::AudioProcessor::Instrument::KICK_DRUM),
				value("SNARE_DRUM", OgreAL::AudioProcessor::Instrument::SNARE_DRUM)
			]
		];

		AddClassToCollection("AudioProcessor", "class", "The audio processor for spectrum analysis.");
		AddClassToCollection("SpectrumPreparationType", "RAW", "The raw spectrum preparation.");
		AddClassToCollection("SpectrumPreparationType", "LINEAR", "The linear spectrum preparation.");
		AddClassToCollection("SpectrumPreparationType", "LOGARITHMIC", "The logarithmnic spectrum preparation.");

		AddClassToCollection("SpectrumArea", "DEEP_BASS", "Deep bass spectrum area (20hz-40hz).");
		AddClassToCollection("SpectrumArea", "LOW_BASS", "Low bass spectrum area (40hz-80hz).");
		AddClassToCollection("SpectrumArea", "MID_BASS", "Mid bass spectrum area (80hz-160hz).");
		AddClassToCollection("SpectrumArea", "UPPER_BASS", "Upper bass spectrum area (160hz-300hz).");
		AddClassToCollection("SpectrumArea", "LOWER_MIDRANGE", "Lower mid range spectrum area (300hz-600hz).");
		AddClassToCollection("SpectrumArea", "MIDDLE_MIDRANGE", "Middle mit range spectrum area (600hz-1200hz).");
		AddClassToCollection("SpectrumArea", "UPPER_MIDRANGE", "Upper mit range spectrum area (1200hz-2400hz).");
		AddClassToCollection("SpectrumArea", "PRESENCE_RANGE", "Presence range spectrum area (2400hz-5000hz).");
		AddClassToCollection("SpectrumArea", "HIGH_END", "High end spectrum area (5000hz-10000hz).");
		AddClassToCollection("SpectrumArea", "EXTREMELY_HIGH_END", "Extremely high end spectrum area (10000hz-20000hz).");

		AddClassToCollection("Instrument", "KICK_DRUM", "Kick drum instrument (60hz-130hz).");
		AddClassToCollection("Instrument", "SNARE_DRUM", "Snare drum instrument (301hz-750hz).");

		module(lua)
		[
			class_<OgreAL::MathWindows>("MathWindows")
			.enum_("WindowType")
			[
				value("RECTANGULAR", OgreAL::MathWindows::WindowType::RECTANGULAR),
				value("BLACKMAN", OgreAL::MathWindows::WindowType::BLACKMAN),
				value("BLACKMAN_HARRIS", OgreAL::MathWindows::WindowType::BLACKMAN_HARRIS),
				value("TUKEY", OgreAL::MathWindows::WindowType::TUKEY),
				value("HANNING", OgreAL::MathWindows::WindowType::HANNING),
				value("HAMMING", OgreAL::MathWindows::WindowType::HAMMING),
				value("BARLETT", OgreAL::MathWindows::WindowType::BARLETT)
			]
		];

		AddClassToCollection("MathWindows", "class", "Math windows class for audio spectrum analysis, for more harmonic visualization.");
		AddClassToCollection("WindowType", "RECTANGULAR", "The rectangular window.");
		AddClassToCollection("WindowType", "BLACKMAN", "The blackman window.");
		AddClassToCollection("WindowType", "BLACKMAN_HARRIS", "The blackman harris window.");
		AddClassToCollection("WindowType", "TUKEY", "The tukey window.");
		AddClassToCollection("WindowType", "HANNING", "The hanning window.");
		AddClassToCollection("WindowType", "HAMMING", "The Hamming window.");
		AddClassToCollection("WindowType", "BARLETT", "The barlett window.");

		module(lua)
		[
			class_<OgreAL::Sound, Ogre::MovableObject>("Sound")
			.def("play", &OgreAL::Sound::play)
			.def("isPlaying", &OgreAL::Sound::isPlaying)
			.def("pause", &OgreAL::Sound::pause)
			.def("isPaused", &OgreAL::Sound::isPaused)
			.def("stop", &OgreAL::Sound::stop)
			.def("isStopped", &OgreAL::Sound::isStopped)
			.def("fadeIn", &OgreAL::Sound::fadeIn)
			.def("fadeOut", &OgreAL::Sound::fadeOut)
			.def("cancelFade", &OgreAL::Sound::cancelFade)
			.def("setPitch", &OgreAL::Sound::setPitch)
			.def("getPitch", &OgreAL::Sound::getPitch)
			.def("setGain", &OgreAL::Sound::setGain)
			.def("getGain", &OgreAL::Sound::getGain)
			.def("setMaxGain", &OgreAL::Sound::setMaxGain)
			.def("getMaxGain", &OgreAL::Sound::getMaxGain)
			.def("setMinGain", &OgreAL::Sound::setMinGain)
			.def("getMinGain", &OgreAL::Sound::getMinGain)
			// .def("setGainScale", &OgreAL::Sound::setGainScale)
			// .def("getGainScale", &OgreAL::Sound::getGainScale)
			// .def("setGainValues", &OgreAL::Sound::setGainValues)
			.def("setMaxDistance", &OgreAL::Sound::setMaxDistance)
			.def("getMaxDistance", &OgreAL::Sound::getMaxDistance)
			.def("setRolloffFactor", &OgreAL::Sound::setRolloffFactor)
			.def("getRolloffFactor", &OgreAL::Sound::getRolloffFactor)
			.def("setReferenceDistance", &OgreAL::Sound::setReferenceDistance)
			.def("getReferenceDistance", &OgreAL::Sound::getReferenceDistance)
			.def("setDistanceValues", &OgreAL::Sound::setDistanceValues)
			.def("setVelocity", (void (OgreAL::Sound::*)(const Vector3&)) & OgreAL::Sound::setVelocity)
			.def("setVelocity", (void (OgreAL::Sound::*)(Real, Real, Real)) & OgreAL::Sound::setVelocity)
			.def("getVelocity", &OgreAL::Sound::getVelocity)
			.def("setRelativeToListener", &OgreAL::Sound::setRelativeToListener)
			.def("isRelativeToListener", &OgreAL::Sound::isRelativeToListener)
			.def("setPosition", (void (OgreAL::Sound::*)(const Vector3&)) & OgreAL::Sound::setPosition)
			.def("setPosition", (void (OgreAL::Sound::*)(Real, Real, Real)) & OgreAL::Sound::setPosition)
			.def("getPosition", &OgreAL::Sound::isRelativeToListener)
			.def("setDirection", (void (OgreAL::Sound::*)(const Vector3&)) & OgreAL::Sound::setDirection)
			.def("setDirection", (void (OgreAL::Sound::*)(Real, Real, Real)) & OgreAL::Sound::setDirection)
			.def("getDirection", &OgreAL::Sound::getDirection)
			.def("setOuterConeGain", &OgreAL::Sound::setOuterConeGain)
			.def("getOuterConeGain", &OgreAL::Sound::getOuterConeGain)
			.def("setInnerConeAngle", &OgreAL::Sound::setInnerConeAngle)
			.def("getInnerConeAngle", &OgreAL::Sound::getInnerConeAngle)
			.def("setOuterConeAngle", &OgreAL::Sound::setOuterConeAngle)
			.def("getOuterConeAngle", &OgreAL::Sound::getOuterConeAngle)
			.def("setLoop", &OgreAL::Sound::setLoop)
			.def("isLooping", &OgreAL::Sound::isLooping)
			.def("isStreaming", &OgreAL::Sound::isStreaming)
			.def("getSecondDuration", &OgreAL::Sound::getSecondDuration)
			.def("setSecondOffset", &OgreAL::Sound::setSecondOffset)
			.def("getSecondOffset", &OgreAL::Sound::getInnerConeAngle)
			.def("getInnerConeAngle", &OgreAL::Sound::getSecondOffset)
			.def("getDerivedPosition", &OgreAL::Sound::getDerivedPosition)
			.def("getDerivedDirection", &OgreAL::Sound::getDerivedDirection)
			.def("getFileName", &OgreAL::Sound::getFileName)
			// .def("setSpectrumSamplingRate", &OgreAL::Sound::setSpectrumSamplingRate)
		];

		AddClassToCollection("Sound", "class", "OgreAL 3D dolby surround sound.");
		// TODO: Lua api necessary?

		module(lua)
			[
			class_<OgreALModule>("OgreALModule")
			// .def("getInstance", &OgreALModule::getInstance)
			/*.def("createSound", &OgreALModule::createSound)
			.def("setSoundDopplerEffect", &OgreALModule::setSoundDopplerEffect)
			.def("setSoundSpeed", &OgreALModule::setSoundSpeed)
			.def("setSoundCullDistance", &OgreALModule::setSoundCullDistance)
			.def("getSoundVolume", &OgreALModule::getSoundVolume)
			.def("getMusicVolume", &OgreALModule::getMusicVolume)*/
			.def("setContinue", &OgreALModule::setContinue)
		];

		AddClassToCollection("OgreALModule", "class", "OgreALModule for some OgreAL utilities operations.");
		AddClassToCollection("OgreALModule", "void setContinue(bool bContinue)", "Sets whether the sound manager continues with the current scene manager. "
			"Note: This can be used when sounds are used in an AppState, but another AppState is pushed on the top, yet still the sounds should continue "
			"playing from the prior AppState. E.g. start music in MenuState and continue playing when pushing LoadMenuState and going back again to MenuState.");

		object globalVars = globals(lua);
		globalVars["OgreALModule"] = OgreALModule::getInstance();
	}

	void bindOgreNewtModule(lua_State* lua)
	{
		module(lua)
		[
			class_<OgreNewtModule>("OgreNewtModule")
			// .def("getInstance", &OgreNewtModule::getInstance)
			.def("showOgreNewtCollisionLines", &OgreNewtModule::showOgreNewtCollisionLines)
			// .def("getOgreNewt", &OgreNewtModule::getOgreNewt)
		];

		AddClassToCollection("OgreNewtModule", "class", "OgreNewtModule for some physics utilities operations.");
		AddClassToCollection("OgreNewtModule", "void showOgreNewtCollisionLines(bool show)", "Shows the debug physics collision lines.");

		// object globalVars = globals(lua);
		// globalVars["OgreNewtModule"] = AppStateManager::getSingletonPtr()->getOgreNewtModule();
	}

	void bindCameraManager(lua_State* lua)
	{
		module(lua)
		[
			class_<CameraManager>("CameraManager")
			.def("setMoveCameraWeight", &CameraManager::setMoveCameraWeight)
			.def("setRotateCameraWeight", &CameraManager::setRotateCameraWeight)
		];

		AddClassToCollection("CameraManager", "class", "CameraManager for some camera utilities operations.");
		AddClassToCollection("CameraManager", "void setMoveCameraWeight(float moveCameraWeight)", "Sets the move camera weight. Default value is 1. If set to 0, the current camera will not be moved.");
		AddClassToCollection("CameraManager", "void setRotateCameraWeight(float rotateCameraWeight)", "Sets the rotate camera weight. Default value is 1. If set to 0, the current camera will not be rotated.");
	}

	void bindOgreRecastModule(lua_State* lua)
	{
		// no singleton anymore, so how to get this module? Only be the current simulation state?
		module(lua)
		[
			class_<OgreRecastModule>("OgreRecastModule")
			// .def("addStaticObstacle", &OgreRecastModule::addStaticObstacle)
			// .def("removeStaticObstacle", &OgreRecastModule::removeStaticObstacle)
			// .def("addDynamicObstacle", &OgreRecastModule::addDynamicObstacle)
			// .def("removeDynamicObstacle", &OgreRecastModule::removeDynamicObstacle)
			// .def("hasNavigationMeshElements", &OgreRecastModule::hasNavigationMeshElements)
			.def("debugDrawNavMesh", &OgreRecastModule::debugDrawNavMesh)
		];

		AddClassToCollection("OgreRecastModule", "class", "OgreRecastModule for navigation mesh operations.");
		AddClassToCollection("OgreNewtModule", "void debugDrawNavMesh(bool draw)", "Draws debug navigation mesh lines.");
	}

	void bindPagedGeometryModule(lua_State* lua)
	{
		// not required
	}

	void bindParticleUniverseModule(lua_State* lua)
	{
		//module(lua)
		//[
		//	class_<ParticleUniverseModule>("ParticleUniverseModule")
		//	// .def("getInstance", &ParticleUniverseModule::getInstance)
		//	.def("createParticleSystem", &ParticleUniverseModule::createParticleSystem)
		//	.def("playParticleSystem", &ParticleUniverseModule::playParticleSystem)
		//	.def("playAndStopFadeParticleSystem", &ParticleUniverseModule::playAndStopFadeParticleSystem)
		//	.def("stopParticleSystem", &ParticleUniverseModule::stopParticleSystem)
		//	.def("pauseParticleSystem", &ParticleUniverseModule::pauseParticleSystem)
		//	.def("resumeParticleSystem", &ParticleUniverseModule::resumeParticleSystem)
		//	.def("removeParticle", &ParticleUniverseModule::removeParticle)
		//];
		//object globalVars = globals(lua);
		//globalVars["ParticleUniverseModule"] = AppStateManager::getSingletonPtr()->getParticleUniverseModule();

		//AddClassToCollection("ParticleUniverseModule", "class", "ParticleUniverseModule for particle effects.");
	}

	void bindRakNetModule(lua_State* lua)
	{
		// not required
	}

	void bindShaderModule(lua_State* lua)
	{
		// required?
	}

	void bindSSAOCompositorModule(lua_State* lua)
	{
		// not required
	}

	void bindSSAOModule(lua_State* lua)
	{
		// not required;
	}

	Ogre::Quaternion lookAt(MathHelper* instance, const Ogre::Vector3& unnormalizedDirection)
	{
		return instance->lookAt(unnormalizedDirection);
	}

	void bindMathHelper(lua_State* lua)
	{
		module(lua)
		[
			class_<MathHelper>("MathHelper")
			// .def("getInstance", &MathHelper::getInstance)
			.def("calibratedFromScreenSpaceToWorldSpace", &MathHelper::calibratedFromScreenSpaceToWorldSpace)
			.def("getCalibratedWindowCoordinates", &MathHelper::getCalibratedWindowCoordinates)
			// .def("getCalibratedOverlayCoordinates", &MathHelper::getCalibratedOverlayCoordinates)
			.def("lowPassFilter", (Ogre::Real(MathHelper::*)(Ogre::Real, Ogre::Real, Ogre::Real)) &MathHelper::lowPassFilter)
			.def("lowPassFilter", (Ogre::Vector3(MathHelper::*)(const Ogre::Vector3& , const Ogre::Vector3& , Ogre::Real)) &MathHelper::lowPassFilter)
			.def("round", (Ogre::Real(MathHelper::*)(Ogre::Real, unsigned int)) & MathHelper::round)
			.def("round", (Ogre::Real(MathHelper::*)(Ogre::Real)) & MathHelper::round)
			.def("round", (Ogre::Vector2(MathHelper::*)(Ogre::Vector2, unsigned int)) & MathHelper::round)
			.def("round", (Ogre::Vector3(MathHelper::*)(Ogre::Vector3, unsigned int)) & MathHelper::round)
			.def("round", (Ogre::Vector4(MathHelper::*)(Ogre::Vector4, unsigned int)) & MathHelper::round)
			.def("diffPitch", &MathHelper::diffPitch)
			.def("diffYaw", &MathHelper::diffYaw)
			.def("diffRoll", &MathHelper::diffRoll)
			.def("diffDegree", &MathHelper::diffDegree)
			.def("faceTarget", (Ogre::Quaternion(MathHelper::*)(Ogre::SceneNode*, Ogre::SceneNode*, const Ogre::Vector3&))& MathHelper::faceTarget)
			.def("faceTarget", (Ogre::Quaternion(MathHelper::*)(Ogre::SceneNode*, Ogre::SceneNode*)) & MathHelper::faceTarget)
			.def("lookAt", &lookAt)
			.def("faceDirectionSlerp", &MathHelper::faceDirectionSlerp)
			.def("getAngle", &MathHelper::getAngle)
			.def("normalizeDegreeAngle", &MathHelper::normalizeDegreeAngle)
			.def("normalizeRadianAngle", &MathHelper::normalizeRadianAngle)
			.def("degreeAngleEquals", &MathHelper::degreeAngleEquals)
			.def("radianAngleEquals", &MathHelper::radianAngleEquals)
			.def("numberEquals", &MathHelper::realEquals)
			.def("vector2Equals", &MathHelper::vector2Equals)
			.def("vector3Equals", &MathHelper::vector3Equals)
			.def("vector4Equals", &MathHelper::vector4Equals)
			.def("quaternionEquals", &MathHelper::quaternionEquals)
			.def("degreeAngleToRadian", &MathHelper::degreeAngleToRadian)
			.def("radianAngleToDegree", &MathHelper::radianAngleToDegree)
			.def("quatToDegreeAngleAxis", &MathHelper::quatToDegreeAngleAxis)
			.def("quatToDegrees", &MathHelper::quatToDegrees)
			.def("quatToDegreesRounded", &MathHelper::quatToDegreesRounded)
			.def("degreesToQuat", &MathHelper::degreesToQuat)
			// .def("getAngleForTargetLocation", &MathHelper::getAngleForTargetLocation) // Does not work, function maybe missing in cpp file
			.def("calculateRotationGridValue", (Ogre::Real(MathHelper::*)(Ogre::Real, Ogre::Real)) & MathHelper::calculateRotationGridValue)
			.def("calculateRotationGridValue", (Ogre::Real(MathHelper::*)(const Ogre::Quaternion&, Ogre::Real, Ogre::Real)) & MathHelper::calculateRotationGridValue)
			.def("getRandomNumberInt", &getRandomNumberInt)
			// .def("getRandomNumberFloat", &getRandomNumberFloat)
			.def("mapValue", &MathHelper::mapValue)
			.def("mapValue2", &MathHelper::mapValue2)
			.def("limit", &MathHelper::limit)
			.def("amplitudeToDecibels", &MathHelper::amplitudeToDecibels)
			.def("decibelsToAmplitude", &MathHelper::decibelsToAmplitude)
		];
		object globalVars = globals(lua);
		globalVars["MathHelper"] = MathHelper::getInstance();

		AddClassToCollection("MathHelper", "class", "MathHelper for utilities functions for mathematical operations and ray casting.");
		AddClassToCollection("MathHelper", "MathHelper", "Gets the singleton instance of the MathHelper.");
		AddClassToCollection("MathHelper", "Vector3 calibratedFromScreenSpaceToWorldSpace(SceneNode node, Camera camera, Vector2 mousePosition, float offset)", "Maps an 3D-object to the graphics scene from 2D-mouse coordinates.");
		AddClassToCollection("MathHelper", "Vector4 getCalibratedWindowCoordinates(float x, float y, float width, float height, Viewport viewport)", "Gets the relative position to the window size. For example mapping an crosshair overlay object and conrolling it via mouse or Wii controller..");
		AddClassToCollection("MathHelper", "float lowPassFilter(float value, float oldValue, float scale)", "A low pass filter in order to smooth chaotic values. This is for example useful for the position determination of infrared camera of the Wii controller or smooth movement at all.");
		AddClassToCollection("MathHelper", "Vector3 lowPassFilter(Vector3 value, Vector3 oldValue, float scale)", "A low pass filter for vector3 in order to smooth chaotic values. This is for example useful for the position determination of infrared camera of the Wii controller or smooth movement at all.");
		AddClassToCollection("MathHelper", "float round(float number, unsigned int places)", "Rounds a number according to the number of places. E.g. 0.5 will be 1.0 and 4.443 will be 4.44 when number of places is set to 2.");
		AddClassToCollection("MathHelper", "float round(float number)", "Rounds a number.");
		AddClassToCollection("MathHelper", "Vector2 round(Vector2 vec, unsigned int places)", "Rounds a vector according to the number of places.");
		AddClassToCollection("MathHelper", "Vector3 round(Vector3 vec, unsigned int places)", "Rounds a vector according to the number of places.");
		AddClassToCollection("MathHelper", "Vector4 round(Vector4 vec, unsigned int places)", "Rounds a vector according to the number of places.");
		AddClassToCollection("MathHelper", "Quaternion diffPitch(Quaternion dest, Quaternion src)", "Gets the difference pitch of two quaternions.");
		AddClassToCollection("MathHelper", "Quaternion diffYaw(Quaternion dest, Quaternion src)", "Gets the difference yaw of two quaternions.");
		AddClassToCollection("MathHelper", "Quaternion diffRoll(Quaternion dest, Quaternion src)", "Gets the difference roll of two quaternions.");
		AddClassToCollection("MathHelper", "Quaternion diffDegree(Quaternion dest, Quaternion, src, int mode)", "Gets the difference of two quaternions. The mode, 1: pitch quaternion, 2: yaw quaternion, 3: roll quaternion will be calculated.");
		AddClassToCollection("MathHelper", "Quaternion faceTarget(SceneNode source, SceneNode dest)", "Gets the orientation in order to face a target, this orientation can be set directly.");
		AddClassToCollection("MathHelper", "Quaternion faceTarget(SceneNode source, SceneNode dest, Vector3 defaultDirection)", "Gets the orientation in order to face a target, this orientation can be set directly, taking the default direction of the source game object into account.");
		AddClassToCollection("MathHelper", "Radian getAngle(Vector3 dir1, Vector3 dir2, Vector3 normal, bool signedAngle)", "Gets angle between two vectors. "
			"The normal of the two vectors is used in conjunction with the signed angle parameter, to determine whether the angle between the two vectors is negative or positive. "
			"This function can be use e.g. when using a gizmo or a grabber to rotate objects via mouse in the correct direction.");
		AddClassToCollection("MathHelper", "Quaternion lookAt(Vector3 unnormalizedDirection)", "Gets the orientation in order to look a the target direction with a fixed axis. Note: The fixed axis y is used so that the result quaternion will not pitch or roll.");
		AddClassToCollection("MathHelper", "Quaternion faceDirectionSlerp(Quaternion sourceOrientation, Vector3 direction, Vector3 defaultDirection, float dt, float rotationSpeed)", "Gets the orientation slerp steps in order to face towards a direction vector and also uses the default direction of the source.");
		
		AddClassToCollection("MathHelper", "float normalizeDegreeAngle(float degree)", "Normalizes the degree angle, e.g. a value bigger 180 would normally be set to -180, so after normalization even 181 degree are possible.");
		AddClassToCollection("MathHelper", "float normalizeRadianAngle(float radian)", "Normalizes the radian angle, e.g. a value bigger pi would normally be set to -pi, so after normalization even pi + 0.03 degree are possible.");
		AddClassToCollection("MathHelper", "bool degreeAngleEquals(float degree0, float degree1)", "Checks whether the given degree angles are equal. Internally the angles are normalized.");
		AddClassToCollection("MathHelper", "bool radianAngleEquals(float radian0, float radian1)", "Checks whether the given radian angles are equal. Internally the radian are normalized.");
		AddClassToCollection("MathHelper", "bool numberEquals(float first, Vector2 float, float tolerance)", "Compares two real objects, using tolerance for inaccuracies.");
		AddClassToCollection("MathHelper", "bool vector2Equals(Vector2 first, Vector2 second, float tolerance)", "Compares two vector2 objects, using tolerance for inaccuracies.");
		AddClassToCollection("MathHelper", "bool vector3Equals(Vector3 first, Vector3 second, float tolerance)", "Compares two vector3 objects, using tolerance for inaccuracies.");
		AddClassToCollection("MathHelper", "bool vector4Equals(Vector4 first, Vector4 second, float tolerance)", "Compares two vector4 objects, using tolerance for inaccuracies.");
		AddClassToCollection("MathHelper", "bool quaternionEquals(Quaternion first, Quaternion second, float tolerance)", "Compares two quaternion objects, using tolerance for inaccuracies.");
		AddClassToCollection("MathHelper", "float degreeAngleToRadian(float degree)", "Converts degree angle to radian.");
		AddClassToCollection("MathHelper", "float radianAngleToDegree(float radian)", "Converts radian angle to degree.");
		AddClassToCollection("MathHelper", "Vector4 quatToDegreeAngleAxis(Quaternion quat)", "Builds the orientation in the form of: degree, x-axis, y-axis, z-axis for better usability.");
		AddClassToCollection("MathHelper", "Vector3 quatToDegrees(Quaternion quat)", "Builds the orientation in the form of: degreeX, degreeY, degreeZ for better usability.");
		AddClassToCollection("MathHelper", "Vector3 quatToDegreesRounded(Quaternion quat)", "Builds the rounded orientation in the form of: degreeX, degreeY, degreeZ for better usability.");
		AddClassToCollection("MathHelper", "Quaternion degreesToQuat(Vector3 degrees)", "Builds the	quaternion from degreeX, degreeY, degreeZ.");
		AddClassToCollection("MathHelper", "float calculateRotationGridValue(float step, float angle)", "Calculates the rotation grid value.");
		AddClassToCollection("MathHelper", "float calculateRotationGridValue(Quaternion orientation, float step, float angle)", "Calculates the rotation grid value.");
		AddClassToCollection("MathHelper", "int getRandomNumberInt(int min, int max)", "Gets a random number between the given min and max.");
		AddClassToCollection("MathHelper", "long getRandomNumberLong(long min, long max)", "Gets a random number between the given min and max.");
	}

	void bindLuaScriptComponent(lua_State* lua)
	{
		module(lua)
		[
			class_<LuaScriptComponent, GameObjectComponent>("LuaScriptComponent")
			.def("callDelayedMethod", &LuaScriptComponent::callDelayedMethod)
		];

		AddClassToCollection("LuaScriptComponent", "class inherits GameObjectComponent", LuaScriptComponent::getStaticInfoText());
		AddClassToCollection("LuaScriptComponent", "void callDelayedMethod(func closureFunction, Ogre::Real delaySec)", "Calls a lua closure function in lua script after a delay in seconds. Note: The game object is optional and may be nil.");
		AddClassToCollection("LuaScriptComponent", "void connect(GameObject gameObject)", "Calls connect for the given game object.");
	}

	void bindLuaScriptEventManager(lua_State* lua)
	{
		module(lua)
		[
			class_<ScriptEventManager>("ScriptEventManager")
			// .def("getInstance", &ScriptEventManager::getInstance) //returns static singleton instance
			.def("registerEvent", &ScriptEventManager::registerEvent)
			.def("registerEventListener", &ScriptEventManager::registerEventListener)
			// .def("removeEventListener", &ScriptEventManager::removeEventListener)
			.def("queueEvent", (bool (ScriptEventManager::*)(EventType, luabind::object)) & ScriptEventManager::queueEvent)
			// .def("triggerEvent", (bool (ScriptEventManager::*)(EventType, luabind::object)) &ScriptEventManager::triggerEvent)
		];

		object globalVars = globals(lua);
		// globalVars["ScriptEventManager"] = AppStateManager::getSingletonPtr()->getScriptEventManager();
		// Create two empty tables as placeholder for in lua created events
		globalVars["Event"] = newtable(lua);
		globalVars["EventType"] = newtable(lua);

		AddClassToCollection("ScriptEventManager", "class", "ScriptEventManager can be used to define events in lua. Its possible to register an event and subscribe for that event in any other lua script file. Note: Its also possible to react in C++ on a lua event.");
		AddClassToCollection("ScriptEventManager", "void registerEvent(String eventName)", "Registers a new event name. Note: registered events must be known by all other lua scripts. Hence its a good idea to use the init.lua script for event registration. For example: 'AppStateManager:getScriptEventManager():registerEvent(""/DestroyStone""/);'");
		AddClassToCollection("ScriptEventManager", "void registerEventListener(EventType eventType, luabind::object callbackFunction)", "Registers an event listener, that wants to react on an event. When the events gets trigger somewhere, the callback function in lua is called. "
			"For example: 'ScriptEventManager:registerEventListener(EventType.DestroyStone, stone_felsite6_0[""/onDestroyStone""/]);'");
		AddClassToCollection("ScriptEventManager", "bool queueEvent(EventType eventType, luabind::object eventData)", "Queues an event, that is send out as soon as possible.");
		// AddClassToCollection("LuaScriptComponent", "bool triggerEvent(EventType eventType, luabind::object eventData)", "Triggers an event immediately.");
	}

	Ogre::String getCurrentDateAndTime(Core* instance)
	{
		// Use default format
		return instance->getCurrentDateAndTime();
	}

	void bindCore(lua_State* lua)
	{
		module(lua)
		[
			class_<Core>("Core")
			.def("getSingletonPtr", &Core::getSingletonPtr)
			.def("getCurrentWorldBoundLeftNear", &Core::getCurrentWorldBoundLeftNear)
			.def("getCurrentWorldBoundRightFar", &Core::getCurrentWorldBoundRightFar)
			.def("isGame", &Core::getIsGame)
			.def("getCurrentDateAndTime", &getCurrentDateAndTime)
			.def("getCurrentDateAndTime2", &Core::getCurrentDateAndTime)
			.def("setCurrentSaveGameName", &Core::setCurrentSaveGameName)
			.def("getCurrentSaveGameName", &Core::getCurrentSaveGameName)
			.def("getSaveFilePathName", &Core::getSaveFilePathName)
		];

		object globalVars = globals(lua);
		globalVars["Core"] = Core::getSingletonPtr();

		module(lua)
		[
			class_<InputDeviceCore>("InputDeviceCore")
			.def("getSingletonPtr", &InputDeviceCore::getSingletonPtr)
			.def("getKeyboard", &InputDeviceCore::getKeyboard)
			.def("getMouse", &InputDeviceCore::getMouse)
			// .def("gettJoystick1", &InputDeviceCore::getJoystick(0))
			// .def("gettJoystick2", &InputDeviceCore::getJoystick(1))
		];

		object globalVars2 = globals(lua);
		globalVars2["InputDeviceCore"] = InputDeviceCore::getSingletonPtr();

		AddClassToCollection("InputDeviceCore", "class", "Input device core functionality.");
		AddClassToCollection("InputDeviceCore", "KeyBoard getKeyboard()", "Gets the keyboard for direct manipulation.");
		AddClassToCollection("InputDeviceCore", "Mouse getMouse()", "Gets the mouse for direct manipulation.");

		AddClassToCollection("Core", "class", "Some functions for NOWA core functionality.");
		AddClassToCollection("InputDeviceCore", "KeyBoard getKeyboard()", "Gets the keyboard for direct manipulation.");
		AddClassToCollection("InputDeviceCore", "Mouse getMouse()", "Gets the mouse for direct manipulation.");
		AddClassToCollection("Core", "Vector3 getCurrentWorldBoundLeftNear()", "Gets left near bounding box of the currently loaded scene.");
		AddClassToCollection("Core", "Vector3 getCurrentWorldBoundRightFar()", "Gets right far bounding box of the currently loaded scene.");
		AddClassToCollection("Core", "bool getIsGame()", "Gets whether the engine is used in a game and not in an editor. Note: Can be used, e.g. if set to false (editor mode) to each time reset game save data etc.");
		AddClassToCollection("Core", "String getCurrentDateAndTime()", "Gets the current date and time. The default format is in the form Year_Month_Day_Hour_Minutes_Seconds.");
		AddClassToCollection("Core", "String getCurrentDateAndTime2(String format)", "Gets the current date and time. E.g. format = '%Y_%m_%d_%X' which formats the value as Year_Month_Day_Hour_Minutes_Seconds.");
		AddClassToCollection("Core", "void setCurrentSaveGameName(String saveGameName)", "Sets the current global save game name, which can be used for loading a game in an application state.");
		AddClassToCollection("Core", "String getCurrentSaveGameName()", "Gets the current save game name, or empty string, if does not exist.");
		AddClassToCollection("Core", "String getSaveFilePathName(String saveGameName)", "Gets the save file path name, or empty string, if does not exist. Note: This is usefull for debug purposes, to see, where a game does store its content and open those files for analysis etc.");
	}

	void bindAppStateManager(lua_State* lua)
	{
		module(lua)
		[
			class_<AppStateManager>("AppStateManager")
			.def("getSingletonPtr", &AppStateManager::getSingletonPtr)
			.def("setSlowMotion", &AppStateManager::setSlowMotion)
			.def("changeAppState", (void (AppStateManager::*)(const Ogre::String&)) & AppStateManager::changeAppState)
			.def("pushAppState", (bool (AppStateManager::*)(const Ogre::String&)) & AppStateManager::pushAppState)
			.def("popAppState", &AppStateManager::popAppState)
			.def("popAllAndPushAppState", (void (AppStateManager::*)(const Ogre::String&)) & AppStateManager::popAllAndPushAppState)
			.def("hasAppStateStarted", &AppStateManager::hasAppStateStarted)
			.def("exitGame", &AppStateManager::exitGame)
			// .def("getWorkspaceModule", &AppStateManager::getWorkspaceModule)
			.def("getCameraManager", (CameraManager * (AppStateManager::*)(void) const) & AppStateManager::getCameraManager)
			.def("getCameraManage2", (CameraManager * (AppStateManager::*)(const Ogre::String&)) & AppStateManager::getCameraManager)
			.def("getGameObjectController", (GameObjectController * (AppStateManager::*)(void) const) & AppStateManager::getGameObjectController)
			.def("getGameObjectController2", (GameObjectController * (AppStateManager::*)(const Ogre::String&)) & AppStateManager::getGameObjectController)
			.def("getGameProgressModule", (GameProgressModule * (AppStateManager::*)(void) const) & AppStateManager::getGameProgressModule)
			.def("getGameProgressModule2", (GameProgressModule * (AppStateManager::*)(const Ogre::String&)) & AppStateManager::getGameProgressModule)

			// .def("getMeshDecalGeneratorModule", &AppStateManager::getMeshDecalGeneratorModule)
			// .def("getMiniMapModule", &AppStateManager::getMiniMapModule)

			.def("getOgreNewtModule", (OgreNewtModule * (AppStateManager::*)(void) const) & AppStateManager::getOgreNewtModule)
			.def("getOgreNewtModule2", (OgreNewtModule * (AppStateManager::*)(const Ogre::String&)) & AppStateManager::getOgreNewtModule)

			// .def("getOgreRecastModule", &AppStateManager::getOgreRecastModule)
			// .def("getParticleUniverseModule", &AppStateManager::getParticleUniverseModule)
			// .def("getRakNetModule", &AppStateManager::getRakNetModule)

			.def("getScriptEventManager", (ScriptEventManager * (AppStateManager::*)(void) const) & AppStateManager::getScriptEventManager)
			.def("getScriptEventManager2", (ScriptEventManager * (AppStateManager::*)(const Ogre::String&)) & AppStateManager::getScriptEventManager)
		];

		object globalVars = globals(lua);
		globalVars["AppStateManager"] = AppStateManager::getSingletonPtr();

		AddClassToCollection("AppStateManager", "class", "Some functions for NOWA game loop and applictation state management. That is, normally only one scene can be running at a time. But putting a scene into an own state. "
			"Its possible to push another scene at the top of the current scene. Like pushing 'MenuState', so that the user can configure something and when he is finished, he pops the 'MenuState', so that the 'GameState' will continue.");
		AddClassToCollection("AppStateManager", "void setSlowMotion(bool slowMotion)", "Sets whether to use slow motion rendering.");
		AddClassToCollection("AppStateManager", "void changeAppState(String stateName)", "Changes this application state to the new given one. Details: Calls 'exit' on the prior state and 'enter' on the new state.");
		AddClassToCollection("AppStateManager", "bool pushAppState(String stateName)", "Pushes a new application state at the top of this one and calls 'enter'.");
		AddClassToCollection("AppStateManager", "bool popAppState()", "Pops the current state (calls 'exit'), and calls 'resume' on a prior state. If no state does exist anymore, the appliction will be shut down.");
		AddClassToCollection("AppStateManager", "void popAllAndPushAppState(String stateName)", "Pops all actives application states (internally calls 'exit' for all active states and pushes a new one and calls 'enter'.");
		AddClassToCollection("AppStateManager", "bool hasAppStateStarted(String stateName)", "Gets whether the given AppState has started. Note: This can be used to ask from another AppState like a MenuState, whether a GameState has already started, so that e.g. a continue button can be shown in the Menu.");
		AddClassToCollection("AppStateManager", "void exitGame()", "Exits the game and destroys all application states.");
		// AddClassToCollection("AppStateManager", "WorkspaceModule getWorkspaceModule()", "Gets the module for the current AppState.");
		AddClassToCollection("AppStateManager", "CameraManagerModule getCameraManager()", "Gets the module for the current AppState.");
		AddClassToCollection("AppStateManager", "CameraManagerModule getCameraManager2(String stateName)", "Gets the module for the given application state name, or null if the application state does not exist.");
		AddClassToCollection("AppStateManager", "GameObjectController getGameObjectController()", "Gets the module for the current AppState.");
		AddClassToCollection("AppStateManager", "GameObjectController getGameObjectController2(String stateName)", "Gets the module for the given application state name, or null if the application state does not exist.");
		AddClassToCollection("AppStateManager", "GameProgressModule getGameProgressModule()", "Gets the module for the current AppState.");
		AddClassToCollection("AppStateManager", "GameProgressModule getGameProgressModule2(String stateName)", "Gets the module for the given application state name, or null if the application state does not exist.");
		// AddClassToCollection("AppStateManager", "MeshDecalGeneratorModuleModule getMeshDecalGeneratorModule()", "Gets the module for the current AppState.");
		// AddClassToCollection("AppStateManager", "MinMapModule getMiniMapModule()", "Gets the module for the current AppState.");
		// AddClassToCollection("AppStateManager", "OgreALModule getOgreALModule()", "Gets the module for the current AppState.");
		AddClassToCollection("AppStateManager", "OgreNewtModule getOgreNewtModule()", "Gets the module for the current AppState.");
		AddClassToCollection("AppStateManager", "OgreNewtModule getOgreNewtModule2(String stateName)", "Gets the module for the given application state name, or null if the application state does not exist.");
		// AddClassToCollection("AppStateManager", "OgreRecastModule getOgreRecastModule()", "Gets the module for the current AppState.");
		// AddClassToCollection("AppStateManager", "ParticleUniverseModule getParticleUniverseModule()", "Gets the module for the current AppState.");
		// AddClassToCollection("AppStateManager", "RakNetModule getRakNetModule()", "Gets the module for the current AppState.");
		AddClassToCollection("AppStateManager", "ScriptEventManager getScriptEventManager()", "Gets the script event manager for the current AppState.");
		AddClassToCollection("AppStateManager", "ScriptEventManager getScriptEventManager2(String stateName)", "Gets the module for the given application state name, or null if the application state does not exist.");
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	LuaScriptApi::LuaScriptApi()
		: lua(0)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[LuaScriptApi] Module created");
	}

	LuaScriptApi::~LuaScriptApi()
	{
	}

	void LuaScriptApi::destroyContent(void)
	{
		if (nullptr != this->lua)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[LuaScriptApi] deleting lua content");
			lua_close(this->lua);
			this->lua = nullptr;
		}
	}

	void LuaScriptApi::clear(void)
	{
		errorMessages.clear();
		errorMessages2.clear();
	}

	LuaScriptApi* LuaScriptApi::getInstance()
	{
		static LuaScriptApi instance;

		return &instance;
	}

	int LuaScriptApi::callBackCustomErrorFunction(lua_State* lua)
	{
		lua_Debug d = {};
		std::stringstream msg;

		Ogre::String error = lua_tostring(lua, -1);

		// Try to get from which function the error has been called and for which game object
		if (false == currentErrorGameObject.empty())
		{
			msg << "[LuaScriptApi]: Error: Cannot get game object '" << currentErrorGameObject << "' from function '" << currentCalledFunction << "'. Details: " << error << "\n\nBacktrace:\n";
			currentErrorGameObject = "";
			currentCalledFunction = "";
		}
		else
		{
			msg << "[LuaScriptApi]: Error: " << error << "\n\nBacktrace:\n";
		}

		Ogre::String scriptName;
		Ogre::String toFind = "module(\"";
		size_t foundStart = error.find(toFind);
		if (foundStart != Ogre::String::npos)
		{
			size_t foundEnd = error.find("\"", foundStart + toFind.length());
			if (foundEnd != Ogre::String::npos)
			{
				scriptName = error.substr(foundStart + toFind.length(), foundEnd - (foundStart + toFind.length()));
			}
		}

		size_t foundUnderline = scriptName.rfind("_");
		if (foundUnderline != Ogre::String::npos)
		{
			scriptName = scriptName.substr(0, foundUnderline);
			scriptName += ".lua";
		}

		for (int stack_depth = 1; lua_getstack(lua, stack_depth, &d); ++stack_depth)
		{
			lua_getinfo(lua, "Sln", &d);
			msg << "#" << stack_depth << " ";
			if (d.name)
			{
				msg << "<" << d.namewhat << "> \"" << d.name << "\"";
			}
			else
			{
				msg << "--";
			}
			msg << " (called";
			if (d.currentline > 0)
			{
				msg << " at line " << d.currentline;
			}
			msg << " in ";
			if (d.linedefined > 0)
			{
				msg << "function block between line " << d.linedefined << ".." << d.lastlinedefined << " of ";
			}
			msg << d.short_src;

			msg << ")\n";

			if (d.currentline > -1)
			{
				// Flooding prevention
				auto& found = errorMessages.find(d.currentline);
				if (found == errorMessages.cend())
				{
					errorMessages.emplace(d.currentline);
					Ogre::LogManager::getSingletonPtr()->logMessage(msg.str(), Ogre::LML_CRITICAL);

					boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(scriptName, d.currentline, msg.str()));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);

					lua_pop(lua, 1);
					lua_pushstring(lua, msg.str().c_str());
				}
			}
		}

#if 0
		if (true == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsSimulating())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(msg.str(), Ogre::LML_CRITICAL);

			// Stop simulation at runtime error, else it could case ugly crashes, if there are critical lua errors, due to ill lua code
			// Not in use, causes more trouble than its worth
			boost::shared_ptr<EventDataStopSimulation> eventDataStopSimulation(new EventDataStopSimulation(error));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataStopSimulation);
		}
#endif

		return 1;
	}

	/* custom panic handler */
	int LuaScriptApi::customLuaAtPanic(lua_State* lua)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LuaScriptApi]: Lua panic has been called, which is a fatal error.");

		throw luabind::error(lua);

		return 1;
	}

	void LuaScriptApi::init(const Ogre::String& resourceGroupName)
	{
		this->resourceGroupName = resourceGroupName;
		// get the complate path
		if (this->resourceGroupName.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LuaScriptApi] Lua could not be initialized, because the group name is empty.");
			return;
		}

		//Create a lua state
		if (nullptr == this->lua)
		{
			this->lua = lua_open();

			lua_atpanic(this->lua, &LuaScriptApi::customLuaAtPanic);

			// Connect LuaBind to this lua state
			luabind::open(this->lua);
			luaL_openlibs(this->lua);

			luabind::set_pcall_callback(&LuaScriptApi::callBackCustomErrorFunction);

			try
			{
				luabind::call_function<void>(this->lua, "print", "*** (Lua)Finished(Lua) ***");

				// Bind components lua api (splitted binding)
				class_<GameObject> gameObjectClass("GameObject");
				class_<GameObjectController> gameObjectControllerClass("GameObjectController");

				// Bind NOWA objects

				// Ogre
				bindString(this->lua);
				bindMatrix3(this->lua);
				bindDegree(this->lua);
				bindRadian(this->lua);
				bindVector2(this->lua);
				bindVector3(this->lua);
				bindVector4(this->lua);
				bindQuaternion(this->lua);
				bindColorValue(this->lua);
				bindInput(this->lua);
				bindMyGUI(this->lua);
				bindMovableObject(this->lua);
				bindEntity(this->lua);
				bindSceneNode(this->lua);
				bindCamera(this->lua);
				bindSceneManager(this->lua);

				// main
				bindCore(this->lua);
				bindAppStateManager(this->lua);

				// GameObject and Components
				bindGameObject(this->lua, gameObjectClass);
				bindGameObjectComponent(this->lua);
				bindAiComponents(this->lua);
				bindAnimationComponent(this->lua);
				bindAttributesComponent(this->lua);
				bindAttributeEffectComponent(this->lua);
				bindDistributedComponent(this->lua);
				bindCameraBehaviorComponents(this->lua);
				bindCameraComponent(this->lua);
				bindCompositorEffects(this->lua);
				bindDatablockComponent(this->lua);
				bindGameObjectTitleComponent(this->lua);
				bindParticleUniverseComponent(this->lua);
				bindPlayerControllerComponents(this->lua);
				bindTagPointComponent(this->lua);
				bindTimeTriggerComponent(this->lua);
				bindTimeLineComponent(this->lua);
				bindMoveMathFunctionComponent(this->lua);
				bindTagChildNodeComponent(this->lua);
				bindNodeTrackComponent(this->lua);
				bindLineComponent(this->lua);
				bindBillboardComponent(this->lua);
				bindOgreNewt(this->lua);
				bindJointHandlers(this->lua);
				bindLightComponents(this->lua);
				bindFogComponent(this->lua);
				bindFadeComponent(this->lua);
				bindPhysicsComponent(this->lua);
				bindPhysicsActiveComponent(this->lua);
				bindPhysicsActiveCompoundComponent(this->lua);
				bindPhysicsActiveDestructableComponent(this->lua);
				bindPhysicsActiveKinematicComponent(this->lua);
				bindPhysicsArtifactComponent(this->lua);
				bindPhysicsCompoundConnectionComponent(this->lua);
				bindPhysicsExplosionComponent(this->lua);
				bindPhysicsPlayerControllerComponent(this->lua);
				bindPhysicsMaterialComponent(this->lua);
				bindPhysicsRagDollComponent(this->lua);
				bindPlaneComponent(this->lua);
				// bindPhysicsMathSliderComponent(this->lua);
				bindPhysicsTerrainComponent(this->lua);
				bindBuoyancyComponent(this->lua);
				bindTriggerComponent(this->lua);
				bindSimpleSoundComponent(this->lua);
				bindSoundComponent(this->lua);
				bindSpawnComponent(this->lua);
				bindVehicleComponent(this->lua);
				bindPhysicsVehicle(this->lua);
				bindPhysicsTire(this->lua);
				bindGameProgressModule(this->lua);
				bindGameObjectController(this->lua, gameObjectControllerClass);
				bindAiLuaComponent(this->lua);
				bindMeshDecalComponent(this->lua);
				bindMyGUIComponents(this->lua);
				// Modules
				bindCaelumModule(this->lua);
				bindDefferedShadingModule(this->lua);
				bindHydraxModule(this->lua);
				bindOgreALModule(this->lua);
				bindOgreNewtModule(this->lua);
				bindCameraManager(this->lua);
				bindOgreRecastModule(this->lua);
				bindPagedGeometryModule(this->lua);
				bindParticleUniverseModule(this->lua);
				bindRakNetModule(this->lua);
				bindShaderModule(this->lua);
				bindSSAOCompositorModule(this->lua);
				bindSSAOModule(this->lua);
				bindProcedural(this->lua);
				bindMathHelper(this->lua);
				bindLuaScriptComponent(this->lua);
				bindLuaScriptEventManager(this->lua);

				// Add dynamic component lua api registrations (if existing)
				for (auto componentInfo : GameObjectFactory::getInstance()->getComponentFactory()->getRegisteredNames())
				{
					GameObjectFactory::getInstance()->getComponentFactory()->createForLuaApi(NOWA::getIdFromName(componentInfo.first), this->lua, gameObjectClass, gameObjectControllerClass);
				}

				// Finish splitted registration
				module(this->lua)[gameObjectClass];
				module(this->lua)[gameObjectControllerClass];

				AddFunctionToCollection("void log(String message)", "Logs information.");
			}
			catch (const luabind::error& err)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScriptManager]: " + Ogre::String(err.what()), Ogre::LML_CRITICAL);
			}
		}
	}

	void LuaScriptApi::stackDump(void)
	{
		int i = lua_gettop(this->lua);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[LuaScriptApi]: ----------------Stack Dump----------------");
		while (i)
		{
			int type = lua_type(this->lua, i);
			switch (type)
			{
			case LUA_TSTRING:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, Ogre::StringConverter::toString(i) + ": '" + lua_tostring(this->lua, i) + "'");
				break;
			case LUA_TBOOLEAN:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, Ogre::StringConverter::toString(i) + ": '" + (lua_toboolean(this->lua, i) ? "true" : "false") + "'");
				break;
			case LUA_TNUMBER:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, Ogre::StringConverter::toString(i) + ": '" + Ogre::StringConverter::toString(lua_tonumber(this->lua, i)) + "'");
				break;
			default:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, Ogre::StringConverter::toString(i) + ": '" + lua_typename(this->lua, type) + "'");
				break;
			}
			i--;
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[LuaScriptApi]: --------------- Stack Dump Finished ---------------");
	}

	int LuaScriptApi::pushTraceBack(void)
	{
		lua_getfield(this->lua, LUA_GLOBALSINDEX, "debug");
		if (!lua_istable(this->lua, -1))
		{
			lua_pop(this->lua, 1);
			return 1;
		}

		lua_getfield(this->lua, -1, "traceback");
		if (!lua_isfunction(this->lua, -1))
		{
			lua_pop(this->lua, 2);
			return 1;
		}
		return 2;
	}

	int LuaScriptApi::runInitScript(const Ogre::String& scriptName)
	{
		int result = 0;
		if (this->lua)
		{
			Ogre::String projectPackagePath = Core::getSingletonPtr()->getAbsolutePath(Core::getSingletonPtr()->getCurrentProjectPath());
			LuaScriptApi::getInstance()->appendLuaFilePathToPackage(projectPackagePath);

			Ogre::String luaScriptFilePathName = projectPackagePath + "/" + scriptName;
			std::ifstream ifs(luaScriptFilePathName, std::ios::in);
			if (false == ifs.good())
			{
				return 0;
			}
			Ogre::String luaInFileContent = std::string{ std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{} };
			if (GetFileAttributes(luaScriptFilePathName.data()) & FileFlag)
			{
				luaInFileContent = Core::getSingletonPtr()->decode64(luaInFileContent, true);

				// Write the readable version, because other scripts require this script in readable form
				std::ofstream outFile(luaScriptFilePathName);
				if (true == outFile.good())
				{
					outFile << luaInFileContent;
					outFile.close();
				}
			}

			// load and run the file
			int result = 0;
			if (result = luaL_dostring(lua, luaInFileContent.data()))
			{
				this->errorHandler();
				return result;
			}
		}
		return result;
	}

	void LuaScriptApi::stopInitScript(const Ogre::String& scriptName)
	{
		if (this->lua)
		{
			Ogre::String projectPackagePath = Core::getSingletonPtr()->getAbsolutePath(Core::getSingletonPtr()->getCurrentProjectPath());
			LuaScriptApi::getInstance()->appendLuaFilePathToPackage(projectPackagePath);

			Ogre::String luaScriptFilePathName = projectPackagePath + "/" + scriptName;
			std::ifstream ifs(luaScriptFilePathName, std::ios::in);
			if (false == ifs.good())
			{
				return;
			}
			Ogre::String luaInFileContent = std::string{ std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{} };
			// If the script has the crypt file flags, encode again
			if (GetFileAttributes(luaScriptFilePathName.data()) & FileFlag)
			{
				luaInFileContent = Core::getSingletonPtr()->encode64(luaInFileContent, true);

				// Write the encoded version back
				std::ofstream outFile(luaScriptFilePathName);
				if (true == outFile.good())
				{
					outFile << luaInFileContent;
					outFile.close();
				}
			}
		}
	}

	void LuaScriptApi::errorHandler()
	{
		Ogre::String error = "[LuaScriptApi]: Got runtime error: \n";
		error += lua_tostring(this->lua, -1);
		lua_pop(this->lua, 1);
		// throw std::runtime_error("[LuaScriptApi] Error: '" + error + "'");
		Ogre::LogManager::getSingletonPtr()->logMessage(error, Ogre::LML_CRITICAL);

		if (true == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsSimulating())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(error, Ogre::LML_CRITICAL);

			// Stop simulation at runtime error, else it could case ugly crashes, if there are critical lua errors, due to ill lua code
			// Not in use, causes more trouble than its worth
			// boost::shared_ptr<EventDataStopSimulation> eventDataStopSimulation(new EventDataStopSimulation(error));
			// NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataStopSimulation);
		}

		/*std::stringstream ss;
		ss << "[LuaScriptApi]:";
		lua_Debug debugInfo;
		int level = -1;
		while (lua_getstack(this->lua, level, &debugInfo))
		{
			lua_getinfo(this->lua, "nSl", &debugInfo);
			fprintf(stderr, "  [%d] %s:%d -- %s [%s]\n",
				level, debugInfo.short_src, debugInfo.currentline,
				(debugInfo.name ? debugInfo.name : "<unknown>"), debugInfo.what);

			ss << "  [" << level << "] " << debugInfo.short_src << ":" << debugInfo.currentline
				<< " -- " << (debugInfo.name ? debugInfo.name : "<unknown>") << " [" << debugInfo.what << "]\n";

			Ogre::LogManager::getSingletonPtr()->logMessage(ss.str(), Ogre::LML_CRITICAL);
			++level;
		}*/

		//// lua_Debug debugInfo;
		//int r = lua_getstack(this->lua, -1, &debugInfo);
		//if (r == 0)
		//{
		//	Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScriptApi]: Requested stack depth exceeded current stack dept", Ogre::LML_CRITICAL);
		//}
		//// http://www.lua.org/manual/5.1/manual.html#lua_getinfo
		//lua_getinfo(this->lua, "nSl", &debugInfo);
		//const char * er = lua_tostring(this->lua, -1);
		//lua_pop(this->lua, 1);

		//std::string error = "Failure to convert error to string";
		//if (er)
		//{
		//	error = er;
		//}

		//// std::stringstream ss;
		//ss << "[LuaScriptApi]: Error " << debugInfo.event << " at (" << debugInfo.currentline << "): " << error << "\n" <<
		//	"\tIn a " << debugInfo.namewhat << " " << debugInfo.source << " function called " << debugInfo.name << ".\n" <<
		//	"\tIn " << debugInfo.short_src << " defined at " << debugInfo.linedefined << ".";

		//Ogre::LogManager::getSingletonPtr()->logMessage(ss.str(), Ogre::LML_CRITICAL);
	}

	void LuaScriptApi::errorHandler(luabind::error& err, Ogre::String file, Ogre::String function)
	{
		Ogre::String error = lua_tostring(err.state(), -1);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LuaScriptApi] Error in lua script: '" + file
			+ "' and function '" + function + "': " + error + " - " + err.what());
	}

	void LuaScriptApi::errorHandler(luabind::cast_failed& castError, Ogre::String file, Ogre::String function)
	{
		Ogre::String error = lua_tostring(castError.state(), -1);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LuaScriptApi] Cast error on return function in lua script: '" + file
			+ "' and function '" + function + "': " + castError.what());
	}

	void LuaScriptApi::printErrorMessage(luabind::error& error, const Ogre::String& function)
	{
		luabind::object errorMsg(luabind::from_stack(error.state(), -1));
		std::stringstream msg;
		msg << errorMsg;

		// Flooding prevention
		auto& found = errorMessages2.find(function);
		if (found == errorMessages2.cend())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScriptApi] Caught error in '" + function + "' Error: " + Ogre::String(error.what())
				+ " details: " + msg.str());
			errorMessages2.emplace(function);
		}
	}

	void LuaScriptApi::forceGarbageCollect()
	{
		lua_pushnil(this->lua);
		luaL_dostring(this->lua, "global = nil");
		lua_settop(this->lua, 0);
		lua_gc(this->lua, LUA_GCCOLLECT, 0);
	}

	bool LuaScriptApi::functionExist(lua_State* lua, const Ogre::String& name)
	{
		using namespace luabind;
		object g = globals(this->lua);
		object function = g[name];

		if (function)
		{
			if (type(function) == LUA_TFUNCTION)
			{
				return true;
			}
		}
		return false;
	}

	void LuaScriptApi::appendLuaFilePathToPackage(const Ogre::String& luaFilePath)
	{
		// Only append new path, if world path has changed
		auto found = this->luaScriptPathes.find(luaFilePath);

		if (this->luaScriptPathes.cend() == found)
		{
			// Ogre::String packagePath = "package.path = package.path .. ';" + luaFilePath + "/?.lua";
			// luaL_dostring(lua, worldPackagePath.c_str());

			lua_getglobal(lua, "package");
			lua_getfield(lua, -1, "path"); // get field "path" from table at top of stack (-1)
			std::string currentPath = lua_tostring(lua, -1); // grab path string from top of stack
			currentPath.append(";"); // do your path magic here
			currentPath.append(luaFilePath);
			currentPath.append("/?.lua");
			lua_pop(lua, 1); // get rid of the string on the stack we just pushed on line 5
			lua_pushstring(lua, currentPath.c_str()); // push the new one
			lua_setfield(lua, -2, "path"); // set the field "path" in table at -2 with value at top of stack
			lua_pop(lua, 1); // get rid of package table from top of stack

			this->luaScriptPathes.emplace(luaFilePath);
		}
	}

	Ogre::String LuaScriptApi::getLuaInitScriptContent(void) const
	{
		return	"-- A better random seed. This was taken from http://lua-users.org/wiki/MathLibraryTutorial"
			"math.randomseed(tonumber(tostring(os.time()):reverse():sub(1,6)))\n\n"
			"function dump(o)\n"
			"\tif type(o) == 'table' then\n"
			"\t\tlocal s = '{ '"
			"\t\tfor k,v in pairs(o) do\n"
			"\t\t\tif type(k) ~= 'number' then k = '\"'..k..'\"' end\n"
			"\t\t\ts = s .. '['..k..'] = ' .. dump(v) .. ','\n"
			"\t\tend\n"
			"\t\treturn s .. '} '\n"
			"\telse\n"
			"\t\treturn tostring(o)\n"
			"\tend\n"
			"end\n\n"
			"-- Register your events here:\n"
			"-- Example: AppStateManager:getScriptEventManager():registerEvent(\"DestroyStone\");\n";
	}

	void LuaScriptApi::addClassToCollection(const Ogre::String& className, const Ogre::String& function, const Ogre::String& description)
	{
		auto& it = classCollection.find(className);
		if (it == classCollection.cend())
		{
			std::vector<std::pair<Ogre::String, Ogre::String>> details;
			details.emplace_back(function, description);
			classCollection.emplace(className, details);
		}
		else
		{
			bool functionAlreadyFound = false;
			for (auto subIt = it->second.cbegin(); subIt != it->second.cend(); ++subIt)
			{
				if (function == subIt->first)
				{
					functionAlreadyFound = true;
					break;
				}
			}

			if (false == functionAlreadyFound)
			{
				it->second.emplace_back(function, description);
			}
		}
	}

	void LuaScriptApi::addFunctionToCollection(const Ogre::String& function, const Ogre::String& description)
	{
		auto& it = classCollection.find("Z-Global");
		if (it == classCollection.cend())
		{
			std::vector<std::pair<Ogre::String, Ogre::String>> details;
			details.emplace_back(function, description);
			classCollection.emplace("Z-Global", details);
		}
		else
		{
			bool functionAlreadyFound = false;
			for (auto subIt = it->second.cbegin(); subIt != it->second.cend(); ++subIt)
			{
				if (function == subIt->first)
				{
					functionAlreadyFound = true;
					break;
				}
			}

			if (false == functionAlreadyFound)
			{
				it->second.emplace_back(function, description);
			}
		}
	}

	const std::map<Ogre::String, std::vector<std::pair<Ogre::String, Ogre::String>>>& LuaScriptApi::getClassCollection(void) const
	{
		return this->classCollection;
	}

	void LuaScriptApi::generateLuaApi(void)
	{
		// AddClassToCollection("CompositorEffectOldTvComponent", "void setActivated(bool activated)", "Activates the given compositor effect.");
		// AddClassToCollection("CompositorEffectOldTvComponent", "bool isActivated()", "Gets whether the compositor effect is activated or not.");

		// TODO: What about inner classes: AddClassToCollection("PhysicsRagDollComponent.RagBone", "class", "The inner class RagBone represents one physically controlled bone.");

		Ogre::String luaJsonApiContent;
		// std::map<Ogre::String, std::vector<std::pair<Ogre::String, Ogre::String>>> classCollection;
		// class -> function, description
		for (auto it = classCollection.cbegin(); it != classCollection.cend(); ++it)
		{
			// Special treating for global functions
			if (it->first == "Z-Global")
			{
				for (auto subIt = it->second.cbegin(); subIt != it->second.cend(); ++subIt)
				{
					Ogre::String identifier;
					Ogre::String functionContent = replaceAll(subIt->first, "unsigned int ", "number ");
					functionContent = replaceAll(functionContent, "int ", "number ");
					functionContent = replaceAll(functionContent, "short ", "number ");
					functionContent = replaceAll(functionContent, "unsigned short ", "number ");
					functionContent = replaceAll(functionContent, "long ", "number ");
					functionContent = replaceAll(functionContent, "float ", "number ");
					functionContent = replaceAll(functionContent, "bool ", "boolean ");
					functionContent = replaceAll(functionContent, "String ", "string ");
					functionContent = replaceAll(functionContent, "void ", "nil ");
					functionContent = replaceAll(functionContent, "null ", "nil ");

					// void log(String message) --> log
					{
						size_t found = functionContent.find(" ");
						if (Ogre::String::npos != found)
						{
							size_t found2 = functionContent.find("(", found);
							if (Ogre::String::npos != found2)
							{
								identifier = functionContent.substr(found + 1, found2 - found - 1);
							}
						}
					}

					luaJsonApiContent += "\t" + identifier + " =\n";
					luaJsonApiContent += "\t{\n";

					// returns first, because after that its possible to determine the type, whether function or method
					Ogre::String returnsPart = "nil";
					{
						size_t found = functionContent.find(" ");
						if (Ogre::String::npos != found)
						{
							returnsPart = functionContent.substr(0, found);
						}
					}

					if ("nil" == returnsPart)
						luaJsonApiContent += "\t\ttype = \"method\",\n";
					else
						luaJsonApiContent += "\t\ttype = \"function\",\n";

					Ogre::String description = replaceAll(subIt->second, "\n", " ");
					luaJsonApiContent += "\t\tdescription = \"" + description + "\",\n";
					// Args
					Ogre::String argsPart = "";
					{
						size_t found = functionContent.find("(");
						if (Ogre::String::npos != found)
						{
							size_t found2 = functionContent.find(")", found);
							if (Ogre::String::npos != found2)
							{
								argsPart = functionContent.substr(found + 1, found2 - found - 1);
							}
						}
					}

					bool isLast = std::next(subIt) == it->second.cend();

					luaJsonApiContent += "\t\targs = \"(" + argsPart + ")\",\n";
					luaJsonApiContent += "\t\treturns = \"(" + returnsPart + ")\",\n";
					luaJsonApiContent += "\t\tvaluetype = \"" + returnsPart + "\"\n";
					if (false == isLast)
						luaJsonApiContent += "\t\t},\n";
					else
					{
						luaJsonApiContent += "\t}\n";
					}
				}
			}
			else
			{
				// class =
				luaJsonApiContent += "\t" + it->first + " =\n";
				luaJsonApiContent += "\t{\n";
				luaJsonApiContent += "\t\ttype = \"class\",\n";

				Ogre::String baseClass;

				size_t foundClass = it->second[0].first.find("class");
				if (false == it->second.empty() && Ogre::String::npos != foundClass)
				{
					// Check if its an class with 'inherits' identifier
					Ogre::String toFind = "class inherits ";
					size_t foundBaseClass = it->second[0].first.find("class inherits ");
					if (Ogre::String::npos != foundBaseClass)
					{
						baseClass = it->second[0].first.substr(toFind.length(), it->second[0].first.length() - toFind.length());
					}

					// AddClassToCollection("DatablockTerraComponent", "class", "This class is responsible for customizing PBS (physically based shading) settings for Ogre terra (terrain).");
					Ogre::String description = replaceAll(it->second[0].second, "\n", " ");
					luaJsonApiContent += "\t\tdescription = \"" + description + "\"";
				}
				else
				{
					Ogre::String description = replaceAll(it->first, "\n", " ");
					luaJsonApiContent += "\t\tdescription = \"" + description + " class\"";
				}

				if (false == baseClass.empty())
				{
					luaJsonApiContent += ",\n\t\tinherits = \"" + baseClass + "\"";
				}

				// Class without children?
				{
					bool isLast = std::next(it->second.cbegin()) == it->second.cend();
					if (false == isLast)
					{
						luaJsonApiContent += ",\n";
					}
					else
					{
						luaJsonApiContent += "\n\t}";
						bool isLast = std::next(it) == classCollection.cend();
						if (false == isLast)
						{
							luaJsonApiContent += ",\n";
						}
						continue;
					}
				}

				luaJsonApiContent += "\t\tchilds = \n";
				luaJsonApiContent += "\t\t{\n";

				// Go through children off class
				for (auto subIt = it->second.cbegin(); subIt != it->second.cend(); ++subIt)
				{
					Ogre::String identifier = "class";

					// class inherits 
					bool isEnum = false;

					Ogre::String functionContent = replaceAll(subIt->first, "unsigned int ", "number ");
					functionContent = replaceAll(functionContent, "int ", "number ");
					functionContent = replaceAll(functionContent, "short ", "number ");
					functionContent = replaceAll(functionContent, "unsigned short ", "number ");
					functionContent = replaceAll(functionContent, "long ", "number ");
					functionContent = replaceAll(functionContent, "float ", "number ");
					functionContent = replaceAll(functionContent, "bool ", "boolean ");
					functionContent = replaceAll(functionContent, "String ", "string ");
					functionContent = replaceAll(functionContent, "void ", "nil ");
					functionContent = replaceAll(functionContent, "null ", "nil ");

					// void setActivated(bool activated) --> setActivated
					{
						size_t found = functionContent.find(" ");
						if (Ogre::String::npos != found)
						{
							size_t found2 = functionContent.find("(", found);
							if (Ogre::String::npos != found2)
							{
								identifier = functionContent.substr(found + 1, found2 - found - 1);
							}
						}
						else
						{
							// Enum value
							isEnum = true;
							identifier = functionContent;
						}
					}

					if ("class" == identifier || "clone" == identifier)
						continue;

					luaJsonApiContent += "\t\t\t" + identifier + " =\n";
					luaJsonApiContent += "\t\t\t{\n";

					// read_write constant
					// AddClassToCollection("Quaternion", "x", "type"); --> x = {type = "value"},
					if ("type" == subIt->second)
					{
						bool isLast = std::next(subIt) == it->second.cend();
						luaJsonApiContent += "\t\t\t\ttype = \"value\"\n\t\t\t}";
						if (false == isLast)
						{
							luaJsonApiContent += ",\n";
						}
						else
						{
							luaJsonApiContent += "\n\t},";
						}
					}
					else
					{
						// returns first, because after that its possible to determine the type, whether function or method
						Ogre::String returnsPart = "nil";
						{
							size_t found = functionContent.find(" ");
							if (Ogre::String::npos != found)
							{
								returnsPart = functionContent.substr(0, found);
							}
						}

						if (true == isEnum)
						{
							luaJsonApiContent += "\t\t\t\ttype = \"value\"\n\t\t\t}";
							bool isLast = std::next(subIt) == it->second.cend();
							if (false == isLast)
							{
								luaJsonApiContent += ",\n";
							}
							else
							{
								luaJsonApiContent += "\n\t\t}\n";
							}
							continue;
						}
						else if ("nil" == returnsPart)
							luaJsonApiContent += "\t\t\t\ttype = \"method\",\n";
						else
							luaJsonApiContent += "\t\t\t\ttype = \"function\",\n";

						// AddClassToCollection("NodeTrackComponent", "void setNodeTrackId(unsigned int index, String id)", "Sets the node track id for the given index in the node track list with @nodeTrackCount elements. Note: The order is controlled by the index, from which node to which node this game object will be tracked.");
						Ogre::String description = replaceAll(subIt->second, "\n", " ");
						luaJsonApiContent += "\t\t\t\tdescription = \"" + description + "\",\n";
						// Args
						Ogre::String argsPart = "";
						{
							size_t found = functionContent.find("(");
							if (Ogre::String::npos != found)
							{
								size_t found2 = functionContent.find(")", found);
								if (Ogre::String::npos != found2)
								{
									argsPart = functionContent.substr(found + 1, found2 - found - 1);
								}
							}
						}

						bool isLast = std::next(subIt) == it->second.cend();

						luaJsonApiContent += "\t\t\t\targs = \"(" + argsPart + ")\",\n";
						luaJsonApiContent += "\t\t\t\treturns = \"(" + returnsPart + ")\",\n";
						luaJsonApiContent += "\t\t\t\tvaluetype = \"" + returnsPart + "\"\n";
						if (false == isLast)
							luaJsonApiContent += "\t\t\t},\n";
						else
						{
							luaJsonApiContent += "\t\t\t}\n";
							luaJsonApiContent += "\t\t}\n";
						}
					}
				}

				bool isLast = std::next(it) == classCollection.cend();
				if (false == isLast)
				{
					luaJsonApiContent += "\t},\n";
				}
				else
				{
					luaJsonApiContent += "\t},";
				}
			}
		}

		if (false == classCollection.empty() && luaJsonApiContent.back() == ',')
			luaJsonApiContent.pop_back();

		// Set return value
		luaJsonApiContent = "return {\n\n" + luaJsonApiContent + "\n}";

		{
			std::ofstream outFile("NOWA_Api.lua");
			if (true == outFile.good())
			{
				outFile << luaJsonApiContent;
				outFile.close();
			}
		}
		// Try to copy to zero brane studio installation set path
		{
			Ogre::String targetPath = Core::getSingletonPtr()->getOptionLuaApiFilePath() + "/NOWA_Api.lua";
			std::ofstream outFile(targetPath);

			if (true == outFile.good())
			{
				outFile << luaJsonApiContent;
				outFile.close();
			}
			else
			{
				Ogre::String sourceFilePathName = Core::getSingletonPtr()->getAbsolutePath("NOWA_Api.lua");
				BOOL success = CopyFile(sourceFilePathName.data(), targetPath.data(), FALSE);
				if (false == success)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[LuaScriptApi] Could not create or copy 'NOWA_Api.lua' file to target folder, because its not writable. Please manually copy the file from: '" + sourceFilePathName + "' to '" + targetPath + "'.");
				}
			}
		}

		// Note:
		// 1) Copy the NOWA_Api.lua into the folder: C:\Program Files (x86)\ZeroBraneStudio\api\lua
		// 2) Restart Zerobrane Studio
		// 3) Open Menu -> Edit -> Preferences -> Settings: User
		// 4) Add: api = {'NOWA_Api'}

		// Note: Prevent naming collision: like having 2x the class 'Path', this will cause serious issues and will look the followin way in json:
		/*
			getWayPoints =
			{
				type = "function",
				description = "Gets the list of all waypoints.",
				args = "()",
				returns = "(table[Vector3])",
				valuetype = "table[Vector3]"
			},
		},
		--> Even valuetype = "table[Vector3]" is the last element, '},' is used, because actualle in the list, there is another class Path!
		*/
	}

	lua_State* LuaScriptApi::getLua(void)
	{
		return this->lua;
	}

}; // namespace end