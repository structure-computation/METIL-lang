#ifndef OP_WITH_SEQ_H
#define OP_WITH_SEQ_H

#include <vector>

struct OpWithSeq {
    static const int NUMBER         = -3;
    static const int SYMBOL         = -2;
    static const int WRITE_ADD      = -10;
    static const int WRITE_INIT     = -11;
    static const int WRITE_RET      = -12;
    static const int WRITE_REASSIGN = -13;
    static const int SEQ            = -14; //
    static const int INV            = -20; // 1 / a
    static const int NEG            = -21; // - a

    OpWithSeq( int t );
    OpWithSeq( const Rationnal &n );
    OpWithSeq( int method, const char *name, OpWithSeq *ch ); // WRITE_...
    OpWithSeq( const char *cpp_name );
    void init_gen();
    
    ~OpWithSeq();
    
    void add_child( OpWithSeq *c );
    double val() const { return num / den; }
    bool is_minus_one() const { return type == NUMBER and num == -den; }
    bool is_mul_minus_one() const;

    int type;
    std::vector<OpWithSeq *> children;
    std::vector<OpWithSeq *> parents;
    double num, den;
    char *cpp_name_str;
    int reg, ordering;
    unsigned id;
    static unsigned current_id;
};

OpWithSeq *make_OpWithSeq_rec( const struct Op *op );
void simplifications( OpWithSeq *op );

std::ostream &operator<<( std::ostream &os, const OpWithSeq &op );

#endif // OP_WITH_SEQ_H
