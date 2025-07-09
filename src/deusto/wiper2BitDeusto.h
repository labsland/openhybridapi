#ifndef HYBRIDAPI_WIPER_2BIT_H
#define HYBRIDAPI_WIPER_2BIT_H

#include "../labsland/simulations/simulation.h"
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

struct Wiper2BitDeustoData : public BaseOutputDataType
{

    bool move1;
    bool move2;

    std::string serialize() const
    {
        std::stringstream stream;
        stream << move1 << "&" << move2 << "&";
        return stream.str();
    }
};

struct Wiper2BitDeustoRequest : public BaseInputDataType
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

class Wiper2BitDeustoSimulation : public Simulation<Wiper2BitDeustoData, Wiper2BitDeustoRequest>
{

public:
    Wiper2BitDeustoSimulation() = default;

    virtual void update(double delta) override;

    virtual void initialize() override;
};

#endif
