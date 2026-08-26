# LabsLand Hybrid API

This API, licensed under GNU AGPL, allows users to develop virtual models for LabsLand remote laboratories.

## Server

The Server Flask application can be used to run a web interface for simulations development and testing.
The server is located in `server` directory and more information can be found in its README at [server/README.md](server/README.md).
Its capabilities are also explained in that README.

## Simulations

Although several components are involved in the Hybrid system, a key component are the simulations.
The simulations are the programs that run in the Pico device and that interact with the real hardware and provide
the simulation. The simulations are developed in C++ and use the hybrid API.

Note that the 3D environments are not part of what is referred to as "simulation" in this document. They are part of the
system and are loaded by the Server's web interface, but separate from the Simulation, and are instead referred to as
"Visualization" or "3D environment".

### Airport Wind Station

The Airport Wind Station is a deterministic wind-turbine control activity. A
controller must align the nacelle before enabling generation, realign it after
a wind-direction change, and stop generation safely during high wind. The same
board-neutral simulation contract supports DE1-SoC and STM32WB55RG activities.

The public implementation is in
[`src/rhlab/airportWindStation.h`](src/rhlab/airportWindStation.h) and
[`src/rhlab/airportWindStation.cpp`](src/rhlab/airportWindStation.cpp). Its
development-server configuration is
[`server/simulations/airportWindStation.yml`](server/simulations/airportWindStation.yml),
and its behavior is covered by
[`tests/airport_wind_station_tests.cpp`](tests/airport_wind_station_tests.cpp).
The companion [browser visualization](https://static-apps.labsland.com/simulations/airportWindStation/index.html)
renders complete state reports produced by the simulation.

The controller interface uses five logical slots in each direction:

| Slot | Simulation to controller | Controller to simulation |
| ---: | --- | --- |
| 0 | `reset` | `align` |
| 1 | `inputCode[0]` | reserved, driven low |
| 2 | `inputCode[1]` | `generatorEnable` |
| 3 | reserved, ignored | `active` |
| 4 | `aligned` | `runwayWindAlert` |

Browser commands select `calm`, `steady`, `realign`, or `highWind`, or request
a reset. Versioned reports describe the complete observable plant and
controller state, allowing the visualization to recover cleanly after a reload
or reconnect.

#### Credits and provenance

- ZZ ([`zzyzzy42`](https://github.com/zzyzzy42)) created the original 3D
  airport and wind-turbine assets and the visualization foundation.
- Luis Rodríguez Gil developed the HybridAPI simulation, browser protocol,
  firmware and infrastructure integration, and multi-platform implementation.
- The activity was developed with the [Remote Hub Lab
  (RHLAB)](https://rhlab.ece.uw.edu/) at the University of Washington and
  integrated with LabsLand.
- Professor Rania Hussein is RHLAB's principal investigator and lab leader and
  serves as principal investigator for [REDTAIL (Remote Experimentation and
  Digital Twinning for Accessible and Innovative
  Learning)](https://redtail.rhlab.ece.uw.edu/).

Development of the Airport Wind Station simulation and visualization was
supported in part by the National Science Foundation through REDTAIL under
[Award No. 2336745](https://www.nsf.gov/awardsearch/showAward?AWD_ID=2336745).
Any opinions, findings, conclusions, or recommendations expressed here are
those of the authors and do not necessarily reflect the views of the National
Science Foundation.

## Terminology

- *DUT*: Device Under Test. The device that the student controls. 

- *Simulation*: The program meant to run on the Pico device that interacts with the real hardware (DUT) directly. Typically developed in C++. For example,
in the Watertank model, this is the component that reads the actual inputs from the DUT, such as the GPIOs to open water pumps. It also simulates (and keeps track)
internally of the water level of the (simulated) watertank; and it is typically authoritative (while the visualization, discussed next, typically won't be).
It also periodically sends messages to the *Web* and the *Visualization* to update their state. In the case of the Watertank, to inform them of the current
water level so that it can be visualized, for example.

- *Web*: The website (either in the dev server or in the actual lab) that hosts the UI with which the user interacts. This contains the various
buttons and widgets to interact with the lab, plus, sometimes, also contains the 2D or 3D visualization. Often this visualization is not contained
directly, but is instead in an iframe.

- *Visualization* or *3D model*: The 2D or 3D application that handles the visuals and that runs within the Web, either directly or in an iframe.
In the Parking model, for example, the *visualization* is the 3D app that shows the parking. This component communicates with the *simulation* 
through the *Web* only, not directly. Since both the *visualization* and the *web* run remotely, communication is typically relatively slow
(compared to the speed at which the *simulation* can interact with the real hardware). Certain aspects of a given model can actually be simulated in the visualization side
too; and communicated through a message system to the *Simulation*. E.g., in the Parking model, the *Visualization* decides whether a car is over a presence sensor,
and communicates the presence sensor activation to the *Simulation*. Typically the Simulation is more authoritative, though.

## Compiling and running the simulations

For development, testing and debugging purposes the simulations are normally designed to be able to be run not only
on the Pico, which is its final target device, but also on a regular development computer. To compile and run them
you may follow these steps:

Go to cmake-build-debug
```
cd cmake-build-debug
```

Compile the simulations:
```
./compile.sh
```

Run one of them (will block)
```
./hybridapi watertank
```

Alternatively, run it fast simulating pauses:
```
./hybridapi watertank files run-fast
```

## Implementation details

### Visualization

The visualization will typically run in an iframe. It will typically need to communicate with the Simulation, but not directly. Instead, it will communicate through the Web.

To send a message to the Web, that will eventually be sent to the simulation:

```
parent.postMessage({
    messageType: "web2sim",
    version: "1.0",
    value: e
}, "*"),
```

It should also be ready to receive and handle sim2web messages, for which it will need to listen for similar messages from within the iframe.
