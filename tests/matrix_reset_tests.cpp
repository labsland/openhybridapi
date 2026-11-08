#include "rhlab/matrix.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

class FakeTimeManager : public LabsLand::Utils::TimeManager {
public:
    void sleepMs(std::uint32_t) const override {}
    void sleepUs(std::uint32_t) const override {}
    LabsLand::Utils::clock_t getAbsoluteTime() const override { return now; }
    std::uint64_t getClocksPerSec() const override { return 1000; }

    void advanceMs(std::uint64_t milliseconds) { now += milliseconds; }

private:
    LabsLand::Utils::clock_t now = 0;
};

class FakeTargetDevice : public LabsLand::Utils::TargetDevice {
public:
    bool checkSimulationSupport(std::shared_ptr<LabsLand::Utils::TargetDeviceConfiguration> config) override {
        return config->getOutputGpios() <= 5 && config->getInputGpios() <= 5;
    }

    bool initializeSimulation(std::shared_ptr<LabsLand::Utils::TargetDeviceConfiguration> config) override {
        if (!checkSimulationSupport(config)) return false;
        plantOutputs.assign(static_cast<std::size_t>(config->getOutputGpios()), false);
        controllerOutputs.assign(static_cast<std::size_t>(config->getInputGpios()), false);
        return true;
    }

    void resetAfterSimulation() override {
        plantOutputs.clear();
        controllerOutputs.clear();
    }

    bool initializeCustomSerial() override { return false; }

    void setGpio(int position, bool value = true) override {
        if (position >= 0 && static_cast<std::size_t>(position) < plantOutputs.size()) {
            plantOutputs[static_cast<std::size_t>(position)] = value;
        }
    }

    void resetGpio(int position) override { setGpio(position, false); }

    bool getGpio(int position) override {
        if (position < 0 || static_cast<std::size_t>(position) >= controllerOutputs.size()) return false;
        return controllerOutputs[static_cast<std::size_t>(position)];
    }

    void setGpio(LabsLand::Protocols::NamedGpio, bool = true) override {}
    void resetGpio(LabsLand::Protocols::NamedGpio) override {}
    bool getGpio(LabsLand::Protocols::NamedGpio) override { return false; }
    std::ostream & log() override { return logStream; }

    std::vector<bool> plantOutputs;
    std::vector<bool> controllerOutputs;

private:
    std::ostringstream logStream;
};


int main() {
    auto time = std::make_shared<FakeTimeManager>();
    auto target = std::make_shared<FakeTargetDevice>();
    RHLab::LEDMatrix::MatrixSimulation simulation;
    simulation.injectTimeManager(time);
    simulation.injectTargetDevice(target);
    simulation._initialize();
    for (int row = 0; row < 16; ++row)
        for (int col = 0; col < 16; ++col)
            simulation.mState.setLed(row, col, (row + col) % 2 ? 'G' : 'R');
    simulation._initialize();
    for (char pixel : simulation.mState.leds) {
        if (pixel != 'B') {
            std::cerr << "Reinitialization retained a previous pixel" << std::endl;
            return 1;
        }
    }
    simulation.mState.setLed(0, 0, 'Y');
    if (simulation.mState.leds[0] != 'Y') return 1;
    return 0;
}
