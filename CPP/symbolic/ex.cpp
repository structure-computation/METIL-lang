#include "ex.h"
#include "op.h"
#include "globaldata.h"
#include "thread.h"
#include <sstream>
#include <complex>
#include <map>

struct ExPtrCmp {
    bool operator()( const Ex &a, const Ex &b ) const {
        if ( a.op->type==Op::NUMBER and b.op->type==Op::NUMBER )
            return a.op->number_data()->val < b.op->number_data()->val;
        return a.op < b.op;
    }
};

inline bool same_ex( const Ex &a, const Ex &b ) {
    return same_op( a.op, b.op );
}

void Ex::init() {
    op = Op::new_number( 0 );
    op->inc_ref();
}

void Ex::init( Op *ex ) {
    op = ex;
    op->inc_ref();
}

void Ex::init( const Ex &ex ) {
    op = ex.op;
    op->inc_ref();
}

void Ex::init( const T &v ) {
    op = Op::new_number( v );
    op->inc_ref();
}

void Ex::init( int v ) {
    op = Op::new_number( v );
    op->inc_ref();
}

void Ex::init( const char *cpp_name_str, unsigned cpp_name_si, const char *tex_name_str, unsigned tex_name_si ) {
    op = Op::new_symbol( cpp_name_str, cpp_name_si, tex_name_str, tex_name_si );
    op->inc_ref();
}

void Ex::reassign( const Ex &c ) {
    c.op->inc_ref();
    dec_ref( op );
    op = c.op;
}

void Ex::destroy() {
    dec_ref( op );
}

bool Ex::known_at_compile_time() const {
    return ( op->type == Op::NUMBER );
}

std::string Ex::graphviz_repr() const {
    std::ostringstream ss;
    op->graphviz_repr( ss );
    return ss.str();
}

std::string Ex::cpp_repr() const {
    std::ostringstream ss;
    op->cpp_repr( ss );
    return ss.str();
}

std::string Ex::tex_repr() const {
    std::ostringstream ss;
    op->tex_repr( ss );
    return ss.str();
}

Ex::T Ex::value() const {
    assert( op->type == Op::NUMBER );
    return op->number_data()->val;
}

bool Ex::depends_on( const Ex &a ) const {
    return op->depends_on( a.op );
}

Op *to_inc_op( const Ex &ex ) {
    return ex.op->inc_ref();
}

std::ostream &operator<<( std::ostream &os, const Ex &ex ) {
    ex.op->cpp_repr( os );
    return os;
}

Ex &Ex::operator+=(const Ex &c) { *this = *this + c; return *this; }

Ex &Ex::operator*=(const Ex &c) { *this = *this * c; return *this; }

void Ex::set_beg_value( T b, bool inclusive ) {
    op->set_beg_value( b, inclusive );
}

void Ex::set_end_value( T e, bool inclusive ) {
    op->set_end_value( e, inclusive );
}

bool Ex::beg_value_valid() const {
    return op->beg_value_valid;
}

bool Ex::end_value_valid() const {
    return op->end_value_valid;
}

bool Ex::beg_value_inclusive() const {
    return op->beg_value_inclusive;
}

bool Ex::end_value_inclusive() const {
    return op->end_value_inclusive;
}

Rationnal Ex::beg_value() const {
    return op->beg_value;
}

Rationnal Ex::end_value() const {
    return op->end_value;
}

// --------------------------------------------------------------------------------------------
struct SumSeq {
    Rationnal c;
    Op *op;
    Ex to_op() {
        if ( c.is_one() )
            return op;
        return Ex( c ) * Ex( op );
    }
};

Ex make_add_seq( SplittedVec<SumSeq,4,16,true> &items_c ) {
    if ( not items_c.size() )
        return 0;
    //
    Ex res = items_c[0].to_op();
    for(unsigned i=1;i<items_c.size();++i)
        res = res + items_c[i].to_op();
    return res;
}

/// @see operator+
void find_add_items_and_coeff_rec( Op *a, SplittedVec<SumSeq,4,16,true> &items ) {
    if ( a->type == STRING_add_NUM ) { // anything_but_a_number + anything_but_a_number
        find_add_items_and_coeff_rec( a->func_data()->children[0], items );
        find_add_items_and_coeff_rec( a->func_data()->children[1], items );
    } else if ( a->type == STRING_mul_NUM and a->func_data()->children[0]->type == Op::NUMBER ) { // 10 * a
        SumSeq *s = items.new_elem();
        s->c.init( a->func_data()->children[0]->number_data()->val );
        s->op = a->func_data()->children[1];
    } else { // a
        SumSeq *s = items.new_elem();
        s->c.init( 1 );
        s->op = a;
    }
}
/// @see operator+
void add_items_and_coeffs( const SplittedVec<SumSeq,4,16,true> &items_a, SplittedVec<SumSeq,4,16,true> &items_b ) {
    for(unsigned i=0;i<items_a.size();++i) {
        for(unsigned j=0;;++j) {
            if ( j == items_b.size() ) {
                items_b.push_back( items_a[i] );
                break;
            } 
            if ( items_a[ i ].op == items_b[ j ].op ) {
                items_b[ j ].c += items_a[ i ].c;
                if ( not items_b[ j ].c )
                    items_b.erase_elem_nb_in_unordered_list( j );
                break;
            }
        }
    }
}

void update_inter_add( Op *res, Op *a, Op *b ) {
    if ( not res->cpt_use ) {
        if ( a->beg_value_valid and b->beg_value_valid )
            res->set_beg_value( a->beg_value + b->beg_value, a->beg_value_inclusive and b->beg_value_inclusive );
        if ( a->end_value_valid and b->end_value_valid )
            res->set_end_value( a->end_value + b->end_value, a->end_value_inclusive and b->end_value_inclusive );
    }
}

Ex add_number_and_expr( const Ex &a, const Ex &b ) { // a is a number
    if ( a.op->number_data()->val.is_zero() )
        return b;
    // 10 + ( 5 + a )
    if ( b.op->type == STRING_add_NUM and b.op->func_data()->children[0]->type == Op::NUMBER ) {
        return Ex( a.op->number_data()->val + b.op->func_data()->children[0]->number_data()->val ) + 
               Ex( b.op->func_data()->children[1] );
    }
    Op *res = Op::new_function( STRING_add_NUM, a.op, b.op );
    update_inter_add( res, a.op, b.op );
    return res;
}

