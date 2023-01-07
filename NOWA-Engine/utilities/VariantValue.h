#ifndef VARIANT_VALUE_H
#define VARIANT_VALUE_H

/*
* Based on: OgreKit.
*  http://gamekit.googlecode.com/
*
*   Copyright (c) 2006-2010 Charlie C.
*/

#include "defines.h"
#include "OgreString.h"
#include "OgreStringConverter.h"
#include "OgreVector3.h"
#include <typeinfo>

namespace NOWA
{

	inline void _fromString(const Ogre::String& s, bool& v)
	{
		v = Ogre::StringConverter::parseBool(s);
	}

	inline void _fromString(const Ogre::String& s, int& v)
	{
		v = Ogre::StringConverter::parseInt(s);
	}

	inline void _fromString(const Ogre::String& s, unsigned int& v)
	{
		v = Ogre::StringConverter::parseUnsignedInt(s);
	}

	inline void _fromString(const Ogre::String& s, unsigned long& v)
	{
		v = Ogre::StringConverter::parseUnsignedLong(s);
	}

	inline void _fromString(const Ogre::String& s, Ogre::Real& v)
	{
		v = Ogre::StringConverter::parseReal(s);
	}

	inline void _fromString(const Ogre::String& s, Ogre::String& v)
	{
		v = s;
	}

	inline void _fromString(const Ogre::String& s, Ogre::Vector2& v)
	{
		v = Ogre::StringConverter::parseVector2(s);
	}

	inline void _fromString(const Ogre::String& s, Ogre::Vector3& v)
	{
		v = Ogre::StringConverter::parseVector3(s);
	}

	inline void _fromString(const Ogre::String& s, Ogre::Vector4& v)
	{
		v = Ogre::StringConverter::parseVector4(s);
	}

	inline void _fromString(const Ogre::String& s, Ogre::Quaternion& v)
	{
		v = Ogre::StringConverter::parseQuaternion(s);
	}

	inline void _fromString(const Ogre::String& s, Ogre::Matrix3& v)
	{
		v = Ogre::StringConverter::parseMatrix3(s);
	}

	inline void _fromString(const Ogre::String& s, char* v)
	{
		v = const_cast<char*>(s.c_str());
	}

	inline void _fromString(const Ogre::String& s, Ogre::Matrix4& v)
	{
		v = Ogre::StringConverter::parseMatrix4(s);
	}

	inline void _fromString(const Ogre::String& s, std::vector<Ogre::String>& v)
	{
		Ogre::String str;
		size_t i = 0;
		size_t nextFound = s.find(" ", i);
		while (Ogre::String::npos != nextFound)
		{
			v.emplace_back(s.substr(i, nextFound - i));
			i = nextFound + 1;
			nextFound = s.find(" ", i);
		}
	}

	inline Ogre::String _toString(bool v)
	{
		return v ? "true" : "false";
	}

	inline Ogre::String _toString(int v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(unsigned int v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(unsigned long v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(Ogre::Real v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(const Ogre::String& v)
	{
		return v;
	}

	inline Ogre::String _toString(const Ogre::Vector2& v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(const Ogre::Vector3& v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(const Ogre::Vector4& v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(const Ogre::Quaternion& v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(const Ogre::Matrix3& v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(const Ogre::Matrix4& v)
	{
		return Ogre::StringConverter::toString(v);
	}

	inline Ogre::String _toString(const std::vector<Ogre::String>& v)
	{
		Ogre::String value;
		for (unsigned int i = 0; i < static_cast<unsigned int>(v.size()); i++)
		{
			value += v[i];
			if (i < static_cast<unsigned int>(v.size()))
			{
				value += " ";
			}
		}
		return value;
	}
	
	class Value
	{
	public:
		typedef std::type_info Info;
	protected:

		class InnerValue
		{
		public:
			virtual ~InnerValue()
			{
			}

			virtual void fromString(const Ogre::String& value) = 0;
			virtual Ogre::String toString(void) const = 0;
			virtual const Info& type(void) const = 0;
			virtual InnerValue* clone(void) const = 0;
		};

		template <typename T>
		class ValueType : public InnerValue
		{
		public:

			ValueType(const T& value)
				: value(value)
			{
			}

			virtual ~ValueType()
			{
			}

			inline const Info& type(void) const override
			{
				return typeid(T);
			}
			inline InnerValue* clone(void) const override
			{
				return new ValueType<T>(this->value);
			}

			inline void fromString(const Ogre::String& v) override
			{
				_fromString(v, this->value);
			}

			inline void fromString(const char* v)
			{
				_fromString(v, this->value);
			}

			inline Ogre::String toString(void) const override
			{
				return _toString(this->value);
			}
		public:
			T value;
		};

		InnerValue* data;

		inline Value& swap(Value& rhs)
		{
			std::swap(this->data, rhs.data);
			return *this;
		}

	public:

		Value()
			: data(nullptr)
		{
		}

		Value(const Value& value)
			: data(value.data ? value.data->clone() : 0)
		{
		}

		template<typename T>
		Value(const T& value)
			: data(new ValueType<T>(value))
		{
		}

		~Value()
		{
			delete this->data;
		}

		template<typename T>
		inline Value& operator = (const T& rhs)
		{
			Value(rhs).swap(*this);
			return *this;
		}

		inline Value& operator = (const Value& rhs)
		{
			Value(rhs).swap(*this);
			return *this;
		}

		template<typename T>
		inline bool operator == (const T& rhs)
		{
			return *this == rhs;
		}

		inline bool operator == (const Value& rhs)
		{
			return *this == rhs;
		}

		template<typename T>
		inline operator T(void) const
		{
			if (this->data && this->data->type() == typeid(T))
			{
				return static_cast<ValueType<T>*>(this->data)->value;
			}
			return T();
		}

		template<typename T>
		inline T get(const T& def = T()) const
		{
			if (this->data && this->data->type() == typeid(T))
			{
				return static_cast<ValueType<T>*>(this->data)->value;
			}
			return def;
		}

		inline Ogre::String toString(void) const
		{
			if (this->data)
			{
				return this->data->toString();
			}
			return "";
		}

		inline void fromString(const Ogre::String& value) const
		{
			if (this->data)
			{
				this->data->fromString(value);
			}
		}

		inline bool isTypeOf(const Value& value) const
		{
			return this->data && value.data && this->data->type() == value.data->type();
		}

		inline bool isTypeOf(const Info& info) const
		{
			return this->data && this->data->type() == info;
		}
	};

}; //namespace end

#endif