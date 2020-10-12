/*
 *   gstr.h
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

#ifndef __cplusplus
#error "This is a c++ header"
#endif

#include <glib.h>
#include <string.h>
#include <vector>

class gstr;
using gstrvec = std::vector<gstr>;

class gstr
{
public:
    gstr();
    gstr(std::nullptr_t);
    explicit gstr(const char* s);
    gstr(char* s, bool);
    ~gstr();

    gstr(const gstr& s);
    gstr(gstr&& s);

    const char* get() const;

    void copy(const char* s);
    void steal(char* s);
    static gstr take(char* src);

    gstr& operator=(const gstr& other);
    gstr& operator=(gstr&& other);

    void clear();
    bool empty() const;

    bool operator==(const gstr& other) const;
    bool operator==(const char* other) const;
    bool operator==(std::nullptr_t) const;
    
    template<typename T>
    bool operator!=(const T& other) const
    {
        return !(*this == other);
    }

    bool operator<(const gstr& other) const;

    static gstrvec copy(char** strv);
    static gstrvec take(char** strv);

    gstrvec split(const char* separator, int max_pieces) const;

private:
    char* m_p;
};

namespace std
{

template<>
struct hash<gstr>
{
    size_t operator()(const gstr& s) const
    {
        return g_str_hash(s.get());
    }
};

} // namespace std

class ConstCharSource
{
public:
    static const char* get(const char* s) { return s; }

#if 0
template<size_t arr_size>
struct ConstCharSource<char[arr_size]>
{
    static const char* get(char (&s)[arr_size]) { return &s[0]; }
};

template<size_t arr_size>
struct ConstCharSource<const char[arr_size]>
{
    static const char* get(const char (&s)[arr_size]) { return &s[0]; }
};
#endif

    static const char* get(const gstr& s) { return s.get(); }
};
