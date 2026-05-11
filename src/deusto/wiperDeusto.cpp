#include <iostream>
#include <string>
#include "wiperDeusto.h"

void WiperDeustoSimulation::initialize() {
    this->targetDevice->initializeSimulation(
        {"rainSensor", "leftSensor", "rightSensor", "mButton", "pButton"},
        {"move"}
    );

    mState.wiperAngle = MAX_ANGLE;
    mState.leftSensor = 1;
    mState.rightSensor = 0;
    mState.moving = false;
    mDirection = -1;

    setReportWhenMarked(true);
}

void WiperDeustoSimulation::update(double delta) {
    this->log() << "Updating simulation. Delta: " << delta << std::endl;

    WiperDeustoRequest request;
    bool requestWasRead = readRequest(request);

    if (requestWasRead) {
        this->log() << "Input:" << std::endl << "; RainSensor: " << request.rainSensor << "; mButton: " << request.mButton << "; pButton: " << request.pButton << std::endl;

        this->targetDevice->setGpio("rainSensor", request.rainSensor);
        this->targetDevice->setGpio("mButton", request.mButton);
        this->targetDevice->setGpio("pButton", request.pButton);

        forcedError = request.error;
    }

    mState.moving = this->targetDevice->getGpio("move");
    this->log() << std::endl << "Move: " << mState.moving << std::endl;

    if (mState.moving){
        mState.wiperAngle += mDirection * SPEED * delta;
    }

    if (mState.wiperAngle >= MAX_ANGLE)
    {
        mState.wiperAngle = MAX_ANGLE;
        mDirection = -1;
    }

    if (mState.wiperAngle <= MIN_ANGLE)
    {
        mState.wiperAngle = MIN_ANGLE;
        mDirection = 1;
    }

    if (forcedError)
    {
        mState.leftSensor = 1;
        mState.rightSensor = 1;
    } 
    else {

        if (mState.wiperAngle >= MIN_ANGLE &&
            mState.wiperAngle <= MIN_ANGLE+15.0f)
        {
            mState.rightSensor = 1;
        }
        else
        {
            mState.rightSensor = 0;
        }

        if (mState.wiperAngle >= MAX_ANGLE-15.0f &&
            mState.wiperAngle <= MAX_ANGLE)
        {
            mState.leftSensor = 1;
        }
        else
        {
            mState.leftSensor = 0;
        }

    }

    this->log() << "Angle: " << mState.wiperAngle << std::endl;
    this->log() << "LeftSensor: " << mState.leftSensor << std::endl;
    this->log() << "RightSensor: " << mState.rightSensor << std::endl;

    requestReportState();
}
