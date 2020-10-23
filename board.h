#include <string>

void init_board();
void print_board();
extern bool is_attacked(int, int);
bool in_check(int);
void save_snapshot(std::string snapshot_name);
void load_snapshot(std::string snapshot_name);
bool parse_move(std::string, int &, int &);
std::string str_move(int, int);
bool empty_move(Move);
void init_state(State &);
void set_state(State &);
void print_state(State &);
int game_over();
bool is_draw();