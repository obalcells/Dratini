#pragma once

#include <vector>
#include <chrono>

enum phase {
    SEARCH,
    Q_SEARCH,
    MOVE_GEN,
    CAP_MOVE_GEN,
    CHECK,
    MOVE_ORD,
    NUM_PHASES
};

class Statistics {
public:
    void init();
    void activate(int new_phase);
    void display();
    float ellapsed_time();
    Statistics() { init(); }
    ~Statistics() {}
private:
    static std::chrono::time_point<std::chrono::system_clock> initial_time;
    static int active_phase, nodes, q_nodes;
    static std::vector<float> ellapsed_time_in_phase;
    static float last_checked_time;
};

extern Statistics stats;