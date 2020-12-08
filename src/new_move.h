
class Move {
    public:
        int get_from() const;
        int get_to() const;
        int get_flags() const;

    private:
        uint16_t bits;
};
