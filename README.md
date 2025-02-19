# Vulkan Renderer

This project focuses on developing a high-performance 3D renderer using the Vulkan API, leveraging its advanced features for optimal graphics rendering and resource management.

## Build Instructions

### Prerequisites

Before building the Vulkan Renderer, ensure you have the following dependencies installed:

1. **CMake** (version 3.27 or higher)
2. **Vulkan SDK** (latest version recommended)
3. **Git** (for cloning the repository)

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

## Linking VulkanEngine in Another Project

You can integrate the VulkanEngine library into your project using CMake's FetchContent module. This approach streamlines the process of fetching, building, and linking external dependencies.

### Example `CMakeLists.txt`

Add the following lines to your `CMakeLists.txt` to fetch and link the VulkanEngine library:

```cmake
include(FetchContent)

# Declare VulkanEngine as a dependency
FetchContent_Declare(
  VulkanEngine
  GIT_REPOSITORY https://github.com/SharkFinPro/VulkanRenderer.git
  GIT_TAG main
)

# Make VulkanEngine available to your project
FetchContent_MakeAvailable(VulkanEngine)

# Link VulkanEngine to your target
target_link_libraries(${PROJECT_NAME} PRIVATE VulkanEngine)

# Include VulkanEngine headers
target_include_directories(${PROJECT_NAME} PRIVATE ${VulkanEngine_SOURCE_DIR}/include)
```
