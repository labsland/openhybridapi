/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#ifndef LL_TARGET_DEVICE
#define LL_TARGET_DEVICE

#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "labsland/protocols.h"

namespace LabsLand::Utils {

    /**
     *
     * These simulations work interacting with a number of devices (e.g., different types of FPGAs, microprocessors and such).
     *
     * These devices have different GPIOs and they are assigned to different numbers.
     *
     * To make the development easier, we have this class. We will later have a class implementing each real device, without
     * having to change the code of the simulations.
     *
     * The design is the following:
     *
     * a) Every device has a set of GPIOs. By default, some are input, some are output. 
     * b) Some simulations might change this (e.g., turn an input into an output) in very specific cases.
     * c) Simulations will allocate a number of input GPIOs for themselves, and a number of output GPIOs for themselves.
     * d) This class will have to take care of everything else.
     *
     * For example:
     *
     * a) A device has 14 outputs (e.g., GPIO0-GPIO5 and GPIO10-GPIO17) and 4 inputs (e.g., GPIO6-GPIO9). By default this is it.
     *
     * b) Additionally, a device has a set of GPIOs that are tagged as "for-simulation", in a specific order. 
     *    For example, maybe there is a relay that depending on the configuration, a pin of the target device will be connected to 
     *    a motor or to GPIO15. If the designer of the board wants to guarantee that the GPIO15 is not used in the board, what he would
     *    typically do is to set that the simulation gpios are GPIO17-GPIO16, GPIO14-GPIO10 in this order.
     *
     * c) A particular simulation allocates 5 of the outputs and 2 of the inputs.
     *
     *    The simulation therefore will get GPIO17, GPIO16, GPIO14, GPIO13, GPIO12 for output, and GPIO8-GPIO9 for inputs.
     *
     *    This means that if the user requests "turn gpio 10 on" (e.g., with a switch), it will work, because it is not allocated by the
     *    simulation, so those GPIOs will still work. But if the user requests "turn gpio 12 on", it will not work, because the GPIO12
     *    is already assigned to the simulation, and instead will return an error.
     *
     * d) Additionally, a simulation might allocate named inputs and outputs. For example, certain well known names (and only those)
     *    can be allocated. This only happens with specific protocols (e.g., custom serial, i2c, spi, etc.), where the board might want
     *    to allow specific names for that.
     *
     * e) Finally, it is sometimes possible to request a superior number of inputs or outputs that there actually is, and the TargetDevice,
     *    if possible, will reassign certain inputs as outputs and viceversa. This will only happen in some boards and in specific pins.
     *
     *
     * Important: simulations will be able to know in advance if a target device works or not.
     */
    class TargetDeviceConfiguration;

    class TargetDevice {
        private:
            std::vector<std::string> inputLabels;
            std::vector<std::string> outputLabels;

        protected:
            // Note: the destructor of the target device will destroy this
            std::unique_ptr<TargetDeviceConfiguration> configuration = nullptr;
        public:
            virtual ~TargetDevice();
            /*
             * Does it support this number of inputs and outputs?
             *
             * Note: it does not store configuration
             */
            virtual bool checkSimulationSupport(std::shared_ptr<TargetDeviceConfiguration> configuration) = 0;
            // simplified version
            virtual bool checkSimulationSupport(int outputGpios, int inputGpios);


            /*
             * Allocate a set of outputs and inputs, in whichever order the device defines.
             *
             * Note: it DOES store configuration
             *
             * It returns true/false if possible.
             */
            virtual bool initializeSimulation(std::shared_ptr<TargetDeviceConfiguration> configuration) = 0;
            // simplified version
            virtual bool initializeSimulation(int outputGpios, int inputGpios);

            /*
             * Allocate a set of outputs and inputs, in whichever order the device defines.
             *
             * It is the same as calling initializeSimulation, but providing strings. Internally
             * the initializeSimulation() is called.
             *
             * This allows providing names instead of positions, and calling later those names instead
             * of numeric positions.
             *
             * The order in the vector is what matters. E.g., {"foo", "bar"} means that setGpio("foo", true) is
             * exactly the same as calling setGpio(0, true), and same with "bar" and 1.
             */
            virtual bool initializeSimulation(std::vector<std::string> outputGpios, std::vector<std::string> inputGpios);

