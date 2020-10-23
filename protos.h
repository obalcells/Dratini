/* board.cpp */
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
bool isDraw();

/* move.cpp */
Move make_move(int, int, int);
void undo_move(Move);
int move_valid(int, int);int eval();

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
void think(int);
int search(int, int, int);
int quiescence_search(int, int);

/* eval.cpp Temporarily disabled
int eval();
int eval_white_pawn(int, bool);
int eval_black_pawn(int, bool);
int eval_king(int, bool);
int eval_sorroundings(int, bool);
*/