/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#include <iostream>
#include <fstream>
#include <sstream>

#include "targetdevicefiles.h"

using namespace std;
using namespace LabsLand::Utils;
using namespace LabsLand::Protocols;

TargetDeviceFiles::TargetDeviceFiles(
        int numberOfOutputs, 
        int numberOfInputs,
        const string & outputGpioFilename, 
        const string & inputGpioFilename, 
        const string & firstOutputI2cFilename,
        const string & firstInputI2cFilename,
        const string & firstSignalI2cFilename,
        const string & secondOutputI2cFilename,
        const string & secondInputI2cFilename,
        const string & secondSignalI2cFilename
    ): 
        inputGpioFilename(inputGpioFilename), 
        outputGpioFilename(outputGpioFilename), 
        numberOfOutputs(numberOfOutputs), 
        numberOfInputs(numberOfInputs) ,
        firstOutputI2cFilename(firstOutputI2cFilename),
        firstInputI2cFilename(firstInputI2cFilename),
        firstSignalI2cFilename(firstSignalI2cFilename),
        secondOutputI2cFilename(secondOutputI2cFilename),
        secondInputI2cFilename(secondInputI2cFilename),
        secondSignalI2cFilename(secondSignalI2cFilename)
{}

TargetDeviceFiles::~TargetDeviceFiles() {
    if (this->firstI2cIoWrapper != 0)
        delete this->firstI2cIoWrapper;

    if (this->secondI2cIoWrapper != 0)
        delete this->secondI2cIoWrapper;
}

bool TargetDeviceFiles::checkSimulationSupport(shared_ptr<TargetDeviceConfiguration> configuration) {
    return configuration->getOutputGpios() <= this->numberOfOutputs && configuration->getInputGpios() <= this->numberOfInputs;
}

bool TargetDeviceFiles::initializeSimulation(shared_ptr<TargetDeviceConfiguration> configuration) {
    if (!checkSimulationSupport(configuration))
        return false;

    this->numberOfSimulationOutputs = configuration->getOutputGpios();
    this->numberOfSimulationInputs = configuration->getInputGpios();

    // Initialize the output GPIO file
    ofstream ofile(this->outputGpioFilename);
    for (int i = 0; i < this->numberOfSimulationOutputs; ++i)
        ofile << "0";

    ofile.close();

    // Initialize I2C (if provided)
    if (configuration->getFirstI2CSlaveConfig() != 0) {
        if (this->firstI2cIoWrapper != 0) {
            delete this->firstI2cIoWrapper;
        }
        this->firstI2cIoWrapper = new I2C_IO_WrapperFiles();
        this->firstI2cIoWrapper->initialize(this->firstOutputI2cFilename, this->firstInputI2cFilename, this->firstSignalI2cFilename, configuration->getFirstI2CSlaveConfig()->getCallback());
    }

    if (configuration->getSecondI2CSlaveConfig() != 0) {
        if (this->secondI2cIoWrapper != 0) {
            delete this->secondI2cIoWrapper;
        }
        this->secondI2cIoWrapper = new I2C_IO_WrapperFiles();
        this->secondI2cIoWrapper->initialize(this->secondOutputI2cFilename, this->secondInputI2cFilename, this->secondSignalI2cFilename, configuration->getSecondI2CSlaveConfig()->getCallback());
    }

    return true;
}

void TargetDeviceFiles::resetAfterSimulation() {
    this->numberOfSimulationOutputs = 0;
    this->numberOfSimulationInputs = 0;

    ofstream ofile(this->outputGpioFilename);
    ofile.close();
}

string TargetDeviceFiles::getOutputValues() {
    ifstream ifile(this->outputGpioFilename);
    stringstream buffer;
    buffer << ifile.rdbuf();
    string gpios = buffer.str();
    ifile.close();

    while(gpios.size() < this->numberOfSimulationOutputs)
        gpios += "0";

    if (gpios.size() > this->numberOfSimulationOutputs)
        gpios = gpios.substr(0, this->numberOfSimulationOutputs);

    return gpios;
}

ostream& TargetDeviceFiles::log() {
    return cout;
}

void TargetDeviceFiles::setGpio(int outputPosition, bool value) {
    string currentOutputs = this->getOutputValues();
    if (outputPosition > currentOutputs.size()) {
        return;
    }

    currentOutputs[outputPosition] = value?'1':'0';
    ofstream ofile(this->outputGpioFilename);
    ofile << currentOutputs;
    ofile.close();
}

void TargetDeviceFiles::resetGpio(int outputPosition) {
    this->setGpio(outputPosition, false);
}

bool TargetDeviceFiles::getGpio(int inputPosition) {
    ifstream ifile(this->inputGpioFilename);
    stringstream buffer;
    buffer << ifile.rdbuf();
    string gpios = buffer.str();
    ifile.close();
    if (inputPosition >= gpios.size())
        return false;

    return gpios[inputPosition] == '1';
}

bool TargetDeviceFiles::initializeCustomSerial() {
    // TODO
    return true;
}

void TargetDeviceFiles::setGpio(NamedGpio outputPosition, bool value) {
    // TODO
}

void TargetDeviceFiles::resetGpio(NamedGpio outputPosition) {
    // TODO
}

bool TargetDeviceFiles::getGpio(NamedGpio inputPosition) {
    // TODO
    return false;
}
