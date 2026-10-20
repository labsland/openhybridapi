# Airport Terminal Garage v0.2

Local prototype for the proposed UW EE/CSE 371 Lab 6 adaptation. Course approval and real-rack qualification remain separate from host verification. The C++ simulation is open-source under this repository's AGPL license. It is implemented here first, independently of the proprietary legacy `parking` simulation.

The browser owns cars, the 25 bays, route motion, queueing and physical world occupancy. This firmware bridges five persistent sensor levels to the student's controller, samples six return lines, latches completed passages and retains one bounded world checkpoint. It never drives car positions or implements assessed occupancy/hour/ASMD/RAM work. It never resets the student's controller.

## GPIO contract

Positions are zero-based simulation slots, not Pico pin numbers.

| Direction | Slot | Signal | Proposed DE1 V_GPIO |
|---|---:|---|---:|
| Simulation → controller | 0 | entryWaiting | 28 |
| Simulation → controller | 1 | exitWaiting | 29 |
| Simulation → controller | 2 | entryComplete | 30 |
| Simulation → controller | 3 | exitComplete | 23 |
| Simulation → controller | 4 | worldReady | 24 |
| Controller → simulation | 0 | openEntrance | 26 |
| Controller → simulation | 1 | openExit | 27 |
| Controller → simulation | 2 | entrancePermitLamp | 32 |
| Controller → simulation | 3 | exitPermitLamp | 34 |
| Controller → simulation | 4 | fullLamp | 31 |
| Controller → simulation | 5 | completionAck | 33 |

This is incompatible with the old three-bay parking bitstream. A supplied synchronized wrapper owns completionAck and emits exactly one arrived/departed clock pulse. Students retain gate/lamp decisions, bounded occupancy, hours, rush-window ASMD/control/datapath, RAM and native displays. The wrapper source and testbench are in `examples/airportTerminalGarage/`, mirrored by the prototype client under `controller/`. They are not a completed student design.

## Reliable passage delivery

Only one vehicle moves through a transfer at a time. After its rear clears the barrier, the browser increments its physical count and submits an increasing `eventId` and checkpoint. Firmware holds entryComplete or exitComplete high until completionAck is observed high, clears the completion input, then waits for completionAck low before reporting `eventStatus=cleared`. The next passage is blocked until that state. Repeated requests or repeated event IDs never create another pulse. A command or gate opening alone cannot count a car.

Reports and requests are URL query strings. All reports have `v=3&activity=airportTerminalGarage`, increasing seq, boot/restart sessionId, requestSeq, requestFloor, eventId, eventStatus, requestStatus, notice, all five sensors, all six sampled return lines, sampleValid and the checkpoint. Missing, duplicate, overflowing, nonbinary, out-of-range and unknown request fields are rejected.

Commands: `sync` (v/requestSeq/action only), `state`, `passage`, `restart`. Other commands include sessionId. `state` adds entryWaiting/exitWaiting/worldReady and every checkpoint field. `passage` adds direction=entry|exit, eventId and every checkpoint field. `restart` explicitly creates a new empty world session; reset the board separately while establishing that baseline. Retries reuse the exact requestSeq and payload, and receive that request's own stored result, even if an older out-of-order copy was rejected in between. A reconnect sync exposes requestFloor so a reloaded browser selects IDs above the firmware's prior writer.

Checkpoint fields: parkedMask (25-bit), physicalCount (0–25), carId (0–64), bayId (0–25), waypoint (0–2048), progress (0–10000), movementState (0 idle, 1 approaching, 2 waiting entry, 3 parking, 4 approaching exit, 5 waiting exit, 6 leaving, 7 flight), passageId. Idle parked scenes can be reconstructed. Reload during movement, an outstanding transfer, or ambiguous controller reset requires explicit recovery; the prototype does not invent missed events. Firmware checkpoints persist for the running simulation, not across a Pico power loss.

A two-second browser timeout deasserts worldReady and reports stale. It preserves outstanding completion levels; receipt of a fresh complete state resumes the handshake. Reports are sent on change and at least every 500ms. The polling loop sleeps 10ms and never blocks on GPIO.

## Local verification

```
cmake -S . -B build-v02 -DBUILD_TESTING=ON
cmake --build build-v02 -j6
ctest --test-dir build-v02 --output-on-failure
```

Desktop runner: `build-v02/hybridapi airportTerminalGarage files run`. The existing Flask development server can select `server/simulations/airportTerminalGarage.yml`; source `devrc-airport-terminal-garage`, then run its normal workflow. The browser supports independent demo/manual controller mode and the normal LabsLand iframe web2sim/sim2web bridge. No production rollout is part of v0.2 preparation.
