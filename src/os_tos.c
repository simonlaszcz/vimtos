/* vi:set ts=4 sts=4 sw=4:
 *
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * os_tos.c
 *
 * TOS system-dependent routines.
 *
 */

#include "vim.h"
#include "proto.h"
#include <osbind.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dirent.h>
#include <mint/sysvars.h>
#include <mint/cookie.h>
#include <sys/types.h>
#include "os_tos_ext.h"
#include "os_tos_vdo.h"

/* 0x00ss00aa */
#define CODE(sc,ac) ((sc<<16)|ac)
#define SC_F1       CODE(59, 0)
#define SC_F2       CODE(61, 0)
#define SC_F3       CODE(62, 0)
#define SC_F4       CODE(63, 0)
#define SC_F5       CODE(64, 0)
#define SC_F6       CODE(65, 0)
#define SC_F7       CODE(66, 0)
#define SC_F8       CODE(67, 0)
#define SC_F9       CODE(68, 0)
#define SC_F10      CODE(69, 0)
#define SC_S_F1     CODE(84, 0)
#define SC_S_F2     CODE(85, 0)
#define SC_S_F3     CODE(86, 0)
#define SC_S_F4     CODE(87, 0)
#define SC_S_F5     CODE(88, 0)
#define SC_S_F6     CODE(89, 0)
#define SC_S_F7     CODE(90, 0)
#define SC_S_F8     CODE(91, 0)
#define SC_S_F9     CODE(92, 0)
#define SC_S_F10    CODE(93, 0)
#define SC_UP       CODE(72, 0)
#define SC_DOWN     CODE(80, 0)
#define SC_LEFT     CODE(75, 0)
#define SC_RIGHT    CODE(77, 0)
#define SC_UNDO     CODE(97, 0)
#define SC_HELP     CODE(98, 0)
#define SC_INSERT   CODE(82, 0)
#define SC_HOME     CODE(71, 0)

#define SC_S_UP     CODE(72, 0x38)
#define SC_S_DOWN   CODE(80, 0x32)
#define SC_S_LEFT   CODE(75, 0x34)
#define SC_S_RIGHT  CODE(77, 0x36)

static struct key_binding
{
    long code;
    char *bstr;
}
/* none of these codes have an ascii char in the lower byte 
we add '[' to prevent these codes being confused with VT52 codes */
key_bindings_1[] = {
    {SC_F1,     "\033[P"},
    {SC_F2,     "\033[Q"},
    {SC_F3,     "\033[R"},
    {SC_F4,     "\033[S"},
    {SC_F5,     "\033[T"},
    {SC_F6,     "\033[U"},
    {SC_F7,     "\033[V"},
    {SC_F8,     "\033[W"},
    {SC_F9,     "\033[X"},
    {SC_F10,    "\033[Y"},
    {SC_S_F1,   "\033[p"},
    {SC_S_F2,   "\033[q"},
    {SC_S_F3,   "\033[r"},
    {SC_S_F4,   "\033[s"},
    {SC_S_F5,   "\033[t"},
    {SC_S_F6,   "\033[u"},
    {SC_S_F7,   "\033[v"},
    {SC_S_F8,   "\033[w"},
    {SC_S_F9,   "\033[x"},
    {SC_S_F10,  "\033[y"},
    {SC_UP,     "\033[A"},
    {SC_DOWN,   "\033[B"},
    {SC_LEFT,   "\033[D"},
    {SC_RIGHT,  "\033[C"},
    {SC_UNDO,   "u"},
    {SC_HELP,   "\033:help\r"},
    {SC_INSERT, "\033I"},
    {SC_HOME,   "H"},
    {0, 0}
},
/* these codes have an ascii char in the lower byte */
key_bindings_2[] = {
    {SC_S_UP,   "\033[a"},
    {SC_S_DOWN, "\033[b"},
    {SC_S_LEFT, "\033[d"},
    {SC_S_RIGHT,"\033[c"},
    {0, 0}
};

union ftime_long
{
    struct ftime ft;
    long l;
};

