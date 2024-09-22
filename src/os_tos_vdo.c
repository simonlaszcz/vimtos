/*
 * vi:ts=4 sts=4 sw=4:expandtab
 *
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"
#include <osbind.h>
#include <stdlib.h>
#include <mint/sysvars.h>
#include <mint/cookie.h>
#include <sys/types.h>
#include "os_tos_magic.h"
#include "os_tos_geneva.h"
#include "os_tos_vdo.h"

#define VsetScreen(laddr, paddr, mode) \
    (void)trap_14_wllww(5, laddr, paddr, 3, mode)
#define VsetMode(mode) \
    (short)trap_14_ww(88, mode)
#define mon_type() \
    (short)trap_14_w(89)
#define VgetSize(mode) \
    (long)trap_14_ww(91, mode)
#define VsetRGB(index, count, array) \
    (void)trap_14_wwwl(93, index, count, array)
#define VgetRGB(index, count, array) \
    (void)trap_14_wwwl(94, index, count, array)

#define RGB333(r,g,b) \
    (__uint16_t)((((r) & 0x7) << 8) | (((g) & 0x7) << 4) | ((b) & 0x7))
#define RGB888(r,g,b) \
    (__uint32_t)((((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF))

#define MAX_NUM_COLORS  (16)

#define VDO_ST          (0)
#define VDO_STE         (1)
#define VDO_TT          (2)
#define VDO_FALCON      (3)
#define VDO_MAX         (3)

#define COM_ST          (1)
#define COM_TT          (2)
#define COM_FALCON      (4)
#define COM_ALL_MCH     (COM_ST|COM_TT|COM_FALCON)
#define COM_ST_MONO     (8)
#define COM_ST_COLOR    (16)
#define COM_TT_MONO     (32)
#define COM_TT_VGA      (64)
#define COM_FALCON_VGA  (128)
#define COM_FALCON_TV   (256)
#define COM_MASK_MCH    (COM_ALL_MCH)
#define COM_MASK_MON    (COM_ST_MONO|COM_ST_COLOR|COM_TT_MONO|COM_TT_VGA|COM_FALCON_VGA|COM_FALCON_TV)
#define COM_MASK_COLOR  (COM_ST_COLOR|COM_TT_VGA|COM_FALCON_VGA|COM_FALCON_TV)

#define REZ_ST_LOW      (0)
#define REZ_ST_MED      (1)
#define REZ_ST_HI       (2)
#define REZ_FALCON      (3)
#define REZ_TT_MED      (4)
#define REZ_TT_HI       (6)
#define REZ_TT_LOW      (7)
/* Falcon mode flags */
#define M_IN            (1<<8)
#define M_LD            (1<<8)
#define M_ST            (1<<7)
#define M_OV            (1<<6)
#define M_PAL           (1<<5)
#define M_VGA           (1<<4)
#define M_80            (1<<3)
#define M_1P            (0)
#define M_2P            (1)
#define M_4P            (2)
/* we propogate these flags */
#define M_FIXED         (M_PAL)
/* used to match the current rez with our defs */
#define M_MATCH         (M_IN|M_ST|M_OV|M_VGA|M_80)

