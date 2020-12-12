
class Position {
    friend class BitBoard; 

    public:
        Position();
        ~Position();

        void set_from_fen(std::string);
        BitBoard get_board();
        uint64_t get_key();
        int get_move_count();

        void make_move(Move);
        void take_back();
        bool move_valid(Move);
        bool in_check();

    private:
        vector<BitBoard> board_history; 
};