typedef long (__attribute__((cdecl)) *shell_call_t)(char *);

#define EXEFILE     (S_IFREG|S_IEXEC)
#define FOREVER     (-1)
#define PSTRSEP     ','

static int is_executable(char_u *name);
static int is_pathsep(char_u c);
static int is_relative(char_u *name);
static int pstrcmp(const void *a, const void *b);
static int sort_keybinding_desc(const void *va, const void *vb);
static int super_call_shell();
static int super_init(void);
static int super_restore(void);
static int tos_expandpath(garray_T *gap, char_u *path, int wildoff, int flags, int didstar);
static int WaitForChar(long msec);
static long to_unix_time(long tos_dt);
static void sort_keybinding_table(struct key_binding *t);

static struct
{
    char    startup_conterm;
    char_u  *shell_cmd;
    const struct vdo_os *os;
} g;

#if defined(TOS_DEBUG) && !defined(NATFEATS)
FILE *tos_trace = NULL;
#endif

void 
mch_early_init(void)
{
#if defined(TOS_DEBUG) && !defined(NATFEATS)
    tos_trace = fopen("C:\\TRACE.TXT", "w");
#endif
    memset(&g, 0, sizeof(g));
    g.os = vdo_init();
    Supexec(super_init);
    sort_keybinding_table(key_bindings_1);
    sort_keybinding_table(key_bindings_2);
}

static void
sort_keybinding_table(struct key_binding *t)
{
    int len = 0;
    for (int i = 0; t[i].code != 0; ++i)
        ++len;
    qsort(t, len, sizeof(struct key_binding), sort_keybinding_desc);
}

static int
sort_keybinding_desc(const void *va, const void *vb)
{
    struct key_binding *a = (struct key_binding *)va;
    struct key_binding *b = (struct key_binding *)vb;
    return b->code - a->code;
}

static int
super_init(void)
{
    /* disable keyclick as it's never in sync with buffered screen output */
    g.startup_conterm = *conterm;
    *conterm &= 0xFE;
    return 0;
}

static int
super_restore(void)
{
    *conterm = g.startup_conterm;
    return 0;
}

void 
mch_breakcheck(void)
{
    /* do nothing */
}

void 
mch_exit(int r)
{
    contrace();
    Supexec(super_restore);
    ml_close_all(TRUE);
    vdo_exit();
#if defined(TOS_DEBUG) && !defined(NATFEATS)
    fclose(tos_trace);
#endif
    exit(r);
}

void 
mch_init(void)
{
    vdo_get_default_tsize(&Rows, &Columns);
    limit_screen_size();
    out_flush();
}

int 
mch_get_shellsize(void)
{
    char *erows = getenv("LINES");
    char *ecols = getenv("COLUMNS");
    Rows = Columns = 0;

    if (erows != NULL && ecols != NULL) {
        Rows = atoi(erows);
        Columns = atoi(ecols);
    }

    if (Rows < 1 || Columns < 1)
        vdo_get_default_tsize(&Rows, &Columns);
    limit_screen_size();

    return OK;
}

void 
mch_new_shellsize(void)
{
    /* do nothing */
}

void 
mch_set_shellsize(void)
{
    /* do nothing */
}

#ifdef FEAT_MOUSE
void
mch_setmouse(int on UNUSED)
{
}
#endif

/**
 * Return free memory in kilobytes
 * 'special' indicates that primary/ST ram will be required. The Amiga implementation
 * interprets this as memory that can be addressed by all chips
 */
long_u
mch_avail_mem(int special)
{
    return ((long)Malloc(-1)) >> 10;
}

int 
mch_call_shell(char_u *cmd, int options)
{
    /* options will include SHELL_COOKED but we can't set tmode */
    contracev("options=%d, cmd=%s", options, cmd);
    out_flush();
    g.shell_cmd = cmd;
    return (int)Supexec(super_call_shell);
}

static int
super_call_shell()
{
    long *sp = _shell_p;
    shell_call_t f = (shell_call_t)*sp;

    if (*sp == 0) {
        EMSG(_(e_no_shell));
        return FAIL;
    }

    return (int)f(g.shell_cmd);
}