Ex operator+( const Ex &a, const Ex &b ) {
    // 10 + ...
    if ( a.op->type == Op::NUMBER ) {
        // 10 + 20
        if ( b.op->type == Op::NUMBER )
            return a.op->number_data()->val + b.op->number_data()->val;
        // 10 + ( ... )
        return add_number_and_expr( a, b );
    }
    // ( not a number ) + 10 -> 10 + ( not a number )
    if ( b.op->type == Op::NUMBER )
        return add_number_and_expr( b, a );
        
    // ( 10 + a ) + ( not a number ) -> 10 + ( a + ( not a number ) )
    if ( a.op->type == STRING_add_NUM and a.op->func_data()->children[0]->type == Op::NUMBER ) {
        // ( 10 + a ) + ( 20 + b ) -> 30 + ( a + b )
        if ( b.op->type == STRING_add_NUM and b.op->func_data()->children[0]->type == Op::NUMBER ) {
            Ex new_num = a.op->func_data()->children[0]->number_data()->val + b.op->func_data()->children[0]->number_data()->val;
            Ex new_sum = Ex( a.op->func_data()->children[1] ) + Ex( b.op->func_data()->children[1] );
            return new_num + new_sum;
        }
        // ( 10 + a ) + b -> 10 + ( a + b )
        Ex new_sum = Ex( a.op->func_data()->children[1] ) + b;
        return Ex( a.op->func_data()->children[0] ) + new_sum;
    }
    // a + ( 10 + b ) -> 10 + ( a + b )
    if ( b.op->type == STRING_add_NUM and b.op->func_data()->children[0]->type == Op::NUMBER ) {
        Ex new_sum = a + b.op->func_data()->children[1];
        return Ex( b.op->func_data()->children[0] ) + new_sum;
    }

    // ( 10 * a + b + anything_but_a_number ) + ( 2 * b + a + anything_but_a_number ) -> ...
    SplittedVec<SumSeq,4,16,true> items_a, items_b;
    find_add_items_and_coeff_rec( a.op, items_a );
    find_add_items_and_coeff_rec( b.op, items_b );
    unsigned old_size = items_a.size() + items_b.size();
    add_items_and_coeffs( items_b, items_a );
    if ( old_size != items_a.size() )
        return make_add_seq( items_a );
    
    //
    Op *res;
    if ( a.op < b.op )
        res = Op::new_function( STRING_add_NUM, a.op, b.op );
    else
        res = Op::new_function( STRING_add_NUM, b.op, a.op );
    
    // interval
    update_inter_add( res, a.op, b.op );
    
    return res;
}

Ex operator-( const Ex &a, const Ex &b ) {
    return a + (-b);
}

Ex operator-( const Ex &a ) {
    if ( a.op->type == Op::NUMBER )
        return - a.op->number_data()->val;
    if ( a.op->type == STRING_add_NUM and a.op->func_data()->children[0]->type == Op::NUMBER ) { // - ( 1 + a ) -> -1 + (-a)
        Ex new_num = - a.op->func_data()->children[0]->number_data()->val;
        return new_num + ( - Ex( a.op->func_data()->children[1] ) );
    }
    if ( a.op->type == STRING_mul_NUM and a.op->func_data()->children[0]->type == Op::NUMBER ) { // - 10 * a -> (-10) * a
        Ex new_num = - a.op->func_data()->children[0]->number_data()->val;
        return new_num * Ex( a.op->func_data()->children[1] );
    }
    return Ex( -1 ) * a; // (-1) * a
}

void update_inter_mul( Op *res, Op *a, Op *b ) {
    if ( not res->cpt_use ) {
        if ( a->beg_value_valid and a->end_value_valid and b->beg_value_valid and b->end_value_valid ) {
            Ex::T v0 = a->beg_value * b->beg_value; bool i0 = a->beg_value_inclusive and b->beg_value_inclusive;
            Ex::T v1 = a->beg_value * b->end_value; bool i1 = a->beg_value_inclusive and b->end_value_inclusive;
            Ex::T v2 = a->end_value * b->beg_value; bool i2 = a->end_value_inclusive and b->beg_value_inclusive;
            Ex::T v3 = a->end_value * b->end_value; bool i3 = a->end_value_inclusive and b->end_value_inclusive;
            // min
            Ex::T miv = v0; bool mii = i0;
            if ( v1 < miv or ( v1==miv and i1 ) ) { miv = v1; mii = i1; }
            if ( v2 < miv or ( v2==miv and i2 ) ) { miv = v2; mii = i2; }
            if ( v3 < miv or ( v3==miv and i3 ) ) { miv = v3; mii = i3; }
            res->set_beg_value( miv, mii );
            // max
            Ex::T mav = v0; bool mai = i0;
            if ( v1 > mav or ( v1==mav and i1 ) ) { mav = v1; mai = i1; }
            if ( v2 > mav or ( v2==mav and i2 ) ) { mav = v2; mai = i2; }
            if ( v3 > mav or ( v3==mav and i3 ) ) { mav = v3; mai = i3; }
            res->set_end_value( mav, mai );
        }
        else if ( a->type == Op::NUMBER ) {
            if ( a->number_data()->val.is_pos() ) {
                if ( b->beg_value_valid )
                    res->set_beg_value( a->number_data()->val * b->beg_value, b->beg_value_inclusive );
                if ( b->end_value_valid )
                    res->set_end_value( a->number_data()->val * b->end_value, b->end_value_inclusive );
            }
            else {
                if ( b->end_value_valid )
                    res->set_beg_value( a->number_data()->val * b->end_value, b->end_value_inclusive );
                if ( b->beg_value_valid )
                    res->set_end_value( a->number_data()->val * b->beg_value, b->beg_value_inclusive );
            }
        }
    }
}

Ex mul_number_and_expr( Ex a, Ex b ) { // a is a number
    if ( a.op->number_data()->val.is_one() )
        return b;
    if ( a.op->number_data()->val.is_zero() )
        return a;
    // 10 * ( 5 * a )
    if ( b.op->type == STRING_mul_NUM and b.op->func_data()->children[0]->type == Op::NUMBER ) {
        Ex new_num = a.op->number_data()->val * b.op->func_data()->children[0]->number_data()->val;
        return mul_number_and_expr( new_num, b.op->func_data()->children[1] );
    }
    // 10 * ( a + 5 * b ) -> 10 * a + 50 * b
    if ( b.op->type == STRING_add_NUM ) {
        SplittedVec<SumSeq,4,16,true> items;
        find_add_items_and_coeff_rec( b.op->func_data()->children[0], items );
        find_add_items_and_coeff_rec( b.op->func_data()->children[1], items );
        for(unsigned i=0;i<items.size();++i)
            items[i].c *= a.op->number_data()->val;
        return make_add_seq( items );
    }
    
    //
    Op *res = Op::new_function( STRING_mul_NUM, a.op, b.op );
    update_inter_mul( res, a.op, b.op );
    return res;
}

Ex to_op( const MulSeq &ms ) {
    if ( ms.e.is_one() )
        return ms.op;
    return pow( Ex( ms.op ), Ex( ms.e ) );
}

