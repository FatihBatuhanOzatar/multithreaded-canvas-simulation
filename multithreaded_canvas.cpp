#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <random>
#include <chrono>
#include <string>

using namespace std;

// configuration
const int WIDTH = 90;   // Canvas width
const int HEIGHT = 40;  // Canvas height

// shared memory & resources
char canvas[WIDTH][HEIGHT];         // Shared buffer
mutex canvas_lock;                  // Global mutex
atomic<int> total_painted(0);       // FIX 1: atomic — plain int causes data race
atomic<bool> system_running(true);  // FIX 2: atomic — plain bool causes data race (compiler/CPU reordering)

// performance metrics struct
struct ThreadMetrics {
    int id = 0;
    long long run_time = 0;   // ms
    long long wait_time = 0;  // ns
    int op_count = 0;         // FIX 3: zero-initialize all fields — uninitialized values cause garbage output
};

inline bool is_safe(int x, int y) {
    return (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT);
}

// FIX 4: each thread gets its own mt19937 instance
// rand() uses global state and is not thread-safe — concurrent calls cause UB
thread_local mt19937 rng(random_device{}());

inline int rand_x() { return uniform_int_distribution<int>(0, WIDTH - 1)(rng); }
inline int rand_y() { return uniform_int_distribution<int>(0, HEIGHT - 1)(rng); }

void painter_worker(int id, char symbol, int brush_size, ThreadMetrics& metrics) {
    auto start_time = chrono::high_resolution_clock::now();
    metrics.id = id;

    for (int k = 0; k < 400; ++k) {
        int x = rand_x(); int y = rand_y();

        auto w_start = chrono::high_resolution_clock::now();
        // FIX 5: lock_guard instead of manual lock/unlock — exception-safe, no risk of forgetting unlock
        lock_guard<mutex> guard(canvas_lock);
        auto w_end = chrono::high_resolution_clock::now();
        metrics.wait_time += chrono::duration_cast<chrono::nanoseconds>(w_end - w_start).count();

        for (int i = 0; i < brush_size; ++i) {
            for (int j = 0; j < brush_size; ++j) {
                int tx = x + i; int ty = y + j;
                if (is_safe(tx, ty)) {
                    if (canvas[tx][ty] == ' ') {
                        total_painted++;
                        metrics.op_count++;
                    }
                    canvas[tx][ty] = symbol;
                }
            }
        }
        // lock_guard releases automatically here
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    auto end_time = chrono::high_resolution_clock::now();
    metrics.run_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
}

void circle_worker(int id, char symbol, int radius, ThreadMetrics& metrics) {
    auto start_time = chrono::high_resolution_clock::now();
    metrics.id = id;

    for (int k = 0; k < 400; ++k) {
        int cx = rand_x(); int cy = rand_y();

        auto w_start = chrono::high_resolution_clock::now();
        lock_guard<mutex> guard(canvas_lock);
        auto w_end = chrono::high_resolution_clock::now();
        metrics.wait_time += chrono::duration_cast<chrono::nanoseconds>(w_end - w_start).count();

        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dy = -radius; dy <= radius; ++dy) {
                if (dx * dx + dy * dy <= radius * radius) {
                    int tx = cx + dx; int ty = cy + dy;
                    if (is_safe(tx, ty)) {
                        if (canvas[tx][ty] == ' ') {
                            total_painted++;
                            metrics.op_count++;
                        }
                        canvas[tx][ty] = symbol;
                    }
                }
            }
        }
        this_thread::sleep_for(chrono::milliseconds(15));
    }
    auto end_time = chrono::high_resolution_clock::now();
    metrics.run_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
}

void line_worker(int id, char symbol, int length, bool is_vertical, ThreadMetrics& metrics) {
    auto start_time = chrono::high_resolution_clock::now();
    metrics.id = id;

    for (int k = 0; k < 400; ++k) {
        int sx = rand_x(); int sy = rand_y();

        auto w_start = chrono::high_resolution_clock::now();
        lock_guard<mutex> guard(canvas_lock);
        auto w_end = chrono::high_resolution_clock::now();
        metrics.wait_time += chrono::duration_cast<chrono::nanoseconds>(w_end - w_start).count();

        for (int i = 0; i < length; ++i) {
            int tx = sx + (is_vertical ? 0 : i);
            int ty = sy + (is_vertical ? i : 0);
            if (is_safe(tx, ty)) {
                if (canvas[tx][ty] == ' ') {
                    total_painted++;
                    metrics.op_count++;
                }
                canvas[tx][ty] = symbol;
            }
        }
        this_thread::sleep_for(chrono::milliseconds(5));
    }
    auto end_time = chrono::high_resolution_clock::now();
    metrics.run_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
}

void eraser_worker(int id, int size, ThreadMetrics& metrics) {
    auto start_time = chrono::high_resolution_clock::now();
    metrics.id = id;

    for (int k = 0; k < 400; ++k) {
        this_thread::sleep_for(chrono::milliseconds(5));
        int x = rand_x(); int y = rand_y();

        auto w_start = chrono::high_resolution_clock::now();
        lock_guard<mutex> guard(canvas_lock);
        auto w_end = chrono::high_resolution_clock::now();
        metrics.wait_time += chrono::duration_cast<chrono::nanoseconds>(w_end - w_start).count();

        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                int tx = x + i; int ty = y + j;
                if (is_safe(tx, ty)) {
                    if (canvas[tx][ty] != ' ') {
                        canvas[tx][ty] = ' ';
                        total_painted--;
                        metrics.op_count++;
                    }
                }
            }
        }
    }
    auto end_time = chrono::high_resolution_clock::now();
    metrics.run_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
}

void print_canvas() {
    lock_guard<mutex> guard(canvas_lock);
    cout << string(5, '\n');
    cout << "=== MULTITHREADED CANVAS STATUS ===\n";
    for (int i = 0; i < WIDTH + 2; ++i) cout << "."; cout << endl;
    for (int y = 0; y < HEIGHT; ++y) {
        cout << '.';
        for (int x = 0; x < WIDTH; ++x) cout << canvas[x][y];
        cout << '.' << endl;
    }
    for (int i = 0; i < WIDTH + 2; ++i) cout << "."; cout << endl;
    cout << "Total Pixels Painted: " << total_painted.load() << endl;
}

void reporter_worker() {
    while (system_running.load()) {
        print_canvas();
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    print_canvas(); // Final snapshot
    cout << "Reporting thread terminating..." << endl;
}

int main() {
    // FIX 6: removed srand(time(0)) — thread_local mt19937 handles seeding per-thread
    for (int i = 0; i < WIDTH; ++i)
        for (int j = 0; j < HEIGHT; ++j) canvas[i][j] = ' ';

    cout << "System initializing..." << endl;
    ThreadMetrics m1, m2, m3, m4, m5;

    thread t1(painter_worker, 1, 'X', 3, ref(m1));
    thread t2(circle_worker,  2, 'O', 4, ref(m2));
    thread t3(line_worker,    3, '|', 10, true,  ref(m3));
    thread t4(line_worker,    4, '-', 10, false, ref(m4));
    thread t5(eraser_worker,  5, 5,        ref(m5));
    thread t_rep(reporter_worker);

    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();
    system_running.store(false);
    t_rep.join();

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
