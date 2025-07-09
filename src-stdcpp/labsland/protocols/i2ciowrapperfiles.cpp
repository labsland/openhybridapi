/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#include <chrono>
#include <cstdio>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#include "i2ciowrapperfiles.h"

using namespace std;
using namespace LabsLand::Protocols;

I2C_IO_WrapperFiles::I2C_IO_WrapperFiles() {
}

I2C_IO_WrapperFiles::~I2C_IO_WrapperFiles() {
    if (this->monitoringThread != 0) {
        this->continueRunning = false;
        this->monitoringThread->join();
        delete this->monitoringThread;
        this->monitoringThread = 0;
    }
}

void runI2cThread(I2C_IO_WrapperFiles * i2cInstance, 
            const std::string & outputFile, const std::string & inputFile, const std::string & signalFile, 
            const i2cSlaveCallback * callback) {
    
    if (callback == 0) {
        cerr << "Received null callback for I2C with output file " << outputFile << " - stopping thread " << endl;
        return;
    }

    while (i2cInstance->shouldContinueRunning()) {
        this_thread::sleep_for(chrono::milliseconds(100));

        ifstream signalFileObj(signalFile);

        // if it does not exist, continue
        if (!signalFileObj || !signalFileObj.good())
            continue;

        stringstream buffer;                      
        buffer << signalFileObj.rdbuf();                    
        string currentSignal = buffer.str();          
        signalFileObj.close();

        if (currentSignal == "request") {
            (*callback)(i2cInstance, I2CEventType::slaveRequest);
        } else if(currentSignal == "receive") {
            (*callback)(i2cInstance, I2CEventType::slaveReceive);
        } else if(currentSignal == "finish") {
            (*callback)(i2cInstance, I2CEventType::slaveFinish);
        } else {
            // Maybe we have a partial read or something, but just ignore it.
            continue;
        }

        remove(signalFile.c_str());
    }
}

void I2C_IO_WrapperFiles::initialize(const string & outputFile, const string & inputFile, const string & signalFile, const i2cSlaveCallback * callback) {
    this->inputFileName = inputFile;
    this->outputFileName = outputFile;

    // truncate the file (so we restart it every time we initialize())
    std::ofstream newOutputFile(this->outputFileName, ios::binary | ios::trunc);
    newOutputFile.close();

    if (this->monitoringThread != 0) {
        this->continueRunning = false;
        this->monitoringThread->join();
        delete this->monitoringThread;
        this->monitoringThread = 0;
    }
    
    this->continueRunning = true;
    this->monitoringThread = new thread(runI2cThread, this, outputFile, inputFile, signalFile, callback);
}

bool I2C_IO_WrapperFiles::shouldContinueRunning () {
    return this->continueRunning;
}


unsigned char I2C_IO_WrapperFiles::readByte() {
    ifstream inputFile(this->inputFileName, ios::binary);
    if (!inputFile || !inputFile.good()) {
        return 0;
    }

    // Read the entire file into a vector
    std::vector<char> buffer(std::istreambuf_iterator<char>(inputFile), {});
    if (buffer.empty()) {
        // No first byte
        return 0;
    }

    // take the first byte (and will return it)
    unsigned char firstByte = (unsigned char)buffer[0];
    buffer.erase(buffer.begin());

    inputFile.close();

    // rewrite the file without that byte
    ofstream inputFileWithoutByte(this->inputFileName, ios::binary | ios::trunc);
    inputFileWithoutByte.write(buffer.data(), buffer.size());
    inputFileWithoutByte.close();

    return firstByte;
}

void I2C_IO_WrapperFiles::writeByte(unsigned char byte) {
    std::ofstream outputFile(this->outputFileName, ios::binary | ios::trunc);
    if (!outputFile) {
        perror("Coult not open I2C output file");
        return;
    }
    char byteToSend = (char)byte;
    outputFile.write(&byteToSend, 1);
    outputFile.close();
}

