#ifndef HYBRIDAPI_AIRPORT_TERMINAL_GARAGE_H
#define HYBRIDAPI_AIRPORT_TERMINAL_GARAGE_H
#include "../labsland/simulations/simulation.h"
#include <cstdint>
#include <string>
enum class GarageAction { Invalid, Sync, State, Passage, Restart };
struct GarageCheckpoint {
 std::uint32_t parkedMask=0,physicalCount=0,carId=0,bayId=0,waypoint=0,progress=0,movementState=0,passageId=0;
};
struct AirportGarageRequest:BaseInputDataType {
 GarageAction action=GarageAction::Invalid;
 std::uint32_t sessionId=0,requestSeq=0,eventId=0;
 bool entry=false,entryWaiting=false,exitWaiting=false,worldReady=false;
 GarageCheckpoint checkpoint;
 bool deserialize(std::string const&) override;
};
struct AirportGarageData:BaseOutputDataType {
 std::uint32_t seq=0,sessionId=1,requestSeq=0,eventId=0,requestFloor=0;
 bool entryWaiting=false,exitWaiting=false,entryComplete=false,exitComplete=false,worldReady=false,sampleValid=false;
 bool openEntrance=false,openExit=false,entrancePermitLamp=false,exitPermitLamp=false,fullLamp=false,completionAck=false;
 GarageCheckpoint checkpoint;
 // Fixed literals only; reports contain no browser-supplied strings.
 const char* eventStatus="none";const char* requestStatus="completed";const char* notice="none";
 std::string serialize() const override;
};
class AirportTerminalGarageSimulation:public Simulation<AirportGarageData,AirportGarageRequest> {
public:
 void initialize() override;
 void update(double) override;
 void reportUpdate() override;
 std::uint32_t getSleepStepInMs() override{return 10;}
private:
 double staleSeconds=0,heartbeat=0;
 std::uint32_t lastRequest=0,lastEvent=0;
 std::string lastPayload;
 bool initialized=false;
 void apply(AirportGarageRequest const&);
 void writeSensors();
};
#endif
