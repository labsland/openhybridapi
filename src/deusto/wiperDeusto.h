#ifndef HYBRIDAPI_WIPER_H
#define HYBRIDAPI_WIPER_H

#include "../labsland/simulations/simulation.h"
#include <sstream>
#include <string>
#include <vector>

struct WiperDeustoData : public BaseOutputDataType {
    bool moving = false;
    float wiperAngle = 0.0f;
    bool leftSensor = false;
    bool rightSensor = false;

    std::string serialize() const {
        std::stringstream stream;
        stream << moving << "&" << wiperAngle << "&" << leftSensor << "&" << rightSensor;
        return stream.str();
    }
};

struct WiperDeustoRequest : public BaseInputDataType {
    bool rainSensor = false;
    bool mButton = false;
    bool pButton = false;
    bool error = false;

    bool deserialize(std::string const & input) {
        std::stringstream stream(input);
        std::string segment;
        std::vector<std::string> segments;

        while (std::getline(stream, segment, '&')) {
            segments.push_back(segment);
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

        if (segments.size() == 4) {
            return parseBit(segments[0], rainSensor)
                && parseBit(segments[1], mButton)
                && parseBit(segments[2], pButton)
                && parseBit(segments[3], error);
        }

        if (segments.size() == 5) {
            error = false;
            return parseBit(segments[0], rainSensor)
                && parseBit(segments[3], mButton)
                && parseBit(segments[4], pButton);
        }

        return false;
    }
};

class WiperDeustoSimulation : public Simulation<WiperDeustoData, WiperDeustoRequest> {
private:
    const float MIN_ANGLE = 5.0f;
    const float MAX_ANGLE = 160.0f;
    const float SPEED = 25.0f;
    int mDirection;
    bool forcedError = false;
public:
    WiperDeustoSimulation() = default;

    virtual void update(double delta) override;
    virtual void initialize() override;
};

#endif
