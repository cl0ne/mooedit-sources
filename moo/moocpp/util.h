/*
 *   util.h
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

#include <algorithm>

template<typename Container, typename Element>
bool contains(const Container& container, const Element& element)
{
    return std::find(container.begin(), container.end(), element) != container.end();
}

template<typename Container, typename Element>
void remove(Container& container, const Element& element)
{
    container.erase(std::remove(container.begin(), container.end(), element), container.end());
}
