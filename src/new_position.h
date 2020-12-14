
class Position {
    friend class Board;

    public:
        Position();
        ~Position();
        void set_from_fen(std::string);
        BitBoard get_board();
        uint64_t get_key();
        int get_move_count();
        void make_move(const Move&);
        bool make_move(const std::string&);
        void take_back();
        bool in_check();

    private:
        vector<BitBoard> board_history; 
};