static struct rezdef
{
    char *name;
    int rez;
    int rows;
    int cols;
    int num_colors;
    int compat;
    /* Falcon mode */
    int mode;
    /* vram_size for falcon modes is calculated in validate_rezdefs */
    int vram_size;
    int valid;
} 
rezdefs[] = 
{
    {"st-low",  REZ_ST_LOW, 25, 40, 16, COM_ALL_MCH|COM_ST_COLOR|COM_TT_VGA|COM_FALCON_VGA, 0, 32000},
    {"st-med",  REZ_ST_MED, 25, 80, 4,  COM_ALL_MCH|COM_ST_COLOR|COM_TT_VGA|COM_FALCON_VGA, 0, 32000},
    {"st-hi",   REZ_ST_HI,  25, 80, 2,  COM_ALL_MCH|COM_ST_MONO |COM_TT_VGA|COM_FALCON_VGA, 0, 32000},
    {"tt-low",  REZ_TT_LOW, 30, 40, 16, COM_TT|COM_TT_VGA,  0, 153600},
    {"tt-med",  REZ_TT_MED, 30, 80, 16, COM_TT|COM_TT_VGA,  0, 153600},
    {"tt-hi",   REZ_TT_HI,  60, 160,2,  COM_TT|COM_TT_MONO, 0, 153600},
    {"320x200", REZ_FALCON, 25, 40, 16, COM_FALCON|COM_ST_COLOR,        M_4P},
    {"384x240", REZ_FALCON, 30, 48, 16, COM_FALCON|COM_FALCON_TV,       M_OV|M_4P},
    {"320x400", REZ_FALCON, 25, 40, 16, COM_FALCON|COM_ST_COLOR,        M_IN|M_4P},
    {"384x480", REZ_FALCON, 30, 48, 16, COM_FALCON|COM_FALCON_TV,       M_OV|M_IN|M_4P},
    {"320x480", REZ_FALCON, 30, 40, 16, COM_FALCON|COM_FALCON_VGA,      M_VGA|M_4P},
    {"320x240", REZ_FALCON, 30, 40, 16, COM_FALCON|COM_FALCON_VGA,      M_LD|M_VGA|M_4P},
    {"640x200", REZ_FALCON, 25, 80, 16, COM_FALCON|COM_ST_COLOR,        M_80|M_4P},
    {"768x240", REZ_FALCON, 30, 96, 16, COM_FALCON|COM_FALCON_TV,       M_OV|M_80|M_4P},
    {"640x400", REZ_FALCON, 25, 80, 16, COM_FALCON|COM_ST_COLOR,        M_80|M_IN|M_4P},
    {"768x480", REZ_FALCON, 30, 96, 16, COM_FALCON|COM_FALCON_TV,       M_OV|M_80|M_IN|M_4P},
    {"640x480", REZ_FALCON, 30, 80, 16, COM_FALCON|COM_FALCON_VGA,      M_80|M_VGA|M_4P},
    {"640x240", REZ_FALCON, 30, 80, 16, COM_FALCON|COM_FALCON_VGA,      M_80|M_LD|M_VGA|M_4P},
    {NULL}
},
/* used when we can't match the startup rez. avoids a null pointer */
rezinvalid = {"unknown",  REZ_ST_LOW, 25, 40};

static float decode_bcd_version(int v);
static float get_magic_version(void);
static float get_mint_version(void);
static float get_geneva_version(void);
static struct rezdef *match_rezdef(int rez, int mode);
static int change_resolution(int rez, int mode);
static void restore_resolution(void);
static void init_colormap(void);
static int init_palvim(void);
static int init_paltos(void);
static void validate_rezdefs(void);
static char *get_machine_name(void);
static struct rezdef *get_rez_by_name(char *name);
static int save_palette(void);
static void restore_palette(void);

#define SCRATCH_BUFSZ   (60)

static struct
{
    int     was_initialised;
    short   startup_kbrate;
    /* from cookie _VDO */
    long    vdo;
    /* display compatibility flags */
    int     compat;
    int     startup_rez;
    int     startup_mode;
    /* bits from the startup Falcon mode that we propagate */
    int     mode_fixed;
    /* number of colors in saved_palette */
    int     saved_palette_ncolors;
    /* the palette is saved on the first rez change or palvim call. 
    elements will be 16 or 32 bit depending on startup rez and mode */
    void    *saved_palette;
    void    *startup_physbase;
    void    *startup_logbase;
    /* max bytes required for all compatible display modes */
    int     max_vram_size;
    /* set when we change rez of the Falcon */
    void    *vram;
    /* current rez */
    struct rezdef *rezdef;
    int     colormap[MAX_NUM_COLORS];
    enum
    {
        PALETTE_NONE,
        PALETTE_TOS,
        PALETTE_VIM
    }       active_palette;
    void    *palvim;
    void    *paltos;
    struct vdo_os os;
    /* temp buffer for strings */
    char    scratch[SCRATCH_BUFSZ];
} g;