/**
 * Return 1 if "name" can be found in $PATH and executed, 0 if not.
 * Return -1 if unknown.
 */
int
mch_can_exe(char_u *name, char_u **path)
{
    char_u *buf;
    char_u *p, *e;
    int retval;
    slash_adjust(name);

    /* If it's an absolute or relative path don't need to use $PATH. */
    if (mch_isFullName(name) || is_relative(name)) {
        if (is_executable(name)) {
            if (path != NULL) {
                if (name[0] == '.')
                    /* n.b. mch_FullName gets called */
                    *path = FullName_save(name, TRUE);
                else
                    *path = vim_strsave(name);
            }
            retval = TRUE;
            goto ret_known;
        }
        retval = FALSE;
        goto ret_known;
    }

    p = mch_getenv("PATH");
    if (p == NULL || *p == NUL)
        goto ret_unknown;
    buf = alloc((unsigned)(STRLEN(name) + STRLEN(p) + 2));
    if (buf == NULL)
        goto ret_unknown;

    /*
     * Walk through all entries in $PATH to check if "name" exists there and
     * is an executable file.
     */
    for (;;) {
        e = (char_u *)strchr((char *)p, PSTRSEP);
        if (e == NULL)
            e = p + STRLEN(p);
        if (e - p <= 1) {
            /* empty entry means current dir */
            STRCPY(buf, "./");
        }
        else {
            vim_strncpy(buf, p, e - p);
            add_pathsep(buf);
        }
        STRCAT(buf, name);
        retval = is_executable(buf);
        if (retval == 1) {
            if (path != NULL) {
                if (buf[0] == '.')
                    *path = FullName_save(buf, TRUE);
                else
                    *path = vim_strsave(buf);
            }
            break;
        }

        if (*e != PSTRSEP)
            break;
        p = e + 1;
    }

    vim_free(buf);
ret_known:
#ifdef TOS_DEBUG
    if (retval) {
        contracev("YES %s", *path);
    }
    else {
        contracev("NO %s", name);
    }
#endif
    return retval;

ret_unknown:
    contracev("UNKNOWN %s", name);
    return -1;
}

int 
mch_chdir(char *path)
{
    slash_adjust(path);

    if (chdir(path) == 0) {
        contracev("OK %s", path);
        return 0;
    }

    contracev("FAIL %s", path);
    return -1;
}

int 
mch_check_win(int argc, char **argv)
{
#ifdef TOS_DEBUG
    int i;
    for (i = 0; i < argc; ++i)
        contracev("i=%d, argv=%s", i, argv[i]);
#endif
     /*
      * Store argv[0], could be used for $VIM.  Only use it if it is an absolute
      * name, mostly it's just "vim" and found in the path, which is unusable.
      */
    if (argc > 0) {
        if (mch_isFullName(argv[0]))
            exe_name = vim_strsave((char_u *)argv[0]);
        contracev("exe_name=%s", exe_name);
    }

    return isatty(fileno(stdout)) ? OK : FAIL;
}

void 
mch_delay(long msec, int ignoreinput)
{
    /* delay uses timer C which has a 5ms granularity, so round up */
    long cycles = (msec + 4) / 5;
    contracev("msec=%d, ignoreinput=%d, cycles=%d", msec, ignoreinput, cycles);

    if (ignoreinput) {
        delay(cycles * 5);
        return;
    }

    /* delay until char available or timeout */
    while (Cconis() != -1 && cycles-- > 0)
        delay(5);
}

int 
mch_dirname(char_u *buf, int len)
{
    char_u *p = NULL;

    buf[0] = NUL;
    if (getcwd(buf, len) == NULL)
        goto ret_fail;
    buf[len - 1] = NUL;

    if ((p = strchr(buf, NUL)) == NULL)
        goto ret_fail;
    /* zero length? */
    if (p == buf)
        goto ret_fail;

    if (!is_pathsep(p[-1])) {
        /* enough space for a slash? */
        if ((p - buf) == len)
            goto ret_fail;
        *p++ = '\\';
        *p = NUL;
    }

    contracev("OK %s", buf);
    return OK;
ret_fail:
    contracev("FAIL %s", buf);
    return FAIL;
}

