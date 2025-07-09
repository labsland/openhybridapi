/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#ifndef LL_TIME_MANAGER_STD
#define LL_TIME_MANAGER_STD

#include "labsland/utils/timemanager.h"

namespace LabsLand::Utils {

    class TimeManagerStd : public TimeManager {
        public:
            // sleep a number of milliseconds
            virtual void sleepMs(uint32_t ms) const override;

            // sleep a number of microseconds
            virtual void sleepUs(uint32_t us) const override;

            // get time since boot in microseconds
            virtual clock_t getAbsoluteTime() const override;

            virtual uint64_t getClocksPerSec() const override;
    };

}

#endif