Ex make_mul_seq( SplittedVec<MulSeq,4,16,true> &items_c ) {
    if ( not items_c.size() )
        return 1;
    //
    Ex res = to_op( items_c[0] );
    Rationnal n = 1;
    for(unsigned i=1;i<items_c.size();++i) {
        if ( items_c[i].op->type == Op::NUMBER )
            n *= to_op( items_c[i] ).op->number_data()->val;
        else
            res = res * to_op( items_c[i] );
    }
    if ( n.is_one() )
        return res;
    return Ex( n ) * res;
}

/// @see operator*
void mul_items_and_coeffs( const SplittedVec<MulSeq,4,16,true> &items_a, SplittedVec<MulSeq,4,16,true> &items_b ) {
    for(unsigned i=0;i<items_a.size();++i) {
        for(unsigned j=0;;++j) {
            if ( j == items_b.size() ) {
                items_b.push_back( items_a[i] );
                break;
            } 
            if ( items_a[ i ].op == items_b[ j ].op ) {
                items_b[ j ].e += items_a[ i ].e;
                if ( not items_b[ j ].e )
                    items_b.erase_elem_nb_in_unordered_list( j );
                break;
            }
        }
    }
}

Ex operator*( const Ex &a, const Ex &b ) {
    // 10 * ...
    if ( a.op->type == Op::NUMBER ) {
        // 10 * 20
        if ( b.op->type == Op::NUMBER )
            return Op::new_number( a.op->number_data()->val * b.op->number_data()->val );
        // 10 * ( ... )
        return mul_number_and_expr( a, b );
    }
    // ( not a number ) * 10 -> 10 * ( not a number )
    if ( b.op->type == Op::NUMBER )
        return mul_number_and_expr( b, a );
        
    // ( 10 * a ) * ( not a number ) -> 10 * ( a * ( not a number ) )
    if ( a.op->type == STRING_mul_NUM and a.op->func_data()->children[0]->type == Op::NUMBER ) {
        // ( 10 * a ) * ( 20 * b ) -> 30 * ( a * b )
        if ( b.op->type == STRING_mul_NUM and b.op->func_data()->children[0]->type == Op::NUMBER ) {
            Ex new_num = a.op->func_data()->children[0]->number_data()->val * b.op->func_data()->children[0]->number_data()->val;
            Ex new_mul = Ex( a.op->func_data()->children[1] ) * Ex( b.op->func_data()->children[1] );
            return new_num * new_mul;
        }
        // ( 10 * a ) * b -> 10 * ( a * b )
        Ex new_mul = Ex( a.op->func_data()->children[1] ) * b;
        return Ex( a.op->func_data()->children[0] ) * new_mul;
    }
    // a * ( 10 * b ) -> 10 * ( a * b )
    if ( b.op->type == STRING_mul_NUM and b.op->func_data()->children[0]->type == Op::NUMBER ) {
        Ex new_mul = a * Ex( b.op->func_data()->children[1] );
        return Ex( b.op->func_data()->children[0] ) * new_mul;
    }
    
    // ( a^10 * b * anything_but_a_number ) * ( a * anything_but_a_number )
    SplittedVec<MulSeq,4,16,true> items_a, items_b;
    find_mul_items_and_coeff_rec( a.op, items_a );
    find_mul_items_and_coeff_rec( b.op, items_b );
    unsigned old_size = items_a.size() + items_b.size();
    mul_items_and_coeffs( items_b, items_a );
    if ( old_size != items_a.size() )
      return make_mul_seq( items_a );
    
    //
    Op *res;
    if ( a.op < b.op )
        res = Op::new_function( STRING_mul_NUM, a.op, b.op );
    else
        res = Op::new_function( STRING_mul_NUM, b.op, a.op );
    
    // interval
    update_inter_mul( res, a.op, b.op );
    return res;
}
Ex inv( const Ex &a ) {
    return pow( a, Ex(-1) );
}

Ex operator/( const Ex &a, const Ex &b ) {
    return a * inv( b );
}

bool need_abs_for_pow( Op *op, const Rationnal &e0, const Rationnal &nn ) {
    if ( op->necessary_positive_or_null() )
        return false;
    if ( not e0.is_even() ) // (a^3)^... or (a^0.5)^... -> non need
        return false;
    if ( nn.is_even() ) // a^(2) -> abs is not useful
        return false;
    // (a^6)^0.5 -> a^3
    return true;
}

void update_inter_pow( Op *res, Op *a, Op *b ) {
    if ( not res->cpt_use ) {
        if ( b->type == Op::NUMBER ) {
            Unsigned32 p( b->number_data()->val );
            if ( b->number_data()->val.is_even() and b->number_data()->val.is_pos() ) { // ^2, ^4, ...
                if ( a->beg_value_valid and a->end_value_valid ) {
                    if ( a->beg_value.is_neg_or_null() and a->end_value.is_pos_or_null() ) { // +-----0-----+
                        res->set_beg_value( 0, true );
                        Ex::T b2 = pow( a->beg_value, p );
                        Ex::T e2 = pow( a->end_value, p );
                        //
                        if ( b2 == e2 )
                            res->set_end_value( b2, a->beg_value_inclusive and a->end_value_inclusive );
                        else if ( b2 > e2 )
                            res->set_end_value( b2, a->beg_value_inclusive );
                        else
                            res->set_end_value( e2, a->end_value_inclusive );
                    }
                    else if ( a->beg_value.is_pos_or_null() and a->end_value.is_pos_or_null() ) { // 0  +-------+
                        res->set_beg_value( pow( a->beg_value, p ), a->beg_value_inclusive );
                        res->set_end_value( pow( a->end_value, p ), a->end_value_inclusive );
                    }
                    else if ( a->beg_value.is_neg_or_null() and a->end_value.is_neg_or_null() ) { // +-------+  0
                        res->set_beg_value( pow( a->end_value, p ), a->end_value_inclusive );
                        res->set_end_value( pow( a->beg_value, p ), a->beg_value_inclusive );
                    }
                }
            }
            else if ( b->number_data()->val.is_pos() and not b->number_data()->val.is_integer() ) { // ^0.5, ^1.3, ... -> should be monotonic
                if ( a->beg_value_valid and a->beg_value.is_pos_or_null() )
                    res->set_beg_value( pow_96( a->beg_value, p ), a->beg_value_inclusive );
                if ( a->end_value_valid and a->end_value.is_pos_or_null() )
                    res->set_end_value( pow_96( a->end_value, p ), a->end_value_inclusive );
            }
        }
        //         if ( a->beg_value_valid and a->end_value_valid and b->beg_value_valid and b->end_value_valid ) {
        //             Ex::T v0 = pow_96( a->beg_value, b->beg_value ); bool i0 = a->beg_value_inclusive and b->beg_value_inclusive;
        //             Ex::T v1 = a->beg_value * b->end_value; bool i1 = a->beg_value_inclusive and b->end_value_inclusive;
        //             Ex::T v2 = a->end_value * b->beg_value; bool i2 = a->end_value_inclusive and b->beg_value_inclusive;
        //             Ex::T v3 = a->end_value * b->end_value; bool i3 = a->end_value_inclusive and b->end_value_inclusive;
        //             // min
        //             Ex::T miv = v0; bool mii = i0;
        //             if ( v1 < miv or ( v1==miv and i1 ) ) { miv = v1; mii = i1; }
        //             if ( v2 < miv or ( v2==miv and i2 ) ) { miv = v2; mii = i2; }
        //             if ( v3 < miv or ( v3==miv and i3 ) ) { miv = v3; mii = i3; }
        //             res->set_beg_value( miv, mii );
        //             // max
        //             Ex::T mav = v0; bool mai = i0;
        //             if ( v1 > mav or ( v1==mav and i1 ) ) { mav = v1; mai = i1; }
        //             if ( v2 > mav or ( v2==mav and i2 ) ) { mav = v2; mai = i2; }
        //             if ( v3 > mav or ( v3==mav and i3 ) ) { mav = v3; mai = i3; }
        //             res->set_end_value( mav, mai );
        //         }
    }
}

