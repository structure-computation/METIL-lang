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
    OpWithSeq( int method, const char *name, OpWithSeq *ch ); // WRITE_...
    OpWithSeq( const char *cpp_name );
    void init_gen();
    
    static OpWithSeq *new_number( const Rationnal &n );
    static void clear_number_set();
    
    ~OpWithSeq();
    
    void add_child( OpWithSeq *c );
    double val() const { return num / den; }
    bool in_Z() const { double a = std::abs(val()); return std::abs( a - int(a+0.5) ) < 1e-6; }
    bool is_minus_one() const { return type == NUMBER and num == -den; }
    bool is_mul_minus_one() const;
    unsigned nb_instr() const;
    void graphviz_rec( std::ostream &ss ) const;
    bool has_couple( const OpWithSeq *a, const OpWithSeq *b ) const;

    int type;
    std::vector<OpWithSeq *> children;
    std::vector<OpWithSeq *> parents;
    double num, den;
    double access_cost;
    int nb_simd_terms;
    char *cpp_name_str;
    int reg, ordering;
    mutable unsigned id;
    static unsigned current_id;
    int integer_type;
};

OpWithSeq *make_OpWithSeq_rec( const struct Op *op );
void simplifications( OpWithSeq *op );
void update_cost_access_rec( OpWithSeq *op );
void update_nb_simd_terms_rec( OpWithSeq *op );

bool same( const OpWithSeq *a, const OpWithSeq *b );

OpWithSeq *new_inv( OpWithSeq *ch );
OpWithSeq *new_neg( OpWithSeq *ch );
OpWithSeq *new_add_or_mul( int type, const std::vector<OpWithSeq *> &l );
OpWithSeq *new_sub_or_div( int type, OpWithSeq *p, OpWithSeq *n );


std::ostream &operator<<( std::ostream &os, const OpWithSeq &op );

#endif // OP_WITH_SEQ_H
