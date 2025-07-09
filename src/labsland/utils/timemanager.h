/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#ifndef LL_TIME_MANAGER
#define LL_TIME_MANAGER

#include <atomic>

namespace LabsLand::Utils {

    typedef uint64_t clock_t;

    /**
     * In different platforms, the time is managed with different structures, and clock()
     * does not necessarily work in a C++ compatible way. For this reason, it is better to
     * rely on a TimeManager that abstracts us from this.
     */
    class TimeManager {
        public:
            // sleep a number of milliseconds
            virtual void sleepMs(uint32_t ms) const = 0;

            // sleep a number of microseconds
            virtual void sleepUs(uint32_t us) const = 0;

            // get time since boot in microseconds
            virtual clock_t getAbsoluteTime() const = 0;

            virtual uint64_t getClocksPerSec() const = 0;
    };

}

#endif
