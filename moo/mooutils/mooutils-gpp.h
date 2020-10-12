/*
 *   mooutils-gpp.h
 *
 *   Copyright (C) 2004-2015 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
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

#include "mooutils/mooutils-cpp.h"

#include <glib.h>

template<typename Data, typename Obj>
inline Data* object_get_data_cast(Obj* obj, const char* key)
{
    return reinterpret_cast<Data*>(g_object_get_data(G_OBJECT(obj), key));
}
