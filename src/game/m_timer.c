#include "types.h"

#if VERSION >= VER_DELUXE

#include "m_timer.h"

/**
 * @brief Resets the timer to its initial state.
 *
 * Sets the current value and step to zero, effectively stopping and
 * rewinding the timer.
 */
template <typename T>
void CTimer<T>::reset(void) {
    mValue = (T)0;
    mStep = (T)0;
}

/**
 * @brief Recalculates the per-frame step for a given timer length.
 *
 * Computes the step as 1/(length*60) so the timer reaches 1.0 after
 * the specified number of seconds at 60 fps. If the length is zero or
 * negative, the timer is set to its completed state.
 *
 * When the length changes while the timer is in progress, the current
 * value is rescaled proportionally so the timer position remains
 * consistent relative to the new duration.
 *
 * @param length Duration of the timer in seconds.
 */
template <typename T>
void CTimer<T>::updateLength(T length) {
    T newStep;
    T oldStep;

    if (length <= (T)0) {
        mValue = (T)1;
        mStep = (T)0;
        return;
    }

    newStep = (T)1 / (length * (T)FRAMES_PER_SECOND);
    oldStep = mStep;

    if (oldStep != newStep) {
        /* Rescale current progress when step changes mid-timer */
        if (oldStep > (T)0 && mValue > (T)0 && mValue < (T)1) {
            mValue = mValue * (oldStep / newStep);
            if (mValue > (T)1) {
                mValue = (T)1;
            }
        }
        mStep = newStep;
    }
}

/**
 * @brief Advances the timer by one frame.
 *
 * First updates the step if the length has changed, then increments the
 * timer value by one step. Clamps the result at 1.0 when complete.
 *
 * @param length Duration of the timer in seconds.
 */
template <typename T>
void CTimer<T>::update(T length) {
    updateLength(length);

    mValue += mStep;
    if (mValue >= (T)1) {
        mValue = (T)1;
    }
}

/**
 * @brief Advances the timer in a cyclic (ping-pong) pattern.
 *
 * First updates the step if the length has changed, then increments the
 * timer value. When the value reaches 1.0 or 0.0, the step sign is
 * reversed, causing the timer to oscillate back and forth.
 *
 * @param length Duration of one half-cycle in seconds.
 */
template <typename T>
void CTimer<T>::updateCycle(T length) {
    updateLength(length);

    mValue += mStep;

    if (mValue >= (T)1) {
        mValue = (T)1;
        mStep = -mStep;
    } else if (mValue <= (T)0) {
        mValue = (T)0;
        mStep = -mStep;
    }
}

/* Force template instantiation for f32 to emit CTimer<f> symbols */
template class CTimer<f32>;

#endif /* VERSION >= VER_DELUXE */