/*
 * Return TRUE if "fname" does not depend on the current directory.
 */
int 
mch_isFullName(char_u *fname)
{
    if (fname == NULL || fname[0] == NUL)
        goto ret_false;
    if (!((fname[0] >= 'A' && fname[0] <= 'Z') || (fname[0] >= 'a' && fname[0] <= 'z')))
        goto ret_false;
    if (fname[1] != ':')
        goto ret_false;
    if (!is_pathsep(fname[2]))
        goto ret_false;

    contracev("TRUE %s", fname);
    return TRUE;
ret_false:
    contracev("FALSE %s", fname);
    return FALSE;
}

/**
 * Convert fname to a full name and put it in buf
 * force: also expand when already absolute path
 */
int 
mch_FullName(char_u *fname, char_u *buf, int len, int force)
{
    char tmp[MAXPATH];
    char *cwd = NULL;
    char *bn = NULL;
    vim_memset(buf, 0, len);
    contrace();

    /* we might have a path like C:_.swp, insert a slash while copying */
    if (fname[0] != NUL && fname[1] == ':' && !is_pathsep(fname[2])) {
        tmp[0] = fname[0];
        tmp[1] = fname[1];
        tmp[2] = '\\';
        vim_strncpy(&tmp[3], &fname[2], MAXPATH - 3);
    }
    else 
        vim_strncpy(tmp, fname, MAXPATH);
    tmp[MAXPATH - 1] = NUL;
    slash_adjust(tmp);

    if (!force && mch_isFullName(tmp) == TRUE) {
        vim_strncpy(buf, tmp, len);
        buf[len - 1] = NUL;
        goto ret_ok;
    }

    if ((cwd = getcwd(NULL, len - 1)) == NULL)
        goto ret_fail;
    if ((bn = basename(tmp)) == NULL)
        goto ret_fail;

    if (bn > tmp) {
        bn[-1] = NUL;
        if (chdir(tmp) != 0)
            goto ret_fail;
        /* ensure we have space for a slash and the basename */
        if (getcwd(buf, len - strlen(bn) - 2) == NULL)
            goto ret_fail;
        strcat(buf, "\\");
        strcat(buf, bn);
    }
    else {
        /* fname has no path so use the cwd */
        int n = snprintf(buf, len, "%s\\%s", cwd, bn);
        if (n < 0 || n >= len)
            goto ret_fail;
    }

ret_ok:
    contracev("OK force=%d, fname=%s, base=%s, cwd=%s", force, fname, bn, cwd);
    if (cwd != NULL)
        chdir(cwd);
    free(cwd);
    return OK;
ret_fail:
    contracev("FAIL force=%d, fname=%s, base=%s, cwd=%s", force, fname, bn, cwd);
    if (cwd != NULL)
        chdir(cwd);
    free(cwd);
    return FAIL;
}

void 
mch_get_host_name(char_u *s, int len)
{
    vim_strncpy(s, "ATARI TOS", len - 1);
    s[len - 1] = NUL;
}

long 
mch_get_pid(void)
{
    return 0;
}

int 
mch_get_user_name(char_u *s, int len)
{
    char *user = mch_getenv("USER");
    
    if (user != NULL) {
        vim_strncpy(s, user, len - 1);
        s[len - 1] = NUL;
        return OK;
    }

    *s = NUL;
    return FAIL;
}

#define ENV_VIM_RT  "VIMRUNTIME"
#define ENV_VIM     "VIM"
char_u *
mch_getenv(char_u *name)
{
    char *val = getenv(name);

    if (val == NULL) { 
        if (STRCMP(name, ENV_VIM_RT) == 0) {
            setenv(ENV_VIM_RT, DFLT_RUNTIMEVAR, 1);
            val = getenv(ENV_VIM_RT);
        }
        else if (STRCMP(name, ENV_VIM) == 0) {
            setenv(ENV_VIM, DFLT_VIMVAR, 1);
            val = getenv(ENV_VIM);
        }
    }

    contracev("%s=%s", name, val);
    return val;
}

