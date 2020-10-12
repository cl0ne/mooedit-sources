#ifndef GTK_SOURCE_VIEW_I18N_H
#define GTK_SOURCE_VIEW_I18N_H

#include "mooutils/mooi18n.h"

#ifdef ENABLE_NLS

#undef _
#define _(string) _moo_gsv_gettext (string)

#undef Q_
#undef N_
#undef D_
#undef QD_

#endif /* ENABLE_NLS */

#define GD_(domain,string) _moo_gsv_dgettext (domain, string)

#endif /* GTK_SOURCE_VIEW_I18N_H */
