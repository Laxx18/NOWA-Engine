#ifndef VARIANT_H
#define VARIANT_H

/*
* Based on: OgreKit.
*  http://gamekit.googlecode.com/
*
*   Copyright (c) 2006-2010 Charlie C.
*/

#include <Utilities/VariantValue.h>

namespace
{
	bool realEquals(Ogre::Real first, Ogre::Real second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon())
	{
		return Ogre::Math::RealEqual(first, second, tolerance);
	}

	bool vector2Equals(const Ogre::Vector2& first, const Ogre::Vector2& second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon())
	{
		return Ogre::Math::RealEqual(first.x, second.x, tolerance) && Ogre::Math::RealEqual(first.y, second.y, tolerance);
	}

	bool vector3Equals(const Ogre::Vector3& first, const Ogre::Vector3& second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon())
	{
		return Ogre::Math::RealEqual(first.x, second.x, tolerance) && Ogre::Math::RealEqual(first.y, second.y, tolerance)
			&& Ogre::Math::RealEqual(first.z, second.z, tolerance);
	}

	bool vector4Equals(const Ogre::Vector4& first, const Ogre::Vector4& second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon())
	{
		return Ogre::Math::RealEqual(first.x, second.x, tolerance) && Ogre::Math::RealEqual(first.y, second.y, tolerance)
			&& Ogre::Math::RealEqual(first.z, second.z, tolerance) && Ogre::Math::RealEqual(first.w, second.w, tolerance);
	}

	bool quaternionEquals(const Ogre::Quaternion& first, const Ogre::Quaternion& second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon())
	{
		return Ogre::Math::RealEqual(first.x, second.x, tolerance) && Ogre::Math::RealEqual(first.y, second.y, tolerance)
			&& Ogre::Math::RealEqual(first.z, second.z, tolerance) && Ogre::Math::RealEqual(first.w, second.w, tolerance);
	}
}

namespace NOWA
{
	class EXPORTED Variant
	{
	public:
		typedef enum Types
		{
			VAR_NULL = 0,
			VAR_BOOL,
			VAR_REAL,
			VAR_INT,
			VAR_UINT,
			VAR_ULONG,
			VAR_VEC2,
			VAR_VEC3,
			VAR_VEC4,
			VAR_QUAT,
			VAR_MAT3,
			VAR_MAT4,
			VAR_STRING,
			VAR_LIST,
		} Types;

		Variant()
			: value((int)0),
			type(VAR_NULL),
			lock(false),
			visible(true),
			isId(false),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
		}
		
