/*
 *   moooutputfilterregex.h
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
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

#ifndef MOO_OUTPUT_FILTER_REGEX_H
#define MOO_OUTPUT_FILTER_REGEX_H

#include "../support/moooutputfilter.h"

G_BEGIN_DECLS


#define MOO_TYPE_OUTPUT_FILTER_REGEX              (_moo_output_filter_regex_get_type ())
#define MOO_OUTPUT_FILTER_REGEX(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_OUTPUT_FILTER_REGEX, MooOutputFilterRegex))
#define MOO_OUTPUT_FILTER_REGEX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MOO_TYPE_OUTPUT_FILTER_REGEX, MooOutputFilterRegexClass))
#define MOO_IS_OUTPUT_FILTER_REGEX(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_OUTPUT_FILTER_REGEX))
#define MOO_IS_OUTPUT_FILTER_REGEX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MOO_TYPE_OUTPUT_FILTER_REGEX))
#define MOO_OUTPUT_FILTER_REGEX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MOO_TYPE_OUTPUT_FILTER_REGEX, MooOutputFilterRegexClass))

typedef struct _MooOutputFilterRegex        MooOutputFilterRegex;
typedef struct _MooOutputFilterRegexPrivate MooOutputFilterRegexPrivate;
typedef struct _MooOutputFilterRegexClass   MooOutputFilterRegexClass;

struct _MooOutputFilterRegex {
    MooOutputFilter base;
    MooOutputFilterRegexPrivate *priv;
};

struct _MooOutputFilterRegexClass {
    MooOutputFilterClass base_class;
};


GType                _moo_output_filter_regex_get_type (void) G_GNUC_CONST;

void                 _moo_command_filter_regex_load    (void);


G_END_DECLS

#endif /* MOO_OUTPUT_FILTER_REGEX_SIMPLE_H */
