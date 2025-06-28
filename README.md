# Advanced Vulkan Demos (AVD)

> **AI-Generated Placeholder Notice**  
> This README is an AI-generated placeholder and will be replaced with a proper, human-authored README once the project becomes presentable and ready for public showcase. Right now it is in a highly WIP state and serves as a basic structure for the project documentation.

This is a comprehensive portfolio project that demonstrates advanced Vulkan rendering techniques through interactive real-time demonstrations. The collection showcases modern graphics programming concepts including subsurface scattering, bloom effects, radiance cascades, and other cutting-edge rendering technologies.

![Build Status](https://img.shields.io/badge/build-in%20development-yellow)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Vulkan](https://img.shields.io/badge/Vulkan-1.2%2B-red)

## Project Overview

Advanced Vulkan Demos is a curated collection of real-time graphics demonstrations built from scratch using the modern Vulkan API. Each demonstration focuses on a specific advanced rendering technique, serving both as a learning resource and a showcase of technical capabilities.

### Key Features

- **Modern Vulkan Architecture**: Built with Vulkan 1.4+ using industry best practices
- **Cross-Platform Support**: Full compatibility with Windows and Linux operating systems
- **Educational Value**: Well-documented codebase serving as comprehensive learning material
- **Real-Time Performance**: Optimized for smooth 60+ FPS performance across target hardware
- **Interactive Demonstrations**: User-controllable parameters and comprehensive camera systems
- **Custom Engine**: Lightweight, purpose-built rendering framework designed for optimal performance

## Available Demonstrations

### 1. Subsurface Scattering
Demonstrates realistic skin and translucent material rendering using screen-space techniques.

### 2. Bloom Effects
Advanced high dynamic range bloom implementation with customizable parameters.
- **Features**: Multi-pass gaussian blur, tone mapping, bloom intensity controls

### 3. Eyeball Rendering (WIP)
Specialized rendering for realistic eye materials with subsurface scattering.
- **Features**: Cornea refraction, iris detail, sclera translucency

### 4. 2D Radiance Cascades
TODO

## Technical Architecture

### Core Systems

- **Vulkan Abstraction Layer**: Modern wrapper around raw Vulkan API
- **Scene Management**: Plugin-based architecture for demonstrations
- **Asset Pipeline**: Build-time asset processing and embedding
- **Font Rendering**: Multi-channel signed distance field based text rendering system
- **Math Library**: Custom linear algebra implementation
- **Shader System**: Integrated HLSL & GLSL compilation includes and auto caching and invalidation
### Rendering Features

- **High Dynamic Range Pipeline**: Linear color space with tone mapping
- **Deferred Rendering**: G-buffer based lighting system
- **Post-Processing Stack**: Bloom, tone mapping, gamma correction
- **Dynamic Loading**: Asynchronous scene initialization

### Dependencies

**Minimal External Dependencies:**
- **GLFW**: Window management and input handling
- **Volk**: Dynamic Vulkan loader for optimal performance
- **STB**: Image loading utilities
- **cgltf**: glTF model loading support
- **TinyObjLoader**: OBJ model support

## Building and Running

### Prerequisites

- **Visual Studio 2019+** (Windows) or **GCC/Clang** (Linux)
- **Vulkan SDK 1.2+** installed and configured
- **Python 3.x** for asset processing
- **CMake 3.12+** for build configuration

### Quick Start

```bash
# Clone the repository
git clone https://github.com/Jaysmito101/AdvancedVulkanDemos.git
cd AdvancedVulkanDemos

# Configure build system
mkdir build && cd build
cmake ..

# Build the project
cmake --build . --config Release

# Run the demos
./avd  # Linux
# or
avd.exe  # Windows
```

### Build Configuration

The project uses CMake with automatic asset generation and dependency management:

```cmake
# Core configuration
cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

# Vulkan detection for cross-platform support
find_package(Vulkan REQUIRED)

# Automatic asset generation
add_custom_target(generate_assets ALL DEPENDS ${ASSETS_TIMESTAMP_FILE})
```

The build system automatically:
- Detects and configures Vulkan SDK
- Processes embedded assets during build
- Handles cross-platform compilation differences
- Manages third-party dependencies through git submodules

## Asset Pipeline

The project uses a sophisticated build-time asset processing system:

### Asset Organization
- **em_assets/**: Embedded assets (shaders, fonts, images, scenes) that are compiled into the executable
- **assets/**: External runtime assets (3D models, textures) loaded at runtime
- **avd_assets/generated/**: Auto-generated C code from the embedded assets

### Asset Processing Workflow
1. **Asset Detection**: CMake monitors changes in asset directories
2. **Python Processing**: `tools/assets.py` processes various asset types:
   - Shaders are compiled to SPIR-V bytecode
   - Fonts are converted to multi-channel signed distance field atlases
   - Images are processed and embedded as byte arrays
   - Scene files are parsed and optimized
3. **Code Generation**: Assets are embedded as C arrays in `avd_assets/generated/`
4. **Build Integration**: Generated code is compiled into the final executable

### Supported Asset Types
- **Shaders**: HLSL and GLSL source files compiled to SPIR-V
- **Fonts**: TTF files converted to MSDF atlases using msdf-atlas-gen
- **Images**: PNG, JPG, HDR textures
- **3D Models**: OBJ and glTF 2.0 format support
- **Scenes**: Custom scene description files

### External Tool Dependencies
- **msdf-atlas-gen**: Automatically downloaded for font atlas generation
- **Python 3.x**: Required for asset processing scripts
- **Vulkan SDK**: Required for shader compilation to SPIR-V

## Controls and Usage

### General Controls
- **Mouse**: Camera rotation (when applicable)
- **Scroll**: Zoom in/out
- **ESC**: Return to main menu
- **F1**: Toggle UI panels

### Demonstration-Specific Controls
Each demonstration includes its own parameter controls accessible through the UI system.

## Project Structure

```
AdvancedVulkanDemos/
├── avd/                     # Core engine code
│   ├── include/            # Public headers
│   │   ├── core/          # Core systems
│   │   ├── vulkan/        # Vulkan abstraction
│   │   ├── scenes/        # Scene management
│   │   ├── math/          # Linear algebra
│   │   └── font/          # Text rendering
│   └── src/               # Implementation files
├── assets/                # External demonstration assets
├── em_assets/             # Embedded assets (shaders, fonts)
├── dep/                   # Third-party dependencies
├── tools/                 # Build and asset tools
├── resources/             # Documentation and references
└── build/                 # Generated build files
```

## Development

### Code Style

The project follows a consistent C-style coding standard:
- **Naming**: `avdFunctionName()`, `AVD_StructName`, `AVD_CONSTANT_NAME`
- **Error Handling**: Consistent return codes with `AVD_CHECK()` macros
- **Memory Management**: RAII-style patterns with init/destroy pairs
- **Documentation**: Comprehensive header documentation

### Adding New Demonstrations

1. Create scene directory in `avd/src/scenes/`
2. Implement scene API functions
3. Register scene in scene manager
4. Add assets to appropriate directories
5. Update build configuration

## Known Limitations

As this is a portfolio project in development:
- Some features may be incomplete or experimental
- Performance optimizations are ongoing
- Documentation is being continuously improved
- Platform support may vary

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Author

**Jaysmito Mukherjee**
- Portfolio: [Jaysmito101](https://github.com/Jaysmito101)
- LinkedIn: [Connect on LinkedIn](https://linkedin.com/in/jaysmito)
- Twitter: [@Jaysmito101](https://twitter.com/Jaysmito101)

