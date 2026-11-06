# LED Matrix visualizations

The `matrix` runner and its GPIO contract are shared by the 2D and 3D views.
No extra firmware, runner ID or controller program is required.
Each report contains 16 colon-separated rows, each with 16 `B`, `G`, `R` or `Y`
characters, in row-major order. `B` means off. Row 1, column 1 is upper left
in the fixed front/top view, matching the existing flat display.

The development server's matrix configuration provides both visualizations.
The companion source is https://github.com/remotehublab/rhl-3d-led-matrix-simulation .
Its live iframe accepts complete `sim2web` reports from its parent only,
buffers the newest report until the model loads, and signals `web2loader`.
The explicit `?demo=1` mode uses local sample frames and never pretends to
show a device. Production entry: https://static-apps.labsland.com/simulations/ledMatrix3D/index.html .

CodeIDE can request `simulation:ledMatrix?view=3d`. The lab-server presentation
adapter selects the 3D URL while its canonical simulation remains `ledMatrix`.
Keep the current `latch`, `pulse`, `green`, `red`, 256-pulse transfer unchanged.
Switching views does not add row scanning, persistence-of-vision physics or
new electrical behavior. Timing acceptance belongs to the existing driver and
real target, independently of rendering.
