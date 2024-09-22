/* vi:ts=4 sts=4 sw=4:expandtab
 *
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifndef OS_TOS_DEBUG_H
#define OS_TOS_DEBUG_H

#if defined(TOS_DEBUG) && defined(TOS)
# if defined(NATFEATS)
#  include <mint/arch/nf_ops.h>
#  define TOSPRINT(...) nf_debugprintf(__VA_ARGS__)
# else
#  include <stdio.h>
   extern FILE *tos_trace;
#  define TOSPRINT(...) \
    do { \
        if (tos_trace != NULL) { \
            fprintf(tos_trace, __VA_ARGS__); \
            fflush(tos_trace); \
        } \
    } while (0);
# endif

# define contracev(fmt, ...) \
    do { \
        TOSPRINT("%-15s %-25s %-4d: ", __FILE__, __func__, __LINE__); \
        TOSPRINT(fmt, __VA_ARGS__); \
        TOSPRINT("\n"); \
    } while(0);
# define contrace(msg) TOSPRINT("%-15s %-25s %-4d: " msg "\n", __FILE__, __func__, __LINE__)

#else
# define contracev(fmt, ...)
# define contrace(msg)
#endif

#endif
