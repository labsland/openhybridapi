#include "rhlab/airportWindStation.h"

#include <cstdlib>
#include <deque>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::cerr << __FILE__ << ":" << __LINE__ << ": check failed: " #condition << std::endl; \
        ++failures; \
    } \
} while (false)

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

class FakeCommunicator
    : public LabsLand::Simulations::Utils::SimulationCommunicator<AirportWindStationData, AirportWindStationRequest> {
public:
    bool enqueue(std::string const & raw) {
        AirportWindStationRequest request;
        if (!request.deserialize(raw)) return false;
        requests.push_back(request);
        return true;
    }

    bool readRequest(AirportWindStationRequest & request) override {
        if (requests.empty()) return false;
        request = requests.front();
        requests.pop_front();
        return true;
    }

    void sendReport(AirportWindStationData & report) override {
        reports.push_back(report.serialize());
    }

    std::deque<AirportWindStationRequest> requests;
    std::vector<std::string> reports;
};

class Fixture {
public:
    Fixture()
        : time(std::make_shared<FakeTimeManager>()),
          target(std::make_shared<FakeTargetDevice>()),
          communicator(std::make_shared<FakeCommunicator>()) {
        simulation.injectTimeManager(time);
        simulation.injectTargetDevice(target);
        simulation.injectCommunicator(communicator);
        simulation._initialize();
    }

    void tick(std::uint64_t milliseconds) {
        time->advanceMs(milliseconds);
        simulation._update(time->getAbsoluteTime());
    }

    void advance(std::uint64_t milliseconds, std::uint64_t step = 50) {
        while (milliseconds > 0) {
            std::uint64_t amount = milliseconds < step ? milliseconds : step;
            tick(amount);
            milliseconds -= amount;
        }
    }

    AirportWindStationSimulation simulation;
    std::shared_ptr<FakeTimeManager> time;
    std::shared_ptr<FakeTargetDevice> target;
    std::shared_ptr<FakeCommunicator> communicator;
};

std::uint32_t readSeq(std::string const & report) {
    std::size_t start = report.find("&seq=");
    if (start == std::string::npos) return 0;
    start += 5;
    std::size_t end = report.find('&', start);
    return static_cast<std::uint32_t>(std::strtoul(report.substr(start, end - start).c_str(), nullptr, 10));
}

void testRequestParsing() {
    AirportWindStationRequest request;
    CHECK(request.deserialize("v=1&requestId=42&action=sync"));
    CHECK(request.requestId == 42);
    CHECK(request.action == AirportWindAction::Sync);

    CHECK(request.deserialize("v=1&requestId=43&action=scenario&name=highWind"));
    CHECK(request.action == AirportWindAction::Scenario);
    CHECK(request.scenario == AirportWindScenario::HighWind);

    CHECK(!request.deserialize("v=2&requestId=1&action=sync"));
    CHECK(!request.deserialize("v=1&requestId=0&action=sync"));
    CHECK(!request.deserialize("v=1&requestId=1&action=scenario&name=storm"));
    CHECK(!request.deserialize("v=1&requestId=1&action=reset&extra=1"));
    CHECK(!request.deserialize("v=1&requestId=1&action=launch"));
    CHECK(!request.deserialize("v=1&requestId=1&action=scenario"));
    CHECK(!request.deserialize("v=1&requestId=4294967296&action=sync"));
    CHECK(!request.deserialize("v=1&requestId=-1&action=sync"));
    CHECK(!request.deserialize("v=1&requestId=abc&action=sync"));
}

void testCompleteSerialization() {
    AirportWindStationData state;
    state.seq = 7;
    state.scenario = AirportWindScenario::Realign;
    state.phase = AirportWindPhase::Generating;
    state.windDirection = 180;
    state.windBand = AirportWindBand::Steady;
    state.aligned = true;
    state.nacelleAngle = 180;
    state.targetAngle = 180;
    state.rotorBand = AirportRotorBand::Running;
    state.align = false;
    state.generatorEnable = true;
    state.active = true;
    state.runwayWindAlert = false;
    state.activeRequestId = 55;
    state.notice = AirportWindNotice::None;

    CHECK(state.serialize() ==
        "v=1&seq=7&scenario=realign&phase=generating&windDirection=180"
        "&windBand=steady&aligned=1&nacelleAngle=180&targetAngle=180"
        "&rotorBand=running&align=0&generatorEnable=1&active=1"
        "&runwayWindAlert=0&activeRequestId=55&notice=none");
}

