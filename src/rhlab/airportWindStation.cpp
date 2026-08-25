#include "airportWindStation.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>
#include <sstream>

namespace {

const char * scenarioName(AirportWindScenario scenario) {
    switch (scenario) {
        case AirportWindScenario::Calm: return "calm";
        case AirportWindScenario::Steady: return "steady";
        case AirportWindScenario::Realign: return "realign";
        case AirportWindScenario::HighWind: return "highWind";
    }
    return "calm";
}

const char * phaseName(AirportWindPhase phase) {
    switch (phase) {
        case AirportWindPhase::Reset: return "reset";
        case AirportWindPhase::Calm: return "calm";
        case AirportWindPhase::Steady: return "steady";
        case AirportWindPhase::Aligning: return "aligning";
        case AirportWindPhase::Realigning: return "realigning";
        case AirportWindPhase::Generating: return "generating";
        case AirportWindPhase::HighWind: return "highWind";
    }
    return "calm";
}

const char * windBandName(AirportWindBand band) {
    switch (band) {
        case AirportWindBand::Calm: return "calm";
        case AirportWindBand::Steady: return "steady";
        case AirportWindBand::High: return "high";
    }
    return "calm";
}

const char * rotorBandName(AirportRotorBand band) {
    switch (band) {
        case AirportRotorBand::Stopped: return "stopped";
        case AirportRotorBand::Accelerating: return "accelerating";
        case AirportRotorBand::Running: return "running";
        case AirportRotorBand::Decelerating: return "decelerating";
    }
    return "stopped";
}

const char * noticeName(AirportWindNotice notice) {
    switch (notice) {
        case AirportWindNotice::None: return "none";
        case AirportWindNotice::UnsafeGenerator: return "unsafeGenerator";
        case AirportWindNotice::UnsafeInactive: return "unsafeInactive";
        case AirportWindNotice::UnsafeAlign: return "unsafeAlign";
        case AirportWindNotice::MissingRunwayAlert: return "missingRunwayAlert";
        case AirportWindNotice::UnexpectedRunwayAlert: return "unexpectedRunwayAlert";
    }
    return "none";
}

bool parseRequestId(std::string const & token, std::uint32_t & value) {
    if (token.empty()) {
        return false;
    }

    std::uint64_t parsed = 0;
    for (char character : token) {
        if (character < '0' || character > '9') {
            return false;
        }
        parsed = parsed * 10u + static_cast<std::uint64_t>(character - '0');
        if (parsed > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
    }

    if (parsed == 0) {
        return false;
    }

    value = static_cast<std::uint32_t>(parsed);
    return true;
}

AirportWindScenario parseScenario(std::string const & name, bool & valid) {
    valid = true;
    if (name == "calm") return AirportWindScenario::Calm;
    if (name == "steady") return AirportWindScenario::Steady;
    if (name == "realign") return AirportWindScenario::Realign;
    if (name == "highWind") return AirportWindScenario::HighWind;
    valid = false;
    return AirportWindScenario::Calm;
}

double shortestAngleDelta(double from, double to) {
    double delta = std::fmod(to - from + 540.0, 360.0) - 180.0;
    return delta;
}

double normalizeAngle(double angle) {
    double normalized = std::fmod(angle, 360.0);
    return normalized < 0.0 ? normalized + 360.0 : normalized;
}

} // namespace

const double AirportWindStationSimulation::RESET_SECONDS = 0.250;
const double AirportWindStationSimulation::ALIGNMENT_SECONDS = 2.500;
const double AirportWindStationSimulation::ROTOR_RAMP_SECONDS = 1.000;
const double AirportWindStationSimulation::UNSAFE_GRACE_SECONDS = 1.400;
const double AirportWindStationSimulation::HEARTBEAT_SECONDS = 0.500;

bool AirportWindStationRequest::deserialize(std::string const & input) {
    action = AirportWindAction::Invalid;
    requestId = 0;
    scenario = AirportWindScenario::Calm;

    std::map<std::string, std::string> args = parseQueryArgs(input);
    auto version = args.find("v");
    auto id = args.find("requestId");
    auto requestedAction = args.find("action");
    if (version == args.end() || version->second != "1"
            || id == args.end() || !parseRequestId(id->second, requestId)
            || requestedAction == args.end()) {
        return false;
    }

    if (requestedAction->second == "sync") {
        if (args.size() != 3) return false;
        action = AirportWindAction::Sync;
        return true;
    }

    if (requestedAction->second == "reset") {
        if (args.size() != 3) return false;
        action = AirportWindAction::Reset;
        return true;
    }

    if (requestedAction->second == "scenario") {
        auto name = args.find("name");
        if (args.size() != 4 || name == args.end()) return false;
        bool valid = false;
        scenario = parseScenario(name->second, valid);
        if (!valid) return false;
        action = AirportWindAction::Scenario;
        return true;
    }

    return false;
}

std::string AirportWindStationData::serialize() const {
    std::stringstream stream;
    stream << "v=1"
           << "&seq=" << seq
           << "&scenario=" << scenarioName(scenario)
           << "&phase=" << phaseName(phase)
           << "&windDirection=" << windDirection
           << "&windBand=" << windBandName(windBand)
           << "&aligned=" << static_cast<int>(aligned)
           << "&nacelleAngle=" << nacelleAngle
           << "&targetAngle=" << targetAngle
           << "&rotorBand=" << rotorBandName(rotorBand)
           << "&align=" << static_cast<int>(align)
           << "&generatorEnable=" << static_cast<int>(generatorEnable)
           << "&active=" << static_cast<int>(active)
           << "&runwayWindAlert=" << static_cast<int>(runwayWindAlert)
           << "&activeRequestId=" << activeRequestId
           << "&notice=" << noticeName(notice);
    return stream.str();
}

void AirportWindStationSimulation::initialize() {
    this->targetDevice->initializeSimulation(
        {"reset", "inputCode0", "inputCode1", "reservedLow", "aligned"},
        {"align", "reservedIgnored", "generatorEnable", "active", "runwayWindAlert"}
    );

    setReportWhenMarked(true);
    setVirtualEnvironmentReportPeriod(0.050f);
    beginReset();
    writePlantOutputs();
    requestReportState();
}

void AirportWindStationSimulation::beginReset() {
    mState.scenario = AirportWindScenario::Calm;
    mState.phase = AirportWindPhase::Reset;
    mState.windDirection = 0;
    mState.windBand = AirportWindBand::Calm;
    mState.aligned = false;
    mState.nacelleAngle = 0;
    mState.targetAngle = 0;
    mState.rotorBand = AirportRotorBand::Stopped;
    mState.align = false;
    mState.generatorEnable = false;
    mState.active = false;
    mState.runwayWindAlert = false;
    mState.notice = AirportWindNotice::None;

    resetAsserted = true;
    resetRemaining = RESET_SECONDS;
    alignmentProgress = 0.0;
    yawStartAngle = 0.0;
    nacelleAngle = 0.0;
    rotorLevel = 0.0;
    unsafeDuration = 0.0;
    hasPendingRequest = false;
}

void AirportWindStationSimulation::chooseNextBearing() {
    int next = (static_cast<int>(std::lround(mState.targetAngle)) + 90) % 360;
    if (std::abs(shortestAngleDelta(nacelleAngle, static_cast<double>(next))) < 1.0) {
        next = (next + 90) % 360;
    }
    yawStartAngle = nacelleAngle;
    alignmentProgress = 0.0;
    mState.targetAngle = static_cast<std::int16_t>(next);
    mState.windDirection = static_cast<std::int16_t>(next);
    mState.aligned = false;
}

void AirportWindStationSimulation::applyScenario(AirportWindScenario scenario) {
    mState.scenario = scenario;
    mState.notice = AirportWindNotice::None;
    unsafeDuration = 0.0;

    switch (scenario) {
        case AirportWindScenario::Calm:
            mState.windBand = AirportWindBand::Calm;
            mState.windDirection = 0;
            mState.aligned = false;
            alignmentProgress = 0.0;
            break;
        case AirportWindScenario::Steady:
        case AirportWindScenario::Realign:
            mState.windBand = AirportWindBand::Steady;
            chooseNextBearing();
            break;
        case AirportWindScenario::HighWind:
            mState.windBand = AirportWindBand::High;
            mState.aligned = false;
            alignmentProgress = 0.0;
            break;
    }
}

void AirportWindStationSimulation::applyRequest(AirportWindStationRequest const & request) {
    if (request.requestId == mState.activeRequestId) {
        requestReportState();
        return;
    }

    if (hasPendingRequest) {
        // The browser protocol permits one outstanding request. A retry of the
        // deferred request must not enqueue or apply it twice, while a newer
        // request remains unacknowledged so its normal retry can be handled
        // once the pending request has completed.
        requestReportState();
        return;
    }

    if (resetAsserted && request.action == AirportWindAction::Scenario) {
        pendingRequest = request;
        hasPendingRequest = true;
        requestReportState();
        return;
    }

    mState.activeRequestId = request.requestId;
    switch (request.action) {
        case AirportWindAction::Sync:
            break;
        case AirportWindAction::Scenario:
            applyScenario(request.scenario);
            break;
        case AirportWindAction::Reset:
            beginReset();
            break;
        case AirportWindAction::Invalid:
            return;
    }
    requestReportState();
}

void AirportWindStationSimulation::applyPendingRequest() {
    if (!hasPendingRequest) {
        return;
    }

    AirportWindStationRequest request = pendingRequest;
    hasPendingRequest = false;
    mState.activeRequestId = request.requestId;
    applyScenario(request.scenario);
    requestReportState();
}

void AirportWindStationSimulation::sampleControllerOutputs() {
    bool firstAlign = this->targetDevice->getGpio(0);
    bool firstGeneratorEnable = this->targetDevice->getGpio(2);
    bool firstActive = this->targetDevice->getGpio(3);
    bool firstRunwayWindAlert = this->targetDevice->getGpio(4);

    bool secondAlign = this->targetDevice->getGpio(0);
    bool secondGeneratorEnable = this->targetDevice->getGpio(2);
    bool secondActive = this->targetDevice->getGpio(3);
    bool secondRunwayWindAlert = this->targetDevice->getGpio(4);

    if (!haveControllerSample || firstAlign == secondAlign) sampledAlign = secondAlign;
    if (!haveControllerSample || firstGeneratorEnable == secondGeneratorEnable) sampledGeneratorEnable = secondGeneratorEnable;
    if (!haveControllerSample || firstActive == secondActive) sampledActive = secondActive;
    if (!haveControllerSample || firstRunwayWindAlert == secondRunwayWindAlert) sampledRunwayWindAlert = secondRunwayWindAlert;
    haveControllerSample = true;

    mState.align = sampledAlign;
    mState.generatorEnable = sampledGeneratorEnable;
    mState.active = sampledActive;
    mState.runwayWindAlert = sampledRunwayWindAlert;
}

void AirportWindStationSimulation::updateYaw(double delta) {
    bool mayYaw = mState.windBand == AirportWindBand::Steady
        && !mState.aligned && mState.align && mState.active;
    if (!mayYaw) {
        return;
    }

    alignmentProgress = std::min(ALIGNMENT_SECONDS, alignmentProgress + delta);
    double fraction = alignmentProgress / ALIGNMENT_SECONDS;
    double travel = shortestAngleDelta(yawStartAngle, static_cast<double>(mState.targetAngle));
    nacelleAngle = normalizeAngle(yawStartAngle + travel * fraction);

    if (alignmentProgress >= ALIGNMENT_SECONDS) {
        nacelleAngle = normalizeAngle(static_cast<double>(mState.targetAngle));
        mState.aligned = true;
    }
    mState.nacelleAngle = static_cast<std::int16_t>(std::lround(nacelleAngle)) % 360;
}

void AirportWindStationSimulation::updateRotor(double delta) {
    bool mayGenerate = mState.windBand == AirportWindBand::Steady
        && mState.aligned && mState.active && mState.generatorEnable;

    if (mayGenerate) {
        rotorLevel = std::min(1.0, rotorLevel + delta / ROTOR_RAMP_SECONDS);
        mState.rotorBand = rotorLevel >= 1.0
            ? AirportRotorBand::Running
            : AirportRotorBand::Accelerating;
        return;
    }

    if (rotorLevel > 0.0) {
        rotorLevel = std::max(0.0, rotorLevel - delta / ROTOR_RAMP_SECONDS);
        mState.rotorBand = rotorLevel <= 0.0
            ? AirportRotorBand::Stopped
            : AirportRotorBand::Decelerating;
    } else {
        mState.rotorBand = AirportRotorBand::Stopped;
    }
}

void AirportWindStationSimulation::updatePhase() {
    if (resetAsserted) {
        mState.phase = AirportWindPhase::Reset;
    } else if (mState.windBand == AirportWindBand::Calm) {
        mState.phase = AirportWindPhase::Calm;
    } else if (mState.windBand == AirportWindBand::High) {
        mState.phase = AirportWindPhase::HighWind;
    } else if (!mState.aligned && mState.scenario == AirportWindScenario::Realign) {
        mState.phase = AirportWindPhase::Realigning;
    } else if (!mState.aligned && mState.align && mState.active) {
        mState.phase = AirportWindPhase::Aligning;
    } else if (mState.rotorBand == AirportRotorBand::Accelerating
            || mState.rotorBand == AirportRotorBand::Running) {
        mState.phase = AirportWindPhase::Generating;
    } else {
        mState.phase = AirportWindPhase::Steady;
    }
}

void AirportWindStationSimulation::updateNotice(double delta) {
    if (resetAsserted) {
        unsafeDuration = 0.0;
        mState.notice = AirportWindNotice::None;
        return;
    }

    bool generationAllowed = mState.windBand == AirportWindBand::Steady
        && mState.aligned && mState.active;
    bool unsafeGenerator = mState.generatorEnable && !generationAllowed;
    bool unsafeInactive = !mState.active
        && (mState.phase == AirportWindPhase::Aligning
            || mState.phase == AirportWindPhase::Realigning
            || rotorLevel > 0.0);
    bool unsafeAlign = mState.align
        && (!mState.active || mState.windBand != AirportWindBand::Steady);
    bool missingRunwayAlert = mState.windBand == AirportWindBand::High
        && !mState.runwayWindAlert;
    bool unexpectedRunwayAlert = mState.windBand != AirportWindBand::High
        && mState.runwayWindAlert;

    if (!(unsafeGenerator || unsafeInactive || unsafeAlign
            || missingRunwayAlert || unexpectedRunwayAlert)) {
        unsafeDuration = 0.0;
        mState.notice = AirportWindNotice::None;
        return;
    }

    unsafeDuration += delta;
    if (unsafeDuration < UNSAFE_GRACE_SECONDS) {
        mState.notice = AirportWindNotice::None;
    } else if (unsafeGenerator) {
        mState.notice = AirportWindNotice::UnsafeGenerator;
    } else if (unsafeInactive) {
        mState.notice = AirportWindNotice::UnsafeInactive;
    } else if (unsafeAlign) {
        mState.notice = AirportWindNotice::UnsafeAlign;
    } else if (missingRunwayAlert) {
        mState.notice = AirportWindNotice::MissingRunwayAlert;
    } else {
        mState.notice = AirportWindNotice::UnexpectedRunwayAlert;
    }
}

void AirportWindStationSimulation::writePlantOutputs() {
    std::uint8_t inputCode = 0x01;
    if (mState.windBand == AirportWindBand::Steady) inputCode = 0x02;
    if (mState.windBand == AirportWindBand::High) inputCode = 0x03;

    this->targetDevice->setGpio(0, resetAsserted);
    this->targetDevice->setGpio(1, (inputCode & 0x01u) != 0);
    this->targetDevice->setGpio(2, (inputCode & 0x02u) != 0);
    this->targetDevice->setGpio(3, false);
    this->targetDevice->setGpio(4, mState.aligned);
}

bool AirportWindStationSimulation::stateChangedFrom(AirportWindStationData const & previous) const {
    return previous.scenario != mState.scenario
        || previous.phase != mState.phase
        || previous.windDirection != mState.windDirection
        || previous.windBand != mState.windBand
        || previous.aligned != mState.aligned
        || previous.nacelleAngle != mState.nacelleAngle
        || previous.targetAngle != mState.targetAngle
        || previous.rotorBand != mState.rotorBand
        || previous.align != mState.align
        || previous.generatorEnable != mState.generatorEnable
        || previous.active != mState.active
        || previous.runwayWindAlert != mState.runwayWindAlert
        || previous.activeRequestId != mState.activeRequestId
        || previous.notice != mState.notice;
}

void AirportWindStationSimulation::update(double delta) {
    AirportWindStationData previous = mState;
    bool resetWasAssertedAtUpdateStart = resetAsserted;

    AirportWindStationRequest request;
    if (readRequest(request)) {
        applyRequest(request);
    }

    sampleControllerOutputs();

    bool resetActiveForThisStep = resetAsserted;
    if (resetAsserted && resetWasAssertedAtUpdateStart) {
        if (delta + 1e-9 >= resetRemaining) {
            resetRemaining = 0.0;
            resetAsserted = false;
            applyPendingRequest();
        } else {
            resetRemaining -= delta;
        }
    }

    if (!resetActiveForThisStep) {
        updateYaw(delta);
        updateRotor(delta);
    }
    updatePhase();
    updateNotice(delta);
    writePlantOutputs();

    heartbeatElapsed += delta;
    if (stateChangedFrom(previous)) {
        requestReportState();
        heartbeatElapsed = 0.0;
    } else if (heartbeatElapsed >= HEARTBEAT_SECONDS) {
        requestReportState();
        heartbeatElapsed = 0.0;
    }
}

void AirportWindStationSimulation::reportUpdate() {
    if (mShouldReportInReportWhenMarkedMode) {
        ++mState.seq;
    }
    Simulation<AirportWindStationData, AirportWindStationRequest>::reportUpdate();
}

std::uint32_t AirportWindStationSimulation::getSleepStepInMs() {
    return 10;
}
