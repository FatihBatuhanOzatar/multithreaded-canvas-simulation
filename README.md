# Multithreaded Drawing Canvas Simulation

A real-time multithreaded simulation developed in C++ that demonstrates **concurrency**, **shared memory management**, and **race condition prevention** using Mutex locks.

This project simulates multiple "artist" threads drawing shapes (Squares, Circles, Lines) onto a shared 2D canvas, while an "eraser" thread dynamically clears areas, creating a live contention scenario.

## Features

* **Shared Memory Model:** A global 2D character array accessed by multiple threads simultaneously.
* **Thread Synchronization:** Uses `std::mutex` to ensure **Mutual Exclusion** and prevent data corruption (Race Conditions).
* **Heterogeneous Workers:**
    *  **Painter:** Draws squares.
    *  **Circle Artist:** Draws mathematical circles ($x^2 + y^2 \le r^2$).
    *  **Line Artist:** Draws vertical/horizontal lines.
    *  **Eraser:** Clears pixels (Dynamic resource contention).
    *  **Reporter:** Monitors and displays the live state of the canvas.
* **Performance Metrics:** Tracks execution time, lock wait times (contention overhead), and operation throughput per thread.
* **Safety:** Implements boundary checks and defensive programming logic.

##  Technologies & Concepts

* **Language:** C++ (Standard Threading Library)
* **Core Concepts:**
    * Multithreading (`std::thread`)
    * Synchronization (`std::mutex`, `lock()`, `unlock()`)
    * Context Switching & Scheduler Fairness
    * Critical Section Management
    * Performance Profiling (Wait Time Analysis)

##  How to Build & Run

### Prerequisites
* A C++ Compiler (GCC, Clang, or MSVC) supporting C++11 or higher.

### Linux / macOS (Terminal)
```bash
# Compile the project (Link with pthread library)
g++ -std=c++11 -pthread multithreaded_canvas.cpp -o canvas_sim

# Run the simulation
./canvas_sim
