# Vulkan Renderer

This project aims to create a 3D renderer utilizing the Vulkan API.

🚧 This project is currently under construction 🚧

## Build Instructions

### Prerequisites

Before building the Vulkan Renderer, ensure you have the following dependencies installed:

1. **CMake** (version 3.27 or higher)
2. **Vulkan SDK** (latest version recommended)
3. **glslang** (for shader compilation)
4. **Git** (for cloning the repository)

### Cloning the Repository and Building the Project

1. Clone the Repository

First, clone the repository to your local machine:

```bash
git clone https://github.com/SharkFinPro/VulkanRenderer.git
cd VulkanRenderer
```

2. Create a Build Directory

Create a separate directory for the build process:

```bash
mkdir build
cd build
```

3. Generate Build Files with CMake

Configure the CMake project and generate the necessary build files:

```bash
cmake ..
```

4. Build the Project

Compile the project using your preferred build system:

```bash
cmake --build .
```

5. Run the Executable

After building, all files will have been written to the `bin` directory. You can run the cube example with:

```bash
cd bin
./Cube
```
