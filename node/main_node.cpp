#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>

std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::cout << "MeshCompute Node v2.0 gestartet" << std::endl;
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Node beendet." << std::endl;
    return 0;
}
