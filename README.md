# Multithreaded Drawing Canvas Simulation
 
A real-time multithreaded simulation developed in C++ that demonstrates concurrency, shared memory management, and race condition prevention using mutex locks and atomic operations.
 
This project simulates multiple "artist" threads drawing shapes (Squares, Circles, Lines) onto a shared 2D canvas, while an "eraser" thread dynamically clears areas, creating a live contention scenario.
 
## Features
 
- **Shared Memory Model:** A global 2D character array accessed by multiple threads simultaneously.
- **Thread Synchronization:** Uses `std::mutex` with `std::lock_guard` for exception-safe Mutual Exclusion over the canvas. Shared scalar state (`total_painted`, `system_running`) is protected via `std::atomic` to avoid unnecessary lock contention.
- **Thread-Safe RNG:** Each thread owns an independent `std::mt19937` instance via `thread_local`, eliminating the data race inherent in `rand()`.
- **Heterogeneous Workers:**
  - `Painter` — Draws square blocks.
  - `Circle Artist` — Draws filled circles using the Pythagorean inequality ($x^2 + y^2 \leq r^2$).
  - `Line Artist` — Draws vertical/horizontal lines.
  - `Eraser` — Clears pixels, demonstrating dynamic resource contention.
  - `Reporter` — Periodically snapshots and prints the live canvas state.
- **Performance Metrics:** Tracks execution time, mutex wait time (contention overhead), and operation throughput per thread.
- **Safety:** Boundary checks on all canvas writes; zero-initialized metrics structs to prevent garbage output.
 
## Technologies & Concepts
 
- **Language:** C++ (Standard Threading Library)
- **Core Concepts:**
  - Multithreading (`std::thread`, `thread_local`)
  - Synchronization (`std::mutex`, `std::lock_guard`)
  - Atomic operations (`std::atomic<int>`, `std::atomic<bool>`)
  - Context Switching & Scheduler Fairness
  - Critical Section Management
  - Performance Profiling (Contention & Wait Time Analysis)
