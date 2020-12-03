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
    void change_phase(int new_phase);
    void revert_phase();
    void display();
    float elapsed_time();
    Statistics() { init(); }
    ~Statistics() {}
    int get_active_phase() const { return active_phase; }
private:
    std::chrono::time_point<std::chrono::system_clock> initial_time;
    int active_phase, prev_phase;
    int nodes, q_nodes;
    std::vector<float> elapsed_time_in_phase;
    float last_checked_time;
};

extern Statistics stats;