Ex pow( const Ex &a, const Ex &b ) {
    if ( b.op->type == Op::NUMBER ) { // a ^ 10
        if ( a.op->type == Op::NUMBER ) // 10 ^ 32
            return Op::new_number( pow_96( a.op->number_data()->val, b.op->number_data()->val ) );
        if ( b.op->is_zero() )
            return 1;
        if ( b.op->is_one() )
            return a;
        if ( a.op->type == STRING_abs_NUM ) { // abs(a) ^ n
            if ( b.op->number_data()->val.is_even() ) // abs(a) ^ 4
                return pow( Ex( a.op->func_data()->children[0] ), b );
        }
        // ( a * b^2 ) ^ 3 -> a^3 * b^6
        if ( a.op->type == STRING_mul_NUM and b.op->number_data()->val.is_integer() ) {
            SplittedVec<MulSeq,4,16,true> items;
            find_mul_items_and_coeff_rec( a.op->func_data()->children[0], items );
            find_mul_items_and_coeff_rec( a.op->func_data()->children[1], items );
            //
            for(unsigned i=0;i<items.size();++i)
                items[i].e *= b.op->number_data()->val;
            return make_mul_seq( items );
            
        }
        // ( a^2 ) ^ 3 -> a^6
        if ( a.op->type == STRING_pow_NUM and a.op->func_data()->children[1]->type == Op::NUMBER ) {
            Rationnal e0 = a.op->func_data()->children[1]->number_data()->val;
            Rationnal nn = e0 * b.op->number_data()->val;
            // (a^0.5)^3 -> a^1.5
            // (a^2)^0.5 -> abs(a)
            if ( need_abs_for_pow( a.op->func_data()->children[0], e0, nn ) ) {
                if ( nn.is_one() )
                    return abs( Ex( a.op->func_data()->children[0] ) );
                return pow( abs( Ex( a.op->func_data()->children[0] ) ), Ex( nn ) );
            } else {
                if ( nn.is_one() )
                    return a.op->func_data()->children[0]->inc_ref();
                return pow( Ex( a.op->func_data()->children[0] ), Ex( nn ) );
            }
        }
    }
    //
    Op *res = Op::new_function( STRING_pow_NUM, a.op, b.op );
    update_inter_pow( res, a.op, b.op );
    //
    return res;
}

Ex mod( const Ex &a, const Ex &b ) {
    if ( are_numbers( a.op, b.op ) ) return mod( a.op->number_data()->val, b.op->number_data()->val );
    if ( a.op->is_zero() ) return a;
    return Op::new_function( STRING_mod_NUM, a.op, b.op );
}

Ex atan2( const Ex &a, const Ex &b ) {
    if ( are_numbers( a.op, b.op ) ) return atan2_96( a.op->number_data()->val, b.op->number_data()->val );
    return Op::new_function( STRING_atan2_NUM, a.op, b.op );
}

Ex abs( const Ex &a ) {
    if ( is_a_number( a.op ) ) return abs( a.op->number_data()->val );
    if ( a.op->necessary_positive_or_null() ) return a;
    Op *res = Op::new_function( STRING_abs_NUM, a.op );
    return res;
}
Ex log( const Ex &a ) {
    if ( is_a_number( a.op ) ) return log_96( a.op->number_data()->val );
    if ( a.op->type == STRING_exp_NUM ) return a.op->func_data()->children[0];
    return Op::new_function( STRING_log_NUM, a.op );
}
Ex heaviside( const Ex &a ) {
    if ( is_a_number( a.op ) ) return heaviside( a.op->number_data()->val );
    if ( a.op->necessary_positive_or_null() ) return 1;
    if ( a.op->necessary_negative()         ) return 0;
    Op *res = Op::new_function( STRING_heaviside_NUM, a.op );
    if ( not res->cpt_use ) {
        res->set_beg_value( 0, true );
        res->set_end_value( 1, true );
    }
    return res;
}
Ex pos_part( const Ex &a ) {
    if ( is_a_number( a.op ) ) return pos_part( a.op->number_data()->val );
    if ( a.op->necessary_positive_or_null() ) return a;
    if ( a.op->necessary_negative()         ) return 0;
    Op *res = Op::new_function( STRING_pos_part_NUM, a.op );
    if ( not res->cpt_use ) {
        if ( a.op->end_value_valid )
            res->set_end_value( a.op->end_value, a.op->end_value_inclusive );
        res->set_beg_value( 0, true );
    }
    return res;
}
Ex eqz( const Ex &a ) {
    if ( is_a_number( a.op ) ) return eqz( a.op->number_data()->val );
    if ( a.op->necessary_positive() ) return 0;
    if ( a.op->necessary_negative() ) return 0;
    if ( a.op->type == STRING_mul_NUM ) {
        if ( a.op->func_data()->children[0]->type == Op::NUMBER )
            return eqz( Ex( a.op->func_data()->children[1] ) );
    }
    if ( a.op->type == STRING_pow_NUM ) { // eqz( m ^ e )
        Op *m = a.op->func_data()->children[0];
        Op *e = a.op->func_data()->children[1];
        if ( e->necessary_positive() ) // m ^ 5
            return eqz( Ex( m ) );
        if ( e->necessary_negative() ) // m ^ -5
            return Ex( 0 ); // -> can't be == 0
    }
    // no simplifications
    Op *res = Op::new_function( STRING_eqz_NUM, a.op );
    if ( not res->cpt_use ) {
        res->set_beg_value( 0, true );
        res->set_end_value( 1, true );
    }
    return res;
}
Ex exp( const Ex &a ) {
    if ( is_a_number( a.op ) ) return exp_96( a.op->number_data()->val );
    if ( a.op->type == STRING_log_NUM ) return a.op->func_data()->children[0];
    Op *res = Op::new_function( STRING_exp_NUM, a.op );
    if ( not res->cpt_use )
        res->set_beg_value( 0, true );
    return res;
}
Ex sin( const Ex &a ) {
    if ( is_a_number( a.op ) ) return sin_96( a.op->number_data()->val );
    return Op::new_function( STRING_sin_NUM, a.op );
}
Ex cos( const Ex &a ) {
    if ( is_a_number( a.op ) ) return cos_96( a.op->number_data()->val );
    return Op::new_function( STRING_cos_NUM, a.op );
}
Ex tan( const Ex &a ) {
    if ( is_a_number( a.op ) ) return tan_96( a.op->number_data()->val );
    return Op::new_function( STRING_tan_NUM, a.op );
}
Ex asin( const Ex &a ) {
    if ( is_a_number( a.op ) ) return asin_96( a.op->number_data()->val );
    return Op::new_function( STRING_asin_NUM, a.op );
}
Ex acos( const Ex &a ) {
    if ( is_a_number( a.op ) ) return acos_96( a.op->number_data()->val );
    return Op::new_function( STRING_acos_NUM, a.op );
}
Ex atan( const Ex &a ) {
    if ( is_a_number( a.op ) ) return atan_96( a.op->number_data()->val );
    return Op::new_function( STRING_atan_NUM, a.op );
}
Ex sinh( const Ex &a ) {
    if ( is_a_number( a.op ) ) return sinh_96( a.op->number_data()->val );
    return Op::new_function( STRING_sinh_NUM, a.op );
}
Ex cosh( const Ex &a ) {
    if ( is_a_number( a.op ) ) return cosh_96( a.op->number_data()->val );
    return Op::new_function( STRING_cosh_NUM, a.op );
}
Ex tanh( const Ex &a ) {
    if ( is_a_number( a.op ) ) return tanh_96( a.op->number_data()->val );
    return Op::new_function( STRING_tanh_NUM, a.op );
}
Ex min( const Ex &a, const Ex &b ) {
    return a - pos_part( a - b );
    //     Ex s = heaviside( b - a );
    //     return s * a + ( 1 - s ) * b;
}
Ex max( const Ex &a, const Ex &b ) {
    return a + pos_part( b - a );
    //     Ex s = heaviside( a - b );
    //     return s * a + ( 1 - s ) * b;
}
Ex min( const Ex &a, const Ex &b, const Ex &c ) {
    return min( min( a, b ), c );
}
Ex max( const Ex &a, const Ex &b, const Ex &c ) {
    return max( max( a, b ), c );
}

