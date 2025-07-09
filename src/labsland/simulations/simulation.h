/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#ifndef HYBRIDAPI_SIMULATION_H
#define HYBRIDAPI_SIMULATION_H

#include <iostream>
#include <map>
#include <vector>
#include <memory>

#include "../utils/timemanager.h"
#include "utils/communicator.h"
#include "targetdevice.h"

/**
 * Each output data type (from the simulation perspective) must have a serialize() method.
 * Also, it must be a struct and it must NOT contain dynamic memory (so no "string" but char buffer[], etc.).
 */
struct BaseOutputDataType {
    virtual std::string serialize() const = 0;
};

/**
 * Each input data type (from the simulation perspective) must have a deserialize(string) method.
 * Also, it must be a struct and it must NOT contain dynamic memory (so no "string" but char buffer[], etc.).
 */
struct BaseInputDataType {
    /*
     * Load the string in the struct. Return true if it succeeded, false if it did not.
     */
    virtual bool deserialize(std::string const & input) = 0;

    /*
     * Given a string with the following formats, creates the map that appears on the right:
     *
     *    ""                 => {}
     *    "foo=bar"          => {"foo": "bar"}
     *    "foo&bar"          => {"foo": "1", "bar": "1"}
     *    "foo=1&bar=2"      => {"foo": "1", "bar", "2"}
     *    "foo=FOO&bar=baz"  => {"foo": "FOO", "bar": "baz"}
     */
    std::map<std::string, std::string> parseQueryArgs(std::string const & input) {
        std::map<std::string, std::string> result;

        std::string currentVariable; // always things like "foo=bar"
        std::string inputRemainder(input);

        while(inputRemainder.size() > 0) {
            size_t posAnd = inputRemainder.find('&');
            if (posAnd == std::string::npos) {
                currentVariable = inputRemainder;                    
                inputRemainder = "";
            } else {
                currentVariable = inputRemainder.substr(0, posAnd);
                inputRemainder.erase(0, posAnd + 1);
            }

            size_t posEquals = currentVariable.find('=');
            if (posEquals == std::string::npos) {
                // foo&bar means that foo=1 and bar=1
                result[currentVariable] = "1";
            } else {
                result[currentVariable.substr(0, posEquals)] = currentVariable.substr(posEquals+1, currentVariable.size());
            }
        };

        return result;
    }

    /*
     * Given a map like the one explained in the previous function, return if all the required variables
     * are present or not.
     */
    bool checkVariablesInArgs(std::map<std::string, std::string> const & args, std::vector<std::string> const requiredVariables) {
        for (std::string requiredVariable : requiredVariables)
            if (args.count(requiredVariable) == 0)
                return false;
        return true;
    }
};


template <class OutputDataType, class InputDataType>
class Simulation
{
    static_assert(std::is_base_of<BaseOutputDataType, OutputDataType>::value, "OutputDataType must inherit from BaseOutputDataType");
    static_assert(std::is_base_of<BaseInputDataType, InputDataType>::value, "InputDataType must inherit from BaseInputDataType");

    private:

        // How often (seconds to wait) should the state of the simulation be reported to the virtual environment.
        float mVirtualEnvironmentReportPeriod = 0.5;

        // If this mode is enabled, the simulation will have to explicitly indicate that a new state should be
        // reported to the virtual environment when possible. By default it is disabled, and the simulation state
        // is reported continuously. When it is enabled, it can be marked through the requestReportState() API.
        bool mReportWhenMarked = false;

        LabsLand::Utils::clock_t mLastUpdate;
        LabsLand::Utils::clock_t mLastReportUpdate;

    protected:

        std::shared_ptr<LabsLand::Utils::TimeManager> timeManager = nullptr;
        std::shared_ptr<LabsLand::Utils::TargetDevice> targetDevice = nullptr;
        std::weak_ptr<LabsLand::Simulations::Utils::SimulationCommunicator<OutputDataType, InputDataType>> communicator;
        bool mShouldReportInReportWhenMarkedMode = false;

        /**
         * Retrieves a request or interaction from the user interface, if any is available. If one was available
         * then the function returns true and will have copied the data to the structure. Otherwise it will return false
         * and copy nothing. Retrieved items are removed from the queue and never returned again.
         *
         * If your simulation does sleeps and the user interface sends many requests fast, some requests might be lost.
         *
         * For this reason, design your simulation to send the full state in each request instead of only diffs.
         *
         * @return if a request has been read.
         */
        bool readRequest(InputDataType &request) {
            if (auto comm = this->communicator.lock()) {
                return comm->readRequest(request);
            }
            return false;
        }

        /**
         * Enables and disables the report when marked mode.
         * @param reportWhenMarked
         */
        void setReportWhenMarked(bool reportWhenMarked) {
            mReportWhenMarked = reportWhenMarked;
        }

        /**
         * Gets the report when marked mode state.
         * @return
         */
        bool getReportWhenMarked() {
            return mReportWhenMarked;
        }

        /**
         * Sets the virtual environment report period.
         * @param period Period to set it to, in seconds.
         */
        void setVirtualEnvironmentReportPeriod(float period) {
            mVirtualEnvironmentReportPeriod = period;
        }

        /**
         * Retrieves the currently set virtual environment report period.
         * @return
         */
        float getVirtualEnvironmentReportPeriod() {
            return mVirtualEnvironmentReportPeriod;
        }

