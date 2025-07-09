/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#ifndef SPI_LABSLAND_PROTOCOLS_IO_WRAPPER_FILES_H
#define SPI_LABSLAND_PROTOCOLS_IO_WRAPPER_FILES_H

#include <string>
#include <thread>
#include <atomic>
#include "labsland/protocols.h"

namespace LabsLand::Protocols {

    /*
     * This class represents a single SPI channel, relying on files and threads.
     * Internally, it monitors the file for SPI events and triggers callbacks.
     */
    class SPI_IO_WrapperFiles : public SPI_IO_Wrapper {
    private:
        std::thread *monitoringThread = nullptr;
        std::atomic<bool> continueRunning{true};
        std::string outputFileName;
        std::string inputFileName;
        std::string signalFileName;

    public:
        SPI_IO_WrapperFiles();
        ~SPI_IO_WrapperFiles();

        void initialize(const std::string &outputFile, const std::string &inputFile, const std::string &signalFile, const spiSlaveCallback *callback);

        virtual void writeByte(unsigned char byte) override;
        virtual unsigned char readByte() override;
        virtual void setChipSelect(bool state) override;

        bool shouldContinueRunning() const;
    };

} // namespace LabsLand::Protocols

#endif