            /*
             * Reset to the default state of the target device (e.g., all GPIOs available for regular use)
             */
            virtual void resetAfterSimulation() = 0;

            /*
             * If custom serial is going to be used, initialize it. It will return false if not possible.
             */
            virtual bool initializeCustomSerial() = 0;

            // Add other protocols in the future

            /*
             * Regular operations with GPIO (set/get) based on the position.
             * Important: setGpio(1) does not set GPIO1 to 1: it sets to 1 whichever is the second
             * output GPIO for simulations reserved. In the example above, where the simulation 
             * reserved GPIO17, GPIO16, GPIO14, GPIO13, GPIO12, setGpio(1) is GPIO16=1
             */
            virtual void setGpio(int outputPosition, bool value = true) = 0;
            virtual void resetGpio(int outputPosition) = 0;
            virtual bool getGpio(int inputPosition) = 0;

            /*
             * Regular operations with GPIO (set/get) based on the position.
             * In this case, you can make the calls using the names that you used
             * in initializeSimulation() providing names. If you did not provide any
             * names, or the name you provided was not in the list, do not expect any
             * result here.
             */
            virtual void setGpio(std::string outputPosition, bool value = true);
            virtual void resetGpio(std::string outputPosition);
            virtual bool getGpio(std::string inputPosition);


            /*
             * Get log() so as to do:
             *
             * targetDevice->log() << "foo";
             */
            virtual std::ostream& log() = 0;

            /**
             * Same, but using custom names
             */
            virtual void setGpio(LabsLand::Protocols::NamedGpio outputPosition, bool value = true) = 0;
            virtual void resetGpio(LabsLand::Protocols::NamedGpio outputPosition) = 0;
            virtual bool getGpio(LabsLand::Protocols::NamedGpio inputPosition) = 0;
    };

    class TargetDeviceConfiguration {
            int outputGpios = 0;
            int inputGpios = 0;
            std::vector<std::string> inputLabels;
            std::vector<std::string> outputLabels;

            // there can be up to 2 I2C slaves. You can initialize one, the other or both.
            // Note the destructor of this class will delete these objects
            LabsLand::Protocols::I2CSlaveConfiguration * firstI2CSlaveConfig = nullptr; // Destroyed by this class
            LabsLand::Protocols::I2CSlaveConfiguration * secondI2CSlaveConfig = nullptr; // Destroyed by this class

        public:
            TargetDeviceConfiguration(int outputGpios = 0, int inputGpios = 0, LabsLand::Protocols::I2CSlaveConfiguration * firstI2CSlaveConfig = nullptr, LabsLand::Protocols::I2CSlaveConfiguration * secondI2CSlaveConfig = nullptr);
            TargetDeviceConfiguration(std::vector<std::string> outputGpios, std::vector<std::string> inputGpios, LabsLand::Protocols::I2CSlaveConfiguration * firstI2CSlaveConfig = nullptr, LabsLand::Protocols::I2CSlaveConfiguration * secondI2CSlaveConfig = nullptr);

            void setOutputGpios(int outputGpios);
            void setOutputGpios(std::vector<std::string> outputGpios);
            int getOutputGpios() const;
            std::vector<std::string> getOutputLabels() const;

            void setInputGpios(int inputGpios);
            void setInputGpios(std::vector<std::string> inputGpios);
            int getInputGpios() const;
            std::vector<std::string> getInputLabels() const;

            void setFirstI2CSlaveConfig(LabsLand::Protocols::I2CSlaveConfiguration * firstI2CSlaveConfig);
            void setFirstI2CSlaveConfig(LabsLand::Protocols::i2cSlaveCallback * callback, int address);

            LabsLand::Protocols::I2CSlaveConfiguration * getFirstI2CSlaveConfig() const;

            void setSecondI2CSlaveConfig(LabsLand::Protocols::I2CSlaveConfiguration * secondI2CSlaveConfig);

            void setSecondI2CSlaveConfig(LabsLand::Protocols::i2cSlaveCallback * callback, int address);

            LabsLand::Protocols::I2CSlaveConfiguration * getSecondI2CSlaveConfig() const;

            ~TargetDeviceConfiguration();
    };

}

#endif
