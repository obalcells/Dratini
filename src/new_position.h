
class Position {
    Position();
    ~Position();
    bool move_valid(Move);
    void make_move(Move);
    void take_back();
    bool in_check(bool);
    void set_from_fen(std::string);

    friend class BitBoard; 

    private:
        vector<BitBoard> board_history; 
};

