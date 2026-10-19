#include "rhlab/airportTerminalGarage.h"

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
        return config->getOutputGpios() <= 5 && config->getInputGpios() <= 6;
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
    : public LabsLand::Simulations::Utils::SimulationCommunicator<AirportGarageData, AirportGarageRequest> {
public:
    bool enqueue(std::string const & raw) {
        AirportGarageRequest request;
        if (!request.deserialize(raw)) return false;
        requests.push_back(request);
        return true;
    }

    bool readRequest(AirportGarageRequest & request) override {
        if (requests.empty()) return false;
        request = requests.front();
        requests.pop_front();
        return true;
    }

    void sendReport(AirportGarageData & report) override {
        reports.push_back(report.serialize());
    }

    std::deque<AirportGarageRequest> requests;
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

    AirportTerminalGarageSimulation simulation;
    std::shared_ptr<FakeTimeManager> time;
    std::shared_ptr<FakeTargetDevice> target;
    std::shared_ptr<FakeCommunicator> communicator;
};


std::string cp(unsigned count=0,unsigned mask=0,unsigned event=0){return "&parkedMask="+std::to_string(mask)+"&physicalCount="+std::to_string(count)+"&carId=1&bayId=1&waypoint=0&progress=0&movementState=0&passageId="+std::to_string(event);}
std::string request(unsigned id,const char*action,std::string fields=""){return "v=3&sessionId=1&requestSeq="+std::to_string(id)+"&action="+action+fields;}
void send(Fixture&f,std::string const&raw){CHECK(f.communicator->enqueue(raw));f.advance(30,10);}
void testParser(){AirportGarageRequest r;CHECK(r.deserialize("v=3&requestSeq=1&action=sync"));CHECK(!r.deserialize("v=3&v=3&requestSeq=1&action=sync"));CHECK(!r.deserialize("v=3&requestSeq=0&action=sync"));CHECK(!r.deserialize("v=3&requestSeq=4294967296&action=sync"));CHECK(!r.deserialize(request(1,"state",cp(26)+"&entryWaiting=0&exitWaiting=0&worldReady=1")));CHECK(!r.deserialize(request(1,"state",cp()+"&entryWaiting=true&exitWaiting=0&worldReady=1")));CHECK(r.deserialize(request(1,"state",cp()+"&entryWaiting=1&exitWaiting=0&worldReady=1")));}
void testDelivery(){Fixture f;CHECK(f.target->plantOutputs.size()==5);CHECK(f.target->controllerOutputs.size()==6);
 f.target->controllerOutputs[0]=true;send(f,request(1,"state",cp()+"&entryWaiting=1&exitWaiting=0&worldReady=1"));CHECK(f.target->plantOutputs[0]);
 auto passage=request(2,"passage",cp(1,0,1)+"&eventId=1&direction=entry");send(f,passage);CHECK(f.target->plantOutputs[2]);CHECK(f.simulation.mState.checkpoint.physicalCount==1);
 send(f,passage);CHECK(f.target->plantOutputs[2]);CHECK(f.simulation.mState.checkpoint.physicalCount==1);
 f.advance(2500);CHECK(!f.target->plantOutputs[4]);CHECK(f.target->plantOutputs[2]);CHECK(std::string(f.simulation.mState.notice)=="stale");
 send(f,request(3,"state",cp(1,0,1)+"&entryWaiting=1&exitWaiting=0&worldReady=1"));f.target->controllerOutputs[5]=true;f.advance(30);CHECK(!f.target->plantOutputs[2]);CHECK(std::string(f.simulation.mState.eventStatus)=="delivered");
 f.target->controllerOutputs[5]=false;f.advance(30);CHECK(std::string(f.simulation.mState.eventStatus)=="cleared");send(f,request(4,"passage",cp(1,0,1)+"&eventId=1&direction=entry"));CHECK(!f.target->plantOutputs[2]);CHECK(f.simulation.mState.checkpoint.physicalCount==1);
 send(f,request(5,"passage",cp(0,0,1)+"&eventId=1&direction=exit"));CHECK(std::string(f.simulation.mState.requestStatus)=="rejected");CHECK(!f.target->plantOutputs[3]);
 send(f,request(6,"state",cp(1,1,1)+"&entryWaiting=0&exitWaiting=1&worldReady=1"));f.target->controllerOutputs[1]=true;f.advance(20);send(f,request(7,"passage",cp(0,0,2)+"&eventId=2&direction=exit"));CHECK(f.target->plantOutputs[3]);CHECK(f.simulation.mState.checkpoint.physicalCount==0);
}
void testValidation(){Fixture f;send(f,request(1,"state",cp()+"&entryWaiting=1&exitWaiting=0&worldReady=1"));send(f,request(2,"passage",cp(1,0,1)+"&eventId=1&direction=entry"));CHECK(std::string(f.simulation.mState.requestStatus)=="rejected");CHECK(!f.target->plantOutputs[2]);
 send(f,request(3,"state",cp(10)+"&entryWaiting=1&exitWaiting=0&worldReady=1"));CHECK(f.simulation.mState.checkpoint.physicalCount==0);
 send(f,"v=3&sessionId=99&requestSeq=4&action=restart");CHECK(f.simulation.mState.sessionId==1);
 send(f,"v=3&requestSeq=400&action=sync");CHECK(f.simulation.mState.requestFloor==3);CHECK(f.simulation.mState.checkpoint.physicalCount==0);
 send(f,request(5,"restart"));CHECK(f.simulation.mState.sessionId==2);for(bool v:f.target->plantOutputs)CHECK(!v);
 auto old=request(6,"state",cp()+"&entryWaiting=1&exitWaiting=0&worldReady=1");send(f,old);CHECK(!f.target->plantOutputs[0]);
}
void testModeReselectionStartsFreshSession() {
    Fixture f;
    f.target->controllerOutputs[0] = true;
    send(f, request(1000, "state", cp() + "&entryWaiting=1&exitWaiting=0&worldReady=1"));
    send(f, request(1001, "passage", cp(1, 0, 1) + "&eventId=1&direction=entry"));
    CHECK(f.simulation.mState.checkpoint.physicalCount == 1);
    CHECK(f.target->plantOutputs[2]);
    f.advance(2500);

    // Firmware reuses this simulation object when a new user selects its mode.
    f.simulation._initialize();
    const auto session = f.simulation.mState.sessionId;
    CHECK(session != 1);
    CHECK(f.simulation.mState.requestFloor == 0);
    CHECK(f.simulation.mState.checkpoint.physicalCount == 0);
    for (bool value : f.target->plantOutputs) CHECK(!value);
    const auto freshRequest = [session](unsigned seq, const char* action, const std::string& fields) {
        return "v=3&sessionId=" + std::to_string(session) + "&requestSeq=" +
            std::to_string(seq) + "&action=" + action + fields;
    };
    f.target->controllerOutputs[0] = true;
    send(f, freshRequest(1, "state", cp() + "&entryWaiting=1&exitWaiting=0&worldReady=1"));
    CHECK(std::string(f.simulation.mState.requestStatus) == "completed");
    CHECK(f.simulation.mState.requestFloor == 1);
    CHECK(f.target->plantOutputs[4]);
    send(f, freshRequest(2, "passage", cp(1, 0, 1) + "&eventId=1&direction=entry"));
    CHECK(std::string(f.simulation.mState.requestStatus) == "accepted");
    CHECK(f.simulation.mState.checkpoint.physicalCount == 1);
    CHECK(f.target->plantOutputs[2]);
}
} // namespace
int main(){testParser();testDelivery();testValidation();testModeReselectionStartsFreshSession();if(failures){std::cerr<<failures<<" checks failed\n";return EXIT_FAILURE;}std::cout<<"Garage parsing, GPIO, delivery, deduplication, stale/reconnect and reset checks passed\n";return EXIT_SUCCESS;}
