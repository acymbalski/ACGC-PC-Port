#ifndef GLIBC_COMPAT_H
#define GLIBC_COMPAT_H

/* Force sscanf and strtol to older GLIBC versions to avoid GLIBC_2.38 dependency
 * when building on modern hosts (like Ubuntu 24.04).
 * This file is forced-included via CMake. */

#if defined(__linux__) && !defined(__ANDROID__)
#if defined(__arm__)
/* Both the standard name and the new ISOC23 variant (emitted by modern GCC/glibc)
 * must be redirected to the old versioned symbol. */
__asm__(".symver sscanf,sscanf@GLIBC_2.4");
__asm__(".symver __isoc23_sscanf,sscanf@GLIBC_2.4");

__asm__(".symver strtol,strtol@GLIBC_2.4");
__asm__(".symver __isoc23_strtol,strtol@GLIBC_2.4");
#elif defined(__i386__)
__asm__(".symver sscanf,sscanf@GLIBC_2.7");
__asm__(".symver __isoc23_sscanf,sscanf@GLIBC_2.7");

__asm__(".symver strtol,strtol@GLIBC_2.0");
__asm__(".symver __isoc23_strtol,strtol@GLIBC_2.0");
#endif
#endif

#endif /* GLIBC_COMPAT_H */
