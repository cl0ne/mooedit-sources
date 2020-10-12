#pragma once

#include <glib.h>
#include <glib/gstdio.h>
#include <config.h>
#ifndef __WIN32__
#include <errno.h>
#endif

G_BEGIN_DECLS

#undef MOO_BUILTIN_MOO_GLIB
#ifndef MOO_BUILD_FROM_MINGW
#define MOO_BUILTIN_MOO_GLIB 1
#endif

typedef struct mgw_errno_t mgw_errno_t;
typedef struct MGW_FILE MGW_FILE;
typedef struct MgwFd MgwFd;
typedef struct mgw_access_mode_t mgw_access_mode_t;

#ifndef __WIN32__
#define MGW_ERROR_IF_NOT_SHARED_LIBC
#else
#define MGW_ERROR_IF_NOT_SHARED_LIBC \
    #error "C libraries may not be shared between medit and glib"
#endif

#ifdef __WIN32__

enum mgw_errno_value_t
{
    MGW_ENOERROR = 0,
    MGW_EACCES,
    MGW_EPERM,
    MGW_EEXIST,
    MGW_ELOOP,
    MGW_ENAMETOOLONG,
    MGW_ENOENT,
    MGW_ENOTDIR,
    MGW_EROFS,
    MGW_EXDEV,
};

typedef enum mgw_errno_value_t mgw_errno_value_t;

#else

typedef int mgw_errno_value_t;

#define MGW_ENOERROR 0
#define MGW_EACCES EACCESS
#define MGW_EPERM EPERM
#define MGW_EEXIST EEXIST
#define MGW_ELOOP ELOOP
#define MGW_ENAMETOOLONG ENAMETOOLONG
#define MGW_ENOENT ENOENT
#define MGW_ENOTDIR ENOTDIR
#define MGW_EROFS EROFS
#define MGW_EXDEV EXDEV

#endif

struct mgw_errno_t
{
    mgw_errno_value_t value;
};

struct MgwFd
{
    int value;
};

#if defined(__WIN32__) && !MOO_BUILTIN_MOO_GLIB
#  ifdef MOO_GLIB_LIBRARY
#    define MOO_GLIB_VAR __declspec(dllexport)
#  else
#    define MOO_GLIB_VAR extern __declspec(dllimport)
#  endif
#else
#  define MOO_GLIB_VAR extern
#endif

MOO_GLIB_VAR const mgw_errno_t MGW_E_NOERROR;
MOO_GLIB_VAR const mgw_errno_t MGW_E_EXIST;

inline static gboolean mgw_errno_is_set (mgw_errno_t err) { return err.value != MGW_ENOERROR; }
const char *mgw_strerror (mgw_errno_t err);
GFileError mgw_file_error_from_errno (mgw_errno_t err);

guint64 mgw_ascii_strtoull (const gchar *nptr, gchar **endptr, guint base, mgw_errno_t *err);
gdouble mgw_ascii_strtod (const gchar *nptr, gchar **endptr, mgw_errno_t *err);

MGW_FILE *mgw_fopen (const char *filename, const char *mode, mgw_errno_t *err);
int mgw_fclose (MGW_FILE *file);
gsize mgw_fread(void *ptr, gsize size, gsize nmemb, MGW_FILE *stream, mgw_errno_t *err);
gsize mgw_fwrite(const void *ptr, gsize size, gsize nmemb, MGW_FILE *stream);
int mgw_ferror (MGW_FILE *file);
char *mgw_fgets(char *s, int size, MGW_FILE *stream);

MgwFd mgw_open (const char *filename, int flags, int mode);
gssize mgw_write(MgwFd fd, const void *buf, gsize count);
int mgw_close (MgwFd fd);
int mgw_pipe (MgwFd *fds);
void mgw_perror (const char *s);

int mgw_unlink (const char *path, mgw_errno_t *err);
int mgw_remove (const char *path, mgw_errno_t *err);
int mgw_rename (const char *oldpath, const char *newpath, mgw_errno_t *err);
int mgw_mkdir (const gchar *filename, int mode, mgw_errno_t *err);
int mgw_mkdir_with_parents (const gchar *pathname, gint mode, mgw_errno_t *err);

gboolean
mgw_spawn_async_with_pipes (const gchar *working_directory,
                            gchar **argv,
                            gchar **envp,
                            GSpawnFlags flags,
                            GSpawnChildSetupFunc child_setup,
                            gpointer user_data,
                            GPid *child_pid,
                            MgwFd *standard_input,
                            MgwFd *standard_output,
                            MgwFd *standard_error,
                            GError **error);
GIOChannel *mgw_io_channel_unix_new (MgwFd fd);

#ifdef __WIN32__
GIOChannel *mgw_io_channel_win32_new_fd (MgwFd fd);
#endif

enum mgw_access_mode_value_t
{
    MGW_F_OK = 0,
    MGW_R_OK = 1,
    MGW_W_OK = 2,
    MGW_X_OK = 4,
};

struct mgw_access_mode_t
{
    enum mgw_access_mode_value_t value;
};

int mgw_access (const char *path, mgw_access_mode_t mode);

#ifndef MOO_DO_NOT_MANGLE_GLIB_FUNCTIONS

#undef g_stat
#undef g_rename
#undef g_access
#undef g_lstat
#undef g_strerror
#undef g_ascii_strtoull
#undef g_file_error_from_errno
#undef g_ascii_strtod
#undef g_fopen
#undef g_unlink
#undef g_mkdir
#undef g_mkdir_with_parents
#undef g_open
#undef g_spawn_async_with_pipes
#undef g_io_channel_unix_new
#undef g_io_channel_win32_new_fd

#define g_stat DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_rename DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_access DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_lstat DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_strerror DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_ascii_strtoull DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_file_error_from_errno DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_ascii_strtod DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_fopen DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_unlink DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_mkdir DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_mkdir_with_parents DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_open DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_spawn_async_with_pipes DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_io_channel_unix_new DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD
#define g_io_channel_win32_new_fd DO_NOT_USE_THIS_DIRECTLY_USE_MGW_WRAPPERS_INSTEAD

#endif // MOO_DO_NOT_MANGLE_GLIB_FUNCTIONS

G_END_DECLS
