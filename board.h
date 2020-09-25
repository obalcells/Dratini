#include <string>

void init_board();
void print_board();
extern bool is_attacked(int, int);
bool in_check(int);
void save_snapshot(std::string snapshot_name);
void load_snapshot(std::string snapshot_name);