/*
 * Get file permissions for 'name'.
 * Returns -1 when it doesn't exist.
 */
long
mch_getperm(char_u *name)
{
    struct stat s;

    if (mch_stat(name, &s) != 0)
        goto ret_err;

    contracev("OK %s st_mode=%d", name, s.st_mode);
    return s.st_mode;
ret_err:
    contracev("FAIL %s not found", name);
    return -1;
}

int 
mch_setperm(char_u *name, long perm)
{
    /* don't bother */
    return OK;
}

void 
mch_hide(char_u *name)
{
    /* don't bother */
}

/*
 * Expand a path into all matching files and/or directories.
 * Returns the number of matches found.
 */
int 
mch_expandpath(garray_T *gap, char_u *path, int flags)
{
    slash_adjust(path);
    int c = tos_expandpath(gap, path, 0, flags, FALSE);

#ifdef TOS_DEBUG
    int i;
    char_u **p = (char_u**)gap->ga_data;
    contracev("flags=%d, path=%s", flags, path);

    for (i = 0; i < c; ++i) {
        contracev("gap %d=%s", i, p[i]);
    }
#endif
    
    return c;
}

/**
 * Return TRUE if "p" contain a wildcard that mch_expandpath() can expand
 * i.e. by FsFirst/FsNext
 */
int
mch_has_exp_wildcard(char_u *p)
{
    for ( ; *p; mb_ptr_adv(p))
        if (vim_strchr((char_u *)"?*", *p) != NULL)
            goto ret_true;

    contracev("FALSE %s", p);
    return FALSE;
ret_true:
    contracev("TRUE %s", p);
    return TRUE;
}

/**
 * Return TRUE if "p" contain any kind of wildcard 
 * that can be expanded
 */
int
mch_has_wildcard(char_u *p)
{
    for ( ; *p; mb_ptr_adv(p))
        if (vim_strchr((char_u *)"?*$", *p) != NULL)
            goto ret_true;

    contracev("FALSE %s", p);
    return FALSE;
ret_true:
    contracev("TRUE %s", p);
    return TRUE;
}

/*
 * mch_inchar(): low level input function.
 * Get a characters from the keyboard.
 * If time == 0 do not wait for characters.
 * If time == n wait a short time for characters.
 * If time == -1 wait forever for characters.
 *
 * return the number of characters obtained
 */
int
mch_inchar(char_u *buf, int maxlen, long time, int tb_change_cnt)
{
    static char *pscan = NULL;
    int len = 0;

    /* we don't explicity check for CTRL-C here. We're using raw input and
    ex_getcmdln will check for CTRL-C in the input buffer */

    /* if we previously ran out of buffer then carry on */
    if (pscan != NULL && *pscan != NUL) {
        while (len < maxlen && *pscan != NUL) {
            *buf++ = *pscan++;
            ++len;
        }

        return len;
    }

    if (time >= 0) {
        if (WaitForChar(time) == 0) {
            /* no character available */
            return 0;
        }
    }
    else {
        /*
         * If there is no character available within 2 seconds (default)
         * write the autoscript file to disk.  Or cause the CursorHold event
         * to be triggered.
         */
        if (WaitForChar(p_ut) == 0) {
#ifdef FEAT_AUTOCMD
            if (trigger_cursorhold() && maxlen >= 3) {
                buf[0] = K_SPECIAL;
                buf[1] = KS_EXTRA;
                buf[2] = (int)KE_CURSORHOLD;
                return 3;
            }
#endif
            before_blocking();
        }
    }

    /* wait for key or mouse click */
    WaitForChar(FOREVER);

    /*
     * Try to read as many characters as there are, until the buffer is full.
     */
    while ((len == 0 || (Cconis() == -1)) && len < maxlen) {
        long cin = Crawcin();
        int asc = cin & 0xFF;
        int matched = 0;
        struct key_binding *keytab = (asc > 0) ? key_bindings_2 : key_bindings_1;

        for (int i = 0;; ++i) {
            if (keytab[i].code == 0 || cin > keytab[i].code)
                break;
            if (keytab[i].code == cin) {
#if defined(TOS_DEBUG) && defined(TRACE_INPUT)
                contracev("matched scan code %d, %s", cin, keytab[i].bstr);
#endif
                matched = 1;
                pscan = keytab[i].bstr;
                while (len < maxlen && *pscan != NUL) {
                    *buf++ = *pscan++;
                    ++len;
                }
                break;
            }
        }

        if (matched)
            return len;

        if (asc > 0) {
            *buf = asc;
            ++buf;
            ++len;
        }
    }

    return len;
}

