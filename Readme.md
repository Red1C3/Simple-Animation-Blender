# Simple-Animation-Blender
A real-time 1D animation blender and player using Vulkan as graphical backend and ImGui for GUI.
## Snapshots
[](Snapshots/Original%20Animation.png)

[](Snapshots/Blending%20by%20half%20as%20a%20factor.png)

[](Snapshots/Blending%20to%201%20results%20to%20diseffect%20the%20first%20Animation.png)
## Usage
```
./simple-animation-blender "Supported-Mesh-Path"
```
without quotation marks, if the app crashes on launch, check the log for info.

An Included Mesh for testing is provided, [here](./Assets/Low%20Poly%20Character.glb)

*Note: You're machine must have a Vulkan supported GPU, and Vulkan runtime in case it was not included with your GPU driver (It probably is), otherwise the app will crash on launch.*

*Note for artists importing their models from 3D software: I highly recommend you to set your mesh in the origin and apply all transformations before exporting.*
### Supported Mesh Formats
This app uses [Assimp library](https://github.com/assimp/assimp) for importing mesh data, if your file is supported on Assimp, it should work fine, check [supported formats](https://github.com/assimp/assimp/blob/master/doc/Fileformats.md).

*Note: Only GLB/GLTF was tested*
## Building dependencies
This app supports Meson Build System, and it's recommended to use it for building, "meson.build" file describes the needed dependencies, however, here they are:
- [Vulkan](https://vulkan.org/)
- [Assimp](https://github.com/assimp/assimp)
- [spdlog](https://github.com/gabime/spdlog)
- [GLFW](https://glfw.org/)
- [GLM](https://github.com/g-truc/glm)
### Building
Using Meson:
```
meson builddir
cd builddir
ninja
```
## Docs
Source is Commented
## Contributions
This project is open for contributions, especially animating-related contributions :)
## License
Developed under MIT License, by Mohammad Issawi, check [License file](./LICENSE).

[Low Poly Character](https://sketchfab.com/3d-models/rigged-low-poly-character-29797abe9c3b46a9bffd7e91183722a1) is under CC Attribution License.

[ImGui](https://github.com/ocornut/imgui/blob/master/LICENSE.txt) was developed by Omar Cornut, all rights reserved.

## Made with Slowly Walking Kittens