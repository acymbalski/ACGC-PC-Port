#ifndef M_TIMER_H
#define M_TIMER_H

#include "types.h"

#ifdef __cplusplus

/**
 * @brief Template timer class used by the Deluxe version.
 *
 * CTimer provides a simple normalized timer that interpolates a value from
 * 0 to 1 over a configurable length (in seconds at 60 fps). It can also run
 * in a cyclic (ping-pong) mode, oscillating between 0 and 1.
 *
 * @tparam T Numeric type for the timer value (typically f32).
 */
template <typename T>
class CTimer {
public:
    T mValue;  /* Current timer value (0.0 to 1.0) */
    T mStep;   /* Per-frame increment; sign encodes cycle direction */

    void reset(void);
    void updateLength(T length);
    void update(T length);
    void updateCycle(T length);
};

#endif /* __cplusplus */

#endif /* M_TIMER_H */
