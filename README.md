# Golf Flight Simulator 3D

This simulator provides a realistic and interactive way to visualize the flight of a golf ball under various conditions.

![Preview Screenshot](screenshots/golf_flight_sim_3d_1.png.png)

## Building

Ensure you have Python, CMake, and a compiler which supports at least C++11 installed on your computer, then run the following commands:

```bash
git clone https://github.com/monde-lointain/golf-flight-sim-3d.git
cd golf-flight-sim-3d
mkdir build && cd build
cmake ..
cmake --build .
```

## Libraries

[imgui](https://github.com/ocornut/imgui) (GUI)<br>
[glfw](https://github.com/glfw/glfw) (Window/Input Handling)<br>
[glad](https://github.com/Dav1dde/glad) (OpenGL Extension Handling)<br>
[spdlog](https://github.com/gabime/spdlog) (Logging)<br>
[glm](https://github.com/g-truc/glm) (Matrix/Vector Math)<br>
[tracy](https://github.com/wolfpld/tracy) (Profiling)<br>
[stb_image](https://github.com/nothings/stb) (Image Loading)<br>
[cgltf](https://github.com/jkuhlmann/cgltf) (Model Loading)