/**
 * Wait for a character to become available
 * 0 immediate
 * FOREVER
 * or msec
 */
static int
WaitForChar(long msec)
{
    /* delay uses timer C which has a 5ms granularity */
    long cycles = msec / 5;

    for (;;) {
        if (Cconis() == -1)
            return TRUE;
        if (msec == 0 || (msec != FOREVER && cycles < 1))
            return FALSE;
        delay(5);
        --cycles;
    }
}

int
mch_char_avail()
{
    return Cconis() == -1;
}

int 
mch_input_isatty(void)
{
    return isatty(fileno(stdin));
}

int 
mch_isdir(char_u *name)
{
    struct stat s;

    if (name == NULL || *name == NUL)
        goto ret_false;
    if (mch_stat(name, &s) != 0)
        goto ret_false;
    if ((s.st_mode & S_IFDIR) != S_IFDIR)
        goto ret_false;

    contracev("TRUE %s", name);
    return TRUE;
ret_false:
    contracev("FALSE %s, st_mode=%d", name, s.st_mode);
    return FALSE;
}

/*
 * Check what "name" is:
 * NODE_NORMAL: file or directory (or doesn't exist)
 * NODE_WRITABLE: writable device, socket, fifo, etc.
 * NODE_OTHER: non-writable things
 */
int 
mch_nodetype(char_u *name)
{
    if (STRICMP(name, "AUX:") == 0 || STRICMP(name, "CON:") == 0 || STRICMP(name, "PRN:") == 0)
    	return NODE_WRITABLE;
    return NODE_NORMAL;
}

int 
mch_screenmode(char_u *arg)
{
    /* always fail */
    return FAIL;
}

void 
mch_settmode(int tmode)
{
    /* do nothing */
}

void 
mch_suspend(void)
{
    suspend_shell();
}

void 
mch_write(char_u *s, int len)
{
#if defined(TOS_DEBUG) && defined(TRACE_OUTPUT)
    int c = 0;
    for (; c < len; ++c)
        contracev("|%3d %c|", s[c], s[c]);
#endif

    Fwrite(fileno(stdout), len, s);
}

/*
 * Replace all slashes by backslashes.
 * When 'shellslash' set do it the other way around.
 */
void
slash_adjust(char_u *p)
{
#ifdef TOS_DEBUG
    char_u *o = p;
#endif

    while (p && *p) {
        if (*p == psepcN)
            *p = psepc;
        mb_ptr_adv(p);
    }

#ifdef TOS_DEBUG
    contracev("%s", o);
#endif
}

int
mch_open(char *filename, int access, ...)
{
    va_list args;
    int fd;

    slash_adjust(filename);
    va_start(args, access);
    fd = open(filename, access, args);
    va_end(args);

    contracev("filename=%s, access=%d, fd=%d, errno=%d", filename, access, fd, errno);
    return fd;
}

int
mch_stat(char *path, struct stat *buff)
{
    char obuf[MAXPATH];
    int rv = stat(sanepath(obuf, path, FALSE), buff);

    if (rv == 0) {
        /* need to convert from TOS to UNIX time */
        buff->st_mtime = to_unix_time(buff->st_mtime);
        buff->st_atime = to_unix_time(buff->st_atime);
        buff->st_ctime = to_unix_time(buff->st_ctime);
    }

    return rv;
}

