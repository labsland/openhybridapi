#include <string>
#include <iostream>
#include "wiperDeusto.h"

void WiperDeustoSimulation::initialize()
{

    this->targetDevice->initializeSimulation(
        {"rainSensor", "rightSensor", "leftSensor", "resetButton", "mButton", "pButton", "uButton", "aButton"},
        {"move"});

    setReportWhenMarked(true);
}

void WiperDeustoSimulation::update(double delta)
{

    this->log() << "Updating simulation. Delta: " << delta << std::endl;

    WiperDeustoRequest request;
    bool requestWasRead = readRequest(request);

    mState.move = this->targetDevice->getGpio("move");

    this->log() << std::endl << "Move: " << mState.move << std::endl;

    if (requestWasRead) {
        this->log() << "Input:" << std::endl
                    << " RainSensor: " << request.rainSensor << "; RightSensor: " << request.rightSensor << "; LeftSensor: " << request.leftSensor << std::endl;

        this->targetDevice->setGpio("rainSensor", request.rainSensor);
        this->targetDevice->setGpio("rightSensor", request.rightSensor);
        this->targetDevice->setGpio("leftSensor", request.leftSensor);
        this->targetDevice->setGpio("resetButton", request.resetButton);
        this->targetDevice->setGpio("mButton", request.mButton);
        this->targetDevice->setGpio("pButton", request.pButton);
        this->targetDevice->setGpio("uButton", request.uButton);
        this->targetDevice->setGpio("aButton", request.aButton);
    }

    requestReportState();
}
