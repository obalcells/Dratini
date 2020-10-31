/* board.cpp */
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

/* move.cpp */
void make_move(Move);
Move make_move(char, char, char);
void undo_move(Move);
int move_valid(char, char);
int eval();

/* gen.cpp */
void generate_moves();
void generate_capture_moves();

/* eval_tscp.cpp */
int eval_light_pawn(int sq);
int eval_dark_pawn(int sq);
int eval_light_king(int sq);
int eval_lkp(int f);
int eval_dark_king(int sq);
int eval_dkp(int f);

/* main.cpp */
int main();
void test();

/* search.cpp */
void age_history();
Move think();
int search(int, int, int);
int quiescence_search(int, int);
bool timeout();

/* eval.cpp Temporarily disabled
int eval();
int eval_white_pawn(int, bool);
int eval_black_pawn(int, bool);
int eval_king(int, bool);
int eval_sorroundings(int, bool);
*/