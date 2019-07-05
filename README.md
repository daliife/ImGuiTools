# ImGuiTools
## Description
Advanced Graphics 2 delivery project. The project consists in:
* the creation of a scenery with some elements with textures and terrain.
* the investigation and creation of tools/modules to be used as debug elements with the ImGUI library.

## Usage information
* You can toggle the Debug panels On and Off by pressing `ALT + 0`.
* The ImGui Windows are resizable, so its best to set the window in fullscreen and then adapt their sizes.

## Window modules
Before the ImGui panels were in the `DebugSytem.cpp` file. To organise and divide the logic I've decoupled the ImGUI functions and moved to a new `ToolsSystem.cpp` file. In them we will find the following modules:

* **Hierarchy module:** List of all the entities in the current scene. We will be able to modify its basic Transform in realtime.

* **Materials module:** List of all the material textures currently displayed in the scene.

* **Inspector module:** When selected any specific object, it will show information about its transform and material components, also live modifiable.

* **Statistics module:** Panel that will display a graph with the current framerate, with its min and max values.

* **Quick Actions module:** Panel with the most common used debug functions: show/hide skybox, change color of background, show/hide debug lines,...

* **Console module:** WIP

Apart from these we will find at the top of the window a **MenuBar**, with the current framerate and a drop-down to show or hide the different modules.

## Screenshots
![Debug Tools](https://i.imgur.com/C2j2jX6.png)

![Balls everywhere](https://i.imgur.com/t0jcNAb.png)
## Interesting references
* [imgui_demo.cpp](https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp)

## More information
Code extracted and modified from [Aluns' repository](https://github.com/AlunAlun/MVD_24_Particles).
