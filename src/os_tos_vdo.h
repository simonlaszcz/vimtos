#ifndef OS_TOS_VDO_H
#define OS_TOS_VDO_H

struct vdo_os
{
    float   magic_version;
    float   mint_version;
    float   geneva_version;
    int     is_singletos;
};

const struct vdo_os * const vdo_init(void);
void vdo_get_default_tsize(long *rows, long *cols);
void vdo_exit(void);

#endif
