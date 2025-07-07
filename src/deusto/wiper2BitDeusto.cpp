#include <string>
#include <iostream>
#include "wiper2BitDeusto.h"

void Wiper2BitDeustoSimulation::initialize()
{

    this->targetDevice->initializeSimulation(
        {"rainSensor", "rightSensor", "leftSensor", "resetButton", "mButton", "pButton", "uButton", "aButton"},
        {"move1", "move2"});

    setReportWhenMarked(true);
}

void Wiper2BitDeustoSimulation::update(double delta)
{

    this->log() << "Updating simulation. Delta: " << delta << std::endl;

    Wiper2BitDeustoRequest request;
    bool requestWasRead = readRequest(request);

    mState.move1 = this->targetDevice->getGpio("move1");
    mState.move2 = this->targetDevice->getGpio("move2");

    this->log() << std::endl << "Move: " << mState.move1 << std::endl;

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
