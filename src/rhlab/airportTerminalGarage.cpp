#include "airportTerminalGarage.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <set>
namespace {
bool number(std::map<std::string,std::string>const&a,const char*k,std::uint32_t&out,std::uint32_t max=0xffffffffu){auto i=a.find(k);if(i==a.end()||i->second.empty()||i->second.size()>10)return false;std::uint64_t n=0;for(char c:i->second){if(c<'0'||c>'9')return false;n=n*10+(c-'0');if(n>max)return false;}out=static_cast<std::uint32_t>(n);return true;}
bool bit(std::map<std::string,std::string>const&a,const char*k,bool&out){std::uint32_t n=0;if(!number(a,k,n,1))return false;out=n;return true;}
bool readCheckpoint(std::map<std::string,std::string>const&a,GarageCheckpoint&c){return number(a,"parkedMask",c.parkedMask,0x1ffffff)&&number(a,"physicalCount",c.physicalCount,25)&&number(a,"carId",c.carId,64)&&number(a,"bayId",c.bayId,25)&&number(a,"waypoint",c.waypoint,2048)&&number(a,"progress",c.progress,10000)&&number(a,"movementState",c.movementState,7)&&number(a,"passageId",c.passageId);}
std::string checkpointText(GarageCheckpoint const&c){std::ostringstream o;o<<"&parkedMask="<<c.parkedMask<<"&physicalCount="<<c.physicalCount<<"&carId="<<c.carId<<"&bayId="<<c.bayId<<"&waypoint="<<c.waypoint<<"&progress="<<c.progress<<"&movementState="<<c.movementState<<"&passageId="<<c.passageId;return o.str();}
}
bool AirportGarageRequest::deserialize(std::string const&raw){
 *this=AirportGarageRequest{};if(raw.empty()||raw.size()>1024)return false;
 // Reject duplicate keys instead of letting a query map silently choose one.
 std::set<std::string> keys;std::istringstream stream(raw);std::string token;
 while(std::getline(stream,token,'&')){auto p=token.find('=');if(p==std::string::npos||!keys.insert(token.substr(0,p)).second)return false;}
 auto a=parseQueryArgs(raw);std::uint32_t v=0;
 if(!number(a,"v",v,3)||v!=3||!number(a,"requestSeq",requestSeq)||!requestSeq||!a.count("action"))return false;
 auto name=a.at("action");
 if(name=="sync"){if(a.size()!=3)return false;action=GarageAction::Sync;return true;}
 if(!number(a,"sessionId",sessionId)||!sessionId)return false;
 if(name=="restart"){if(a.size()!=4)return false;action=GarageAction::Restart;return true;}
 if(name=="state"){
  if(a.size()!=15||!readCheckpoint(a,this->checkpoint)||!bit(a,"entryWaiting",entryWaiting)||!bit(a,"exitWaiting",exitWaiting)||!bit(a,"worldReady",worldReady))return false;
  action=GarageAction::State;return true;
 }
 if(name=="passage"){
  if(a.size()!=14||!readCheckpoint(a,this->checkpoint)||!number(a,"eventId",eventId)||!eventId||!a.count("direction"))return false;
  if(a.at("direction")!="entry"&&a.at("direction")!="exit")return false;entry=a.at("direction")=="entry";action=GarageAction::Passage;return true;
 }
 return false;
}
std::string AirportGarageData::serialize()const{
 std::ostringstream o;o<<"v=3&activity=airportTerminalGarage&seq="<<seq<<"&sessionId="<<sessionId<<"&requestSeq="<<requestSeq<<"&requestFloor="<<requestFloor<<"&eventId="<<eventId
 <<"&entryWaiting="<<entryWaiting<<"&exitWaiting="<<exitWaiting<<"&entryComplete="<<entryComplete<<"&exitComplete="<<exitComplete<<"&worldReady="<<worldReady
 <<"&sampleValid="<<sampleValid<<"&openEntrance="<<openEntrance<<"&openExit="<<openExit<<"&entrancePermitLamp="<<entrancePermitLamp<<"&exitPermitLamp="<<exitPermitLamp<<"&fullLamp="<<fullLamp<<"&completionAck="<<completionAck
 <<checkpointText(checkpoint)<<"&eventStatus="<<eventStatus<<"&requestStatus="<<requestStatus<<"&notice="<<notice;return o.str();
}
void AirportTerminalGarageSimulation::initialize(){
 initialized=targetDevice->initializeSimulation({"entryWaiting","exitWaiting","entryComplete","exitComplete","worldReady"},{"openEntrance","openExit","entrancePermitLamp","exitPermitLamp","fullLamp","completionAck"});
 mState=AirportGarageData{};mState.sessionId=static_cast<std::uint32_t>(timeManager->getAbsoluteTime())|1u;
 mState.notice=initialized?"none":"recoveryRequired";setReportWhenMarked(true);setVirtualEnvironmentReportPeriod(.02f);writeSensors();requestReportState();
}
void AirportTerminalGarageSimulation::apply(AirportGarageRequest const&r){
 if(r.action==GarageAction::Sync){mState.requestSeq=r.requestSeq;mState.requestStatus="completed";requestReportState();return;}
 if(r.sessionId!=mState.sessionId){mState.notice="invalidRequest";requestReportState();return;}
 const std::string payload=std::to_string(static_cast<int>(r.action))+checkpointText(r.checkpoint)+std::to_string(r.eventId)+std::to_string(r.entry)+std::to_string(r.entryWaiting)+std::to_string(r.exitWaiting)+std::to_string(r.worldReady);
 mState.requestSeq=r.requestSeq;
 if(r.requestSeq<lastRequest||(r.requestSeq==lastRequest&&payload!=lastPayload)){mState.requestStatus="rejected";mState.notice="invalidRequest";requestReportState();return;}
 if(r.requestSeq==lastRequest){staleSeconds=0;requestReportState();return;}
 lastRequest=r.requestSeq;mState.requestFloor=lastRequest;lastPayload=payload;staleSeconds=0;mState.requestStatus="completed";mState.notice="none";
 if(r.action==GarageAction::Restart){
  // Explicit world baseline only. Never assert a controller-reset GPIO.
  const auto seq=mState.seq,session=mState.sessionId;const bool valid=mState.sampleValid;
  mState=AirportGarageData{};mState.seq=seq;mState.sessionId=session+1u;if(!mState.sessionId)mState.sessionId=1;mState.requestSeq=r.requestSeq;mState.sampleValid=valid;lastRequest=lastEvent=0;lastPayload.clear();lastEventPayload.clear();
 }else if(r.action==GarageAction::State){
  unsigned occupied=0;for(auto mask=r.checkpoint.parkedMask;mask;mask>>=1)occupied+=mask&1u;
  if(r.checkpoint.passageId!=lastEvent||r.checkpoint.physicalCount!=mState.checkpoint.physicalCount||occupied>r.checkpoint.physicalCount){mState.requestStatus="rejected";mState.notice="invalidRequest";requestReportState();return;}
  mState.entryWaiting=r.entryWaiting;mState.exitWaiting=r.exitWaiting;mState.worldReady=r.worldReady&&initialized;
  mState.checkpoint=r.checkpoint;
 }else if(r.action==GarageAction::Passage){
  const std::string eventPayload=std::to_string(r.entry)+checkpointText(r.checkpoint);
  if(r.eventId==lastEvent){if(eventPayload!=lastEventPayload){mState.requestStatus="rejected";mState.notice="invalidRequest";}}
  else if(!mState.worldReady||mState.entryComplete||mState.exitComplete||mState.completionAck||std::string(mState.eventStatus)=="delivered"){mState.requestStatus="rejected";mState.notice="busy";}
  else if(r.eventId!=lastEvent+1||r.checkpoint.passageId!=r.eventId||
   (r.entry?(!mState.entryWaiting||!mState.openEntrance||r.checkpoint.physicalCount!=mState.checkpoint.physicalCount+1):( !mState.exitWaiting||!mState.openExit||mState.checkpoint.physicalCount==0||r.checkpoint.physicalCount+1!=mState.checkpoint.physicalCount))){mState.requestStatus="rejected";mState.notice="invalidRequest";}
  else{lastEvent=r.eventId;lastEventPayload=eventPayload;mState.eventId=r.eventId;mState.checkpoint=r.checkpoint;mState.entryComplete=r.entry;mState.exitComplete=!r.entry;mState.eventStatus="latched";mState.requestStatus="accepted";}
 }
 writeSensors();requestReportState();
}
void AirportTerminalGarageSimulation::writeSensors(){if(!initialized)return;targetDevice->setGpio(0,mState.entryWaiting);targetDevice->setGpio(1,mState.exitWaiting);targetDevice->setGpio(2,mState.entryComplete);targetDevice->setGpio(3,mState.exitComplete);targetDevice->setGpio(4,mState.worldReady);}
void AirportTerminalGarageSimulation::update(double delta){
 if(!std::isfinite(delta)||delta<0)return;staleSeconds+=delta;heartbeat+=delta;
 if(initialized){bool* outputs[]={&mState.openEntrance,&mState.openExit,&mState.entrancePermitLamp,&mState.exitPermitLamp,&mState.fullLamp,&mState.completionAck};for(int i=0;i<6;i++){const bool v=targetDevice->getGpio(i);if(v!=*outputs[i])requestReportState();*outputs[i]=v;}mState.sampleValid=true;}
 AirportGarageRequest r;for(int i=0;i<8&&readRequest(r);i++)apply(r);
 if(staleSeconds>2&&mState.worldReady){mState.worldReady=false;mState.notice="stale";requestReportState();}
 // A stale world holds an in-flight completion; never lose an unacknowledged event.
 if(mState.worldReady&&mState.completionAck&&(mState.entryComplete||mState.exitComplete)){mState.entryComplete=mState.exitComplete=false;mState.eventStatus="delivered";requestReportState();}
 else if(mState.worldReady&&!mState.completionAck&&std::string(mState.eventStatus)=="delivered"){mState.eventStatus="cleared";mState.requestStatus="completed";requestReportState();}
 writeSensors();if(heartbeat>=.5){heartbeat=0;requestReportState();}
}
void AirportTerminalGarageSimulation::reportUpdate(){if(mShouldReportInReportWhenMarkedMode)++mState.seq;Simulation<AirportGarageData,AirportGarageRequest>::reportUpdate();}
