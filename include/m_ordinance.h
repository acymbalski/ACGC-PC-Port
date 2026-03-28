#ifndef M_ORDINANCE_H
#define M_ORDINANCE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Ordinance flags byte layout (in Save_t at offset 0x241A8):
 *   bits [7:6] = time_shift (0=none, 1=Early Bird, 2=Night Owl)
 *   bit  [5]   = bell_boom
 *   bit  [4]   = beautiful_town
 *   bits [3:0] = reserved
 */

#define mOR_TIME_SHIFT_NONE  0
#define mOR_TIME_SHIFT_EARLY 1
#define mOR_TIME_SHIFT_LATE  2
#define mOR_TIME_SHIFT_MAX   3

#define mOR_COST 50000 /* bells to enact an ordinance */

extern int mOR_GetTimeShift(void);
extern int mOR_GetBellBoom(void);
extern int mOR_GetBeautiful(void);
extern void mOR_SetTimeShift(int value);
extern void mOR_SetBellBoom(int value);
extern void mOR_SetBeautiful(int value);
extern int mOR_IsActive(void);

#ifdef __cplusplus
}
#endif

#endif