void testInitializationAndHeartbeat() {
    Fixture fixture;
    CHECK(fixture.target->plantOutputs.size() == 5);
    CHECK(fixture.target->controllerOutputs.size() == 5);
    CHECK(fixture.target->plantOutputs[0]);
    CHECK(fixture.target->plantOutputs[1]);
    CHECK(!fixture.target->plantOutputs[2]);
    CHECK(!fixture.target->plantOutputs[3]);
    CHECK(!fixture.target->plantOutputs[4]);

    fixture.advance(300);
    CHECK(!fixture.target->plantOutputs[0]);
    CHECK(fixture.simulation.mState.phase == AirportWindPhase::Calm);

    std::size_t reportsBefore = fixture.communicator->reports.size();
    fixture.advance(600);
    CHECK(fixture.communicator->reports.size() > reportsBefore);

    std::uint32_t previous = 0;
    for (std::string const & report : fixture.communicator->reports) {
        std::uint32_t current = readSeq(report);
        CHECK(current > previous);
        previous = current;
        CHECK(report.find("&activeRequestId=") != std::string::npos);
        CHECK(report.find("&notice=") != std::string::npos);
    }
}

void testResetTimingAndSerializedRequests() {
    Fixture fixture;
    fixture.advance(249, 249);
    CHECK(fixture.target->plantOutputs[0]);
    fixture.tick(1);
    CHECK(!fixture.target->plantOutputs[0]);
    CHECK(fixture.simulation.mState.phase == AirportWindPhase::Calm);

    CHECK(fixture.communicator->enqueue("v=1&requestId=20&action=scenario&name=steady"));
    CHECK(fixture.communicator->enqueue("v=1&requestId=21&action=scenario&name=highWind"));
    fixture.tick(50);
    CHECK(fixture.simulation.mState.activeRequestId == 20);
    CHECK(fixture.simulation.mState.windBand == AirportWindBand::Steady);
    fixture.tick(50);
    CHECK(fixture.simulation.mState.activeRequestId == 21);
    CHECK(fixture.simulation.mState.windBand == AirportWindBand::High);
}

void testGenerationGatesAndHighWindOverride() {
    Fixture fixture;
    fixture.advance(300);
    CHECK(fixture.communicator->enqueue("v=1&requestId=30&action=scenario&name=steady"));
    fixture.tick(50);

    fixture.target->controllerOutputs[0] = true;
    fixture.target->controllerOutputs[3] = true;
    fixture.advance(2600);
    CHECK(fixture.simulation.mState.aligned);
    CHECK(fixture.simulation.mState.rotorBand == AirportRotorBand::Stopped);

    fixture.target->controllerOutputs[0] = false;
    fixture.target->controllerOutputs[2] = true;
    fixture.advance(500);
    CHECK(fixture.simulation.mState.rotorBand == AirportRotorBand::Accelerating);
    fixture.target->controllerOutputs[3] = false;
    fixture.advance(600);
    CHECK(fixture.simulation.mState.rotorBand == AirportRotorBand::Stopped);

    fixture.target->controllerOutputs[3] = true;
    fixture.advance(1100);
    CHECK(fixture.simulation.mState.rotorBand == AirportRotorBand::Running);

    fixture.target->controllerOutputs[4] = true;
    CHECK(fixture.communicator->enqueue("v=1&requestId=31&action=scenario&name=highWind"));
    fixture.tick(50);
    CHECK(fixture.simulation.mState.rotorBand == AirportRotorBand::Decelerating);
    fixture.advance(950);
    CHECK(fixture.simulation.mState.rotorBand == AirportRotorBand::Stopped);
    CHECK(fixture.simulation.mState.notice == AirportWindNotice::None);
    fixture.advance(350);
    CHECK(fixture.simulation.mState.notice == AirportWindNotice::None);
    fixture.advance(100);
    CHECK(fixture.simulation.mState.notice == AirportWindNotice::UnsafeGenerator);

    fixture.target->controllerOutputs[2] = false;
    fixture.target->controllerOutputs[3] = false;
    fixture.tick(50);
    CHECK(fixture.simulation.mState.notice == AirportWindNotice::None);
}

