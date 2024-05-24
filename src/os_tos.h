/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * TOS Machine-dependent things. Based on MSDOS
 */

#ifdef FEAT_NORMAL
# define FEAT_SUFFIX ""
#elif defined(FEAT_SMALL)
# define FEAT_SUFFIX "s"
#else
# define FEAT_SUFFIX "t"
#endif

#ifndef SYS_VIMRC_FILE
# define SYS_VIMRC_FILE         "$VIM\\vimrc" FEAT_SUFFIX
#endif
#ifndef USR_VIMRC_FILE
# define USR_VIMRC_FILE         "$HOME\\_vimrc"
#endif
#ifndef USR_VIMRC_FILE2
# define USR_VIMRC_FILE2        "$HOME\\vimfiles\\vimrc"
#endif
#ifndef USR_VIMRC_FILE3
# define USR_VIMRC_FILE3        "$VIM\\_vimrc"
#endif
#ifndef EVIM_FILE
# define EVIM_FILE              "$VIMRUNTIME\\evim.vim"
#endif

#ifndef USR_EXRC_FILE
# define USR_EXRC_FILE          "$HOME\\_exrc"
#endif
#ifndef USR_EXRC_FILE2
# define USR_EXRC_FILE2         "$VIM\\_exrc"
#endif

#ifdef FEAT_GUI
# ifndef SYS_GVIMRC_FILE
#  define SYS_GVIMRC_FILE       "$VIM\\gvimrc"
# endif
# ifndef USR_GVIMRC_FILE
#  define USR_GVIMRC_FILE       "$HOME\\_gvimrc"
# endif
# ifndef USR_GVIMRC_FILE2
#  define USR_GVIMRC_FILE2      "$HOME\\vimfiles\\gvimrc"
# endif
# ifndef USR_GVIMRC_FILE3
#  define USR_GVIMRC_FILE3      "$VIM\\_gvimrc"
# endif
# ifndef SYS_MENU_FILE
#  define SYS_MENU_FILE         "$VIMRUNTIME\\menu.vim"
# endif
#endif

#ifndef SYS_OPTWIN_FILE
# define SYS_OPTWIN_FILE        "$VIMRUNTIME\\optwin.vim"
#endif

#ifdef FEAT_VIMINFO
# ifndef VIMINFO_FILE
#  define VIMINFO_FILE          "$HOME\\_viminfo"
# endif
# ifndef VIMINFO_FILE2
#  define VIMINFO_FILE2         "$VIM\\_viminfo"
# endif
#endif

#ifndef VIMRC_FILE
# define VIMRC_FILE     "_vimrc"
#endif

#ifndef EXRC_FILE
# define EXRC_FILE      "_exrc"
#endif

#ifdef FEAT_GUI
# ifndef GVIMRC_FILE
#  define GVIMRC_FILE   "_gvimrc"
# endif
#endif

#ifndef DFLT_HELPFILE
# define DFLT_HELPFILE  "$VIMRUNTIME\\doc\\help.txt"
#endif

#ifndef FILETYPE_FILE
# define FILETYPE_FILE  "filetype.vim"
#endif
#ifndef FTPLUGIN_FILE
# define FTPLUGIN_FILE  "ftplugin.vim"
#endif
#ifndef INDENT_FILE
# define INDENT_FILE    "indent.vim"
#endif
#ifndef FTOFF_FILE
# define FTOFF_FILE     "ftoff.vim"
#endif
#ifndef FTPLUGOF_FILE
# define FTPLUGOF_FILE  "ftplugof.vim"
#endif
#ifndef INDOFF_FILE
# define INDOFF_FILE    "indoff.vim"
#endif

#ifndef SYNTAX_FNAME
# define SYNTAX_FNAME   "$VIMRUNTIME\\syntax\\%s.vim"
#endif

#ifndef DFLT_BDIR
# define DFLT_BDIR      ".,c:\\tmp,c:\\temp,$VIMRUNTIME\\tmp"    /* default for 'backupdir' */
#endif

#ifndef DFLT_VDIR
# define DFLT_VDIR      "$VIM\\vimfiles\\view"  /* default for 'viewdir' */
#endif

