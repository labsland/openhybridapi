 #include <string>
 #include <iostream>
 #include <cmath>
 #include "watertankDeusto.h"
 
 
 void WatertankDeustoSimulation::initialize() {
 
     this->targetDevice->initializeSimulation(
             {"lowSensorActive", "midSensorActive", "highSensorActive", "pump1Hot", "pump2Hot"},
             {"pump1Bit0","pump1Bit1", "pump2Bit0", "pump2Bit1"}
     );
 
     float radius = WATERTANK_DIAMETER / 2;
     mState.totalVolume = M_PI * radius * radius * WATERTANK_HEIGHT * 1000;
 
     mState.volume = mState.totalVolume / 2;
     mState.level = 0.9;
 
     mState.pump1ActiveBit0 = false;
     mState.pump1ActiveBit1 = false;
     mState.pump2ActiveBit0 = false;
     mState.pump2ActiveBit1 = false;
 
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

     if (mState.pump1Temperature>=85) mState.pump1Hot = true;
     else mState.pump1Hot = false;
     if (mState.pump2Temperature>=85) mState.pump2Hot = true;
     else mState.pump2Hot = false;

     if (mState.pump1Temperature>=100) mState.pump1Broken = true;
     else mState.pump1Broken = false;
     if (mState.pump2Temperature>=100) mState.pump2Broken = true;
     else mState.pump2Broken = false;

     mState.pump1ActiveBit0 = this->targetDevice->getGpio("pump1Bit0");
     mState.pump1ActiveBit1 = this->targetDevice->getGpio("pump1Bit1");

     mState.pump2ActiveBit0 = this->targetDevice->getGpio("pump2Bit0");
     mState.pump2ActiveBit1 = this->targetDevice->getGpio("pump2Bit1");

     this->log() << "Pumps: pump1: " << mState.pump1ActiveBit0 << mState.pump1ActiveBit1 << "; pump2: " << mState.pump2ActiveBit0 << mState.pump2ActiveBit1 << std::endl;
 
     if(mState.pump1ActiveBit0 || mState.pump1ActiveBit1) {
        if(mState.pump1Temperature<100) mState.pump1Temperature += 3 * delta;
        else mState.pump1Temperature = 100;

        if (!mState.pump1Broken) {
            if (mState.pump1ActiveBit0 == 1 && mState.pump1ActiveBit1 == 1) {
               addedWater += 2 * PUMP1_FLOWRATE * delta;
            } else {
               addedWater += PUMP1_FLOWRATE * delta;
            }
         }
     }else{
        if(mState.pump1Temperature>0) mState.pump1Temperature -= 6 * delta;
        else mState.pump1Temperature = 0;
     }
 
     if(mState.pump2ActiveBit0 || mState.pump2ActiveBit1) {
        if(mState.pump2Temperature<100) mState.pump2Temperature += 3 * delta;
        else mState.pump2Temperature = 100;
        
        if (!mState.pump2Broken) {
            if (mState.pump2ActiveBit0 == 1 && mState.pump2ActiveBit1 == 1) {
               addedWater += 2 * PUMP2_FLOWRATE * delta;
            } else {
               addedWater += PUMP2_FLOWRATE * delta;
            }
         }
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
     this->targetDevice->setGpio("pump1Hot", mState.pump1Hot);
     this->targetDevice->setGpio("pump2Hot", mState.pump2Hot);
     
     this->log() << "of: " << request.outputFlow << "; err: " << request.makeError << "; res: " << request.resetError << std::endl;

     requestReportState();
 }


 
 