void
vdo_early_init(void)
{
    memset(&g, 0, sizeof(g));
}

const struct vdo_os * const
vdo_init(void)
{
    int rez = 0;
    int mode = 0;
    g.was_initialised = TRUE;
    g.startup_kbrate = Kbrate(-1, -1);
    g.rezdef = &rezinvalid;

    if (Getcookie(C__VDO, &g.vdo) == C_NOTFOUND)
        g.vdo = VDO_ST;
    else
        g.vdo >>= 16;

    g.startup_rez = rez = Getrez();
    g.startup_physbase = Physbase();
    g.startup_logbase = Logbase();

    /* disable mouse events */
    char kbs[] = {0x12};
    Ikbdws(0, kbs);

    /* set some flags that determine which modes are valid */
    switch (g.vdo) {
    case VDO_FALCON:
        g.startup_mode = mode = VsetMode(-1);
        g.mode_fixed = mode & M_FIXED;
        g.compat = COM_FALCON;
        switch (mon_type()) {
        case 0:
            g.compat |= COM_ST_MONO;
            break;
        case 1:
            g.compat |= COM_ST_COLOR;
            break;
        case 3:
            g.compat |= (COM_ST_COLOR | COM_FALCON_TV);
            break;
        default:
            g.compat |= COM_FALCON_VGA;
            break;
        }
        break;
    case VDO_TT:
        g.compat = COM_TT;
        g.compat |= (rez == REZ_TT_HI ? COM_TT_MONO : COM_TT_VGA);
        break;
    default:
        g.compat = COM_ST;
        g.compat |= (rez == REZ_ST_HI ? COM_ST_MONO : COM_ST_COLOR);
        break;
    }
    contracev("rez=%d, mode=%d, compat=%d", rez, mode, g.compat);

    validate_rezdefs();
    if ((g.rezdef = match_rezdef(rez, mode)) == &rezinvalid)
        EMSG(_(e_rez_unknown));
    init_colormap();
    (void)save_palette();

    static char *os_names[] = {"TOS", "MagiC", "MiNT", "Geneva"};
    g.os.magic_version = get_magic_version();
    g.os.mint_version = get_mint_version();
    g.os.geneva_version = get_geneva_version();
    g.os.is_singletos = g.os.magic_version == 0 && g.os.mint_version == 0 && g.os.geneva_version == 0;
#ifdef FEAT_EVAL
    int os_idx = 
        g.os.magic_version  != 0 ? 1 : 
        g.os.mint_version   != 0 ? 2 : 
        g.os.geneva_version != 0 ? 3 : 0;
    set_vim_var_string(VV_OS, os_names[os_idx], -1);
    set_vim_var_string(VV_MACHINE, get_machine_name(), -1);
    set_vim_var_string(VV_RESOLUTION, g.rezdef->name, -1);
#endif

    return &g.os;
}

static void
validate_rezdefs(void)
{
    g.max_vram_size = 0;

    for (int i = 0; rezdefs[i].name != NULL; ++i) {
        struct rezdef *r = &rezdefs[i];
        r->valid = FALSE;

        if ((r->compat & COM_MASK_MCH) & (g.compat & COM_MASK_MCH))
            if ((r->compat & COM_MASK_MON) & (g.compat & COM_MASK_MON))
                r->valid = TRUE;

        if (r->valid && r->mode > 0 && g.vdo == VDO_FALCON) {
            /* propogate these flags (e.g. PAL/NTSC) to all modes */
            r->mode |= g.mode_fixed;
            r->vram_size = VgetSize(r->mode);

            if (r->vram_size > g.max_vram_size)
                g.max_vram_size = r->vram_size;
        }

        contracev("rez %s, valid=%d, mode=%d, vram_size=%d", r->name, r->valid, r->mode, r->vram_size);
    }
}

