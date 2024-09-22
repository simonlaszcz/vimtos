/* vi:ts=4 sts=4 sw=4:expandtab
 *
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifndef OS_TOS_VDO_H
#define OS_TOS_VDO_H

struct vdo_os
{
    float   magic_version;
    float   mint_version;
    float   geneva_version;
    int     is_singletos;
};

void vdo_early_init(void);
const struct vdo_os * const vdo_init(void);
void vdo_get_default_tsize(long *rows, long *cols);
void vdo_exit(void);

#endif
