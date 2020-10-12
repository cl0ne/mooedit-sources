#ifndef MEDIT_PYTHON_H
#define MEDIT_PYTHON_H

#include <mooglib/moo-glib.h>

G_BEGIN_DECLS

typedef struct MooPythonState MooPythonState;

gboolean         moo_python_enabled         (void);

MooPythonState  *moo_python_state_new       (gboolean        default_init);
void             moo_python_state_free      (MooPythonState *state);

gboolean         moo_python_run_string      (MooPythonState *state,
                                             const char     *string);
gboolean         moo_python_run_file        (MooPythonState *state,
                                             const char     *filename);

gboolean         medit_python_run_string    (const char     *string,
                                             gboolean        default_init);
gboolean         medit_python_run_file      (const char     *filename,
                                             gboolean        default_init);

G_END_DECLS

#endif /* MEDIT_PYTHON_H */
