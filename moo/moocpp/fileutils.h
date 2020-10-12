/*
 *   gutil.h
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

#include <glib.h>
#include <moocpp/gstr.h>

namespace g
{

gstr build_filename_impl(const char* comp1, const char* comp2 = nullptr, const char* comp3 = nullptr);

template<typename T1>
gstr build_filename(const T1& comp1)
{
    return build_filename_impl(ConstCharSource::get(comp1));
}

template<typename T1, typename T2>
gstr build_filename(const T1& comp1, const T2& comp2)
{
    return build_filename_impl(ConstCharSource::get(comp1), ConstCharSource::get(comp2));
}

template<typename T1, typename T2, typename T3>
gstr build_filename(const T1& comp1, const T2& comp2, const T3& comp3)
{
    return build_filename_impl(ConstCharSource::get(comp1), ConstCharSource::get(comp2), ConstCharSource::get(comp3));
}

gstr build_filenamev(const std::vector<gstr>& components);
gstr get_current_dir();
gstr path_get_basename(const gchar *file_name);
gstr path_get_dirname(const gchar *file_name);
gstr get_current_dir();

} // namespace g