static long
to_unix_time(long tos_dt)
{
    union ftime_long un;
    struct tm *t;

    un.l = tos_dt;
    t = ftimtotm(&un.ft);

    return mktime(t);
}

static int 
is_pathsep(char_u c)
{
    return c == '\\' || c == '/';
}

static int
is_relative(char_u *name)
{
    if (name == NULL || *name != '.')
        goto ret_false;

    if (!(is_pathsep(name[1]) || (name[1] == '.' && is_pathsep(name[2]))))
        goto ret_false;

    contracev("TRUE %s", name);
    return TRUE;
ret_false:
    contracev("FALSE %s", name);
    return FALSE;
}

static int
is_executable(char_u *name)
{
    struct stat st;

    if (name == NULL || *name == NUL)
        goto ret_false;
    if (mch_stat(name, &st) != 0)
        goto ret_false;
    if ((st.st_mode & EXEFILE) != EXEFILE)
        goto ret_false;

    contracev("TRUE %s", name);
    return TRUE;
ret_false:
    contracev("FALSE %s", name);
    return FALSE;
}

/*
 * Recursively expand one path component into all matching files and/or
 * directories.  Adds matches to "gap".  Handles "*", "?", "[a-z]", "**", etc.
 * "path" has backslashes before chars that are not to be expanded, starting
 * at "path + wildoff".
 * Return the number of matches found.
 * NOTE: much of this is identical to dos_expandpath(), keep in sync!
 */