#ifndef DFLT_DIR
# define DFLT_DIR       ".,c:\\tmp,c:\\temp"    /* default for 'directory' */
#endif

#define DFLT_ERRORFILE          "errors.err"
#define DFLT_RUNTIMEPATH        "$HOME\\vimfiles,$VIM\\vimfiles,$VIMRUNTIME,$VIM\\vimfiles\\after,$HOME\\vimfiles\\after,C:\\VIM\\RT"
#define DFLT_VIMVAR             "C:\\VIM"
#define DFLT_RUNTIMEVAR         DFLT_VIMVAR "\\RT"

#define CASE_INSENSITIVE_FILENAME   /* ignore case when comparing file names */
#define SPACE_IN_FILENAME
#define BACKSLASH_IN_FILENAME
#define USE_CRNL                /* lines end in CR-NL instead of NL */
#define HAVE_DUP                /* have dup() */
#define HAVE_STDARG_H
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_GETCWD
#define HAVE_DIRENT
#define HAVE_SYS_TIME_H
#define HAVE_TYPES_H
#define HAVE_STAT_H
#define HAVE_ST_MODE
#define HAVE_UNISTD_H
#define HAVE_STRCSPN
#define HAVE_STRICMP
#define HAVE_STRFTIME
#define HAVE_STRNICMP
#define HAVE_MEMSET
#define HAVE_QSORT
#define HAVE_AVAIL_MEM
#define HAVE_SETENV
#if defined(__DATE__) && defined(__TIME__)
# define HAVE_DATE_TIME
#endif
#define USE_MCH_ACCESS
#define BINARY_FILE_IO
#define USE_EXE_NAME            /* use argv[0] for $VIM */
#define USE_TERM_CONSOLE
#define SHORT_FNAME             /* always 8.3 file name */
#define BREAKCHECK_SKIP     (32768)   /* we don't trap ctrl-c */
#define FNAME_ILLEGAL "\"*?><|" /* illegal characters in a file name */

/*
 * Try several directories to put the temp files.
 */
#define TEMPDIRNAMES    "$TMP", "$TEMP", "C:\\TMP", "C:\\TEMP", "$VIMRUNTIME\\TMP"
#define TEMPNAMELEN     PATH_MAX
#ifndef DFLT_MAXMEM
# define DFLT_MAXMEM    256             /* use up to 256Kbyte for buffer */
#endif
#ifndef DFLT_MAXMEMTOT
# define DFLT_MAXMEMTOT 0               /* decide in set_init */
#endif
#define BASENAMELEN     8               /* length of base of file name */

#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_STAT_H
# include <ext.h>
#endif

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

/*
 * Using getcwd() is preferred, because it checks for a buffer overflow.
 * Don't use getcwd() on systems do use system("sh -c pwd").  There is an
 * autoconf check for this.
 * Use getcwd() anyway if getwd() isn't present.
 */
#if defined(HAVE_GETCWD) && !(defined(BAD_GETCWD) && defined(HAVE_GETWD))
# define USE_GETCWD
#endif

#ifndef __ARGS
    /* The AIX VisualAge cc compiler defines __EXTENDED__ instead of __STDC__
     * because it includes pre-ansi features. */
# if defined(__STDC__) || defined(__GNUC__) || defined(__EXTENDED__)
#  define __ARGS(x) x
# else
#  define __ARGS(x) ()
# endif
#endif

#ifdef HAVE_DIRENT_H
# include <dirent.h>
# ifndef NAMLEN
#  define NAMLEN(dirent) strlen((dirent)->d_name)
# endif
#endif

#if !defined(HAVE_SYS_TIME_H) || defined(TIME_WITH_SYS_TIME)
# include <time.h>          /* on some systems time.h should not be
                               included together with sys/time.h */
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif

#include "os_tos_debug.h"

#define mch_rename rename
#define vim_mkdir(x, y) mkdir((char *)(x), y)
#define mch_rmdir(x) rmdir((char *)(x))
#define mch_remove(x) unlink((char *)(x))
#define mch_setenv(x, y, z) setenv((char *)(x), y, z)
