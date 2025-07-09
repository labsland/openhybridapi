/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#include "targetdevice.h"
#include <algorithm>

using namespace std;
using namespace LabsLand::Utils;
using namespace LabsLand::Protocols;

/*
 *
 * TargetDeviceConfiguration implementation
 *
 */

TargetDeviceConfiguration::TargetDeviceConfiguration(int outputGpios, int inputGpios, I2CSlaveConfiguration * firstI2CSlaveConfig, I2CSlaveConfiguration * secondI2CSlaveConfig) {
    this->outputGpios = outputGpios;
    this->inputGpios = inputGpios;
    this->firstI2CSlaveConfig = firstI2CSlaveConfig;
    this->secondI2CSlaveConfig = secondI2CSlaveConfig;
}

TargetDeviceConfiguration::TargetDeviceConfiguration(vector<string> outputGpios, vector<string> inputGpios, I2CSlaveConfiguration * firstI2CSlaveConfig, I2CSlaveConfiguration * secondI2CSlaveConfig) {
    this->outputGpios = outputGpios.size();
    this->outputLabels = outputGpios;
    this->inputGpios = inputGpios.size();
    this->inputLabels = inputGpios;
    this->firstI2CSlaveConfig = firstI2CSlaveConfig;
    this->secondI2CSlaveConfig = secondI2CSlaveConfig;
}

void TargetDeviceConfiguration::setOutputGpios(int outputGpios) {
    this->outputGpios = outputGpios;
}

void TargetDeviceConfiguration::setOutputGpios(vector<string> outputGpios) {
    this->outputGpios = outputGpios.size();
    this->outputLabels = outputGpios;
}

int TargetDeviceConfiguration::getOutputGpios() const {
    return this->outputGpios;
}

vector<string> TargetDeviceConfiguration::getOutputLabels() const {
    return this->outputLabels;
}

void TargetDeviceConfiguration::setInputGpios(int inputGpios) {
    this->inputGpios = inputGpios;
}

void TargetDeviceConfiguration::setInputGpios(vector<string> inputGpios) {
    this->inputGpios = inputGpios.size();
    this->inputLabels = inputGpios;
}

int TargetDeviceConfiguration::getInputGpios() const {
    return this->inputGpios;
}

vector<string> TargetDeviceConfiguration::getInputLabels() const {
    return this->inputLabels;
}

void TargetDeviceConfiguration::setFirstI2CSlaveConfig(I2CSlaveConfiguration * firstI2CSlaveConfig) {
    if (this->firstI2CSlaveConfig != nullptr) {
        delete this->firstI2CSlaveConfig;
        this->firstI2CSlaveConfig = nullptr;
    }

    this->firstI2CSlaveConfig = firstI2CSlaveConfig;
}

void TargetDeviceConfiguration::setFirstI2CSlaveConfig(i2cSlaveCallback * callback, int address) {
    setFirstI2CSlaveConfig(new I2CSlaveConfiguration(callback, address));
}

I2CSlaveConfiguration * TargetDeviceConfiguration::getFirstI2CSlaveConfig() const {
    return this->firstI2CSlaveConfig;
}

void TargetDeviceConfiguration::setSecondI2CSlaveConfig(I2CSlaveConfiguration * secondI2CSlaveConfig) {
    if (this->secondI2CSlaveConfig != nullptr) {
        delete this->secondI2CSlaveConfig;
        this->secondI2CSlaveConfig = nullptr;
    }

    this->secondI2CSlaveConfig = secondI2CSlaveConfig;
}

void TargetDeviceConfiguration::setSecondI2CSlaveConfig(i2cSlaveCallback * callback, int address) {
    setSecondI2CSlaveConfig(new I2CSlaveConfiguration(callback, address));
}

I2CSlaveConfiguration * TargetDeviceConfiguration::getSecondI2CSlaveConfig() const {
    return this->secondI2CSlaveConfig;
}

TargetDeviceConfiguration::~TargetDeviceConfiguration() {
    if (this->firstI2CSlaveConfig != nullptr) {
        delete this->firstI2CSlaveConfig;
        this->firstI2CSlaveConfig = nullptr;
    }
    if (this->secondI2CSlaveConfig != nullptr) {
        delete this->secondI2CSlaveConfig;
        this->secondI2CSlaveConfig = nullptr;
    }
}

/*
 *
 * TargetDevice basic implementation
 *
 */

bool TargetDevice::initializeSimulation(std::vector<std::string> outputGpios, std::vector<std::string> inputGpios) {
    bool succeeded = this->initializeSimulation(outputGpios.size(), inputGpios.size());
    if (!succeeded)
        return false;

    this->outputLabels = outputGpios;
    this->inputLabels = inputGpios;

    return true;
}

bool TargetDevice::checkSimulationSupport(int outputGpios, int inputGpios) {
    shared_ptr<TargetDeviceConfiguration> configuration = make_shared<TargetDeviceConfiguration>(outputGpios, inputGpios);
    bool result = this->checkSimulationSupport(configuration);
    return result;
}

bool TargetDevice::initializeSimulation(int outputGpios, int inputGpios) {
    shared_ptr<TargetDeviceConfiguration> configuration = make_shared<TargetDeviceConfiguration>(outputGpios, inputGpios);
    return this->initializeSimulation(configuration);
}

void TargetDevice::setGpio(std::string outputPosition, bool value) {
    auto it = std::find (this->outputLabels.begin(), this->outputLabels.end(), outputPosition);
    // Not found
    if (it == this->outputLabels.end())
        return;

    int position = it - this->outputLabels.begin();
    setGpio(position, value);
}

void TargetDevice::resetGpio(std::string outputPosition) {
    this->setGpio(outputPosition, false);
}

bool TargetDevice::getGpio(std::string inputPosition) {
    auto it = std::find (this->inputLabels.begin(), this->inputLabels.end(), inputPosition);
    // Not found
    if (it == this->inputLabels.end())
        return false;

    int position = it - this->inputLabels.begin();
    return getGpio(position);
}


TargetDevice::~TargetDevice() {
    if (this->configuration != nullptr) {
        this->configuration.reset();
    }
}
