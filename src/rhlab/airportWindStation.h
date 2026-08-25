#ifndef HYBRIDAPI_AIRPORT_WIND_STATION_H
#define HYBRIDAPI_AIRPORT_WIND_STATION_H

#include "../labsland/simulations/simulation.h"

#include <cstdint>
#include <string>

enum class AirportWindAction : std::uint8_t {
    Invalid,
    Sync,
    Scenario,
    Reset
};

enum class AirportWindScenario : std::uint8_t {
    Calm,
    Steady,
    Realign,
    HighWind
};

enum class AirportWindPhase : std::uint8_t {
    Reset,
    Calm,
    Steady,
    Aligning,
    Realigning,
    Generating,
    HighWind
};

enum class AirportWindBand : std::uint8_t {
    Calm,
    Steady,
    High
};

enum class AirportRotorBand : std::uint8_t {
    Stopped,
    Accelerating,
    Running,
    Decelerating
};

enum class AirportWindNotice : std::uint8_t {
    None,
    UnsafeGenerator,
    UnsafeInactive,
    UnsafeAlign,
    MissingRunwayAlert,
    UnexpectedRunwayAlert
};

struct AirportWindStationRequest : public BaseInputDataType {
    std::uint32_t requestId = 0;
    AirportWindAction action = AirportWindAction::Invalid;
    AirportWindScenario scenario = AirportWindScenario::Calm;

    bool deserialize(std::string const & input) override;
};

struct AirportWindStationData : public BaseOutputDataType {
    std::uint32_t seq = 0;
    AirportWindScenario scenario = AirportWindScenario::Calm;
    AirportWindPhase phase = AirportWindPhase::Reset;
    std::int16_t windDirection = 0;
    AirportWindBand windBand = AirportWindBand::Calm;
    bool aligned = false;
    std::int16_t nacelleAngle = 0;
    std::int16_t targetAngle = 0;
    AirportRotorBand rotorBand = AirportRotorBand::Stopped;
    bool align = false;
    bool generatorEnable = false;
    bool active = false;
    bool runwayWindAlert = false;
    std::uint32_t activeRequestId = 0;
    AirportWindNotice notice = AirportWindNotice::None;

    std::string serialize() const override;
};

class AirportWindStationSimulation
    : public Simulation<AirportWindStationData, AirportWindStationRequest> {
public:
    AirportWindStationSimulation() = default;

    void initialize() override;
    void update(double delta) override;
    void reportUpdate() override;
    std::uint32_t getSleepStepInMs() override;

private:
    static const double RESET_SECONDS;
    static const double ALIGNMENT_SECONDS;
    static const double ROTOR_RAMP_SECONDS;
    static const double UNSAFE_GRACE_SECONDS;
    static const double HEARTBEAT_SECONDS;

    double resetRemaining = 0.0;
    double alignmentProgress = 0.0;
    double yawStartAngle = 0.0;
    double nacelleAngle = 0.0;
    double rotorLevel = 0.0;
    double unsafeDuration = 0.0;
    double heartbeatElapsed = 0.0;

    bool resetAsserted = false;
    bool haveControllerSample = false;
    bool sampledAlign = false;
    bool sampledGeneratorEnable = false;
    bool sampledActive = false;
    bool sampledRunwayWindAlert = false;

    void applyRequest(AirportWindStationRequest const & request);
    void applyScenario(AirportWindScenario scenario);
    void beginReset();
    void chooseNextBearing();
    void sampleControllerOutputs();
    void updateYaw(double delta);
    void updateRotor(double delta);
    void updatePhase();
    void updateNotice(double delta);
    void writePlantOutputs();
    bool stateChangedFrom(AirportWindStationData const & previous) const;
};

#endif
