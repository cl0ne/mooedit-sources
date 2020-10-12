#define MOO_DO_NOT_MANGLE_GLIB_FUNCTIONS
#include "moo-glib.h"
#include "moo-time.h"
#include "moo-stat.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __WIN32__
#include <io.h>
#endif // __WIN32__

const mgw_errno_t MGW_E_NOERROR = { MGW_ENOERROR };
const mgw_errno_t MGW_E_EXIST   = { MGW_EEXIST };


static mgw_time_t convert_time_t (time_t t)
{
    mgw_time_t result;
    result.value = t;
    return result;
}


static void convert_g_stat_buf (const GStatBuf* gbuf, MgwStatBuf* mbuf)
{
    mbuf->atime = convert_time_t (gbuf->st_atime);
    mbuf->mtime = convert_time_t (gbuf->st_mtime);
    mbuf->ctime = convert_time_t (gbuf->st_ctime);

    mbuf->size = gbuf->st_size;

#ifdef _MSC_VER
    mbuf->isreg = (gbuf->st_mode & _S_IFREG) != 0;
    mbuf->isdir = (gbuf->st_mode & _S_IFDIR) != 0;
#else
    mbuf->isreg = S_ISREG (gbuf->st_mode);
    mbuf->isdir = S_ISDIR (gbuf->st_mode);
#endif

#ifdef S_ISLNK
    mbuf->islnk = S_ISLNK (gbuf->st_mode);
#else
    mbuf->islnk = 0;
#endif

#ifdef S_ISSOCK
    mbuf->issock = S_ISSOCK (gbuf->st_mode);
#else
    mbuf->issock = 0;
#endif

#ifdef S_ISFIFO
    mbuf->isfifo = S_ISFIFO (gbuf->st_mode);
#else
    mbuf->isfifo = 0;
#endif

#ifdef S_ISCHR
    mbuf->ischr = S_ISCHR (gbuf->st_mode);
#else
    mbuf->ischr = 0;
#endif

#ifdef S_ISBLK
    mbuf->isblk = S_ISBLK (gbuf->st_mode);
#else
    mbuf->isblk = 0;
#endif
}


template<typename Func>
void _call_with_errno(const Func& func, mgw_errno_t *err)
{
    errno = 0;
    func();
    if (err != nullptr)
        err->value = (mgw_errno_value_t) errno;
}

#define call_with_errno0(func__, result__)                                  \
    _call_with_errno([](){return (func__)();}, err);

#define call_with_errno1(func__, result__, a1__)                            \
    _call_with_errno([&](){result__ = (func__)((a1__));}, err)

#define call_with_errno2(func__, result__, a1__, a2__)                      \
    _call_with_errno([&](){result__ = (func__)((a1__), (a2__));}, err);

#define call_with_errno3(func__, result__, a1__, a2__, a3__)                \
    _call_with_errno([&](){result__ = (func__)((a1__), (a2__), (a3__));}, err);

#define call_with_errno4(func__, result__, a1__, a2__, a3__, a4__)          \
    _call_with_errno([&](){result__ = (func__)((a1__), (a2__), (a3__), (a4__));}, err);


const char *
mgw_strerror (mgw_errno_t err)
{
    return g_strerror (err.value);
}

GFileError
mgw_file_error_from_errno (mgw_errno_t err)
{
    return g_file_error_from_errno (err.value);
}


int
mgw_stat (const gchar *filename, MgwStatBuf *buf, mgw_errno_t *err)
{
    GStatBuf gbuf = { 0 };
    int result;
    call_with_errno2 (g_stat, result, filename, &gbuf);
    convert_g_stat_buf (&gbuf, buf);
    return result;
}

int
mgw_lstat (const gchar *filename, MgwStatBuf *buf, mgw_errno_t *err)
{
    GStatBuf gbuf = { 0 };
    int result;
    call_with_errno2 (g_lstat, result, filename, &gbuf);
    convert_g_stat_buf (&gbuf, buf);
    return result;
}


const struct tm *
mgw_localtime (const mgw_time_t *timep)
{
    time_t t = timep->value;
    return localtime(&t);
}

#ifdef __WIN32__
static struct tm *
localtime_r (const time_t *timep,
             struct tm *result)
{
    struct tm *res;
    res = localtime (timep);
    if (res)
        *result = *res;
    return res;
}
#endif

const struct tm *
mgw_localtime_r (const mgw_time_t *timep, struct tm *result, mgw_errno_t *err)
{
    time_t t = timep->value;
    struct tm *ret;
    call_with_errno2 (localtime_r, ret, &t, result);
    return ret;
}

