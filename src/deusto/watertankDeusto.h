 #ifndef HYBRIDAPI_WATERTANKDEUSTO_H
 #define HYBRIDAPI_WATERTANKDEUSTO_H
 
 #include "../labsland/simulations/simulation.h"
 #include <string>
 #include <sstream>
 #include <vector>
 #include <algorithm>
 
 struct WatertankDeustoData : public BaseOutputDataType {
 
     float level;
     float totalVolume;
     float volume;
     bool pump1Active;
     bool pump2Active;
     float pump1Temperature;
     float pump2Temperature;
     float currentLoad;
     bool lowSensorActive;
     bool midSensorActive;
     bool highSensorActive;
     bool pump1Hot;
     bool pump2Hot;
     bool pump1Broken;
     bool pump2Broken;

     std::string serialize() const {
         std::stringstream stream;
         stream << level << "&" << totalVolume << "&" << volume << "&" << pump1Active << "&" << pump2Active << "&" <<
             pump1Temperature << "&" << pump2Temperature << "&" << currentLoad << "&" << lowSensorActive << "&" <<
             midSensorActive << "&" << highSensorActive << "&"<< pump1Hot << "&" << pump2Hot << "&" <<
             pump1Broken << "&" << pump2Broken << "&";
         return stream.str();
     }
 };
 
 struct WatertankDeustoRequest : public BaseInputDataType {
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
 

 class WatertankDeustoSimulation : public Simulation<WatertankDeustoData, WatertankDeustoRequest> {
 private:
 
     const float WATERTANK_HEIGHT = 6.0f;
     const float WATERTANK_DIAMETER = 4.0f;
 
     const float PUMP1_FLOWRATE = 2000;
     const float PUMP2_FLOWRATE = 2000;
 
     float mCurrentDemandFlowrate = 0;

     bool makeError = false;
     bool resetError = false;
     bool isBroken = false;
 
 public:
 
     WatertankDeustoSimulation() = default;
 
     virtual void update(double delta) override;
 
     virtual void initialize() override;
 };
 
 
 #endif
 