/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#include <chrono>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
#include "spiiowrapperfiles.h"

using namespace std;
using namespace LabsLand::Protocols;

SPI_IO_WrapperFiles::SPI_IO_WrapperFiles() {}

SPI_IO_WrapperFiles::~SPI_IO_WrapperFiles() {
    if (this->monitoringThread) {
        this->continueRunning = false;
        this->monitoringThread->join();
        delete this->monitoringThread;
        this->monitoringThread = nullptr;
    }
}

void runSpiThread(SPI_IO_WrapperFiles *spiInstance,
                  const std::string &outputFile,
                  const std::string &inputFile,
                  const std::string &signalFile,
                  const spiSlaveCallback *callback) {

    if (callback == nullptr) {
        cerr << "Received null callback for SPI with output file " << outputFile << " - stopping thread." << endl;
        return;
    }

    while (spiInstance->shouldContinueRunning()) {
        this_thread::sleep_for(chrono::milliseconds(100));

        ifstream signalFileObj(signalFile);

        if (!signalFileObj || !signalFileObj.good()) {
            continue;
        }

        stringstream buffer;
        buffer << signalFileObj.rdbuf();
        string currentSignal = buffer.str();
        signalFileObj.close();

        if (currentSignal == "transmit") {
            (*callback)(spiInstance, SPIEventType::slaveTransmit);
        } else if (currentSignal == "receive") {
            (*callback)(spiInstance, SPIEventType::slaveReceive);
        } else if (currentSignal == "finish") {
            (*callback)(spiInstance, SPIEventType::slaveFinish);
        } else {
            continue; // Ignore unknown or incomplete signals
        }

        remove(signalFile.c_str());
    }
}

void SPI_IO_WrapperFiles::initialize(const string &outputFile, const string &inputFile, const string &signalFile, const spiSlaveCallback *callback) {
    this->outputFileName = outputFile;
    this->inputFileName = inputFile;
    this->signalFileName = signalFile;

    // Clear the output file on initialization
    ofstream newOutputFile(this->outputFileName, ios::binary | ios::trunc);
    newOutputFile.close();

    if (this->monitoringThread) {
        this->continueRunning = false;
        this->monitoringThread->join();
        delete this->monitoringThread;
        this->monitoringThread = nullptr;
    }

    this->continueRunning = true;
    this->monitoringThread = new thread(runSpiThread, this, outputFile, inputFile, signalFile, callback);
}

bool SPI_IO_WrapperFiles::shouldContinueRunning() const {
    return this->continueRunning;
}

unsigned char SPI_IO_WrapperFiles::readByte() {
    ifstream inputFile(this->inputFileName, ios::binary);
    if (!inputFile || !inputFile.good()) {
        return 0;
    }

    // Read all data into a vector
    vector<char> buffer((istreambuf_iterator<char>(inputFile)), istreambuf_iterator<char>());
    inputFile.close();

    if (buffer.empty()) {
        return 0;
    }

    // Extract the first byte and remove it from the buffer
    unsigned char firstByte = static_cast<unsigned char>(buffer[0]);
    buffer.erase(buffer.begin());

    // Write the remaining buffer back to the file
    ofstream inputFileWithoutByte(this->inputFileName, ios::binary | ios::trunc);
    inputFileWithoutByte.write(buffer.data(), buffer.size());
    inputFileWithoutByte.close();

    return firstByte;
}

void SPI_IO_WrapperFiles::writeByte(unsigned char byte) {
    ofstream outputFile(this->outputFileName, ios::binary | ios::trunc);
    if (!outputFile) {
        perror("Could not open SPI output file");
        return;
    }
    char byteToSend = static_cast<char>(byte);
    outputFile.write(&byteToSend, 1);
    outputFile.close();
}

void SPI_IO_WrapperFiles::setChipSelect(bool state) {
    ofstream signalFile(this->signalFileName, ios::trunc);
    if (!signalFile) {
        perror("Could not open SPI signal file");
        return;
    }

    signalFile << (state ? "active" : "inactive");
    signalFile.close();

    cout << "Chip select state: " << (state ? "Active" : "Inactive") << endl;
}
