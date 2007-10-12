#include "ex.h"
#include "op.h"
#include "globaldata.h"
#include "thread.h"
#include <sstream>


void Ex::init() {
    op = Op::new_number( 0 );
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
    return false;
}

Rationnal Ex::subs_numerical( Thread *th, const void *tok, const Rationnal &a ) const {
    return 0;
}

Op *to_inc_op( const Ex &ex ) {
    return ex.op->inc_ref();
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

Ex add_number_and_expr( const Ex &a, const Ex &b ) { // a is a number
    if ( a.op->number_data()->val.is_zero() )
        return b;
    // 10 + ( 5 + a )
    if ( b.op->type == STRING_add_NUM and b.op->func_data()->children[0]->type == Op::NUMBER ) {
        return Ex( a.op->number_data()->val + b.op->func_data()->children[0]->number_data()->val ) + 
               Ex( b.op->func_data()->children[1] );
    }
    return Op::new_function( STRING_add_NUM, a.op, b.op );
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
    if ( a.op < b.op )
        return Op::new_function( STRING_add_NUM, a.op, b.op );
    return Op::new_function( STRING_add_NUM, b.op, a.op );
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
    return Op::new_function( STRING_mul_NUM, a.op, b.op );
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
    if ( a.op < b.op )
        return Op::new_function( STRING_mul_NUM, a.op, b.op );
    return Op::new_function( STRING_mul_NUM, b.op, a.op );
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
    return Op::new_function( STRING_pow_NUM, a.op, b.op );
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
    return Op::new_function( STRING_abs_NUM, a.op );
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
    return Op::new_function( STRING_heaviside_NUM, a.op );
}
Ex eqz( const Ex &a ) {
    if ( is_a_number( a.op ) ) return eqz( a.op->number_data()->val );
    if ( a.op->necessary_positive() ) return 0;
    if ( a.op->necessary_negative() ) return 0;
    return Op::new_function( STRING_eqz_NUM, a.op );
}
Ex exp( const Ex &a ) {
    if ( is_a_number( a.op ) ) return exp_96( a.op->number_data()->val );
    if ( a.op->type == STRING_log_NUM ) return a.op->func_data()->children[0];
    return Op::new_function( STRING_exp_NUM, a.op );
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
        ++Op::current_op;
        destroy_rec( expr.op );
        if ( d.op->op_id != Op::current_op )
            dec_ref( d.op->additional_info );
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
            case STRING_exp_NUM:       MAKE_D0( Ex( d0 ) * Ex( c0 ) ); break;
            
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
                MAKE_D0D1(
                    Ex( d0 ) * Ex( c1 ) * pow( Ex( c0 ), Ex( c1 ) - Rationnal( 1 ) ) + // dx * x ^ ( y - 1 )
                    Ex( d1 ) * log( Ex( c0 ) ) * Ex( a ) // dy * log( x ) * x ^ y
                );
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
                Op *s0 = c0->additional_info; \
                a->additional_info = to_inc_op( res ); \
            }
        #define MAKE_S0S1( res ) \
            { \
                Op *c0 = a->func_data()->children[0]; subs_rec( c0 ); \
                Op *c1 = a->func_data()->children[1]; subs_rec( c1 ); \
                Op *s0 = c0->additional_info; \
                Op *s1 = c1->additional_info; \
                a->additional_info = to_inc_op( res ); \
            }

        switch ( a->type ) {
            case Op::NUMBER:           a->additional_info = a->inc_ref(); break;
            case Op::SYMBOL:           a->additional_info = a->inc_ref(); break;
            case STRING_add_NUM:       MAKE_S0S1( Ex( s0 ) + Ex( s1 ) ); break;
            case STRING_mul_NUM:       MAKE_S0S1( Ex( s0 ) * Ex( s1 ) ); break;
            case STRING_pow_NUM:       MAKE_S0S1( pow( Ex( s0 ), Ex( s1 ) ) ); break;
            case STRING_atan2_NUM:     MAKE_S0S1( atan2( Ex( s0 ), Ex( s1 ) ) ); break;
            
            case STRING_heaviside_NUM: MAKE_S0( heaviside( Ex( s0 ) ) ); break;
            case STRING_eqz_NUM:       MAKE_S0( eqz      ( Ex( s0 ) ) ); break;
            case STRING_log_NUM:       MAKE_S0( log      ( Ex( s0 ) ) ); break;
            case STRING_abs_NUM:       MAKE_S0( abs      ( Ex( s0 ) ) ); break;
            case STRING_exp_NUM:       MAKE_S0( exp      ( Ex( s0 ) ) ); break;
            
            case STRING_sin_NUM:       MAKE_S0( sin      ( Ex( s0 ) ) ); break;
            case STRING_cos_NUM:       MAKE_S0( cos      ( Ex( s0 ) ) ); break;
            case STRING_tan_NUM:       MAKE_S0( tan      ( Ex( s0 ) ) ); break;
            case STRING_asin_NUM:      MAKE_S0( asin     ( Ex( s0 ) ) ); break;
            case STRING_acos_NUM:      MAKE_S0( acos     ( Ex( s0 ) ) ); break;
            case STRING_atan_NUM:      MAKE_S0( atan     ( Ex( s0 ) ) ); break;
            
            default:
                th->add_error( "for now, no rules to subs function of type '"+std::string(Nstring(a->type))+"'.", tok );
        }
    }
    
    Thread *th;
    const void *tok;
    Ex zero, one, expr;
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
//     SplittedVec<const Op *> from;
//     SplittedVec<const Op *> to;
//     from.push_back( a.op );
//     to  .push_back( b.op );
//     return subs( th, tok, from, to );
    return 666;
}


// ------------------------------------------------------------------------------------------------------------
Ex integration( Thread *th, const void *tok, Ex &expr, Ex &var, Ex &beg, Ex &end, Int32 deg_poly_max ) {
    return expr;
}