static struct rezdef *
match_rezdef(int rez, int mode)
{
    /*  When matching Falcon modes, ignore the number of color planes.
        We'll switch to 16 colors when palvim() is called */
    if (g.vdo == VDO_FALCON && mode > 0) {
        int my_mode = mode & M_MATCH;

        for (int i = 0; rezdefs[i].name != NULL; ++i) {
            struct rezdef *r = &rezdefs[i];

            if (!(r->valid && r->mode > 0))
                continue;
            if (my_mode == (r->mode & M_MATCH))
                return r;
        }
    }

    for (int i = 0; rezdefs[i].name != NULL; ++i) {
        struct rezdef *r = &rezdefs[i];

        if (!r->valid)
            continue;
        if (r->rez == rez)
            return r;
    }

    return &rezinvalid;
}

static struct rezdef *
get_rez_by_name(char *name)
{
    strlwr(name);

    for (int i = 0; rezdefs[i].name != NULL; ++i)
        if (strcmp(rezdefs[i].name, name) == 0 && rezdefs[i].valid)
            return &rezdefs[i];

    return &rezinvalid;
}

static int
get_ncolors_for_mode(int mode)
{
    switch (mode & 0x7) {
    case 0:     return 2;
    case 1:     return 4;
    case 2:     return 16;
    case 3:     return 256;
    default:    return 65536;
    }
}

static int
save_palette(void)
{
    if (g.saved_palette != NULL)
        return OK;

    if (g.vdo == VDO_FALCON) {
        /* n.b we only ever change the first 16 colors so that's the maximum we'll save */
        g.saved_palette_ncolors = get_ncolors_for_mode(VsetMode(-1));
        if (g.saved_palette_ncolors > MAX_NUM_COLORS)
            g.saved_palette_ncolors = MAX_NUM_COLORS;
        if ((g.saved_palette = malloc(sizeof(__int32_t) * g.saved_palette_ncolors)) == NULL) {
            EMSG(_(e_no_ram));
            return FAIL;
        }
        VgetRGB(0, g.saved_palette_ncolors, g.saved_palette);
        return OK;
    }

    /* the current rez might not be using all colors but the palette has a fixed size */
    g.saved_palette_ncolors = MAX_NUM_COLORS;
    if ((g.saved_palette = malloc(sizeof(__int16_t) * g.saved_palette_ncolors)) == NULL) {
        EMSG(_(e_no_ram));
        return FAIL;
    }

    __int16_t *p = (__int16_t *)g.saved_palette;
    for (int i = 0; i < g.saved_palette_ncolors; ++i)
        p[i] = Setcolor(i, COL_INQUIRE);

    return OK;
}

static void
restore_palette(void)
{
    if (g.saved_palette == NULL)
        return;

    if (g.vdo == VDO_FALCON)
        VsetRGB(0, g.saved_palette_ncolors, g.saved_palette);
    else
        Setpalette(g.saved_palette);
}

void 
vdo_get_default_tsize(long *rows, long *cols)
{
    *cols = g.rezdef->cols;
    *rows = g.rezdef->rows;
}

int
vdo_get_max_colors(void)
{
    return (g.compat & COM_MASK_COLOR) ? 16 : 1;
}

void 
vdo_exit(void)
{
    if (!g.was_initialised)
        return;

    /* enable mouse relative reporting */
    char kbs[] = {8};
    Ikbdws(0, kbs);
    Kbrate(g.startup_kbrate >> 8, g.startup_kbrate & 0xFF);
    restore_resolution();
    restore_palette();
    free(g.vram);
    free(g.saved_palette);
    free(g.palvim);
    free(g.paltos);
    g.vram = NULL;
    g.saved_palette = NULL;
    g.palvim = NULL;
    g.paltos = NULL;
    g.active_palette = PALETTE_NONE;
}

