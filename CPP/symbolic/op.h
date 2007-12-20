#ifndef OP_H
#define OP_H

#include "splittedvec.h"
#include "config.h"
#include <usual_strings.h>

struct Op {
    static const int NUMBER = -3;
    static const int SYMBOL = -2;
    static const int SUMSEQ = -1;
    
    // ------------------------------------
    struct NumberData { /// if NUMBER
        Rationnal val;
    };
    struct SymbolData { /// if SYMBOL
        char *cpp_name_str;
        char *tex_name_str;
    };
    struct FuncData { /// if type >= 0
        static const unsigned max_nb_children = 2;
        Op *children[max_nb_children];
    };
    
    // ----------------------------------
    static Op *new_number( const Rationnal &val ); ///
    static Op *new_symbol( const char *cpp_name_str, unsigned cpp_name_si, const char *tex_name_str, unsigned tex_name_si );
    static Op *new_function( int type, Op *a );
    static Op *new_function( int type, Op *a, Op *b );
    
    void init( int type_ );
    void destroy();
    inline Op *inc_ref() { ++cpt_use; return this; }
    
    //     void clear_additional_info_rec();
    //     void clear_additional_info_rec( unsigned clear_id );
    
    bool is_zero() const;
    bool is_one() const;
    bool is_minus_one() const;
    bool necessary_positive() const;
    bool necessary_negative() const;
    bool necessary_positive_or_null() const;
    bool necessary_negative_or_null() const;
    
    void cpp_repr( std::ostream &os ) const;
    void tex_repr( std::ostream &os ) const;
    void graphviz_repr( std::ostream &os ) const;
    bool depends_on( const Op *a ) const;
    bool depends_on_rec( const Op *a ) const; // without update of current_op

    const Op *find_discontinuity( const Op *var ) const;
    const Op *find_discontinuity_rec( const Op *var ) const; // without update of current_op
    
    
    int nb_nodes() const; //
    int nb_nodes_rec() const; // without update of current_op
    
    int nb_nodes_of_type( int t ) const; //
    int nb_nodes_of_type_rec( int t ) const; // without update of current_op
    
    void set_beg_value( Rationnal b, bool inclusive );
    void set_end_value( Rationnal e, bool inclusive );

    // ..._data 
    NumberData *number_data() { return reinterpret_cast<NumberData *>( this + 1 ); }
    SymbolData *symbol_data() { return reinterpret_cast<SymbolData *>( this + 1 ); }
    FuncData   *func_data  () { return reinterpret_cast<FuncData   *>( this + 1 ); }
    // const versions
    const NumberData *number_data() const { return reinterpret_cast<const NumberData *>( this + 1 ); }
    const SymbolData *symbol_data() const { return reinterpret_cast<const SymbolData *>( this + 1 ); }
    const FuncData   *func_data  () const { return reinterpret_cast<const FuncData   *>( this + 1 ); }

    // ----------------------------------
    int type;
    SplittedVec<Op *,2,8> parents;
    mutable Op *additional_info;
    mutable unsigned op_id;
    mutable unsigned cpt_use;
    
    bool beg_value_valid, end_value_valid, beg_value_inclusive, end_value_inclusive;
    Rationnal beg_value, end_value;
    
    static unsigned current_op;
};

inline void dec_ref( Op *op ) {
    if ( --op->cpt_use == 0 ) {
        op->destroy();
        free( op );
    }
}

inline bool are_numbers( const Op *a, const Op *b ) {
    return a->type == Op::NUMBER and b->type == Op::NUMBER;
}

inline bool is_a_number( const Op *a ) {
    return a->type == Op::NUMBER;
}

inline bool same_op( const Op *a, const Op *b ) {
    if ( are_numbers( a, b ) )
        return a->number_data()->val == b->number_data()->val;
    return a == b;
}

inline bool is_a_sub( const Op *a ) {
    return a->type == STRING_mul_NUM and a->func_data()->children[0]->is_minus_one();
}

inline bool is_a_inv( const Op *a ) {
    return a->type == STRING_pow_NUM and a->func_data()->children[1]->is_minus_one();
}

inline bool is_a_mul_by_neg_number( const Op *a ) {
    return a->type == STRING_mul_NUM and a->func_data()->children[0]->type == Op::NUMBER and a->func_data()->children[0]->number_data()->val.is_neg();
}

struct MulSeq {
    Rationnal e;
    const Op *op;
};
void find_mul_items_and_coeff_rec( const Op *a, SplittedVec<MulSeq,4,16,true> &items );

void get_sub_symbols( Op *op, SplittedVec<Op *,32> &symbols );

void get_child_not_of_type_mul( Op *op, SplittedVec<Op *,32> &res );

#endif


