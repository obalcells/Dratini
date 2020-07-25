
extern bool is_attacked(int, int);
extern int move_valid(int, int);
extern Move make_move(int, int, int);
extern void take_back(Move);
void generate_moves();
bool in_check(int);
bool valid_distance(int, int);