void sort( Ex &a, Ex &b, Ex &c ) {
    int perm[] = {
        0, 1, 2,
        0, 2, 1,
        1, 0, 2,
        1, 2, 0,
        2, 0, 1,
        2, 1, 0,
    };
    Ex val[] = { a, b, c };
    Ex cond( 0 );
    Ex ra, rb, rc;
    for(unsigned num_perm=0;num_perm<6;++num_perm) {
        cond = ( 1 - cond ) * heaviside( val[ perm[num_perm*3+2] ] - val[ perm[num_perm*3+1] ] ) * heaviside( val[ perm[num_perm*3+1] ] - val[ perm[num_perm*3+0] ] );
        ra += cond * val[ perm[num_perm*3+0] ];
        rb += cond * val[ perm[num_perm*3+1] ];
        rc += cond * val[ perm[num_perm*3+2] ];
    }
    //
    a = ra; b = rb; c = rc;
}


// ------------------------------------------------------------------------------------------------------------
void destroy_rec( Op *a ) {
    if ( a->op_id != Op::current_op ) { // if not done ?
        a->op_id = Op::current_op;
        dec_ref( a->additional_info );
        if ( a->type > 0 ) {
            if ( a->func_data()->children[0] )
                destroy_rec( a->func_data()->children[0] );
            if ( a->func_data()->children[1] )
                destroy_rec( a->func_data()->children[1] );
        }
    }
}

// ------------------------------------------------------------------------------------------------------------
struct DiffRec {
    DiffRec( Thread *th, const void *tok, const Ex &expr, const Ex &d ) : th(th), tok(tok), zero(0), one(1), expr( expr ), d( d ) {
        d.op->additional_info = one.op->inc_ref();
        d.op->op_id = ++Op::current_op;
        //
        diff_rec( expr.op );
    }
    
    ~DiffRec() {
        //         ++Op::current_op;
        //         destroy_rec( expr.op );
        //         if ( d.op->op_id != Op::current_op )
        //             dec_ref( d.op->additional_info );
    }
    
    
    void diff_rec( Op *a ) {
        if ( a->op_id == Op::current_op ) // already done ?
            return;
        a->op_id = Op::current_op;
        //
        #define MAKE_D0( res ) \
            { \
                Op *c0 = a->func_data()->children[0]; diff_rec( c0 ); \
                Op *d0 = c0->additional_info; \
                a->additional_info = to_inc_op( res ); \
            }
        #define MAKE_D0D1( res ) \
            { \
                Op *c0 = a->func_data()->children[0]; Op *c1 = a->func_data()->children[1]; diff_rec( c0 ); diff_rec( c1 ); \
                Op *d0 = c0->additional_info; Op *d1 = c1->additional_info; \
                a->additional_info = to_inc_op( res ); \
            }
        switch ( a->type ) {
            case Op::NUMBER:           a->additional_info = zero.op->inc_ref(); break;
            case Op::SYMBOL:           a->additional_info = zero.op->inc_ref(); break;
            case STRING_heaviside_NUM: a->additional_info = zero.op->inc_ref(); break;
            case STRING_eqz_NUM:       a->additional_info = zero.op->inc_ref(); break;
            case STRING_add_NUM:       MAKE_D0D1( Ex( d0 ) + Ex( d1 ) ); break;
            case STRING_mul_NUM:       MAKE_D0D1( Ex( d0 ) * Ex( c1 ) + Ex( c0 ) * Ex( d1 ) ); break;
            case STRING_log_NUM:       MAKE_D0( Ex( d0 ) / c0 ); break; // { GET_D0; Op &n = o / c0; Op &r = da * n; of.push_back(&n); of.push_back(&r); a.additional_info = &r; return r; }
            case STRING_abs_NUM:       MAKE_D0( ( 2 * heaviside( Ex( c0 ) ) - 1 ) * Ex( d0 ) ); break;
            case STRING_pos_part_NUM:  MAKE_D0( heaviside( Ex( c0 ) ) * Ex( d0 ) ); break;
            case STRING_exp_NUM:       MAKE_D0( Ex( d0 ) * Ex( a ) ); break;
            
            case STRING_sin_NUM:       MAKE_D0( Ex( d0 ) * cos( Ex( c0 ) ) ); break;
            case STRING_cos_NUM:       MAKE_D0( - Ex( d0 ) * sin( Ex( c0 ) ) ); break;
            case STRING_tan_NUM:       MAKE_D0( Ex( d0 ) * ( 1 + pow( Ex( a ), 2 ) ) ); break;
            case STRING_asin_NUM:      MAKE_D0( Ex( d0 ) / pow( 1 - pow( Ex( c0 ), 2 ), Rationnal(1,2) ) ); break;
            case STRING_acos_NUM:      MAKE_D0( - Ex( d0 ) / pow( 1 - pow( Ex( c0 ), 2 ), Rationnal(1,2) ) ); break;
            case STRING_atan_NUM:      MAKE_D0( Ex( d0 ) / ( 1 + pow( Ex( c0 ), 2 ) ) ); break;
            case STRING_pow_NUM:
                if ( a->func_data()->children[1]->type == Op::NUMBER ) {
                    Op *c0 = a->func_data()->children[0]; Op *c1 = a->func_data()->children[1]; diff_rec( c0 ); diff_rec( c1 );
                    Op *d0 = c0->additional_info;
                    a->additional_info = to_inc_op(
                        Ex( c1 ) * Ex( d0 ) * pow( Ex( c0 ), Ex( a->func_data()->children[1]->number_data()->val - Rationnal( 1 ) ) )
                    );
                    break;
                }
                MAKE_D0D1( Ex( d0 ) * Ex( c1 ) * pow( Ex( c0 ), Ex( c1 ) - Rationnal( 1 ) ) + /* dx * x ^ ( y - 1 ) */ Ex( d1 ) * log( Ex( c0 ) ) * Ex( a ) /*dy * log( x ) * x ^ y*/ ); 
                break;
            default:
                th->add_error( "for now, no rules to differentiate function of type '"+std::string(Nstring(a->type))+"'.", tok );
        }
    }
    
