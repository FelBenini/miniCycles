# MiniCycles

A Pathtracer made with C and OpenGL Compute Shader, inspired by the Blender's cycles render mechanism. It uses the montecarlo path tracing algorithm for better simulating the lighting physics and ray bounces.

<img width="1710" height="613" alt="image" src="https://github.com/user-attachments/assets/88af3918-e074-44cf-9d8c-81bf2bd84fe1" />

## Installing, compiling and running.

```bash
git clone https://github.com/FelBenini/mini-cycles.git
cd mini-cycles
make
./cycles ./scenes/empty.rt
```

## Requirements

- GLFW installed
- GNU C Compiler
- And that's all folks

## Features

### Tonemapping

Currently the AGX tonemapping is done at the fragment shader, by simulating the look and feel of it. Actual LUT for color grading is on the plans.

<img width="1958" height="769" alt="AGX_comparison" src="https://github.com/user-attachments/assets/370f61a8-69ed-4afc-856b-36e0fe50fb0d" />
<img width="1332" height="1118" alt="New Project" src="https://github.com/user-attachments/assets/8f6a7fa4-1e13-4f0c-a0d0-ac4b873ad93d" />