void
vdo_sane(char_u *arg UNUSED)
{
    restore_resolution();
    restore_palette();
    free(g.vram);
    free(g.palvim);
    free(g.paltos);
    g.vram = NULL;
    g.palvim = NULL;
    g.paltos = NULL;
    g.active_palette = PALETTE_NONE;

    if ((g.rezdef = match_rezdef(g.startup_rez, g.startup_mode)) == &rezinvalid)
        EMSG(_(e_rez_unknown));
#ifdef FEAT_EVAL
    set_vim_var_string(VV_RESOLUTION, g.rezdef->name, -1);
#endif
    init_colormap();
    set_shellsize(g.rezdef->cols, g.rezdef->rows, TRUE);
}

void
vdo_kbrate(char_u *arg)
{
    int initial = -1;
    int repeat = -1;
    contracev("arg=%s", arg);

    if (arg == NULL || *arg == NUL) {
        initial = Kbrate(-1, -1);
        repeat = initial & 0xFF;
        initial >>= 8;
        snprintf(g.scratch, SCRATCH_BUFSZ, "Repeat after %d ticks then every %d", initial, repeat);
        g.scratch[SCRATCH_BUFSZ - 1] = NUL;
        MSG(g.scratch);
        return;
    }

    if (sscanf(arg, "%d %d", &initial, &repeat) != 2) {
        EMSG(_(e_bad_kbrate));
        return;
    }

    contracev("%d, %d", initial, repeat);
    if (initial > -1 || repeat > -1)
        Kbrate(initial, repeat);
}

int
vdo_remap_colornum(int n)
{
    /* since the border color is taken from palette[0], we may have done a swap */
    if (n > -1 && n < MAX_NUM_COLORS) 
        return g.colormap[n];
    /* out of range so assume it's the fg color */
    return g.colormap[MAX_NUM_COLORS - 1];
}

char_u *
vdo_iter_resolutions(int idx)
{
    /* use our own index as we may skip entries */
    static int i = 0;

    if (!g.os.is_singletos)
        return NULL;

    if (idx == 0)
        i = 0;

    while (rezdefs[i].name != NULL && (!rezdefs[i].valid || g.rezdef == &rezdefs[i]))
        ++i;

    if (rezdefs[i].name == NULL)
        return NULL;

    return rezdefs[i++].name;
}

void
vdo_resolution(char_u *arg)
{
    if (arg == NULL || *arg == NUL) {
        MSG(_(g.rezdef->name));
        return;
    }

    if (!g.os.is_singletos) {
        EMSG(_(e_not_multitos));
        return;
    }

    struct rezdef *p = get_rez_by_name(arg);

    if (p == &rezinvalid || !p->valid) {
        EMSG(_(e_bad_rez));
        return;
    }

    if (p == g.rezdef) {
        EMSG(_(e_no_change));
        return;
    }

    if (save_palette() == FAIL)
        return;
    if (change_resolution(p->rez, p->mode) == FAIL)
        return;

    g.rezdef = p;
#ifdef FEAT_EVAL
    set_vim_var_string(VV_RESOLUTION, p->name, -1);
#endif
    set_shellsize(p->cols, p->rows, TRUE);
    if (g.active_palette == PALETTE_VIM)
        vdo_palvim(NULL);
    else
        vdo_paltos(NULL);
}

static int
change_resolution(int rez, int mode)
{
    if (rez == REZ_FALCON) {
        if (g.vram == NULL) {
            if ((g.vram = malloc(g.max_vram_size)) == NULL) {
                EMSG(_(e_no_ram));
                return FAIL;
            }
            VsetScreen(g.vram, g.vram, mode);
        }
        else
            VsetScreen(-1, -1, mode);
    }
    else
        Setscreen(-1, -1, rez);

    return OK;
}

static void
restore_resolution(void)
{
    if (g.vdo == VDO_FALCON)
        VsetScreen(g.startup_logbase, g.startup_physbase, g.startup_mode);
    else
        Setscreen(-1, -1, g.startup_rez);
}

/**
 * called when a color scheme is reloaded
 */
