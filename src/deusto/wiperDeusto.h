#ifndef HYBRIDAPI_WIPER_H
#define HYBRIDAPI_WIPER_H

#include "../labsland/simulations/simulation.h"
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

struct WiperDeustoData : public BaseOutputDataType
{

    bool move;

    std::string serialize() const
    {
        std::stringstream stream;
        stream << move << "&";
        return stream.str();
    }
};

struct WiperDeustoRequest : public BaseInputDataType
{
    bool rainSensor;
    bool leftSensor;
    bool rightSensor;
    bool mButton;
    bool pButton;


    bool deserialize(std::string const &input)
    {
        std::stringstream stream(input);
        std::string segment;
        std::vector<std::string> segments;

        while (std::getline(stream, segment, '&'))
        {
            segments.push_back(segment);
        }

        if (segments.size() != 5) {
            return false;
        }

        rainSensor = (segments[0] == "1");
        leftSensor = (segments[1] == "1");
        rightSensor = (segments[2] == "1");
        mButton = (segments[3] == "1");
        pButton = (segments[4] == "1");

        return true;
    }
};

class WiperDeustoSimulation : public Simulation<WiperDeustoData, WiperDeustoRequest>
{

public:
    WiperDeustoSimulation() = default;

    virtual void update(double delta) override;

    virtual void initialize() override;
};

#endif
