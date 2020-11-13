#include "protos.h"
#include "defs.h"
#include "stats.h"
#include <iostream>
#include <string>
#include <iomanip>

void Statistics::init() {
    nodes = q_nodes = 0;
    initial_time = std::chrono::system_clock::now(); 
    last_checked_time = ellapsed_time();
    active_phase = SEARCH;
    ellapsed_time_in_phase.assign(NUM_PHASES, 0.0);
}

void Statistics::activate(int new_phase) {
    if(new_phase == SEARCH) nodes++;
    else if(new_phase == Q_SEARCH) q_nodes++;
    float time = ellapsed_time();  
    ellapsed_time_in_phase[active_phase] += time - last_checked_time;
    active_phase = new_phase;
    last_checked_time = time;
}

void Statistics::display() {
    std::cout << "Spent a total of " << last_checked_time << "ms searching" << endl;
    std::cout << "Searched a total of " << nodes << " main nodes and " << q_nodes << " " << q_nodes << endl;
    std::cout << "Time allocated for each task as percentage:" << endl;
    std::cout << "  MAIN  |  Q_SEARCH  |  MOVE_GEN  |  CAP_MOVE_GEN  |  CHECK  |  MOVE_ORD  " << endl;
    for(int phase = 0; phase < NUM_PHASES; phase++) {
        float percentage = ellapsed_time_in_phase[phase] / last_checked_time;
        if(phase > 0) std::cout << "|";
        std::cout << " ";
        if((int)percentage < 10) std::cout << " ";
        std::cout << std::setprecision (2) << percentage << "% ";
    }
    std::cout << endl;
}

float Statistics::ellapsed_time() {
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<float, std::milli> duration = time_now - initial_time;
    return duration.count();
}