void
vdo_init_bg(void)
{
    /* may be called via init_highlight -> load_colors */
    static int recursive = FALSE;

    if (recursive)
        return;

    recursive = TRUE;
    if (g.active_palette == PALETTE_VIM)
        vdo_palvim(NULL);
    else
        vdo_paltos(NULL);
    recursive = FALSE;
}

void
vdo_palvim(exarg_T *eap UNUSED)
{
    if (!g.os.is_singletos) {
        EMSG(_(e_not_multitos));
        return;
    }

    if (save_palette() == FAIL)
        return;

    /* on the Falcon, use more colors if we can */
    if (g.vdo == VDO_FALCON && get_ncolors_for_mode(VsetMode(-1)) < g.rezdef->num_colors)
        if (change_resolution(g.rezdef->rez, g.rezdef->mode) == FAIL)
            return;

    if (init_palvim() == FAIL)
        return;
    g.active_palette = PALETTE_VIM;
    init_colormap();

    int bg = 0;
    int fg = 0;
    hl_get_normal_cterm_colors(&bg, &fg);
    /* when same (usually zero), colors haven't been initialised yet */
    if (bg < 0)
        bg = 0;
    if (fg < 0)
        fg = 0;
    if (bg > 15)
        bg = 15;
    if (fg > 15)
        fg = 15;

    if (bg != fg) {
        /* the atari border color is taken from palette[0] so we do a swap
        so that the vim bg color matches the border color */
        g.colormap[bg] = 0;
        g.colormap[0] = bg;

        if (g.rezdef->mode > 0) {
            __int32_t *pal32 = (__int32_t *)g.palvim;
            __int32_t tmp = pal32[bg];
            pal32[bg] = pal32[0];
            pal32[0] = tmp;
        }
        else {
            __int16_t *pal16 = (__int16_t *)g.palvim;
            __int16_t tmp = pal16[bg];
            pal16[bg] = pal16[0];
            pal16[0] = tmp;
        }
    }

    /* always change the palette */
    if (g.rezdef->mode > 0)
        VsetRGB(0, MAX_NUM_COLORS, g.palvim);
    else
        Setpalette(g.palvim);
    init_highlight(TRUE, TRUE);
}

void
vdo_paltos(exarg_T *eap UNUSED)
{
    if (!g.os.is_singletos) {
        EMSG(_(e_not_multitos));
        return;
    }

    if (save_palette() == FAIL)
        return;
    if (init_paltos() == FAIL)
        return;
    g.active_palette = PALETTE_TOS;
    init_colormap();

    /* always change the palette */
    if (g.rezdef->mode > 0)
        VsetRGB(0, MAX_NUM_COLORS, g.paltos);
    else
        Setpalette(g.paltos);
    init_highlight(TRUE, TRUE);
}

/**
 * init our palette data in vim color number order
 */