    Thread *th;
    const void *tok;
    Ex zero, one, expr, d;
};

Ex Ex::diff( Thread *th, const void *tok, const Ex &a ) const {
    DiffRec dr( th, tok, *this, a );
    return op->additional_info;
}

// ------------------------------------------------------------------------------------------------------------
struct SubsRec {
    SubsRec( Thread *th, const void *tok, const Ex &expr ) : th(th), tok(tok), expr( expr ) {
        subs_rec( expr.op );
    }
    
    //     ~SubsRec() {
    //         ++Op::current_op;
    //         destroy_rec( expr.op );
    //         if ( d.op->op_id != Op::current_op )
    //             dec_ref( d.op->additional_info );
    //     }
    
    
    void subs_rec( Op *a ) {
        if ( a->op_id == Op::current_op ) // already done ?
            return;
        a->op_id = Op::current_op;
        //
        #define MAKE_S0( res ) \
            { \
                Op *c0 = a->func_data()->children[0]; subs_rec( c0 ); \
                Ex e0 = c0->additional_info; \
                a->additional_info = to_inc_op( res ); \
            }
        #define MAKE_S0S1( res ) \
            { \
                Op *c0 = a->func_data()->children[0]; subs_rec( c0 ); \
                Op *c1 = a->func_data()->children[1]; subs_rec( c1 ); \
                Ex e0 = c0->additional_info; \
                Ex e1 = c1->additional_info; \
                a->additional_info = to_inc_op( res ); \
            }

        switch ( a->type ) {
            case Op::NUMBER:           a->additional_info = a->inc_ref(); break;
            case Op::SYMBOL:           a->additional_info = a->inc_ref(); break;
            case STRING_add_NUM:       MAKE_S0S1( e0 + e1 ); break;
            case STRING_mul_NUM:       MAKE_S0S1( e0 * e1 ); break;
            case STRING_pow_NUM:       MAKE_S0S1( pow( e0, e1 ) ); break;
            case STRING_atan2_NUM:     MAKE_S0S1( atan2( e0, e1 ) ); break;
            
            case STRING_heaviside_NUM: MAKE_S0( heaviside( e0 ) ); break;
            case STRING_pos_part_NUM:  MAKE_S0( pos_part ( e0 ) ); break;
            case STRING_eqz_NUM:       MAKE_S0( eqz      ( e0 ) ); break;
            case STRING_log_NUM:       MAKE_S0( log      ( e0 ) ); break;
            case STRING_abs_NUM:       MAKE_S0( abs      ( e0 ) ); break;
            case STRING_exp_NUM:       MAKE_S0( exp      ( e0 ) ); break;
            
            case STRING_sin_NUM:       MAKE_S0( sin      ( e0 ) ); break;
            case STRING_cos_NUM:       MAKE_S0( cos      ( e0 ) ); break;
            case STRING_tan_NUM:       MAKE_S0( tan      ( e0 ) ); break;
            case STRING_asin_NUM:      MAKE_S0( asin     ( e0 ) ); break;
            case STRING_acos_NUM:      MAKE_S0( acos     ( e0 ) ); break;
            case STRING_atan_NUM:      MAKE_S0( atan     ( e0 ) ); break;
            
            default:
                th->add_error( "for now, no rules to subs function of type '"+std::string(Nstring(a->type))+"'.", tok );
        }
    }
    
    Thread *th;
    const void *tok;
    Ex expr;
};

Ex Ex::subs( Thread *th, const void *tok, const VarArgs &a, const VarArgs &b ) const {
    ++Op::current_op;
    for(unsigned i=0;i<a.variables.size();++i) {
        reinterpret_cast<Ex *>(a.variables[i].data)->op->op_id = Op::current_op;
        reinterpret_cast<Ex *>(a.variables[i].data)->op->additional_info = reinterpret_cast<Ex *>(b.variables[i].data)->op->inc_ref();
    }
    SubsRec sr( th, tok, *this );
    return op->additional_info;
}

Ex Ex::subs( Thread *th, const void *tok, const Ex &a, const Ex &b ) const {
    ++Op::current_op;
    a.op->op_id = Op::current_op;
    a.op->additional_info = b.op->inc_ref();
    SubsRec sr( th, tok, *this );
    return op->additional_info;
}

// ------------------------------------------------------------------------------------------------------------
Rationnal Ex::subs_numerical( Thread *th, const void *tok, const Rationnal &a ) const {
    SplittedVec<Op *,32> symbols;
    get_sub_symbols( op, symbols );
    if ( symbols.size() > 1 ) {
        std::ostringstream ss;
        ss << "subs_numerical works only with expressions which contains at most 1 variable ( remaining ones are ";
        for(unsigned i=0;i<symbols.size();++i)
            ss << Ex( symbols[i] ) << " ";
        ss << ")";
        th->add_error(ss.str(),tok);
        return 0;
    }
    //
    if ( symbols.size() == 0 )
        return value();
        
    // TODO : Optimize
    Ex res = subs( th, tok, Ex( symbols[0] ), Ex( a ) );
    if ( symbols.size() > 1 ) {
        th->add_error("Weird : substitution should have given a Number.",tok);
        return 0;
    }
    return res.value();
}


