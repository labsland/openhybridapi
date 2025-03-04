 #include <string>
 #include <iostream>
 #include <cmath>
 #include "watertankDeusto.h"
 
 
 void WatertankDeustoSimulation::initialize() {
 
     this->targetDevice->initializeSimulation(
             {"lowSensorActive", "midSensorActive", "highSensorActive"},
             {"pump1", "pump2"}
     );
 
     float radius = WATERTANK_DIAMETER / 2;
     mState.totalVolume = M_PI * radius * radius * WATERTANK_HEIGHT * 1000;
 
     mState.volume = mState.totalVolume / 2;
     mState.level = 0.5;
 
     mState.pump1Active = true;
     mState.pump2Active = true;
 
     setReportWhenMarked(true);
 }
 
 int error[4][3] = {{ 0, 1, 0 },{ 1, 0, 0 },{ 1, 0, 1 },{ 1, 1, 0 }};
  void WatertankDeustoSimulation::update(double delta) {
 
     this->log() << "Updating simulation. Delta: " << delta << std::endl;
 
     WatertankDeustoRequest request;
     bool requestWasRead = readRequest(request);
     if (requestWasRead) {
          mCurrentDemandFlowrate = request.outputFlow;
          makeError = request.makeError;
          resetError = request.resetError;
     }
 
     double addedWater = 0;

     if (mState.pump1Temperature>=70) mState.pump1Hot = true;
     else mState.pump1Hot = false;
     if (mState.pump2Temperature>=70) mState.pump2Hot = true;
     else mState.pump2Hot = false;

     if (mState.pump1Temperature>=100) mState.pump1Broken = true;
     else mState.pump1Broken = false;
     if (mState.pump2Temperature>=100) mState.pump2Broken = true;
     else mState.pump2Broken = false;

     mState.pump1Active = this->targetDevice->getGpio("pump1");
     mState.pump2Active = this->targetDevice->getGpio("pump2");
 
     this->log() << "Pumps: pump1: " << mState.pump1Active << "; pump2: " << mState.pump2Active << std::endl;
 
     if(mState.pump1Active) {
        if(mState.pump1Temperature<100) mState.pump1Temperature += 6 * delta;
        else mState.pump1Temperature = 100;

        if(!mState.pump1Broken) addedWater += PUMP1_FLOWRATE * delta;
     }else{
        if(mState.pump1Temperature>0) mState.pump1Temperature -= 6 * delta;
        else mState.pump1Temperature = 0;
     }
 
     if(mState.pump2Active) {
        if(mState.pump2Temperature<100) mState.pump2Temperature += 6 * delta;
        else mState.pump2Temperature = 100;

        if(!mState.pump2Broken) addedWater += PUMP2_FLOWRATE * delta;
     }else{
        if(mState.pump2Temperature>0) mState.pump2Temperature -= 6 * delta;
        else mState.pump2Temperature = 0;
     }
 
     float removedWater = 0;
     removedWater = mCurrentDemandFlowrate * delta;
 
     this->log() << "Old volume: " << mState.volume << "; adding: " << addedWater << "; removing (flow rate=" << mCurrentDemandFlowrate << "): " << removedWater << std::endl;
 
     mState.volume = mState.volume + addedWater - removedWater;
     if (mState.volume < 0) {
         mState.volume = 0;
     }
     if (mState.volume > mState.totalVolume) {
         mState.volume = mState.totalVolume;
     }
 
     mState.level = mState.volume / mState.totalVolume;
     
     if(resetError && isBroken){
            isBroken = false;
      }
      if(makeError && !isBroken){

            isBroken = true;
            int randomError = rand() % 4;

            mState.lowSensorActive = error[randomError][2];
            mState.midSensorActive = error[randomError][1];
            mState.highSensorActive = error[randomError][0];
      }
     
     this->log() << "New volume: " << mState.volume << "; which is level=" << mState.level << std::endl;
     if(!isBroken)
     {
        mState.lowSensorActive = mState.level >= 0.20;
        mState.midSensorActive = mState.level >= 0.50;
        mState.highSensorActive = mState.level >= 0.80;
     }
     this->log() << "Sensors: Low (0.2): " << mState.lowSensorActive << "; Mid (0.5): " << mState.midSensorActive << "; High (0.8): " << mState.highSensorActive << std::endl;
 
     this->targetDevice->setGpio("lowSensorActive", mState.lowSensorActive);
     this->targetDevice->setGpio("midSensorActive", mState.midSensorActive);
     this->targetDevice->setGpio("highSensorActive", mState.highSensorActive);

     
     this->log() << "of: " << request.outputFlow << "; err: " << request.makeError << "; res: " << request.resetError << std::endl;

     requestReportState();
 }


 
 