/* vi:ts=4 sts=4 sw=4:expandtab
 *
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifndef OS_TOS_GENEVA_H
#define OS_TOS_GENEVA_H

#include <sys/types.h>

typedef struct G_vectors        /* Release 004 */
{
  __int16_t used;
  __int16_t (*keypress)( __int32_t *key );
  __int16_t (*app_switch)( __int8_t *process_name, __int16_t apid );
  __int16_t (*gen_event)(void);
} G_VECTORS;

typedef struct
{
  __int16_t ver;
  __int8_t *process_name;
  __int16_t apid;
  __int16_t (**aes_funcs)();
  __int16_t (**xaes_funcs)();
  struct G_vectors *vectors;    /* Release 004 */
} G_COOKIE;

#endif