// ------------------------------------------------------------------------------------------------------------
void get_taylor_expansion( Thread *th, const void *tok, Ex expr, const Ex &beg, const Ex &var, Int32 deg_poly_max, SplittedVec<Ex,8,8,true> &res ) {
    Rationnal r( 1 );
    for(Int32 i=0;i<=deg_poly_max;++i) {
        res.push_back( r * expr.subs( th, tok, var, beg ) );
        if ( i < deg_poly_max ) {
            expr = expr.diff( th, tok, var );
            r /= i + 1;
        }
    }
}

Ex integration_with_taylor_expansion( Thread *th, const void *tok, const Ex &expr, const Ex &var, const Ex &beg, const Ex &end, Int32 deg_poly_max ) {
    //
    SplittedVec<Ex,8,8,true> taylor_expansion;
    get_taylor_expansion( th, tok, expr, ( beg + end ) / 2, var, deg_poly_max, taylor_expansion );
    //
    Ex res( 0 ), d = ( end - beg ) / 2;
    for( Int32 i=0; i<(Int32)taylor_expansion.size(); i += 2 )
        res = res + 2 * taylor_expansion[i] * pow( d, Ex( Rationnal( i + 1 ) ) ) * Rationnal( 1, i + 1 );
    return res;
}

Ex integration_with_discontinuities_rec( Thread *th, const void *tok, const Ex &expr, const Ex &var, const Ex &beg, const Ex &end, Int32 deg_poly_max ) {
    const Op *disc = expr.op->find_discontinuity( var.op );
    if ( disc ) {
        Ex ex_disc( disc );
        // substitutions -> pos part and neg part
        const Op *ch = disc->func_data()->children[0];
        Ex ex_ch( ch );
        Ex subs_p, subs_n;
        if ( disc->type == STRING_heaviside_NUM ) {
            subs_p = expr.subs( th, tok, ex_disc, Ex( 1 ) );
            subs_n = expr.subs( th, tok, ex_disc, Ex( 0 ) );
        }
        else if ( disc->type == STRING_abs_NUM ) {
            subs_p = expr.subs( th, tok, ex_disc,   ex_ch );
            subs_n = expr.subs( th, tok, ex_disc, - ex_ch );
        }
        else if ( disc->type == STRING_pos_part_NUM ) {
            subs_p = expr.subs( th, tok, ex_disc,   ex_ch );
            subs_n = expr.subs( th, tok, ex_disc, Ex( 0 ) );
        }
        else
            assert( 0 ); //
            
        // intervals
        SplittedVec<Ex,8,8,true> taylor_expansion;
        
        get_taylor_expansion( th, tok, ex_ch, beg, var, 3, taylor_expansion );
        Ex res( 0 );
        
        // order 1 or 0
        Ex o1 = eqz( taylor_expansion[2] ) * eqz( taylor_expansion[3] );
        if ( o1.known_at_compile_time()==false or o1.value().is_zero()==false ) {
            //
            Ex p_beg = heaviside( ex_ch.subs( th, tok, var, beg ) );
            Ex p_end = heaviside( ex_ch.subs( th, tok, var, end ) );
            Ex n_beg = 1 - p_beg;
            Ex n_end = 1 - p_end;
            //
            Ex cut = beg - taylor_expansion[0] / ( taylor_expansion[1] + eqz( taylor_expansion[1] ) );
            //
            Ex nb = beg + ( cut - beg ) * p_beg * n_end;
            Ex ne = end + ( beg - end + ( cut - beg ) * n_beg ) * p_end;
            Ex pb = beg + ( end - beg + ( cut - end ) * p_end ) * n_beg;
            Ex pe = end + ( cut - end ) * p_beg * n_end;
            
            Ex int_n = integration( th, tok, subs_n, var, nb, ne, deg_poly_max );
            Ex int_p = integration( th, tok, subs_p, var, pb, pe, deg_poly_max );
            res += o1 * ( int_p + int_n );
        }
        // order 2
        Ex o2 = ( 1 - eqz( taylor_expansion[2] ) ) * eqz( taylor_expansion[3] );
        if ( o2.known_at_compile_time()==false or o2.value().is_zero()==false ) {
            std::cout << "yaap" << std::endl;
            Ex p2 = heaviside( taylor_expansion[2] );
            Ex sg = p2 * 2 - 1;
            //
            Ex a = taylor_expansion[2], b = taylor_expansion[1], c = taylor_expansion[0];
            Ex delta = pow( b, 2 ) - 4 * a * c;
            Ex sq_delta = pow( delta * heaviside( delta ), Rationnal( 1, 2 ) );
            Ex x0 = beg - ( b + sg * sq_delta ) / ( 2 * a + eqz( a ) ); // first root
            Ex x1 = beg - ( b - sg * sq_delta ) / ( 2 * a + eqz( a ) ); // x1 >= x0
            res += o2 * (
                integration( th, tok, p2 * subs_p + ( 1 - p2 ) * subs_n, var, beg        , min(end,x0), deg_poly_max ) * heaviside( x0 - beg )                         +
                integration( th, tok, p2 * subs_n + ( 1 - p2 ) * subs_p, var, max(beg,x0), min(end,x1), deg_poly_max ) * heaviside( x1 - beg ) * heaviside( end - x0 ) +
                integration( th, tok, p2 * subs_p + ( 1 - p2 ) * subs_n, var, max(beg,x1), end        , deg_poly_max ) * heaviside( end - x1 )
            );
        }
        // order 3
        Ex o3 = ( 1 - eqz( taylor_expansion[3] ) );
        if ( o3.known_at_compile_time()==false or o3.value().is_zero()==false ) {
            std::cout << "yep" << std::endl;
            Ex z = taylor_expansion[3] + eqz( taylor_expansion[3] );
            Ex a = taylor_expansion[2] / z, b = taylor_expansion[1] / z, c = taylor_expansion[0] / z;
            Ex p3 = heaviside( taylor_expansion[3] );
            Ex p = b - pow( a, 2 ) * Rationnal( 1, 3 );
            Ex q = pow( a, 3 ) * Rationnal( 2, 27 ) - a * b * Rationnal( 1, 3 ) + c;
            Ex delta = 4 * pow( p, 3 ) + 27 * pow(q,2);
            Ex delta_pos = heaviside( delta );
            Ex sq_27_delta = pow( 27 * abs( delta ), Rationnal( 1, 2 ) );
            // delta >= 0
            if ( delta_pos.known_at_compile_time()==false or delta_pos.value().is_zero()==false ) {
                Ex u = Rationnal( 1, 2 ) * ( -27 * q + sq_27_delta );
                Ex v = Rationnal( 1, 2 ) * ( -27 * q - sq_27_delta );
                Ex sgn_u = 2 * heaviside( u ) - 1;
                Ex sgn_v = 2 * heaviside( v ) - 1;
                Ex r = Rationnal( 1, 3 ) * ( sgn_u * pow( abs(u), Rationnal( 1, 3 ) ) + sgn_v * pow( abs(v), Rationnal( 1, 3 ) ) - a );
                res += o3 * delta_pos * (
                    integration( th, tok, p3 * subs_n + ( 1 - p3 ) * subs_p, var, beg, min(end,r), deg_poly_max ) * heaviside( r - beg ) +
                    integration( th, tok, p3 * subs_p + ( 1 - p3 ) * subs_n, var, max(beg,r), end, deg_poly_max ) * heaviside( end - r )
                );
            }
            // delta < 0
            Ex delta_neg = 1 - delta_pos;
            if ( delta_neg.known_at_compile_time()==false or delta_neg.value().is_zero()==false ) {
                std::complex<Ex> j( Rationnal( -1, 2 ), Rationnal( sqrt( (long double)3 ) / 2 ) );
                std::complex<Ex> v( Rationnal( -27, 2 ) * q, Rationnal( 1, 2 ) * sq_27_delta );
                Ex a_mod = v.real() * v.real() + v.imag() * v.imag();
                Ex a_arg = atan2( v.imag(), v.real() );
                std::complex<Ex> u = exp( Ex( Rationnal( 1, 3 ) ) * std::complex<Ex>( Rationnal( 1, 2 ) * log( a_mod + eqz(a_mod) ), a_arg ) ) * ( 1 - eqz( a_mod ) );
                Ex x0 = Rationnal( 1, 3 ) * ( 2 * std::real(    u) - a );
                Ex x1 = Rationnal( 1, 3 ) * ( 2 * std::real(  j*u) - a );
                Ex x2 = Rationnal( 1, 3 ) * ( 2 * std::real(j*j*u) - a );
                sort( x0, x1, x2 );
                //
                res += o3 * delta_neg * (
                    integration( th, tok, p3 * subs_n + ( 1 - p3 ) * subs_p, var, beg        , min(end,x0), deg_poly_max ) * heaviside( x0 - beg )                         +
                    integration( th, tok, p3 * subs_p + ( 1 - p3 ) * subs_n, var, max(beg,x0), min(end,x1), deg_poly_max ) * heaviside( x1 - beg ) * heaviside( end - x0 ) +
                    integration( th, tok, p3 * subs_n + ( 1 - p3 ) * subs_p, var, max(beg,x1), min(end,x2), deg_poly_max ) * heaviside( x2 - beg ) * heaviside( end - x1 ) +
                    integration( th, tok, p3 * subs_p + ( 1 - p3 ) * subs_n, var, max(beg,x2), end        , deg_poly_max ) * heaviside( end - x2 )
                );
                // std::cout << double(r0.value()) << " " << double(r1.value()) << " " << double(r2.value()) << std::endl;
            }
        }
        
        //
        return res;
    }
    return integration_with_taylor_expansion( th, tok, expr, var, beg, end, deg_poly_max );
}

