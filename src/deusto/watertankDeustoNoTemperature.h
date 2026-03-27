 #ifndef HYBRIDAPI_WATERTANKDEUSTO_NOTEMPERATURE_H
 #define HYBRIDAPI_WATERTANKDEUSTO_NOTEMPERATURE_H
 
 #include "../labsland/simulations/simulation.h"
 #include <algorithm>
 #include <sstream>
 #include <string>
 #include <vector>
 
struct WatertankDeustoNoTemperatureData : public BaseOutputDataType {
    float level = 0.0f;
    float totalVolume = 0.0f;
    float volume = 0.0f;
     bool pump1ActiveBit0 = false;
     bool pump1ActiveBit1 = false;
     bool pump2ActiveBit0 = false;
     bool pump2ActiveBit1 = false;
     float currentLoad = 0.0f;
    bool lowSensorActive = false;
    bool midSensorActive = false;
    bool highSensorActive = false;

    static std::string serializePumpState(bool bit0, bool bit1) {
        if (bit0 && bit1) {
            return "11";
        }
        if (bit0 || bit1) {
            return "01";
        }
        return "00";
    }

    std::string serialize() const {
        std::stringstream stream;
        stream << level << "&" << totalVolume << "&" << volume << "&"
               << serializePumpState(pump1ActiveBit0, pump1ActiveBit1) << "&"
               << serializePumpState(pump2ActiveBit0, pump2ActiveBit1) << "&"
               << currentLoad << "&" << lowSensorActive << "&"
               << midSensorActive << "&" << highSensorActive << "&";
        return stream.str();
    }
};
 
 struct WatertankDeustoNoTemperatureRequest : public BaseInputDataType {
     float outputFlow = 0.0f;
     bool makeError = false;
     bool resetError = false;
 
     bool deserialize(std::string const & input) {
         std::string data = input;
         std::replace(data.begin(), data.end(), '&', ' ');
         std::replace(data.begin(), data.end(), ',', '.');
         std::stringstream stream(data);
         int errorInt = 0;
         int resetInt = 0;
         if (!(stream >> outputFlow >> errorInt >> resetInt)) {
             return false;
         }
         makeError = (errorInt != 0);
         resetError = (resetInt != 0) && !makeError;
         return true;
     }
 };
 
 class WatertankDeustoNoTemperatureSimulation
     : public Simulation<WatertankDeustoNoTemperatureData, WatertankDeustoNoTemperatureRequest> {
 private:
     const float WATERTANK_HEIGHT = 6.0f;
     const float WATERTANK_DIAMETER = 4.0f;
     const float PUMP1_FLOWRATE = 2000.0f;
     const float PUMP2_FLOWRATE = 2000.0f;
 
     float mCurrentDemandFlowrate = 0.0f;
     int error[4][3] = {{0, 1, 0}, {1, 0, 0}, {1, 0, 1}, {1, 1, 0}};
     bool makeError = false;
     bool resetError = false;
     bool isBroken = false;
 
 public:
     WatertankDeustoNoTemperatureSimulation() = default;
 
     virtual void update(double delta) override;
     virtual void initialize() override;
 };
 
 #endif
