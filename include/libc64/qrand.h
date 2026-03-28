#ifndef LQRAND_H
#define LQRAND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 qrand(void);
void sqrand(u32);
f32 fqrand(void);
f32 fqrand2(void);

#ifdef __cplusplus
}
#endif

#endif