# MiniCycles

A physically-based path tracer written in C and OpenGL Compute Shaders, inspired by Blender's Cycles rendering engine. Implements Monte Carlo path tracing for realistic light transport simulation including global illumination, soft shadows, and physically-based materials.

![MiniCycles Render](https://github.com/user-attachments/assets/3a897d05-1624-480a-9861-37d8aca07537)

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
  - [Linux](#linux)
  - [macOS](#macos)
  - [Windows](#windows)
- [Building and Installation](#building-and-installation)
- [Usage](#usage)
- [Scene File Format](#scene-file-format)
  - [Camera](#camera)
  - [Ambient Light](#ambient-light)
  - [Lights](#lights)
  - [Objects](#objects)
  - [Materials and Textures](#materials-and-textures)
  - [Tonemapping](#tonemapping)
- [Controls](#controls)
- [Examples](#examples)
- [Project Structure](#project-structure)
- [Future Plans](#future-plans)
- [Acknowledgments](#acknowledgments)

## Features

### Rendering
- **Wavefront Path Tracing** - Splits the rendering pipeline into separate compute shader stages (ray gen → intersect → shade → shadow → accumulate) for better GPU SIMT utilization compared to monolithic megakernels
- **Monte Carlo Path Tracing** - Physically accurate light transport simulation
- **Multiple Bounce Lighting** - Configurable ray bounce depth
- **Next Event Estimation** - Direct light sampling for faster convergence
- **Russian Roulette Path Termination** - Optimal path length management

### Geometry
- **Procedural Primitives**: Sphere, Plane, Cube, Cone, Cylinder, Torus
- **Triangle Mesh Support** - Load OBJ files with automatic BVH generation
- **Smooth Shading** - Per-vertex normal interpolation

### Acceleration Structures
- **BVH (Bounding Volume Hierarchy)** - Per-mesh acceleration structure
- **TLAS (Top-Level Acceleration Structure)** - Scene-wide BVH for efficient ray traversal
- **Fast Ray-Triangle Intersection** - Optimized compute shader implementation

### Materials
- **Physically-Based Shading** - Metallic-roughness workflow
- **Dielectric Materials** - Refraction with configurable IOR
- **Emissive Materials** - Area lights via emissive meshes
- **Texture Support**:
  - Albedo maps
  - Normal maps
  - Roughness maps
  - Displacement maps

### Lighting
- **Sun/Directional Lights** - Parallel light source with configurable angle
- **Point Lights** - Omnidirectional point sources
- **Spot Lights** - Cone-shaped lights with inner/outer angles
- **Sky Lighting** - HDR environment maps
- **Emissive Meshes** - Mesh-based area lights

### Depth of Field
- **Thin Lens Model** - Physically-based depth of field via aperture sampling
- **Configurable Aperture** - `lens_radius` controls blur strength (adjustable at runtime with `Q`/`E`)
- **Configurable Focal Distance** - Sets the focal plane distance (adjustable at runtime with `Z`/`C`)

### Post-Processing
- **Tonemapping** - HDR to LDR conversion
- **LUT Support** - `.cube` format lookup tables for color grading
- **Progressive Rendering** - Accumulates samples over time

## Requirements

### All Platforms

MiniCycles requires the following dependencies regardless of platform:

| Dependency | Version | Notes |
|------------|---------|-------|
| C compiler | C99-compatible | GCC, Clang, or MSVC |
| OpenGL | 4.3+ | Compute Shader support required |
| GLFW | 3.x | Window and input management |
| make | any | Build system |

**GPU compatibility:** OpenGL 4.3 Compute Shaders require a relatively modern GPU. Generally this means NVIDIA Kepler (GTX 600 series) or newer, AMD GCN (HD 7000 series) or newer, or Intel Haswell (4th gen Core) or newer with up-to-date drivers.

---

### Linux

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential libglfw3-dev mesa-common-dev libgl-dev

# Fedora
sudo dnf install gcc make glfw-devel mesa-libGL-devel

# Arch Linux
sudo pacman -S base-devel glfw mesa
```

---

### macOS

> **⚠️ OpenGL deprecation notice:** Apple deprecated OpenGL in macOS 10.14 (Mojave) and the OpenGL stack is frozen at version 4.1 on most hardware. OpenGL 4.3 (required for Compute Shaders) is **not supported on Apple Silicon (M1/M2/M3)** and may not work on many Intel Macs either. MiniCycles does not currently run on macOS.

If you are on an Intel Mac and want to attempt a build:

```bash
# Install Xcode Command Line Tools (provides make, clang)
xcode-select --install

# Using Homebrew
brew install glfw glew

# Using MacPorts
sudo port install glfw glew
```

---

### Windows

#### Option 1: MSYS2 / MinGW-w64 (recommended)

```bash
# Install MSYS2 from https://www.msys2.org/, then in the MSYS2 MinGW64 terminal:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-glfw mingw-w64-x86_64-glew
```

#### Option 2: Visual Studio

1. Install [Visual Studio](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload
2. Download GLFW from [glfw.org](https://www.glfw.org/download.html)
3. Configure include paths and library paths in your project settings

#### Option 3: WSL (Windows Subsystem for Linux)

```bash
# In a WSL2 terminal (Ubuntu)
sudo apt update
sudo apt install build-essential libglfw3-dev mesa-common-dev libgl-dev
```

> **⚠️ WSL GPU passthrough:** Running a GPU-accelerated OpenGL app under WSL requires **WSL2 with WSLg**, which is only available on **Windows 11** (or Windows 10 build 21362+) with an up-to-date GPU driver that supports WSLg. Without this, OpenGL context creation will fail at runtime. See [Microsoft's WSLg documentation](https://github.com/microsoft/wslg) for setup instructions.

---

## Building and Installation

```bash
git clone https://github.com/FelBenini/mini-cycles.git
cd mini-cycles
make
```

The compiled binary will be named `cycles`.

### Build Options

The Makefile uses the following flags:
- `-Wall -Wextra -Werror` - Strict compilation warnings
- `-O3` - Optimized release build

To rebuild from scratch:
```bash
make re
```

To clean build artifacts:
```bash
make clean
```

## Usage

```bash
./cycles <scene_file.rt>
```

### Example
```bash
./cycles ./scenes/room.rt
```

## Scene File Format

Scene files use the `.rt` extension and contain ASCII text commands. Comments start with `//`.

### Camera

```
C x,y,z dx,dy,dz fov [lens_radius] [focal_distance]
```

| Parameter | Description |
|-----------|-------------|
| `x,y,z` | Camera position |
| `dx,dy,dz` | Forward direction vector |
| `fov` | Field of view in degrees (0-180) |
| `lens_radius` | Depth of field aperture size (optional, default 0.0) |
| `focal_distance` | Focal plane distance for DOF (optional, default 0.0) |

Examples:
```
C 0.0,0.0,5.0 0.0,0.0,-1.0 75
C -30.0,2.0,0.0 0.5,0.0,0.0 70 0.05 8.0
```

The two optional parameters enable depth of field via a thin lens model. When both are > 0, rays are jittered on a disk of radius `lens_radius` (larger = stronger blur) and redirected toward the focal plane at `focal_distance`. Parameters can also be tweaked at runtime with `Q`/`E` (aperture) and `Z`/`C` (focal distance).

### Ambient Light

```
A r,g,b intensity sky_texture_path
```

| Parameter | Description |
|-----------|-------------|
| `r,g,b` | Ambient color (0-255) |
| `intensity` | Light intensity |
| `sky_texture_path` | Optional HDR/skybox texture |

Example:
```
A 135,206,235 0.1
```
or

```
A 0.1 assets/skyboxes/daysky.jpg
```

### Lights

```
L intensity r,g,b type dx,dy,dz x,y,z cos_inner cos_outer
```

| Parameter | Description |
|-----------|-------------|
| `intensity` | Light brightness |
| `r,g,b` | Light color (0-255) |
| `type` | `SUN`, `POINT`, or `SPOT` |
| `dx,dy,dz` | Direction (for SUN/SPOT) |
| `x,y,z` | Position (for POINT/SPOT) |
| `cos_inner` | Inner cone cosine (SPOT only) |
| `cos_outer` | Outer cone cosine (SPOT only) |

Examples:
```
L 1000 255,255,255 SUN 0.0,-1.0,0.0 0.0,0.0,0.0 0.0 0.0
L 500 255,255,200 POINT 0.0,0.0,0.0 2.0,5.0,0.0 0.0 0.0
L 800 255,255,255 SPOT 0.0,-1.0,0.0 0.0,10.0,0.0 0.95 0.85
```

### Objects

#### Sphere
```
sp x,y,z radius r,g,b material_type roughness metallic
```

#### Plane
```
pl x,y,z nx,ny,nz size_x,size_z r,g,b material_type roughness metallic
```

#### Cube
```
cb x,y,z size r,g,b material_type roughness metallic
```

#### Cone
```
co x,y,z radius,height stacks,slices r,g,b material_type roughness metallic
```

#### Cylinder
```
cy x,y,z radius,height stacks,slices r,g,b material_type roughness metallic
```

#### Torus
```
to x,y,z major_radius,minor_radius stacks,slices r,g,b material_type roughness metallic
```

#### OBJ Mesh
```
obj filepath radius smooth r,g,b material_type roughness metallic
```

| Parameter | Description |
|-----------|-------------|
| `x,y,z` | Position |
| `r,g,b` | Color (0-255) |
| `material_type` | `0`=diffuse, `1`=metallic, `2`=dielectric |
| `roughness` | Surface roughness (0.0-1.0) |
| `metallic` | Metallic factor (0.0-1.0) |


### Materials and Textures

Materials are defined per-object with these properties:

- **Albedo**: Base color texture or solid color
- **Roughness**: Surface micro-facet roughness
- **Metallic**: Metal vs dielectric material
- **IOR**: Index of refraction for transparent materials

Textures are specified in the scene file using the object's texture fields (loaded via OBJ materials or external files).

### Tonemapping

Apply color grading using `.cube` LUT files:

```
lut path/to/file.cube
```

Example:
```
lut assets/lut/vivid_lut.cube
```

Available LUTs in `assets/lut/`:
- `vivid_lut.cube` - Enhanced colors
- `punchy.cube` - High contrast
- `good_colors.cube` / `good_colors2.cube` - Balanced color grading
- `bw_red.cube` - Black and white with red tint
- `bright.cube` - Increased brightness

## Controls

| Key/Mouse | Action |
|-----------|--------|
| `W` / `S` | Move forward / backward |
| `A` / `D` | Move left / right |
| `Shift` | Move down |
| `Spacebar` | Move up |
| `Mouse` + hold `Scroll` | Look around |
| `Arrow Keys` | Look around |
| `Mouse Scroll` | Change FOV |
| `Q` / `E` | Modify Depth of Field aperture size (lens blur) |
| `Z` / `C` | Modify the focal distance |
| `ESC` | Exit |

## Examples

### Room Scene
```
./cycles ./scenes/room.rt
```
<img width="1900" height="1012" alt="29" src="https://github.com/user-attachments/assets/033781e1-21a4-43af-bb4a-64a1ba507350" />


### Suzanne Scene
```
./cycles ./scenes/suzanne.rt
```
<img width="1900" height="1012" alt="27" src="https://github.com/user-attachments/assets/08038382-dbd3-4a6d-a8a5-a57bda50c443" />


### Dragon Scene
```
./cycles ./scenes/dragon.rt
```
<img width="1258" height="614" alt="25" src="https://github.com/user-attachments/assets/7d7ed006-9331-41c7-8ae9-cb2e0fd45d39" />

### Lucy Scene (with LUT tonemapping)
```
./cycles ./scenes/lucy.rt
```
![Lucy Scene](https://github.com/user-attachments/assets/f9eeb5fd-18f1-4dc2-bed1-7d2933b41bd3)

### Room 2 Scene
```
./cycles ./scenes/room2.rt
```
<img width="1900" height="1012" alt="31" src="https://github.com/user-attachments/assets/1d077ee4-7e1d-4f24-9aad-85a8c50fbb74" />


### Complex Scenes

More complex scenes including the Bistro scene are available at:
**[miniCycles Scenes Repository](https://github.com/FelBenini/miniCycles-scenes)**

## Project Structure

```
mini-cycles/
├── srcs/
│   ├── main.c                 # Entry point
│   ├── frame.c                # Wavefront tile dispatch: gen → intersect → shade → shadow → accumulate
│   ├── init/                  # Initialization code
│   ├── parser/                # .rt scene file parser
│   ├── mesh/                  # Procedural mesh generators
│   ├── bvh/                   # Acceleration structures
│   ├── scene/                 # Scene management
│   ├── shader/                # OpenGL shader management
│   ├── input/                 # Input handling
│   └── math/                  # Math utilities
├── include/                   # Header files
├── shaders/
│   ├── gen_ray.comp.glsl      # Primary ray generation (per-tile)
│   ├── intersect.comp.glsl    # Ray-scene intersection (bounce loop)
│   ├── shade.comp.glsl        # Material shading & BSDF sampling
│   ├── shadow.comp.glsl       # Shadow/visibility rays
│   ├── accumulate.comp.glsl   # Sample accumulation & tonemap
│   ├── fullscreen.vert.glsl   # Fullscreen triangle vertex shader
│   └── fullscreen.frag.glsl   # Tonemapping & LUT fragment shader
├── assets/
│   ├── objs/                  # 3D models
│   ├── skyboxes/              # HDR environment maps
│   └── lut/                   # Color grading LUTs
├── scenes/                    # Example .rt scene files
└── Makefile
```

## Future Plans

- **Denoising** - Atrous Wavelet Denoiser
- **More Primitives** - Additional procedural shapes

## Acknowledgments

- Inspired by [Blender Cycles](https://www.cycles-renderer.org/)
- Montecarlo implementation learned from [Physically Based Rendering PBRT](https://pbr-book.org/4ed/contents)
- Initial structure based on the MiniRT project from 42 School
- BVH implementation based on standard ray tracing acceleration literature
