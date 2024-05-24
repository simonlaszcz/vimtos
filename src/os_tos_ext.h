/*
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * os_tos_ext.h
 *
 * unistd/libc functions used by os_tos that are not present in libcmini
 *
 */

#ifndef OS_TOS_EXT_H
#define OS_TOS_EXT_H

#include "os_tos_debug.h"

#ifndef R_OK
# define R_OK   (4)
#endif
#ifndef W_OK
# define W_OK   (2)
#endif
#ifndef X_OK
# define X_OK   (1)
#endif
#ifndef F_OK
# define F_OK   (0)
#endif

#ifndef EIO
# define EIO        (5)
#endif
#ifndef EMFILE
# define EMFILE     (24)
#endif
#ifndef ENOTEMPTY
# define ENOTEMPTY  (41)
#endif

int access(const char *pathname, int mode);
char *ctime(const time_t *timep);
char *asctime(const struct tm *timep);
int dup(int oldfd);
void perror(const char *s);
int mkdir(const char *path, mode_t mode);
int rmdir(const char *path);
char *sanepath(char *obuf, const char *path, int trailing_sep);

#endif