void testCompleteScenarioFlow() {
    Fixture fixture;
    fixture.advance(300);

    CHECK(fixture.communicator->enqueue("v=1&requestId=10&action=scenario&name=steady"));
    fixture.tick(50);
    CHECK(fixture.simulation.mState.activeRequestId == 10);
    CHECK(fixture.simulation.mState.windBand == AirportWindBand::Steady);
    CHECK(!fixture.target->plantOutputs[1]);
    CHECK(fixture.target->plantOutputs[2]);
    CHECK(!fixture.target->plantOutputs[3]);
    std::int16_t firstTarget = fixture.simulation.mState.targetAngle;

    fixture.target->controllerOutputs[0] = true;
    fixture.target->controllerOutputs[1] = true; // Reserved hole must be ignored.
    fixture.target->controllerOutputs[3] = true;
    fixture.advance(2600);
    CHECK(fixture.simulation.mState.aligned);
    CHECK(fixture.target->plantOutputs[4]);
    CHECK(fixture.simulation.mState.nacelleAngle == firstTarget);

    fixture.target->controllerOutputs[0] = false;
    fixture.target->controllerOutputs[2] = true;
    fixture.advance(1100);
    CHECK(fixture.simulation.mState.rotorBand == AirportRotorBand::Running);
    CHECK(fixture.simulation.mState.phase == AirportWindPhase::Generating);

    CHECK(fixture.communicator->enqueue("v=1&requestId=10&action=scenario&name=steady"));
    fixture.tick(50);
    CHECK(fixture.simulation.mState.targetAngle == firstTarget);

    CHECK(fixture.communicator->enqueue("v=1&requestId=11&action=scenario&name=realign"));
    fixture.tick(50);
    CHECK(fixture.simulation.mState.targetAngle != firstTarget);
    CHECK(!fixture.simulation.mState.aligned);
    CHECK(fixture.simulation.mState.phase == AirportWindPhase::Realigning);
    CHECK(!fixture.target->plantOutputs[1]);
    CHECK(fixture.target->plantOutputs[2]);

    fixture.target->controllerOutputs[4] = false;
    CHECK(fixture.communicator->enqueue("v=1&requestId=12&action=scenario&name=highWind"));
    fixture.tick(50);
    CHECK(fixture.target->plantOutputs[1]);
    CHECK(fixture.target->plantOutputs[2]);
    CHECK(!fixture.target->plantOutputs[4]);
    fixture.advance(1500);
    CHECK(fixture.simulation.mState.rotorBand == AirportRotorBand::Stopped);
    CHECK(fixture.simulation.mState.notice == AirportWindNotice::UnsafeGenerator);

    CHECK(fixture.communicator->enqueue("v=1&requestId=13&action=reset"));
    fixture.tick(50);
    CHECK(fixture.target->plantOutputs[0]);
    CHECK(fixture.simulation.mState.phase == AirportWindPhase::Reset);
    CHECK(fixture.simulation.mState.activeRequestId == 13);
    CHECK(fixture.simulation.mState.notice == AirportWindNotice::None);
}

} // namespace

int main() {
    testRequestParsing();
    testCompleteSerialization();
    testInitializationAndHeartbeat();
    testResetTimingAndSerializedRequests();
    testGenerationGatesAndHighWindOverride();
    testCompleteScenarioFlow();

    if (failures != 0) {
        std::cerr << failures << " airport wind station test(s) failed" << std::endl;
        return 1;
    }
    std::cout << "Airport wind station tests passed" << std::endl;
    return 0;
}