static int
tos_expandpath(garray_T *gap, char_u *path, int wildoff, int flags, int didstar)
{
    char_u      *buf;
    char_u      *path_end;
    char_u      *p, *s, *e;
    int         start_len = gap->ga_len;
    char_u      *pat;
    regmatch_T  regmatch;
    int         starts_with_dot;
    int         matches;
    int         len;
    int         starstar = FALSE;
    static int  stardepth = 0;      /* depth for "**" expansion */

    DIR         *dirp;
    struct dirent *dp;

    /* Expanding "**" may take a long time, check for CTRL-C. */
    if (stardepth > 0)
    {
        ui_breakcheck();
        if (got_int)
            return 0;
    }

    /* make room for file name */
    buf = alloc((int)STRLEN(path) + BASENAMELEN + 5);
    if (buf == NULL)
        return 0;

    /*
     * Find the first part in the path name that contains a wildcard.
     * When EW_ICASE is set every letter is considered to be a wildcard.
     * Copy it into "buf", including the preceding characters.
     */
    p = buf;
    s = buf;
    e = NULL;
    path_end = path;
    while (*path_end != NUL)
    {
        /* May ignore a wildcard that has a backslash before it; it will
         * be removed by rem_backslash() or file_pat_to_reg_pat() below. */
        if (path_end >= path + wildoff && rem_backslash(path_end))
            *p++ = *path_end++;
        else if (is_pathsep(*path_end))
        {
            if (e != NULL)
                break;
            s = p + 1;
        }
        else if (path_end >= path + wildoff
                         && (vim_strchr((char_u *)"*?[{~$", *path_end) != NULL
                             || (!p_fic && (flags & EW_ICASE)
                                             && isalpha(PTR2CHAR(path_end)))))
            e = p;
#ifdef FEAT_MBYTE
        if (has_mbyte)
        {
            len = (*mb_ptr2len)(path_end);
            STRNCPY(p, path_end, len);
            p += len;
            path_end += len;
        }
        else
#endif
            *p++ = *path_end++;
    }
    e = p;
    *e = NUL;

    /* Now we have one wildcard component between "s" and "e". */
    /* Remove backslashes between "wildoff" and the start of the wildcard
     * component. */
    for (p = buf + wildoff; p < s; ++p)
        if (rem_backslash(p))
        {
            STRMOVE(p, p + 1);
            --e;
            --s;
        }

    /* Check for "**" between "s" and "e". */
    for (p = s; p < e; ++p)
        if (p[0] == '*' && p[1] == '*')
            starstar = TRUE;

    /* convert the file pattern to a regexp pattern */
    starts_with_dot = (*s == '.');
    pat = file_pat_to_reg_pat(s, e, NULL, FALSE);
    if (pat == NULL)
    {
        vim_free(buf);
        return 0;
    }

    /* compile the regexp into a program */
    if (flags & EW_ICASE)
        regmatch.rm_ic = TRUE;          /* 'wildignorecase' set */
    else
        regmatch.rm_ic = p_fic; /* ignore case when 'fileignorecase' is set */
    if (flags & (EW_NOERROR | EW_NOTWILD))
        ++emsg_silent;
    regmatch.regprog = vim_regcomp(pat, RE_MAGIC);
    if (flags & (EW_NOERROR | EW_NOTWILD))
        --emsg_silent;
    vim_free(pat);

    if (regmatch.regprog == NULL && (flags & EW_NOTWILD) == 0)
    {
        vim_free(buf);
        return 0;
    }

    /* If "**" is by itself, this is the first time we encounter it and more
     * is following then find matches without any directory. */
    if (!didstar && stardepth < 100 && starstar && e - s == 2
                                                          && is_pathsep(*path_end))
    {
        STRCPY(s, path_end + 1);
        ++stardepth;
        (void)tos_expandpath(gap, buf, (int)(s - buf), flags, TRUE);
        --stardepth;
    }

    /* open the directory for scanning */
    *s = NUL;
    dirp = opendir(*buf == NUL ? "." : (char *)buf);

    /* Find all matching entries */
    if (dirp != NULL)
    {
        for (;;)
        {
            dp = readdir(dirp);
            if (dp == NULL)
                break;
            if ((dp->d_name[0] != '.' || starts_with_dot)
                 && ((regmatch.regprog != NULL && vim_regexec(&regmatch,
                                             (char_u *)dp->d_name, (colnr_T)0))
                   || ((flags & EW_NOTWILD)
                     && fnamencmp(path + (s - buf), dp->d_name, e - s) == 0)))
            {
                STRCPY(s, dp->d_name);
                len = STRLEN(buf);

                if (starstar && stardepth < 100)
                {
                    /* For "**" in the pattern first go deeper in the tree to
                     * find matches. */
                    STRCPY(buf + len, "\\**");
                    STRCPY(buf + len + 3, path_end);
                    ++stardepth;
                    (void)tos_expandpath(gap, buf, len + 1, flags, TRUE);
                    --stardepth;
                }

                STRCPY(buf + len, path_end);
                if (mch_has_exp_wildcard(path_end)) /* handle more wildcards */
                {
                    /* need to expand another component of the path */
                    /* remove backslashes for the remaining components only */
                    (void)tos_expandpath(gap, buf, len + 1, flags, FALSE);
                }
                else
                {
                    /* no more wildcards, check if there is a match */
                    /* remove backslashes for the remaining components only */
                    if (*path_end != NUL)
                        backslash_halve(buf + len + 1);
                    if (mch_getperm(buf) >= 0)  /* add existing file */
                    {
#ifdef MACOS_CONVERT
                        size_t precomp_len = STRLEN(buf)+1;
                        char_u *precomp_buf =
                            mac_precompose_path(buf, precomp_len, &precomp_len);

                        if (precomp_buf)
                        {
                            mch_memmove(buf, precomp_buf, precomp_len);
                            vim_free(precomp_buf);
                        }
#endif
                        addfile(gap, buf, flags);
                    }
                }
            }
        }

        closedir(dirp);
    }

    vim_free(buf);
    vim_regfree(regmatch.regprog);

    matches = gap->ga_len - start_len;
    if (matches > 0)
        qsort(((char_u **)gap->ga_data) + start_len, matches,
                                                   sizeof(char_u *), pstrcmp);
    return matches;
}

/*
 * comparison function for qsort in dos_expandpath()
 */
static int
pstrcmp(const void *a, const void *b)
{
    return (pathcmp(*(char **)a, *(char **)b, -1));
}
