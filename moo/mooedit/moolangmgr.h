/*
 *   moolangmgr.h
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

#ifndef MOO_LANG_MGR_H
#define MOO_LANG_MGR_H

#include <mooedit/moolang.h>
#include <mooedit/mootextstylescheme.h>
#include <mooedit/mooeditconfig.h>

G_BEGIN_DECLS


#define MOO_TYPE_LANG_MGR              (moo_lang_mgr_get_type ())
#define MOO_LANG_MGR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MOO_TYPE_LANG_MGR, MooLangMgr))
#define MOO_IS_LANG_MGR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MOO_TYPE_LANG_MGR))

typedef struct MooLangMgr MooLangMgr;


GType           moo_lang_mgr_get_type               (void) G_GNUC_CONST;

MooLangMgr     *moo_lang_mgr_new                    (void);
MooLangMgr     *moo_lang_mgr_default                (void);

MooLang        *moo_lang_mgr_get_lang               (MooLangMgr     *mgr,
                                                     const char     *lang_id);
MooLang        *moo_lang_mgr_get_lang_for_file      (MooLangMgr     *mgr,
                                                     GFile          *file);

/* list must be freed, content unref'ed */
GSList         *moo_lang_mgr_get_available_langs    (MooLangMgr     *mgr);
/* list must be freed together with content */
GSList         *moo_lang_mgr_get_sections           (MooLangMgr     *mgr);

MooLang        *_moo_lang_mgr_find_lang             (MooLangMgr     *mgr,
                                                     const char     *name);

MooTextStyleScheme *moo_lang_mgr_get_active_scheme  (MooLangMgr     *mgr);
void            _moo_lang_mgr_set_active_scheme     (MooLangMgr     *mgr,
                                                     const char     *scheme_name);
/* list must be freed, content unref'ed */
GSList         *moo_lang_mgr_list_schemes           (MooLangMgr     *mgr);

/* list must be freed together with content */
GSList         *_moo_lang_mgr_get_globs             (MooLangMgr     *mgr,
                                                     const char     *lang_id);
/* list must be freed together with content */
GSList         *_moo_lang_mgr_get_mime_types        (MooLangMgr     *mgr,
                                                     const char     *lang_id);
void            _moo_lang_mgr_set_mime_types        (MooLangMgr     *mgr,
                                                     const char     *lang_id,
                                                     const char     *mime);
void            _moo_lang_mgr_set_globs             (MooLangMgr     *mgr,
                                                     const char     *lang_id,
                                                     const char     *globs);
const char     *_moo_lang_mgr_get_config            (MooLangMgr     *mgr,
                                                     const char     *lang_id);
void            _moo_lang_mgr_set_config            (MooLangMgr     *mgr,
                                                     const char     *lang_id,
                                                     const char     *config);
void            _moo_lang_mgr_update_config         (MooLangMgr     *mgr,
                                                     MooEditConfig  *config,
                                                     const char     *lang_id);
void            _moo_lang_mgr_save_config           (MooLangMgr     *mgr);


G_END_DECLS

#endif /* MOO_LANG_MGR_H */
