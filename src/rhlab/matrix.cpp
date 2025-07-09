#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "matrix.h"

using namespace std;
using namespace RHLab::LEDMatrix;

void MatrixSimulation::initialize(){
    this->targetDevice->initializeSimulation({}, {"latch", "pulse", "green", "red"});

    setReportWhenMarked(true);
}

bool MatrixSimulation::readSerialCommunication(vector<vector<bool>>& buffer, vector<string>& gpios) {
    try {
        for (int i = 0; i < buffer.size(); i++) { // reading 2 at a time
            // Wait for a rising edge on the pulse GPIO
            while (this->targetDevice->getGpio("pulse") == 0) {}

            // Read the data bit when the pulse is high
            for (int j = 0; j < buffer[i].size(); j++) {
                buffer[i][j] = this->targetDevice->getGpio(gpios[j]);
                this->log() << "buffer[" << i << "][ " << j << "] = " << ((buffer[i][j] == true)?"1":"0") << "; // " << gpios[j] << endl;
            }
            // Wait for the pulse to go low again before reading the next bit
            while (this->targetDevice->getGpio("pulse") == 1) {}
        }
    } catch (exception e) {
        // TODO: add some form of exception dump or handling
        return false;
    }
    
    // Return true to indicate successful reading
    return true;
}

void MatrixSimulation::update(double delta) {
    // If latch is low, there's nothing to process.
    if (this->targetDevice->getGpio("latch") == 0) {
        return;
    }
    
    // Input GPIO channel names
    vector<string> inputGPIOs = {"green", "red"};
    // 2D Vector of ROWS * COLS x BITS_PER_LED (inputGPIOs.size()), bools for input data
    vector<vector<bool>> targetDeviceInputData(ROWS * COLS, vector<bool>(BITS_PER_LED, false));

    // Wait for latch to become 0
    while (this->targetDevice->getGpio("latch") == 1) {}

    // Process the received data and update the matrix
    if (readSerialCommunication(targetDeviceInputData, inputGPIOs)) {
        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLS; col++) {
                // Map the received data to LED matrix states
                int bitIndex = row * COLS + col;
                bool green = targetDeviceInputData[bitIndex][0];
                bool red = targetDeviceInputData[bitIndex][1];
                char color = ' ';
                if (red && green) {
                    color = 'Y';
                } else if (green) {
                    color = 'G';
                } else if (red) {
                    color = 'R';
                } else {
                    color = 'B';
                }
                this->mState.setLed(row, col, color);
            }
        }
        this->log() << "Reporting:" << this->mState.serialize() << endl;
        requestReportState();
    }
}
