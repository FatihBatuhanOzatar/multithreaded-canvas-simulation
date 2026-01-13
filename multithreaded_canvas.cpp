#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <random>
#include <chrono>
#include <string>

using namespace std;

// configuration
const int WIDTH = 90;   // Canvas width
const int HEIGHT = 40;  // Canvas height

// shared memory & resources
char canvas[WIDTH][HEIGHT]; // Shared buffer
mutex canvas_lock;          // Global mutex
int total_painted = 0;      // Shared counter
bool system_running = true; // Flag for reporter thread

// performance metrics struct
struct ThreadMetrics {
    int id;                 // Thread ID
    long long run_time;     // ms
    long long wait_time;    // ns
    int op_count;           // operations performed
};

inline bool is_safe(int x, int y) {
    return (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT); // Boundary check
}

void painter_worker(int id, char symbol, int brush_size, ThreadMetrics& metrics) {
    auto start_time = chrono::high_resolution_clock::now(); // Start timer
    metrics.id = id; metrics.wait_time = 0; metrics.op_count = 0; // Init metrics

    for (int k = 0; k < 400; ++k) { // Increased loop for balance
        int x = rand() % WIDTH; int y = rand() % HEIGHT; // Random position

        auto w_start = chrono::high_resolution_clock::now(); // Wait start
        canvas_lock.lock(); // Acquire lock
        auto w_end = chrono::high_resolution_clock::now(); // Wait end
        metrics.wait_time += chrono::duration_cast<chrono::nanoseconds>(w_end - w_start).count(); // Record wait

        for (int i = 0; i < brush_size; ++i) {
            for (int j = 0; j < brush_size; ++j) {
                int tx = x + i; int ty = y + j;
                if (is_safe(tx, ty)) { // Check bounds
                    // Only increment if the spot is empty
                    if (canvas[tx][ty] == ' ') {
                        total_painted++; // Update counter
                        metrics.op_count++; // Local metric
                    }
                    canvas[tx][ty] = symbol; // Draw pixel
                }
            }
        }
        canvas_lock.unlock(); // Release lock
        this_thread::sleep_for(chrono::milliseconds(10)); // Simulate work
    }
    auto end_time = chrono::high_resolution_clock::now(); // Stop timer
    metrics.run_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count(); // Calc duration
}

void circle_worker(int id, char symbol, int radius, ThreadMetrics& metrics) {
    auto start_time = chrono::high_resolution_clock::now(); // Start timer
    metrics.id = id; metrics.wait_time = 0; metrics.op_count = 0;

    for (int k = 0; k < 400; ++k) {
        int cx = rand() % WIDTH; int cy = rand() % HEIGHT; // Center coords

        auto w_start = chrono::high_resolution_clock::now();
        canvas_lock.lock(); // Enter critical section
        auto w_end = chrono::high_resolution_clock::now();
        metrics.wait_time += chrono::duration_cast<chrono::nanoseconds>(w_end - w_start).count();

        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dy = -radius; dy <= radius; ++dy) {
                if (dx * dx + dy * dy <= radius * radius) { // Pythagoras
                    int tx = cx + dx; int ty = cy + dy;
                    if (is_safe(tx, ty)) {
                        // Prevent double counting
                        if (canvas[tx][ty] == ' ') {
                            total_painted++;
                            metrics.op_count++;
                        }
                        canvas[tx][ty] = symbol; // Draw shape
                    }
                }
            }
        }
        canvas_lock.unlock(); // Exit critical section
        this_thread::sleep_for(chrono::milliseconds(15));
    }
    auto end_time = chrono::high_resolution_clock::now();
    metrics.run_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
}