mgw_time_t
mgw_time (mgw_time_t *t, mgw_errno_t *err)
{
    time_t t1;
    mgw_time_t result;
    call_with_errno1 (time, result.value, &t1);
    if (t != NULL)
        t->value = t1;
    return result;
}


guint64
mgw_ascii_strtoull (const gchar *nptr, gchar **endptr, guint base, mgw_errno_t *err)
{
    guint64 result;
    call_with_errno3 (g_ascii_strtoull, result, nptr, endptr, base);
    return result;
}

gdouble
mgw_ascii_strtod (const gchar *nptr, gchar **endptr, mgw_errno_t *err)
{
    double result;
    call_with_errno2 (g_ascii_strtod, result, nptr, endptr);
    return result;
}


MGW_FILE *
mgw_fopen (const char *filename, const char *mode, mgw_errno_t *err)
{
    FILE* result;
    call_with_errno2 (g_fopen, result, filename, mode);
    return (MGW_FILE*) result;
}

int mgw_fclose (MGW_FILE *file)
{
    return fclose ((FILE*) file);
}

gsize
mgw_fread(void *ptr, gsize size, gsize nmemb, MGW_FILE *stream, mgw_errno_t *err)
{
    gsize result;
    call_with_errno4 (fread, result, ptr, size, nmemb, (FILE*) stream);
    return result;
}

gsize
mgw_fwrite(const void *ptr, gsize size, gsize nmemb, MGW_FILE *stream)
{
    return fwrite (ptr, size, nmemb, (FILE*) stream);
}

int
mgw_ferror (MGW_FILE *file)
{
    return ferror ((FILE*) file);
}

char *
mgw_fgets(char *s, int size, MGW_FILE *stream)
{
    return fgets(s, size, (FILE*) stream);
}


MgwFd
mgw_open (const char *filename, int flags, int mode)
{
    MgwFd fd;
    fd.value = g_open (filename, flags, mode);
    return fd;
}

int
mgw_close (MgwFd fd)
{
    return close (fd.value);
}

gssize
mgw_write(MgwFd fd, const void *buf, gsize count)
{
    return write (fd.value, buf, count);
}

int
mgw_pipe (MgwFd *fds)
{
    int t[2];

#ifndef __WIN32__
    int result = pipe (t);
#else
    int result = _pipe(t, 4096, O_BINARY);
#endif

    fds[0].value = t[0];
    fds[1].value = t[1];
    return result;
}

void
mgw_perror (const char *s)
{
    perror (s);
}


int
mgw_unlink (const char *path, mgw_errno_t *err)
{
    int result;
    call_with_errno1 (g_unlink, result, path);
    return result;
}

int
mgw_remove (const char *path, mgw_errno_t *err)
{
    int result;
    call_with_errno1 (g_remove, result, path);
    return result;
}

int
mgw_rename (const char *oldpath, const char *newpath, mgw_errno_t *err)
{
    int result;
    call_with_errno2 (g_rename, result, oldpath, newpath);
    return result;
}

int
mgw_mkdir (const gchar *filename, int mode, mgw_errno_t *err)
{
    int result;
    call_with_errno2 (g_mkdir, result, filename, mode);
    return result;
}

int
mgw_mkdir_with_parents (const gchar *pathname, gint mode, mgw_errno_t *err)
{
    int result;
    call_with_errno2 (g_mkdir_with_parents, result, pathname, mode);
    return result;
}

int
mgw_access (const char *path, mgw_access_mode_t mode)
{
    int gmode = F_OK;
    if (mode.value & MGW_R_OK)
        gmode |= R_OK;
    if (mode.value & MGW_W_OK)
        gmode |= W_OK;
    if (mode.value & MGW_X_OK)
        gmode |= X_OK;
    return g_access (path, gmode);
}


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
                            GError **error)
{
    return g_spawn_async_with_pipes (working_directory, argv, envp, flags,
                                     child_setup, user_data, child_pid,
                                     standard_input ? &standard_input->value : NULL,
                                     standard_output ? &standard_output->value : NULL,
                                     standard_error ? &standard_error->value : NULL,
                                     error);
}

GIOChannel *
mgw_io_channel_unix_new (MgwFd fd)
{
    return g_io_channel_unix_new (fd.value);
}

#ifdef __WIN32__
GIOChannel *
mgw_io_channel_win32_new_fd (MgwFd fd)
{
    return g_io_channel_win32_new_fd (fd.value);
}
#endif // __WIN32__
