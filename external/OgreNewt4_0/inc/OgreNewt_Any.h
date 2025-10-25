/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2013 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
// -- Based on boost::any, original copyright information follows --
// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompAnying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// -- End original copyright --

// Adapted Ogre Any, because of does write tons of logs when an object cannot be casted!

#ifndef OGRE_NEWT_ANY
#define OGRE_NEWT_ANY

#include "OgreException.h"
#include "OgreString.h"
#include <algorithm>
#include <typeinfo>

namespace OgreNewt
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */
    /** Variant type that can hold Any other type.
    */
    class Any
    {
    public: // constructors

        Any()
            : mContent(0)
        {
        }

        template<typename ValueType>
        explicit Any(const ValueType& value)
            : mContent(new holder<ValueType>(value))
        {
        }

        Any(const Any& other)
            : mContent(other.mContent ? other.mContent->clone() : 0)
        {
        }

        virtual ~Any()
        {
            destroy();
        }

    public: // modifiers

        Any& swap(Any& rhs)
        {
            std::swap(mContent, rhs.mContent);
            return *this;
        }

        template<typename ValueType>
        Any& operator=(const ValueType& rhs)
        {
            Any(rhs).swap(*this);
            return *this;
        }

        Any& operator=(const Any& rhs)
        {
            Any(rhs).swap(*this);
            return *this;
        }

    public: // queries

        bool isEmpty() const
        {
            return !mContent;
        }

        const std::type_info& getType() const
        {
            return mContent ? mContent->getType() : typeid(void);
        }

        inline friend std::ostream& operator <<
            (std::ostream& o, const Any& v)
        {
            if (v.mContent)
                v.mContent->writeToStream(o);
            return o;
        }

        void destroy()
        {
            delete mContent;
            mContent = nullptr;
        }

    protected: // types

        class placeholder
        {
        public: // structors

            virtual ~placeholder()
            {
            }

        public: // queries

            virtual const std::type_info& getType() const = 0;

            virtual placeholder* clone() const = 0;

            virtual void writeToStream(std::ostream& o) = 0;

        };

        template<typename ValueType>
        class holder : public placeholder
        {
        public: // structors

            holder(const ValueType& value)
                : held(value)
            {
            }

        public: // queries

            virtual const std::type_info& getType() const
            {
                return typeid(ValueType);
            }

            virtual placeholder* clone() const
            {
                return new holder(held);
            }

            virtual void writeToStream(std::ostream& o)
            {
                o << held;
            }


        public: // representation

            ValueType held;

        };



    protected: // representation
        placeholder* mContent;

        template<typename ValueType>
        friend ValueType* any_cast(Any*);


    public:

        template<typename ValueType>
        ValueType operator()() const
        {
            if (!mContent)
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                    "Bad cast from uninitialised Any",
                    "Any::operator()");
            }
            else if (getType() == typeid(ValueType))
            {
                return static_cast<Any::holder<ValueType> *>(mContent)->held;
            }
            /*else
            {
                StringUtil::StrStreamType str;
                str << "Bad cast from type '" << getType().name() << "' "
                    << "to '" << typeid(ValueType).name() << "'";
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                     str.str(),
                    "Any::operator()");
            }*/
        }

        template <typename ValueType>
        ValueType get(void) const
        {
            if (!mContent)
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                    "Bad cast from uninitialised Any",
                    "Any::operator()");
            }
            else if (getType() == typeid(ValueType))
            {
                return static_cast<Any::holder<ValueType> *>(mContent)->held;
            }
            /*else
            {
                StringUtil::StrStreamType str;
                str << "Bad cast from type '" << getType().name() << "' "
                    << "to '" << typeid(ValueType).name() << "'";
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                     str.str(),
                    "Any::operator()");
            }*/
        }

    };



    template<typename ValueType>
    ValueType* any_cast(Any* operand)
    {
        return operand && (std::strcmp(operand->getType().name(), typeid(ValueType).name()) == 0)
            ? &static_cast<Any::holder<ValueType> *>(operand->mContent)->held
            : 0;
    }

    template<typename ValueType>
    const ValueType* any_cast(const Any* operand)
    {
        return any_cast<ValueType>(const_cast<Any*>(operand));
    }

    template<typename ValueType>
    ValueType any_cast(const Any& operand)
    {
        const ValueType* result = any_cast<ValueType>(&operand);
        if (!result)
        {
            /*StringUtil::StrStreamType str;
            str << "Bad cast from type '" << operand.getType().name() << "' "
                << "to '" << typeid(ValueType).name() << "'";
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                str.str(),
                "Ogre::any_cast");*/
            return nullptr;
        }
        return *result;
    }
    /** @} */
    /** @} */


}

#endif