void line_worker(int id, char symbol, int length, bool is_vertical, ThreadMetrics& metrics) {
    auto start_time = chrono::high_resolution_clock::now();
    metrics.id = id; metrics.wait_time = 0; metrics.op_count = 0;

    for (int k = 0; k < 400; ++k) {
        int sx = rand() % WIDTH; int sy = rand() % HEIGHT; // Start point

        auto w_start = chrono::high_resolution_clock::now();
        canvas_lock.lock(); // Acquire mutex
        auto w_end = chrono::high_resolution_clock::now();
        metrics.wait_time += chrono::duration_cast<chrono::nanoseconds>(w_end - w_start).count();

        for (int i = 0; i < length; ++i) {
            int tx = sx + (is_vertical ? 0 : i); // calculate X
            int ty = sy + (is_vertical ? i : 0); // calculate Y
            if (is_safe(tx, ty)) {
                // Prevent double counting
                if (canvas[tx][ty] == ' ') {
                    total_painted++;
                    metrics.op_count++;
                }
                canvas[tx][ty] = symbol;
            }
        }
        canvas_lock.unlock(); // Release mutex
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    auto end_time = chrono::high_resolution_clock::now();
    metrics.run_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
}


void eraser_worker(int id, int size, ThreadMetrics& metrics) {
    auto start_time = chrono::high_resolution_clock::now();
    metrics.id = id; metrics.wait_time = 0; metrics.op_count = 0;

    for (int k = 0; k < 400; ++k) {
        this_thread::sleep_for(chrono::milliseconds(5)); // Slower eraser
        int x = rand() % WIDTH; int y = rand() % HEIGHT;

        auto w_start = chrono::high_resolution_clock::now();
        canvas_lock.lock(); // Lock for modification
        auto w_end = chrono::high_resolution_clock::now();
        metrics.wait_time += chrono::duration_cast<chrono::nanoseconds>(w_end - w_start).count();

        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                int tx = x + i; int ty = y + j;
                if (is_safe(tx, ty)) {
                    // Decrease only if something is erased
                    if (canvas[tx][ty] != ' ') {
                        canvas[tx][ty] = ' '; // Clear pixel
                        total_painted--; // Update global counter
                        metrics.op_count++;
                    }
                }
            }
        }
        canvas_lock.unlock(); // Unlock
    }
    auto end_time = chrono::high_resolution_clock::now();
    metrics.run_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
}

void print_canvas() {
    canvas_lock.lock(); // Thread-safe printing
    cout << string(5, '\n'); // Clear console
    cout << "=== MULTITHREADED CANVAS STATUS ===\n";
    for (int i = 0; i < WIDTH + 2; ++i) cout << "."; cout << endl; // Top Border
    for (int y = 0; y < HEIGHT; ++y) {
        cout << '.';
        for (int x = 0; x < WIDTH; ++x) cout << canvas[x][y];
        cout << '.' << endl;
    }
    for (int i = 0; i < WIDTH + 2; ++i) cout << "."; cout << endl; // Bottom Border
    cout << "Total Pixels Painted: " << total_painted << endl;
    canvas_lock.unlock();
}


void reporter_worker() {
    while (system_running) { // Check global flag
        print_canvas(); // Visualize
        this_thread::sleep_for(chrono::milliseconds(200)); // Refresh rate
    }
    print_canvas(); // Final print
    cout << "Reporting thread terminating..." << endl;
}

int main() {
    srand(time(0));
    for (int i = 0; i < WIDTH; ++i)
        for (int j = 0; j < HEIGHT; ++j) canvas[i][j] = ' '; // Init with spaces

    cout << "System initializing..." << endl;
    ThreadMetrics m1, m2, m3, m4, m5; // Metrics storage

    // Start threads with metrics references
    thread t1(painter_worker, 1, 'X', 3, ref(m1));
    thread t2(circle_worker, 2, 'O', 4, ref(m2));
    thread t3(line_worker, 3, '|', 10, true, ref(m3));
    thread t4(line_worker, 4, '-', 10, false, ref(m4));
    thread t5(eraser_worker, 5, 5, ref(m5));
    thread t_rep(reporter_worker);

    t1.join(); t2.join(); t3.join(); t4.join(); t5.join(); // Wait for workers
    system_running = false; // Stop reporter
    t_rep.join(); // Join reporter

    cout << "\n=== PERFORMANCE METRICS ===\n";
    cout << "ID | Type    | Ops  | Time(ms) | Wait(ns)\n";
    cout << "------------------------------------------\n";
    cout << m1.id << "  | Painter | " << m1.op_count << " | " << m1.run_time << "      | " << m1.wait_time << endl;
    cout << m2.id << "  | Circle  | " << m2.op_count << " | " << m2.run_time << "      | " << m2.wait_time << endl;
    cout << m3.id << "  | Line(V) | " << m3.op_count << " | " << m3.run_time << "      | " << m3.wait_time << endl;
    cout << m4.id << "  | Line(H) | " << m4.op_count << " | " << m4.run_time << "      | " << m4.wait_time << endl;
    cout << m5.id << "  | Eraser  | " << m5.op_count << " | " << m5.run_time << "      | " << m5.wait_time << endl;

    cout << "\nProgram completed." << endl;
    return 0;
}