// encoding of <, >, <, ...
struct IneqSet {
    IneqSet() : impossible( false ) {}
    IneqSet( const IneqSet &is, const Ex &ex, bool bo ) {
        impossible = is.impossible;
        ineq = is.ineq;
        for(unsigned i=0;i<ineq.size();++i) {
            if ( same_ex( ineq[i].first, ex ) ) {
                impossible |= ( bo != ineq[i].second );
                return;
            }
            else if ( same_ex( ineq[i].first, - ex ) ) {
                impossible |= ( bo == ineq[i].second );
                return;
            }
        }
        ineq.push_back( std::pair<Ex,bool>( ex, bo ) );
    }
    std::vector<std::pair<Ex,bool> > ineq;
    bool impossible;
};
std::ostream &operator<<(std::ostream &os,const IneqSet &is) {
    if ( is.impossible )
        os << "impossible";
    else
        for(unsigned i=0;i<is.ineq.size();++i)
            os << is.ineq[i].first << ( is.ineq[i].second ? ">=0 " : "<0 " );
    return os;
}
void get_mutimap_of_ineq_set_rec( Thread *th, const void *tok, const Ex &expr, const Ex &var, std::map<Ex,std::vector<IneqSet>,ExPtrCmp> &mutimap_of_ineq_set, const IneqSet &is ) {
    const Op *disc = expr.op->find_discontinuity( var.op );
    if ( disc ) {
        Ex ex_disc( disc );
        Ex ch( disc->func_data()->children[0] );
        //
        Ex subs_p, subs_n;
        if ( disc->type == STRING_heaviside_NUM ) {
            subs_p = expr.subs( th, tok, ex_disc, Ex( 1 ) );
            subs_n = expr.subs( th, tok, ex_disc, Ex( 0 ) );
        }
        else if ( disc->type == STRING_abs_NUM ) {
            subs_p = expr.subs( th, tok, ex_disc,   ch );
            subs_n = expr.subs( th, tok, ex_disc, - ch );
        }
        else
            assert( 0 ); //
        //
        get_mutimap_of_ineq_set_rec( th, tok, subs_n, var, mutimap_of_ineq_set, IneqSet( is, ch, false ) );
        get_mutimap_of_ineq_set_rec( th, tok, subs_p, var, mutimap_of_ineq_set, IneqSet( is, ch, true  ) );
    }
    else if ( not expr.op->is_zero() )
        mutimap_of_ineq_set[ expr ].push_back( is );
}

Ex integration_with_discontinuities( Thread *th, const void *tok, Ex expr, Ex var, const Ex &beg, const Ex &end, Int32 deg_poly_max ) {
    std::map<Ex,std::vector<IneqSet>,ExPtrCmp> mutimap_of_ineq_set;
    get_mutimap_of_ineq_set_rec( th, tok, expr, var, mutimap_of_ineq_set, IneqSet() );

    for( std::map<Ex,std::vector<IneqSet>,ExPtrCmp>::const_iterator iter = mutimap_of_ineq_set.begin(); iter != mutimap_of_ineq_set.end(); ++iter ) {
        std::cout << iter->first;
        for(unsigned i=0;i<iter->second.size();++i)
            std::cout << " ; " << iter->second[i];
        std::cout << std::endl;
    }

    return 0; // integration_with_taylor_expansion( th, tok, expr, var, beg, end, deg_poly_max );
}

Ex integration( Thread *th, const void *tok, Ex expr, Ex var, const Ex &beg, const Ex &end, Int32 deg_poly_max ) {
    if ( same_op( beg.op, end.op ) )
        return Ex( 0 );
    //
    if ( beg.known_at_compile_time() or end.known_at_compile_time() ) {
        Ex old_var = var;
        var = Ex( "tmp", 3, "tmp", 3 );
        if ( beg.known_at_compile_time() )
            var.set_beg_value( beg.value(), true );
        if ( end.known_at_compile_time() )
            var.set_end_value( end.value(), true );
        expr = expr.subs( th, tok, old_var, var );
    }
    
    //     return integration_with_discontinuities( th, tok, expr, var, beg, end, deg_poly_max );
    return integration_with_discontinuities_rec( th, tok, expr, var, beg, end, deg_poly_max );
}