static int
init_palvim(void)
{
    if (g.palvim == NULL) {
        /* malloc the maximum we might need */
        size_t sz = g.vdo == VDO_FALCON ? sizeof(__int32_t) : sizeof(__int16_t);
        if ((g.palvim = malloc(MAX_NUM_COLORS * sz)) == NULL) {
            EMSG(_(e_no_ram));
            return FAIL;
        }
    }

    int i = 0;

    if (g.rezdef->mode > 0) {
        __int32_t *pal32 = (__int32_t *)g.palvim;
        pal32[i++] = 0;                         /* Black */
        pal32[i++] = RGB888(0,    0,    0x80);  /* DarkBlue */
        pal32[i++] = RGB888(0,    0x80, 0);     /* DarkGreen */
        pal32[i++] = RGB888(0,    0x80, 0x80);  /* DarkCyan */
        pal32[i++] = RGB888(0x80, 0,    0);     /* DarkRed */
        pal32[i++] = RGB888(0x80, 0,    0x80);  /* DarkMagenta */
        pal32[i++] = RGB888(0x80, 0x80, 0);     /* Brown */
        pal32[i++] = RGB888(0xC0, 0xC0, 0xC0);  /* LightGray */
        pal32[i++] = RGB888(0x80, 0x80, 0x80);  /* DarkGray */
        pal32[i++] = RGB888(0,    0,    0xFF);  /* Blue */
        pal32[i++] = RGB888(0,    0xFF, 0);     /* Green */
        pal32[i++] = RGB888(0,    0xFF, 0xFF);  /* Cyan */
        pal32[i++] = RGB888(0xFF, 0,    0);     /* Red */
        pal32[i++] = RGB888(0xFF, 0,    0xFF);  /* Magenta */
        pal32[i++] = RGB888(0xFF, 0xFF, 0);     /* Yellow */
        pal32[i++] = RGB888(0xFF, 0xFF, 0xFF);  /* White */

        return OK;
    }

    __int16_t *pal16 = (__int16_t *)g.palvim;
    pal16[i++] = 0;                     /* Black */
    if (g.rezdef->num_colors == 16) {
        pal16[i++] = RGB333(0, 0, 3);   /* DarkBlue */
        pal16[i++] = RGB333(0, 3, 0);   /* DarkGreen */
        pal16[i++] = RGB333(0, 3, 3);   /* DarkCyan */
        pal16[i++] = RGB333(3, 0, 0);   /* DarkRed */
        pal16[i++] = RGB333(3, 0, 3);   /* DarkMagenta */
        pal16[i++] = RGB333(3, 3, 0);   /* Brown */
        pal16[i++] = RGB333(5, 5, 5);   /* LightGray */
        pal16[i++] = RGB333(3, 3, 3);   /* DarkGray */
        pal16[i++] = RGB333(0, 0, 7);   /* Blue */
        pal16[i++] = RGB333(0, 7, 0);   /* Green */
        pal16[i++] = RGB333(0, 7, 7);   /* Cyan */
        pal16[i++] = RGB333(7, 0, 0);   /* Red */
        pal16[i++] = RGB333(7, 0, 7);   /* Magenta */
        pal16[i++] = RGB333(7, 7, 0);   /* Yellow */
    }
    else if (g.rezdef->num_colors == 4) {
        pal16[i++] = RGB333(0, 3, 3);   /* DarkCyan */
        pal16[i++] = RGB333(7, 0, 7);   /* Magenta */
    }
    pal16[i++] = RGB333(7, 7, 7);       /* White */

    return OK;
}

/**
 * init the TOS palette if changing resolutions with native colors
 */
static int
init_paltos(void)
{
    if (g.paltos == NULL) {
        /* malloc the maximum we might need */
        size_t sz = g.vdo == VDO_FALCON ? sizeof(__int32_t) : sizeof(__int16_t);
        if ((g.paltos = malloc(MAX_NUM_COLORS * sz)) == NULL) {
            EMSG(_(e_no_ram));
            return FAIL;
        }
    }

    int i = 0;

    if (g.rezdef->mode > 0) {
        __int32_t *pal32 = (__int32_t *)g.paltos;
        pal32[i++] = 4095;
        pal32[i++] = 3840;
        pal32[i++] = 240;
        pal32[i++] = 4080;
        pal32[i++] = 15;
        pal32[i++] = 3855;
        pal32[i++] = 255;
        pal32[i++] = 1365;
        pal32[i++] = 819;
        pal32[i++] = 3891;
        pal32[i++] = 1011;
        pal32[i++] = 4083;
        pal32[i++] = 831;
        pal32[i++] = 3903;
        pal32[i++] = 1023;
        pal32[i++] = 0;
        return OK;
    }

    __int16_t *pal16 = (__int16_t *)g.paltos;
    pal16[i++] = 4095;
    if (g.rezdef->num_colors == 16) {
        pal16[i++] = 3840;
        pal16[i++] = 240;
        pal16[i++] = 4080;
        pal16[i++] = 15;
        pal16[i++] = 3855;
        pal16[i++] = 255;
        pal16[i++] = 1365;
        pal16[i++] = 819;
        pal16[i++] = 3891;
        pal16[i++] = 1011;
        pal16[i++] = 4083;
        pal16[i++] = 831;
        pal16[i++] = 3903;
        pal16[i++] = 1023;
    }
    else if (g.rezdef->num_colors == 4) {
        pal16[i++] = 3840;
        pal16[i++] = 240;
    }
    pal16[i++] = 0;

    return OK;
}