        /**
         * Queues the current state to be sent to the user interface. This should normally NOT be called
         * explicitly by the specific simulation developer; it should be called periodically on its own, from their
         * perspective.
         */
        void sendVirtualEnvironmentReport() {
            if (auto comm = this->communicator.lock())
                comm->sendReport(mState);
        }

        /**
         * This only takes an effect in the report-when-marked mode. In this mode, the simulation can notify that the
         * state of the simulation is ready to be reported to the virtual environment by calling this method. When called,
         * the report will not be sent straightaway, but it will be queued in the next report cycle.
         */
        void requestReportState() {
            mShouldReportInReportWhenMarkedMode = true;
        }

    public:

        /**
         * The part of the simulation-specific state that will be serialized periodically and transmitted to the
         * virtual environment. This state may be changed freely by the simulation. The state will only be propagated
         * periodically to the virtual environment, not after every single change.
         *
         * Note: Should probably be protected, but it is public for debugging purposes.
         */
        OutputDataType mState;

        /**
         *
         * Time works different in different embedded platforms and not necessarily compatible with standard C++. 
         * The way we handle this is by relying on a TimeManager, and later injecting a Time Manager compatible with
         * the target platform.
         */
        void injectTimeManager(std::shared_ptr<LabsLand::Utils::TimeManager> timeManager) {
            this->timeManager = timeManager;
        }

        /**
         *
         * communications are very dependant on the implementation. We abstract them in a different
         * class, so we could have implementations in files, network or anything else.
         */
        void injectCommunicator(std::weak_ptr<LabsLand::Simulations::Utils::SimulationCommunicator<OutputDataType, InputDataType>> communicator) {
            this->communicator = communicator;
        }

        /**
         * Provide the target device that is going to use in the simulation
         */
        void injectTargetDevice(std::shared_ptr<LabsLand::Utils::TargetDevice> targetDevice) {
            this->targetDevice = targetDevice;
        }

        /*
        LabsLand::Simulations::Utils::SimulationCommunicator<OutputDataType, InputDataType> * getCommunicator() {
            return this->communicator;
        }
        */

        /**
         * This internal method is meant to be invoked externally as often as possible, and it is not meant to be
         * invoked by specific simulation developers. Calling the method will ensure that the update() and reportUpdate()
         * methods be invoked periodically.
         */
        void _update() {
            LabsLand::Utils::clock_t currentClock = this->timeManager->getAbsoluteTime();
            _update(currentClock);
        }

        /**
         * This internal method is meant to be invoked externally as often as possible, and it is not meant to be
         * invoked by specific simulation developers. Calling the method will ensure that the update() and reportUpdate()
         * methods be invoked periodically.
         *
         * This version of the method accepts the current clock time as an argument. This way simulation developers may
         * test the simulation updates more freely and make the simulation run faster than the processor's clock.
         * That is, to run faster than it normally would.
         *
         * @param clk Current clock()
         */
        void _update(LabsLand::Utils::clock_t currentClock) {
            LabsLand::Utils::clock_t elapsedUpdate = currentClock - mLastUpdate;
            LabsLand::Utils::clock_t elapsedReportUpdate = currentClock - mLastReportUpdate;

            update(elapsedUpdate / (double)this->timeManager->getClocksPerSec());
            mLastUpdate = currentClock;

            if(elapsedReportUpdate / (double)this->timeManager->getClocksPerSec() > mVirtualEnvironmentReportPeriod) {
                reportUpdate();
                mLastReportUpdate = currentClock;
            }
        }

        /**
         * Internal method to be invoked to initialize the simulation. It will conduct some initial housekeeping and
         * then invoke the virtual initialize() function that simulation developers are meant to override.
         */
        void _initialize() {
            mLastUpdate = this->timeManager->getAbsoluteTime();
            mLastReportUpdate = this->timeManager->getAbsoluteTime();

            initialize();
        }

        /**
         * Invoked periodically and very often by the system. This function is meant to be override so as to update the state of the
         * simulation. This is meant to be called with a high-frequency, and generally it should not block. Updates to the
         * simulation's state during the execution of this function will not necessarily be propagated immediately to the
         * server. This function may read from the GPIOs or other hardware devices.
         *
         * @param delta Time that has elapsed since this function was called, in seconds.
         */
        virtual void update(double delta) {
        }

        /**
         * Invoked periodically by the system to send the simulation's state to the environment/external system. The frequecy
         * at which this is run should be the mVirtualEnvironmentReportPeriod. If in reportWhenMarked mode, then it will only
         * queue the data to be sent if the flag was set through the requestReportState API.
         */
        virtual void reportUpdate() {
            if(mReportWhenMarked) {
                if(mShouldReportInReportWhenMarkedMode) {
                    sendVirtualEnvironmentReport();
                }
            } else {
                sendVirtualEnvironmentReport();
            }

            mShouldReportInReportWhenMarkedMode = false;
        }

        /**
         * How long to sleep between updates (in milliseconds)
         */
        virtual uint32_t getSleepStepInMs() {
            return 0;
        }

        /**
         * How long to sleep between updates (in microseconds)
         */
        virtual uint32_t getSleepStepInUs() {
            return 0;
        }

        /*
         * Log a message
         */
        virtual void log(std::string message) {
            this->log() << message << std::endl;
        }

        /*
         * Get an ostream to log to
         */
        virtual std::ostream& log() {
            return this->targetDevice->log();
        }

        /**
         * To initialize the simulation.
         */
        virtual void initialize() {

        }
};

#endif //HYBRIDAPI_SIMULATION_H
