#include <string>

void init_board();
void print_board();
extern bool is_attacked(char, bool);
bool in_check(bool);
void save_snapshot(std::string snapshot_name);
void load_snapshot(std::string snapshot_name);
bool parse_move(std::string, char &, char &);
std::string str_move(char, char);
bool empty_move(Move);
int game_over();
bool is_draw();