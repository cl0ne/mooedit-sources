/*
 *   gobjptr.h
 *
 *   Copyright (C) 2004-2016 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *
 *   This file is part of medit.  medit is free software; you can
 *   redistribute it and/or modify it under the terms of the
 *   GNU Lesser General Public License as published by the
 *   Free Software Foundation; either version 2.1 of the License,
 *   or (at your option) any later version.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with medit.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

enum class ObjectMemPolicy
{
    TakeOwnership,
    CopyReference,
};

template<typename FooObject>
class ObjectPtr
{
public:
    ObjectPtr()
        : m_p(nullptr)
    {
    }

    ~ObjectPtr()
    {
        reset();
    }

    ObjectPtr(FooObject* obj, ObjectMemPolicy policy)
        : m_p(obj)
    {
        if (policy == ObjectMemPolicy::CopyReference && obj != nullptr)
            g_object_ref_sink(m_p);
    }

    ObjectPtr(const ObjectPtr& p)
        : ObjectPtr(p.m_p, ObjectMemPolicy::CopyReference)
    {
    }

    ObjectPtr(ObjectPtr&& p)
        : ObjectPtr(p.m_p, ObjectMemPolicy::TakeOwnership)
    {
        p.m_p = nullptr;
    }

    FooObject* operator->() const
    {
        return m_p;
    }

    FooObject& operator*() const
    {
        g_assert(m_p != nullptr);
        return *m_p;
    }

    void reset()
    {
        if (m_p != nullptr)
            g_object_unref(m_p);
        m_p = nullptr;
    }

    FooObject* get() const
    {
        return m_p;
    }

    static ObjectPtr take(FooObject* obj)
    {
        return ObjectPtr(obj, ObjectMemPolicy::TakeOwnership);
    }

    static ObjectPtr ref(FooObject* obj)
    {
        return ObjectPtr(obj, ObjectMemPolicy::CopyReference);
    }

    ObjectPtr& operator=(std::nullptr_t)
    {
        reset();
        return *this;
    }

    ObjectPtr& operator=(const ObjectPtr& p)
    {
        if (this != &p)
        {
            FooObject* tmp = m_p;
            m_p = p.m_p;
            if (m_p != nullptr)
                g_object_ref_sink(m_p);
            if (tmp != nullptr)
                g_object_unref(tmp);
        }

        return *this;
    }

    ObjectPtr& operator=(ObjectPtr&& p)
    {
        FooObject* tmp = m_p;
        m_p = p.m_p;
        p.m_p = nullptr;
        if (tmp != nullptr)
            g_object_unref(tmp);
        return *this;
    }

    operator bool() const
    {
        return m_p != nullptr;
    }

    bool operator!() const
    {
        return m_p == nullptr;
    }

    bool operator==(const std::nullptr_t&) const
    {
        return m_p == nullptr;
    }

    bool operator==(const ObjectPtr& p) const
    {
        return m_p == p.m_p;
    }

    bool operator==(const FooObject* p) const
    {
        return m_p == p;
    }

    template<typename U>
    bool operator!=(const U& other) const
    {
        return !(*this == other);
    }

private:
    FooObject* m_p;
};

namespace g 
{

template<typename T>
inline ObjectPtr<T> ref_obj(T* p)
{
    return ObjectPtr<T>(p, ObjectMemPolicy::CopyReference);
}

template<typename T>
inline ObjectPtr<T> take_obj(T* p)
{
    return ObjectPtr<T>(p, ObjectMemPolicy::TakeOwnership);
}

} // namespace g
