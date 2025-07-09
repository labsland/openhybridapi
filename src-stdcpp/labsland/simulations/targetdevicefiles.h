/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#ifndef LL_TARGET_DEVICE_STD
#define LL_TARGET_DEVICE_STD

#include <string>
#include "labsland/simulations/targetdevice.h"
#include "../protocols/i2ciowrapperfiles.h"
#include "../protocols/spiiowrapperfiles.h"

namespace LabsLand::Utils {

    class TargetDeviceFiles: public TargetDevice {

        private:
            const std::string inputGpioFilename;
            const std::string outputGpioFilename;

            const std::string firstInputI2cFilename;
            const std::string firstOutputI2cFilename;
            const std::string firstSignalI2cFilename;

            const std::string secondInputI2cFilename;
            const std::string secondOutputI2cFilename;
            const std::string secondSignalI2cFilename;

            const std::string spiInputFilename;
            const std::string spiOutputFilename;
            const std::string spiSignalFilename;

            const int numberOfOutputs;
            const int numberOfInputs;

            int numberOfSimulationOutputs;
            int numberOfSimulationInputs;

            std::string getOutputValues();

            LabsLand::Protocols::I2C_IO_WrapperFiles * firstI2cIoWrapper = 0;
            LabsLand::Protocols::I2C_IO_WrapperFiles * secondI2cIoWrapper = 0;

            LabsLand::Protocols::SPI_IO_WrapperFiles * spiIoWrapper = nullptr;

        public:
            TargetDeviceFiles(
                    int numberOfOutputs, int numberOfInputs, 
                    const std::string & outputGpioFilename = "output-gpios.txt", const std::string & inputGpioFilename = "input-gpios.txt", 
                    const std::string & firstOutputI2cFilename = "output-i2c-1.txt", const std::string & firstInputI2cFilename = "input-i2c-1.txt", const std::string & firstSignalI2cFilename = "signal-i2c-1.txt",
                    const std::string & secondOutputI2cFilename = "output-i2c-2.txt", const std::string & secondInputI2cFilename = "input-i2c-2.txt", const std::string & secondSignalI2cFilename = "signal-i2c-2.txt"
            );
            ~TargetDeviceFiles();

            /*
             * Does it support this number of inputs and outputs?
             */
            virtual bool checkSimulationSupport(std::shared_ptr<TargetDeviceConfiguration> configuration);

            /*
             * Allocate a set of outputs and inputs, in whichever order the device defines.
             *
             * It returns true/false if possible.
             */
            virtual bool initializeSimulation(std::shared_ptr<TargetDeviceConfiguration> configuration);

            /*
             * Reset to the default state of the target device (e.g., all GPIOs available for regular use)
             */
            virtual void resetAfterSimulation();

            /*
             * If custom serial is going to be used, initialize it. It will return false if not possible.
             */
            virtual bool initializeCustomSerial();

            virtual std::ostream& log();

            // Add other protocols in the future

            /*
             * Regular operations with GPIO (set/get) based on the position.
             * Important: setGpio(1) does not set GPIO1 to 1: it sets to 1 whichever is the second
             * output GPIO for simulations reserved. In the example above, where the simulation 
             * reserved GPIO17, GPIO16, GPIO14, GPIO13, GPIO12, setGpio(1) is GPIO16=1
             */
            virtual void setGpio(int outputPosition, bool value = true);
            virtual void resetGpio(int outputPosition);
            virtual bool getGpio(int inputPosition);

            /**
             * Same, but using custom names
             */
            virtual void setGpio(LabsLand::Protocols::NamedGpio outputPosition, bool value = true);
            virtual void resetGpio(LabsLand::Protocols::NamedGpio outputPosition);
            virtual bool getGpio(LabsLand::Protocols::NamedGpio inputPosition);

            /**
             * SPI-specific functionality
             */
            /*
            virtual void spiWriteByte(unsigned char byte);
            virtual unsigned char spiReadByte();
            virtual void setSpiChipSelect(bool state);
            */
    };
}

#endif
