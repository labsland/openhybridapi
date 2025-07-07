/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 */
#include <iostream>
#include <thread>
#include <chrono>
#include "labsland/simulations/watertanksimulation.h"
#include "rhlab/butterfly.h"
#include "deusto/door.h"
#include "deusto/watertankDeusto.h"
#include "deusto/watertankDeustoTemperature.h"
#include "deusto/wiperDeusto.h"
#include "deusto/wiper2BitDeusto.h"
#include "labsland/simulations/utils/communicatorfiles.h"
#include "labsland/simulations/targetdevicefiles.h"
#include "labsland/utils/timemanagerstd.h"

using namespace std;
using namespace LabsLand::Simulations::Utils;

class SimulationRunner {
    public:
        virtual void run() = 0;
};

template <class SimulationClass, class OutputDataType, class InputDataType>
class ConcreteSimulationRunner : public SimulationRunner {
    private:
        string configuration; // "files" or anything else in the future (e.g., maybe provide another class or whatever)
        string mode; // "run" or "run-fast"
    public:
        ConcreteSimulationRunner(const string & config, const string & mode): configuration(config), mode(mode) {}

        void run() {
            auto timeManager = new LabsLand::Utils::TimeManagerStd();
            LabsLand::Utils::TargetDevice * targetDevice = 0;
            SimulationCommunicator<OutputDataType, InputDataType> * communicator;
            if (configuration == "files") {
                targetDevice = new LabsLand::Utils::TargetDeviceFiles(20, 10);
                communicator = new SimulationCommunicatorFiles<OutputDataType, InputDataType>("output-messages.txt", "input-messages.txt");
            } else {
                // Add here other implementations
                cerr << "Unsupported configuration: " << configuration << endl;
                return;
            }
            SimulationClass simulation;
            simulation.injectTimeManager(timeManager);
            simulation.injectCommunicator(communicator);
            simulation.injectTargetDevice(targetDevice);

            simulation._initialize();

            if (mode == "run-fast") {
                std::clock_t currentClock = clock();
                int i = 0;
                while(i < 100) {
                    currentClock += 0.1 * CLOCKS_PER_SEC; // Make the simulation advance 100 ms.
                    simulation._update(currentClock);
                    i++;

                    cout << "Current state: " << simulation.mState.serialize() << endl;
                    cout << endl;
                }
            } else if (mode == "run") {
                time_t end_time;
                while (true) {
                    auto end = std::chrono::system_clock::now();
                    end_time = std::chrono::system_clock::to_time_t(end);
                    string timeString(ctime(&end_time));

                    cout << "[" << timeString.substr(0, timeString.size() - 1) << "] Running _update()" << endl;
                    simulation._update();
                    this_thread::sleep_for (chrono::milliseconds(100));
                }
            } else {
                cerr << "Unsupported mode: " << mode << endl;
            }
        }
};


int main(int argc, char * argv[]) {
    if (argc == 1) {
        cerr << "No simulation requested. Run " << argv[0] << " <simulation>" << endl;
        return 1;
    }

    string simulation(argv[1]);
    string configuration;
    if (argc >= 3) {
        configuration = argv[2];
    } else {
        configuration = "files";
    }
    string mode;
    if (argc >= 4) {
        mode = argv[3];
    } else {
        mode = "run";
    }

    SimulationRunner * runner = 0;

    if (simulation == "watertank") {
        runner = new ConcreteSimulationRunner<WatertankSimulation, WatertankData, WatertankRequest>(configuration, mode);
    } else if (simulation == "butterfly") {
        runner = new ConcreteSimulationRunner<ButterflySimulation, ButterflyData, ButterflyRequest>(configuration, mode);
    } else if (simulation == "door") {
        runner = new ConcreteSimulationRunner<DoorSimulation, DoorData, DoorRequest>(configuration, mode);
    } else if (simulation == "watertankDeusto") {
        runner = new ConcreteSimulationRunner<WatertankDeustoSimulation, WatertankDeustoData, WatertankDeustoRequest>(configuration, mode);
    } else if (simulation == "watertankDeustoTemperature") {
        runner = new ConcreteSimulationRunner<WatertankDeustoTemperatureSimulation, WatertankDeustoTemperatureData, WatertankDeustoTemperatureRequest>(configuration, mode);
    }else if (simulation == "wiperDeusto") {
        runner = new ConcreteSimulationRunner<WiperDeustoSimulation, WiperDeustoData, WiperDeustoRequest>(configuration, mode);
    }else if (simulation == "wiper2BitDeusto") {
        runner = new ConcreteSimulationRunner<Wiper2BitDeustoSimulation, Wiper2BitDeustoData, Wiper2BitDeustoRequest>(configuration, mode);
    }else {
        cerr << "Invalid simulation: '" << simulation << "'. Use a valid name" << endl;
        return 2;
    }

    runner->run();

    return 0;
}
