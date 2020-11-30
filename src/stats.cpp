#include <iostream>
#include <string>
#include <iomanip>

#include "defs.h"
#include "data.h"
#include "stats.h"

void Statistics::init() {
    nodes = q_nodes = 0;
    initial_time = std::chrono::system_clock::now(); 
    last_checked_time = elapsed_time();
    active_phase = SEARCH;
    elapsed_time_in_phase.assign(NUM_PHASES, 0.0);
}

void Statistics::change_phase(int new_phase) {
    if(new_phase == SEARCH) nodes++;
    else if(new_phase == Q_SEARCH) q_nodes++;
    // std::string phases[6] = { "SEARCH", "Q_SEARCH", "MOVE_GEN", "CAP_MOVE_GEN", "CHECK", "MOVE_ORD" };
    // std::cerr << "Now at phase " << phases[new_phase] << '\n'; 
    float time = elapsed_time();  
    elapsed_time_in_phase[active_phase] += time - last_checked_time;
    prev_phase = active_phase;
    active_phase = new_phase;
    last_checked_time = time;
}

void Statistics::revert_phase() {
    change_phase(prev_phase);
}

void Statistics::display() {
    std::cout << "Spent a total of " << last_checked_time << "ms searching" << endl;
    std::cout << "Searched a total of " << nodes << " main nodes and " << q_nodes << " q nodes" << endl;
    std::cout << "Time allocated for each task:" << endl;
    std::cout << "    MAIN        Q_SEARCH    MOVE_GEN    CMOVE_GEN      CHECK       MOVE_ORD" << endl;
    float total_time = elapsed_time();
    for(int phase = 0; phase < NUM_PHASES; phase++) {
        int percentage = int((elapsed_time_in_phase[phase] / total_time) * 10000.0);
        // we want to pad the output so that it's always printed with 2 decimals
        std::cout << "   ";
        if((percentage / 1000) % 10 != 0)
            std::cout << (percentage / 1000) % 10; 
        else
            std::cout << ' ';
        std::cout << (percentage / 100) % 10
                  << "."
                  << (percentage / 10) % 10
                  << (percentage / 1) % 10
                  << '%';
        std::cout << "    ";
    }
    std::cout << endl;
}

float Statistics::elapsed_time() {
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<float, std::milli> duration = time_now - initial_time;
    return duration.count();
}