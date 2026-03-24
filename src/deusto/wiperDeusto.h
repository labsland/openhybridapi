#ifndef HYBRIDAPI_WIPER_H
#define HYBRIDAPI_WIPER_H

#include "../labsland/simulations/simulation.h"
#include <sstream>
#include <string>
#include <vector>

struct WiperDeustoData : public BaseOutputDataType {
    bool move = false;

    std::string serialize() const {
        std::stringstream stream;
        stream << move << "&";
        return stream.str();
    }
};

struct WiperDeustoRequest : public BaseInputDataType {
    bool rainSensor = false;
    bool leftSensor = false;
    bool rightSensor = false;
    bool mButton = false;
    bool pButton = false;

    bool deserialize(std::string const & input) {
        std::stringstream stream(input);
        std::string segment;
        std::vector<std::string> segments;

        while (std::getline(stream, segment, '&')) {
            segments.push_back(segment);
        }

        if (segments.size() != 5) {
            return false;
        }

        auto parseBit = [](std::string const & token, bool & out) {
            if (token == "1") {
                out = true;
                return true;
            }
            if (token == "0") {
                out = false;
                return true;
            }
            return false;
        };

        return parseBit(segments[0], rainSensor)
            && parseBit(segments[1], rightSensor)
            && parseBit(segments[2], leftSensor)
            && parseBit(segments[3], mButton)
            && parseBit(segments[4], pButton);
    }
};

class WiperDeustoSimulation : public Simulation<WiperDeustoData, WiperDeustoRequest> {
public:
    WiperDeustoSimulation() = default;

    virtual void update(double delta) override;
    virtual void initialize() override;
};

#endif
