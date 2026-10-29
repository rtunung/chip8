# CHIP-8 Emulator

A lightweight CHIP-8 emulator written in C23 using SDL3.

## Features

* **Core Emulator**: Full implementation of the CHIP-8 instruction set.
* **Graphics**: Graphics rendering with **SDL3**.
* **Audio**: Sound support via **SDL3**.
* **Modern C**: Written using the **C23** standard.

## Prerequisites

Before building the project, ensure you have the following installed:

* **CMake** (version 4.0 or higher)
* **A C compiler** supporting C23 (e.g., GCC or Clang)
* **SDL3** development libraries

## Building the Project

The project uses CMake for its build system.

1. **Create a build directory:**
   ```bash
   mkdir build && cd build
   ```

2. **Configure the project:**
   ```bash
   cmake ..
   ```

3. **Build the emulator:**
   ```bash
   make
   ```

## Running the Emulator

Once built, you can run the emulator from the `build` directory:

```bash
./chip8 <path_to_rom>
```
