/*
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * os_tos_ext.c
 *
 * unistd/libc functions used by os_tos that are not present in libcmini
 *
 */

#include <osbind.h>
#include <errno.h>
#include <ext.h>
#include <unistd.h>
#include <dirent.h>
#include "os_tos_ext.h"

#define A_E_OK      (0)
#define A_EFILNF    (-33)
#define A_EPTHNF    (-34)
#define A_EACCDN    (-36)
#define A_ENHNDL    (-35)
#define A_EBADF     (-37)
#define FALSE       (0)
#define NUL         '\0'

int
access(const char *pathname, int mode)
{
    struct stat s;
    char p[PATH_MAX];

    if (stat(sanepath(p, pathname, FALSE), &s) < 0) {
        contracev("pathname=%s, sane=%s, mode=%d, errno=%d FAILED", pathname, p, mode, errno);
        return -1;
    }

    if (mode == F_OK || mode == R_OK) {
        contracev("pathname=%s, sane=%s, mode=%d F_OK||R_OK OK", pathname, p, mode);
        return 0;
    }

    if ((mode & W_OK) && (s.st_mode & S_IWRITE) == 0) {
        contracev("pathname=%s, sane=%s, mode=%d, errno=EACCES NOT W_OK", pathname, p, mode);
        errno = EACCES;
        return -1;
    }

    if ((mode & X_OK) && (s.st_mode & S_IEXEC) == 0) {
        contracev("pathname=%s, sane=%s, mode=%d, errno=EACCES NOT X_OK", pathname, p, mode);
        errno = EACCES;
        return -1;
    }

    contracev("pathname=%s, sane=%s, mode=%d OK", pathname, p, mode);
    return 0;
}

char *
ctime(const time_t *timep)
{
    char *rv = asctime(localtime(timep));
    contracev("timep=%d, rv=%s", *timep, rv);
    return rv;
}

#define ASCTIME_BUFLEN (26)
char *
asctime(const struct tm *timep)
{
    static char buf[ASCTIME_BUFLEN];
    static char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    buf[0] = NUL;
    sprintf(buf, "%s %s %d %02d:%02d:%02d %04d\n", 
        days[timep->tm_wday], 
        months[timep->tm_mon], 
        timep->tm_mday, 
        timep->tm_hour, timep->tm_min, timep->tm_sec,
        timep->tm_year + 1900);

    contracev("%s", buf);
    return buf;
}

int
dup(int oldfd)
{
    /* Fdup only works for standard file handles 0..5 
    but vim only dupes stdout and stderr */
    int newfd = Fdup(oldfd);

    if (newfd < 0 || newfd == oldfd) {
        if (newfd == A_ENHNDL)
            errno = EMFILE;
        else if (newfd == A_EBADF)
            errno = EBADF;
        else if (newfd == oldfd)
            errno = EINVAL;
        else
            errno = EIO;

        contracev("oldfd=%d, newfd=%d, errno=%d FAILED", oldfd, newfd, errno);
        return -1;
    }

    contracev("oldfd=%d, newfd=%d OK", oldfd, newfd);
    return newfd;
}

void
perror(const char *s)
{
    fprintf(stderr, s);
    contracev("%s", s);
}

int
mkdir(const char *path, mode_t mode)
{
    char p[PATH_MAX];
    int r = 0;

    /* Dcreate can't handle trailing backslash */
    sanepath(p, path, FALSE);

    if (access(p, F_OK) >= 0) {
        contracev("path=%s, mode=%d, sane=%s, errno=EEXIST", path, mode, p);
        errno = EEXIST;
        return -1;
    }

    if ((r = Dcreate(p)) < 0) {
        switch (r) {
        case A_EACCDN:
            errno = EACCES;
            break;
        case A_EPTHNF:
            errno = ENOENT;
            break;
        default:
            errno = EIO;
            break;
        }

        contracev("path=%s, mode=%d, sane=%s, errno=%d FAILED", path, mode, p, errno);
        return -1;
    }

    contracev("path=%s, mode=%d, sane=%s OK", path, mode, p);
    return 0;
}

int
rmdir(const char *path)
{
    DIR *d = NULL;
    struct dirent *e = NULL;
    int count = 0;
    char p[PATH_MAX];

    /* Ddelete doesn't care if there's a trailing backslash or not
    but we remove it anyway */
    sanepath(p, path, FALSE);

    if ((d = opendir(p)) == NULL) {
        contracev("path=%s, sane=%s, errno=ENOTDIR", path, p);
        errno = ENOTDIR;
        return -1;
    }

    while ((e = readdir(d)) != NULL && count == 0)
        if (!(e->d_name[0] == '.' && (e->d_name[1] == NUL || e->d_name[1] == '.')))
            ++count;

    closedir(d);

    if (count > 0) {
        contracev("path=%s, sane=%s, errno=ENOTEMPTY", path, p);
        errno = ENOTEMPTY;
        return -1;
    }

    if (Ddelete(p) != A_E_OK) {
        contracev("path=%s, sane=%s, errno=EACCES", path, p);
        errno = EACCES;
        return -1;
    }

    contracev("path=%s, sane=%s OK", path, p);
    return 1;
}


/**
 * Sanitise the path for TOS
 * obuf should be at least PATH_MAX long
 */
char *
sanepath(char *obuf, const char *path, int trailing_sep)
{
#ifdef TOS_DEBUG
    const char *orig = path;
#endif
    obuf[0] = NUL;

    if (path == NULL || *path == NUL)
        goto ret_obuf;

    /* skip leading spaces */
    while (*path == ' ' && *path != NUL)
        ++path;

    /* all spaces? */
    if (*path == NUL)
        goto ret_obuf;

    /* copy and fix slashes */
    int len = 0;

    while (*path != NUL && len < (PATH_MAX - 1)) {
        if (*path == '/')
            obuf[len] = '\\';
        else
            obuf[len] = *path;
        ++len;
        ++path;
    }

    obuf[len] = NUL;

    /* trim trailing spaces */
    while (len > 0 && obuf[len] == ' ')
        obuf[len--] = NUL;

    /*  fix trailing backslash */
    if (trailing_sep) {
        if (obuf[len - 1] != '\\' && len < (PATH_MAX - 1)) {
            obuf[len++] = '\\';
            obuf[len] = NUL;
        }
    }
    else if (obuf[len - 1] == '\\')
        obuf[--len] = NUL;

ret_obuf:
#ifdef TOS_DEBUG
    contracev("sep=%d, len=%d, %s => %s", trailing_sep, len, orig, obuf);
#endif
    return obuf;
}