static void
init_colormap(void)
{
    /* TOS: white=0, dark red, dark green, dark yellow, dark blue, dark magenta, dark cyan, light grey,
    dark grey, red, green, yellow, blue, magenta, cyan, black=15
    */
    static int tos16[] = {15,4,2,6,1,5,3,7,8,12,10,14,9,13,11,0};
    /* TOS: white=0, red=1, green=2, black=3 */
    static int tos4[]  = {3,3,2,2,1,1,1,2,2,2,2,2,1,1,2,0};
    /* mapping the vim palette to 4 color mode */
    static int vim4[] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3};

    switch (g.rezdef->num_colors) {
    case 16:
        if (g.active_palette == PALETTE_VIM) {
            for (int i = 0; i < MAX_NUM_COLORS; ++i)
                g.colormap[i] = i;
        }
        else
            memcpy(g.colormap, tos16, sizeof(tos16));
        break;
    case 4:
        if (g.active_palette == PALETTE_VIM)
            memcpy(g.colormap, vim4, sizeof(vim4));
        else
            memcpy(g.colormap, tos4, sizeof(tos4));
        break;
    case 2:
        for (int i = 0; i < MAX_NUM_COLORS; ++i)
            g.colormap[i] = 1;
        break;
    }
}

/**
 * The user has given us a custom palette->vim mapping
 */
void
vdo_palmap(char_u *arg)
{
    if (arg == NULL) {
        EMSG(_(e_bad_palmap));
        return;
    }

    strlwr(arg);
    if (strcmp(arg, "off") == 0) {
        init_colormap();
        init_highlight(TRUE, TRUE);
        return;
    }

    int m[MAX_NUM_COLORS];
    int n = 0;

    while (*arg != NUL && n < MAX_NUM_COLORS) {
        m[n] = hex2nr(*arg);

        if (m[n] < 0 || m[n] > 15) {
            EMSG(_(e_bad_palmap));
            return;
        }

        ++n;
        ++arg;
    }

    if (n != MAX_NUM_COLORS || *arg != NUL) {
        EMSG(_(e_bad_palmap));
        return;
    }

    memcpy(g.colormap, m, sizeof(m));
    init_highlight(TRUE, TRUE);
}

static char *
get_machine_name(void)
{
    static char *names[] = {"unknown", "ST", "STe", "TT", "Falcon"};

    if (g.vdo > -1 && g.vdo <= VDO_MAX)
        return names[g.vdo + 1];

    return names[0];
}

static float
get_magic_version(void)
{
    MAGX_COOKIE *cv;

    if (Getcookie(C_MagX, (long *)&cv) == C_NOTFOUND)
        return 0;
    /* if aesvars is null, we were started from AUTO and aes is not ready */
    if (cv->aesvars == NULL)
        return -1;

    return decode_bcd_version(cv->aesvars->version);
}

static float
get_mint_version(void)
{
    long v;

    if (Getcookie(C_MiNT, &v) == C_NOTFOUND)
        return 0;

    return decode_bcd_version(v);
}

static float
get_geneva_version(void)
{
    /* cookie value points to a struct where the first member is the version */
    G_COOKIE *cv;

    if (Getcookie(C_Gnva, (long *)&cv) == C_NOTFOUND)
        return 0;
    if (cv == NULL)
        return 0;

    return decode_bcd_version(cv->ver);
}

static float
decode_bcd_version(int v)
{
    float major = (v >> 8) & 0xFF;
    float minor = v & 0xFF;

    while (minor > 0)
        minor /= 10.0;

    return major + minor;
}
