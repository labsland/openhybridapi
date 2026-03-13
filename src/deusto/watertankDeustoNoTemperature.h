 #ifndef HYBRIDAPI_WATERTANKDEUSTO_NOTEMPERATURE_H
 #define HYBRIDAPI_WATERTANKDEUSTO_NOTEMPERATURE_H
 
 #include "../labsland/simulations/simulation.h"
 #include <string>
 #include <sstream>
 #include <vector>
 #include <algorithm>
 
 struct WatertankDeustoNoTemperatureData : public BaseOutputDataType {
 
     float level;
     float totalVolume;
     float volume;
     bool pump1ActiveBit0;
     bool pump1ActiveBit1;
     bool pump2ActiveBit0;
     bool pump2ActiveBit1;
     float currentLoad;
     bool lowSensorActive;
     bool midSensorActive;
     bool highSensorActive;

     std::string serialize() const {
         std::stringstream stream;
         stream << level << "&" << totalVolume << "&" << volume << "&" << pump1ActiveBit0 << pump1ActiveBit1 << "&" << pump2ActiveBit0 << pump2ActiveBit1 << "&" <<
            currentLoad << "&" << lowSensorActive << "&" << midSensorActive << "&" << highSensorActive << "&";
         return stream.str();
     }
 };
 
 struct WatertankDeustoNoTemperatureRequest : public BaseInputDataType {
     float outputFlow;
     bool makeError;
     bool resetError;
     bool deserialize(std::string const & input) {
        std::string data = input;
        std::replace(data.begin(), data.end(), '&', ' ');
        std::replace(data.begin(), data.end(), ',', '.');
        std::stringstream stream(data);
        int errorInt, resetInt;
        if (!(stream >> outputFlow >> errorInt >> resetInt)) {
            return false; 
        }
        makeError = (errorInt != 0);
        resetError = (resetInt != 0) && !makeError;

        return true;
    }
 };
 

 class WatertankDeustoNoTemperatureSimulation : public Simulation<WatertankDeustoNoTemperatureData, WatertankDeustoNoTemperatureRequest> {
 private:
 
     const float WATERTANK_HEIGHT = 6.0f;
     const float WATERTANK_DIAMETER = 4.0f;
 
     const float PUMP1_FLOWRATE = 2000;
     const float PUMP2_FLOWRATE = 2000;
 
     float mCurrentDemandFlowrate = 0;
     
     int error[4][3] = {{ 0, 1, 0 },{ 1, 0, 0 },{ 1, 0, 1 },{ 1, 1, 0 }};
     bool makeError = false;
     bool resetError = false;
     bool isBroken = false;
 
 public:
 
     WatertankDeustoNoTemperatureSimulation() = default;
 
     virtual void update(double delta) override;
 
     virtual void initialize() override;
 };
 
 
 #endif
 