		Variant(const Ogre::String& name)
			: value((int)0),
			type(VAR_NULL),
			name(name),
			lock(false),
			visible(true),
			isId(false),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
		}

		Variant(const Ogre::String& name, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value((int)0),
			type(VAR_NULL),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, bool value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_BOOL),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, int value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_INT),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, unsigned int value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_UINT),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, unsigned long value, std::vector<std::pair<Ogre::String, Variant*>>& attributes, bool isId = false)
			: value(value),
			type(VAR_ULONG),
			name(name),
			isId(isId),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, Ogre::Real value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_REAL),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, const Ogre::Vector2& value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_VEC2),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, const Ogre::Vector3& value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_VEC3),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, const Ogre::Vector4& value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_VEC4),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, const Ogre::Quaternion& value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_QUAT),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, const Ogre::Matrix3& value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_MAT3),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, const Ogre::Matrix4& value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_MAT4),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, const Ogre::String& value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(value),
			type(VAR_STRING),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, const char* value, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(Ogre::String(value)),
			type(VAR_STRING),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			attributes.emplace_back(this->name, this);
		}

		Variant(const Ogre::String& name, const std::vector<Ogre::String>& list, std::vector<std::pair<Ogre::String, Variant*>>& attributes)
			: value(list),
			type(VAR_LIST),
			name(name),
			isId(false),
			lock(false),
			visible(true),
			changed(false),
			minValue(std::numeric_limits<Ogre::Real>::min()),
			maxValue(std::numeric_limits<Ogre::Real>::max())
		{
			if (list.size() > 0)
			{
				this->selectedListValue = list[0];
			}
			attributes.emplace_back(this->name, this);
		}

		~Variant()
		{
		}

		inline int getType(void) const
		{
			return this->type;
		}

		inline void setReadOnly(bool readOnly)
		{
			this->lock = readOnly;
		}

		inline bool isReadOnly(void)
		{
			return this->lock;
		}

		inline void setVisible(bool visible)
		{
			this->visible = visible;
		}

		inline bool isVisible(void)
		{
			return this->visible;
		}

		inline const Ogre::String& getName(void) const
		{
			return this->name;
		}

		void setDescription(const Ogre::String& description)
		{
			this->description = description;
		}

		inline const Ogre::String& getDescription(void) const
		{
			return this->description;
		}

		Variant* clone(void)
		{
			Variant* variant = new Variant(*this);
			variant->value = Value(this->value);
			variant->setDescription(this->description);
			variant->setReadOnly(this->lock);
			variant->setVisible(this->visible);

			for (size_t i = 0; i < this->userDataList.size(); i++)
			{
				auto userData = this->userDataList[i];
				variant->addUserData(userData.first, userData.second);
			}
			return variant;
		}

		void setValue(int type, const Ogre::String& value)
		{
			if (!this->lock)
			{
				this->type = type;
				assign(value);
			}
		}

		void setListSelectedValue(const Ogre::String& value)
		{
			if (type == VAR_LIST)
			{
				if (this->selectedListValue.get<Ogre::String>() != value)
				{
					this->changed = true;
					this->selectedListValue = value;
				}
			}
		}

		Ogre::String getListSelectedValue(void) const
		{
			return this->selectedListValue;
		}

		void setListSelectedOldValue(const Ogre::String& value)
		{
			if (type == VAR_LIST)
			{
				this->selectedListOldValue = value;
			}
		}

		Ogre::String getListSelectedOldValue(void) const
		{
			return this->selectedListOldValue;
		}

		void resetChange(void)
		{
			this->changed = false;
		}

		bool hasChanged(void) const
		{
			return this->changed;
		}

		void setValue(Ogre::Real value)
		{
			if (!this->lock)
			{
				if (false== realEquals(this->value.get<Ogre::Real>(), value))
				{
					this->changed = true;
					if (true == this->hasConstraints())
					{
						if (value > this->maxValue)
							value = this->maxValue;
						else if (value < this->minValue)
							value = this->minValue;
					}

					this->type = VAR_REAL;
					this->value = value;
				}
			}
		}

		void setValue(bool value)
		{
			if (!this->lock)
			{
				if (this->value.get<bool>() != value)
				{
					this->changed = true;
					this->type = VAR_BOOL;
					this->value = value;
				}
			}
		}

		void setValue(int value)
		{
			if (!this->lock)
			{
				if (this->value.get<int>() != value)
				{
					this->changed = true;
					if (true == this->hasConstraints())
					{
						if (value > static_cast<int>(this->maxValue))
							value = static_cast<int>(this->maxValue);
						else if (value < static_cast<int>(this->minValue))
							value = static_cast<int>(this->minValue);
					}
					this->type = VAR_INT;
					this->value = value;
				}
			}
		}

		void setValue(unsigned int value)
		{
			if (!this->lock)
			{
				if (this->value.get<unsigned int>() != value)
				{
					this->changed = true;
					if (true == this->hasConstraints())
					{
						if (value > static_cast<unsigned int>(this->maxValue))
							value = static_cast<unsigned int>(this->maxValue);
						else if (value < static_cast<unsigned int>(this->minValue))
							value = static_cast<unsigned int>(this->minValue);
					}
					this->type = VAR_UINT;
					this->value = value;
				}
			}
		}

		void setValue(unsigned long value)
		{
			if (!this->lock)
			{
				if (this->value.get<unsigned long>() != value)
				{
					this->changed = true;
					if (true == this->hasConstraints())
					{
						if (value > static_cast<unsigned long>(this->maxValue))
							value = static_cast<unsigned long>(this->maxValue);
						else if (value < static_cast<unsigned long>(this->minValue))
							value = static_cast<unsigned long>(this->minValue);
					}
					this->type = VAR_ULONG;
					this->value = value;
				}
			}
		}

		void setValue(const Ogre::String& value)
		{
			if (!this->lock)
			{
				if (this->value.get<Ogre::String>() != value)
				{
					this->changed = true;
					this->type = VAR_STRING;
					this->value = value;
				}
			}
		}

		void setValue(const char* value)
		{
			if (!this->lock)
			{
				if (this->value.get<char*>() != value)
				{
					this->changed = true;
					this->type = VAR_STRING;
					this->value = Ogre::String(value);
				}
			}
		}

		void setValue(const Ogre::Vector2& value)
		{
			if (!this->lock)
			{
				if (false == vector2Equals(this->value.get<Ogre::Vector2>(), value))
				{
					this->changed = true;
					this->type = VAR_VEC2;
					this->value = value;
				}
			}
		}

		void setValue(const Ogre::Vector3& value)
		{
			if (!this->lock)
			{
				if (false == vector3Equals(this->value.get<Ogre::Vector3>(), value))
				{
					this->changed = true;
					this->type = VAR_VEC3;
					this->value = value;
				}
			}
		}

		void setValue(const Ogre::Vector4& value)
		{
			if (!this->lock)
			{
				if (false == vector4Equals(this->value.get<Ogre::Vector4>(), value))
				{
					this->changed = true;
					this->type = VAR_VEC4;
					this->value = value;
				}
			}
		}

		void setValue(const Ogre::Quaternion& value)
		{
			if (!this->lock)
			{
				if (false == quaternionEquals(this->value.get<Ogre::Quaternion>(), value))
				{
					this->changed = true;
					this->type = VAR_QUAT;
					this->value = value;
				}
			}
		}

		void setValue(const Ogre::Matrix3& value)
		{
			if (!this->lock)
			{
				if (this->value.get<Ogre::Matrix3>() != value)
				{
					this->changed = true;
					this->type = VAR_MAT3;
					this->value = value;
				}
			}
		}

		void setValue(const Ogre::Matrix4& value)
		{
			if (!this->lock)
			{
				if (this->value.get<Ogre::Matrix4>() != value)
				{
					this->changed = true;
					this->type = VAR_MAT4;
					this->value = value;
				}
			}
		}

		void setValue(const std::vector<Ogre::String> value)
		{
			if (!this->lock)
			{
				if (this->value.get<std::vector<Ogre::String>>() != value)
				{
					this->changed = true;
					this->type = VAR_LIST;
					this->value = value;
					if (value.size() > 0 && true == this->selectedListValue.toString().empty())
					{
						this->selectedListValue = value[0];
					}
				}
			}
		}

		void setValue(const Variant& variant)
		{
			if (!this->lock)
			{
				if (*this != variant)
				{
					this->changed = true;
					this->type = variant.type;
					this->value = variant.value;
					this->name = variant.name;
				}
			}
		}

		void setConstraints(Ogre::Real minValue, Ogre::Real maxValue)
		{
			this->minValue = minValue;
			this->maxValue = maxValue;
		}

		bool hasConstraints(void) const
		{
			return this->minValue != std::numeric_limits<Ogre::Real>::min() && this->maxValue != std::numeric_limits<Ogre::Real>::max();
		}

		std::pair<Ogre::Real, Ogre::Real> getConstraints(void) const
		{
			return std::make_pair(this->minValue, this->maxValue);
		}

		void addUserData(const Ogre::String& userDataKey, const Ogre::String& userDataValue = Ogre::String())
		{
			bool found = false;
			for (size_t i = 0; i < this->userDataList.size(); i++)
			{
				if (this->userDataList[i].first == userDataKey)
				{
					found = true;
					this->userDataList[i].second = userDataValue;
				}
			}

			if (false == found)
			{
				this->userDataList.emplace_back(userDataKey, userDataValue);
			}
		}

		void setUserData(const Ogre::String& userDataKey, const Ogre::String& userDataValue = Ogre::String())
		{
			for (size_t i = 0; i < this->userDataList.size(); i++)
			{
				if (this->userDataList[i].first == userDataKey)
				{
					this->userDataList[i].second = userDataValue;
					return;
				}
			}
		}

		void removeUserData(const Ogre::String& userDataKey)
		{
			for (size_t i = 0; i < this->userDataList.size(); i++)
			{
				if (this->userDataList[i].first == userDataKey)
				{
					this->userDataList.erase(this->userDataList.begin() + i);
					return;
				}
			}
		}

		Ogre::String getUserDataValue(const Ogre::String& userDataKey) const
		{
			for (size_t i = 0; i < this->userDataList.size(); i++)
			{
				if (this->userDataList[i].first == userDataKey)
				{
					return this->userDataList[i].second;
				}
			}
			return "";
		}

		Ogre::String findUserDataValue(const Ogre::String& userDataKey, bool& success) const
		{
			for (size_t i = 0; i < this->userDataList.size(); i++)
			{
				if (this->userDataList[i].first == userDataKey)
				{
					success = true;
					return this->userDataList[i].second;
				}
			}
			success = false;
			return "";
		}

		bool hasUserDataKey(const Ogre::String& userDataKey)
		{
			for (size_t i = 0; i < this->userDataList.size(); i++)
			{
				if (this->userDataList[i].first == userDataKey)
				{
					return true;
				}
			}
			return false;
		}

		std::pair<Ogre::String, Ogre::String> getUserData(size_t i) const
		{
			if (i >= this->userDataList.size())
				return std::make_pair("", "");

			return this->userDataList[i];
		}

		size_t getUserDataCount(void) const
		{
			return this->userDataList.size();
		}

		bool getBool(void) const
		{
			switch (this->type)
			{
			case VAR_INT:
				return this->value.get<int>() != 0;
			case VAR_UINT:
				return this->value.get<unsigned int>() != 0;
			case VAR_ULONG:
				return this->value.get<unsigned long>() != 0;
			case VAR_REAL:
				return this->value.get<Ogre::Real>() != 0.f;
			case VAR_BOOL:
				return this->value.get<bool>();
			default:
				{
					bool value;
					_fromString(this->value.toString(), value);
					return value;
				}
			}
			return false;
		}

		Ogre::Real getReal(void) const
		{
			switch (this->type)
			{
			case VAR_INT:
				return (Ogre::Real)this->value.get<int>();
			case VAR_UINT:
				return (Ogre::Real)this->value.get<unsigned int>();
			case VAR_ULONG:
				return (Ogre::Real)this->value.get<unsigned long>();
			case VAR_REAL:
				return this->value.get<Ogre::Real>();
			case VAR_BOOL:
				return this->value.get<bool>() ? 1.0f : 0.0f;
			default:
				{
					Ogre::Real value;
					_fromString(this->value.toString(), value);
					return value;
				}
			}
			return 0.0f;
		}

		int getInt(void) const
		{
			switch (this->type)
			{
			case VAR_INT:
				return this->value.get<int>(0);
			case VAR_UINT:
				return this->value.get<unsigned int>(0);
			case VAR_ULONG:
				return this->value.get<unsigned long>(0);
			case VAR_REAL:
				return (int)this->value.get<Ogre::Real>(0.0f);
			case VAR_BOOL:
				return this->value.get<bool>(false) ? 1 : 0;
			default:
				{
					int value;
					_fromString(this->value.toString(), value);
					return value;
				}
			}
			return 0;
		}

		unsigned int getUInt(void) const
		{
			switch (this->type)
			{
			case VAR_INT:
				return this->value.get<int>(0);
			case VAR_UINT:
				return this->value.get<unsigned int>(0);
			case VAR_ULONG:
				return this->value.get<unsigned long>(0);
			case VAR_REAL:
				return (int)this->value.get<Ogre::Real>(0.0f);
			case VAR_BOOL:
				return this->value.get<bool>(false) ? 1 : 0;
			default:
			{
				int value;
				_fromString(this->value.toString(), value);
				return value;
			}
			}
			return 0;
		}

		unsigned long getULong(void) const
		{
			switch (this->type)
			{
			case VAR_ULONG:
				return this->value.get<unsigned long>(0);
			case VAR_INT:
				return this->value.get<int>(0);
			case VAR_UINT:
				return this->value.get<unsigned int>(0);
			case VAR_REAL:
				return (int)this->value.get<Ogre::Real>(0.0f);
			case VAR_BOOL:
				return this->value.get<bool>(false) ? 1 : 0;
			default:
			{
				int value;
				_fromString(this->value.toString(), value);
				return value;
			}
			}
			return 0;
		}

		Ogre::String getString(void) const
		{
			return this->value.toString();
		}

		std::vector<Ogre::String> getList(void) const
		{
			return this->value.get<std::vector<Ogre::String>>(std::vector<Ogre::String>());
		}

		Ogre::Vector2 getVector2(void) const
		{
			return this->value.get<Ogre::Vector2>(Ogre::Vector2(0, 0));
		}

		Ogre::Vector3 getVector3(void) const
		{
			return this->value.get<Ogre::Vector3>(Ogre::Vector3(0, 0, 0));
		}

		Ogre::Vector4 getVector4(void) const
		{
			return this->value.get<Ogre::Vector4>(Ogre::Vector4(0, 0, 0, 0));
		}

		Ogre::Quaternion getQuaternion(void) const
		{
			return this->value.get<Ogre::Quaternion>(Ogre::Quaternion::IDENTITY);
		}

		Ogre::Matrix3 getMatrix3(void) const
		{
			return this->value.get<Ogre::Matrix3>(Ogre::Matrix3::IDENTITY);
		}

		Ogre::Matrix4 getMatrix4(void) const
		{
			return this->value.get<Ogre::Matrix4>(Ogre::Matrix4::IDENTITY);
		}

		bool operator < (const Variant& other) const
		{
			switch (this->type)
			{
			case VAR_BOOL: return (int)getBool()  < (int)other.getBool();
			case VAR_INT:  return getInt()        < other.getInt();
			case VAR_UINT:  return getUInt()      < other.getUInt();
			case VAR_ULONG:  return getULong()      < other.getULong();
			case VAR_REAL: return getReal()       < other.getReal();
			case VAR_VEC2: return getVector2()    < other.getVector2();
			case VAR_VEC3: return getVector3()    < other.getVector3();
			default:
				return getString() < other.getString();
			}
			return false;
		}


		bool operator > (const Variant& other) const
		{
			switch (this->type)
			{
			case VAR_BOOL: return (int)getBool()  > (int)other.getBool();
			case VAR_INT:  return getInt()        > other.getInt();
			case VAR_UINT:  return getUInt()      > other.getUInt();
			case VAR_ULONG:  return getULong()      > other.getULong();
			case VAR_REAL: return getReal()       > other.getReal();
			case VAR_VEC2: return getVector2()    > other.getVector2();
			case VAR_VEC3: return getVector3()    > other.getVector3();
			default:
				return getString() > other.getString();
			}
			return false;
		}

		bool operator <= (const Variant& other) const
		{
			return (*this).operator < (other) || (*this).operator == (other);
		}

		bool operator >= (const Variant& other) const
		{
			return (*this).operator > (other) || (*this).operator == (other);
		}

		bool operator == (const Variant& other) const
		{
			switch (this->type)
			{
			case VAR_BOOL: return (int)getBool() == (int)other.getBool();
			case VAR_INT:  return getInt() == other.getInt();
			case VAR_UINT:  return getUInt() == other.getUInt();
			case VAR_ULONG:  return getULong() == other.getULong();
			case VAR_REAL: return getReal() == other.getReal();
			case VAR_VEC2: return getVector2() == other.getVector2();
			case VAR_VEC3: return getVector3() == other.getVector3();
			case VAR_VEC4: return getVector4() == other.getVector4();
			case VAR_QUAT: return getQuaternion() == other.getQuaternion();
			case VAR_MAT3: return getMatrix3() == other.getMatrix3();
			case VAR_MAT4: return getMatrix4() == other.getMatrix4();
			// case VAR_GUID: return getGUID() == other.getGUID();
			case VAR_LIST: return getListSelectedValue() == other.getListSelectedValue();
			default:
				return getString() == other.getString();
			}
			return false;
		}

		bool equals (const Variant* other) const
		{
			switch (this->type)
			{
			case VAR_BOOL: return (int)getBool() == (int)other->getBool();
			case VAR_INT:  return getInt() == other->getInt();
			case VAR_UINT:  return getUInt() == other->getUInt();
			case VAR_ULONG:  return getULong() == other->getULong();
			case VAR_REAL: return getReal() == other->getReal();
			case VAR_VEC2: return getVector2() == other->getVector2();
			case VAR_VEC3: return getVector3() == other->getVector3();
			case VAR_VEC4: return getVector4() == other->getVector4();
			case VAR_QUAT: return getQuaternion() == other->getQuaternion();
			case VAR_MAT3: return getMatrix3() == other->getMatrix3();
			case VAR_MAT4: return getMatrix4() == other->getMatrix4();
			// case VAR_GUID: return getGUID() == other.getGUID();
			case VAR_LIST: return getListSelectedValue() == other->getListSelectedValue();
			default:
				return getString() == other->getString();
			}
			return false;
		}

		bool operator != (const Variant& other) const
		{
			return !(*this).operator == (other);
		}

		void assign(const Ogre::String& other)
		{
			if (!this->lock)
			{
				if (this->value.get<Ogre::String>() != other)
				{
					this->type = VAR_STRING;
					this->value = other;
					this->changed = true;
				}
			}
		}

		void inverse(const Ogre::String& other)
		{
			if (!this->lock)
			{
				Variant nv;
				nv.setValue(other);
				inverse(nv);
			}
		}

		void toggle(const Ogre::String& other)
		{
			if (!this->lock)
			{
				inverse(other);
			}
		}

		void assign(const Variant& variant)
		{
			if (!this->lock)
			{
				if (*this != variant)
				{
					this->type = variant.type;
					this->value = variant.value;
				}
			}
		}

		void increment(const Variant& variant)
		{
			if (!this->lock)
			{
				switch (this->type)
				{
				case VAR_BOOL:  setValue(getBool() != variant.getBool());       break;
				case VAR_INT:   setValue(getInt() + variant.getInt());        break;
				case VAR_UINT:  setValue(getUInt() + variant.getUInt());        break;
				case VAR_ULONG:  setValue(getULong() + variant.getULong());        break;
				case VAR_REAL:  setValue(getReal() + variant.getReal());       break;
				case VAR_VEC2:  setValue(getVector2() + variant.getVector2());    break;
				case VAR_VEC3:  setValue(getVector3() + variant.getVector3());    break;
				case VAR_VEC4:  setValue(getVector4() + variant.getVector4());    break;
				case VAR_QUAT:  setValue(getQuaternion() * variant.getQuaternion()); break;
				case VAR_MAT3:  setValue(getMatrix3()    * variant.getMatrix3());    break;
				case VAR_MAT4:  setValue(getMatrix4()    * variant.getMatrix4());    break;
				default:
					setValue(getString() + variant.getString());
					break;
				}
			}
		}

		void decrement(const Variant& variant)
		{
			if (!this->lock)
			{
				switch (this->type)
				{
				case VAR_BOOL:  setValue(getBool() != variant.getBool());       break;
				case VAR_INT:   setValue(getInt() - variant.getInt());        break;
				case VAR_UINT:  setValue(getUInt() - variant.getUInt());        break;
				case VAR_ULONG:  setValue(getULong() - variant.getULong());        break;
				case VAR_REAL:  setValue(getReal() - variant.getReal());       break;
				case VAR_VEC2:  setValue(getVector2() - variant.getVector2());    break;
				case VAR_VEC3:  setValue(getVector3() - variant.getVector3());    break;
				case VAR_VEC4:  setValue(getVector4() - variant.getVector4());    break;
				case VAR_QUAT:  setValue(variant.getQuaternion().Inverse() * getQuaternion()); break;
				case VAR_MAT3:  setValue(variant.getMatrix3().Inverse() * getMatrix3());    break;
				case VAR_MAT4:  setValue(variant.getMatrix4().inverse() * getMatrix4());    break;
				default:
					break;
				}
			}
		}

		bool hasInverse(void)
		{
			switch (this->type)
			{
			case VAR_BOOL:
			case VAR_INT:
			case VAR_UINT:
			case VAR_ULONG:
			case VAR_REAL:
			case VAR_VEC2:
			case VAR_VEC3:
			case VAR_VEC4:
				return true;
			case VAR_QUAT:
			case VAR_MAT3:
			case VAR_MAT4:
			default:
				break;
			}
			return false;
		}

		void inverse(const Variant& variant)
		{
			if (!this->lock)
			{
				switch (this->type)
				{
				case VAR_BOOL:  setValue(!variant.getBool());            break;
				case VAR_INT:   setValue(variant.getInt() ? 0 : 1);	  break;
				case VAR_UINT:  setValue(variant.getUInt() ? 0 : 1);	  break;
				case VAR_ULONG:  setValue(variant.getULong() ? 0 : 1);	  break;
				case VAR_REAL:  setValue(variant.getReal() ? 0.f : 1.f); break;
				case VAR_VEC2:  setValue(-(variant.getVector2()));       break;
				case VAR_VEC3:  setValue(-(variant.getVector3()));       break;
				case VAR_VEC4:  setValue(-(variant.getVector4()));       break;
				case VAR_QUAT:
				case VAR_MAT3:
				case VAR_MAT4:
				default:
					break;
				}
			}
		}

		void toggle(const Variant& other)
		{
			if (!this->lock)
			{
				inverse(other);
			}
		}

		bool getIsId(void) const
		{
			return this->isId;
		}
	private:
		Value value;
		int  type;
		Ogre::String name;
		Ogre::String description;
		bool isId;
		bool lock;
		bool visible;
		bool changed;
		Value selectedListValue;
		Value selectedListOldValue;
		Ogre::Real minValue;
		Ogre::Real maxValue;

		std::vector<std::pair<Ogre::String, Ogre::String>> userDataList;
	};

}; //namespace end

#endif