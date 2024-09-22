/* vi:ts=4 sts=4 sw=4:expandtab
 *
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifndef OS_TOS_MAGIC_H
#define OS_TOS_MAGIC_H

/**
 * Types used by the MagiC AES (ATARI)
 */

#include <sys/types.h>

typedef struct
{
    __int8_t     *in_dos;                 /* Adress of the DOS semaphore  */
    __int16_t    *dos_time;               /* Adress of the DOS time       */
    __int16_t    *dos_date;               /* Adress of the DOS date       */
    __int32_t    res1;                    /*                              */
    __int32_t    res2;                    /*                              */
    __int32_t    res3;                    /* is 0L                        */
    void         *act_pd;                 /* Running program              */
    __int32_t    res4;                    /*                              */
    __int16_t    res5;                    /*                              */
    void         *res6;                   /*                              */
    void         *res7;                   /* Internal DOS memory list     */
    void         (*_resv_intmem)(void);        /* Extend DOS memory            */
    __int32_t    (*_etv_critic)(void);         /* etv_critic of the GEMDOS     */
    __int8_t     *((*_err_to_str)(__int8_t e)); /* Conversion code->plain text */
    void         *xaes_appls;             /*                              */
    void         *mem_root;               /*                              */
    void         *ur_pd;                  /*                              */
} DOSVARS;

typedef struct
{
    __int32_t magic;               /* Has to be 0x87654321       */
    void *membot;                  /* End of the AES variables   */
    void *aes_start;               /* Start address              */
    __int32_t magic2;              /* Is 'MAGX' or 'KAOS'        */
    __int32_t date;                /* Creation date              */
    void (*chgres)(__int16_t res, __int16_t txt);  /* Change resolution */
    __int32_t (**shel_vector)(void); /* Resident desktop           */
    __int8_t *aes_bootdrv;         /* Booting will be from here  */
    __int16_t *vdi_device;         /* Driver used by AES         */
    void *reservd1;                /* Reserved                   */
    void *reservd2;                /* Reserved                   */
    void *reservd3;                /* Reserved                   */
    __int16_t version;             /* Version (0x0201 is V2.1)   */
    __int16_t release;             /* 0=alpha..3=release         */
} AESVARS;

typedef struct
{
    __int32_t    config_status;
    DOSVARS      *dosvars;
    AESVARS      *aesvars;
    void         *res1;
    void         *hddrv_functions;
    __int32_t    status_bits;
} MAGX_COOKIE;

#endif
