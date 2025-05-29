# Decals

WASM implementation of a decal generator. Live demo here at: https://www.felipegutierrezduque.xyz/Decals/ 

#Technologies and features used:

 -Jump Flooding Algorithm to create an SDF of the decal texture’s mask, this enables a smooth transition when doing alpha blending between the albedo and the decal texture. Implemented with float textures and a clever optimization that allows for only one draw call to be called to compute the inside and outside distance transforms. Those distances are used on a second pass to compute the true Signed Distance Function. 
Ray tracing using a BVH (NanoRT) to get the normal of the hit position to set up the projection for the decal.

 -C++/WASM/OpenGL/Javascript through Emscripten.
 
 -Texture space decals. Based on https://github.com/diharaw/texture-space-decals and https://devfault.wordpress.com/2018/06/13/texture-space-decals/ with the difference that I am using a box SDF to clip the coordinates of the projected decal.
 
 -Tangent space normal mapping. Based on https://learnopengl.com/Advanced-Lighting/Normal-Mapping if no tangents are provided, the app can recompute it them from the texture coordinates and the normals.
 
 -CAD-like camera. Use WASD to move around.
 
 -Turntable camera. Uses quaternions, so you can turn completely on both axes.
 
 -Blinn-Phong shading. Based on the LearnOpenGL's implementation, with added tangent space normal mapping.
 
 -CMake script so that it can be compiled on:
 
  -Mac
  
  -Windows
  
  -Emscripten
  
 -Guizmo to control the model’s transformations using ImGuizmo.
 
 -UI with ImGui.
 
 -Three viewports:
 
  -The viewport with the turntable or FPS style camera
  
  -Side view
  
  -Frontal view
  
 -Model uploading:
 
  -GLTF
  
  -GLB. Support for Draco compressed models.
  
  -Changeable models, this means that from compilation, models can be preloaded and changed on the fly. through the UI.
  
  -Visual debugging of the algorithms through different viewports or composited on the main viewport:
  
  -Decal projection matrix
  
  -Bounding box
  
  -Ray tracing hit point on the model from the mouse coordinates
  
  -Signed Distance Function of the alpha mask
  
  -Texture Space resulting albedo with the projected decal.
  
  -Download/Upload of textures and models for the WASM version.

#To compile: 
 -Windows and Mac:
 
  cmake ..
  
  cmake --build .
  
 -Emscripten:
 
  emcmake cmake ..
  
  On Windows:
  
   cmake -G Ninja --build .
   
  On Mac:
  
   cmake --build .
   
  Note you have to have installed the dependencies needed which are added as git submodules and to have SDL available for CMake.
 
#Screenshots:
<img width="796" alt="Screen Shot 2025-05-29 at 4 21 34 PM" src="https://github.com/user-attachments/assets/c3e99520-8413-4c5d-a2f0-a22764a4742e" />
In this image you can see the visual debugging tools that enable the user to be able to see the hit point, decal projector and the bounding box. 

<img width="800" alt="Screen Shot 2025-05-29 at 5 37 07 PM" src="https://github.com/user-attachments/assets/276c13fb-dec6-453a-ad73-fbe0a8f34b67" />
Here you can see the UI and the result.

<img width="793" alt="Screen Shot 2025-05-29 at 5 37 50 PM" src="https://github.com/user-attachments/assets/b57bcf8b-9309-41d5-8e4b-c96066b0901a" />
This shows the texture space render with the projected decal, we are rendering the texture coordinate as the positions we pass to the fragment shader from the vertex shader.

<img width="792" alt="Screen Shot 2025-05-29 at 5 38 46 PM" src="https://github.com/user-attachments/assets/be38bb06-091e-4266-b64d-00ee1d215a04" />
This image shows the result of the JFA, this enables for smooth transitions of alpha maps.

<img width="793" alt="Screen Shot 2025-05-29 at 5 39 21 PM" src="https://github.com/user-attachments/assets/104843bb-4599-453b-8614-8ac4b229af64" />
This is another model that can be changed from the UI, it also has a different texture.

<img width="792" alt="Screen Shot 2025-05-29 at 5 41 01 PM" src="https://github.com/user-attachments/assets/362f39b8-b217-4069-9135-a1799250137c" />
Here is a capture of the split screen posibility in which you can see a top and side view as well.


