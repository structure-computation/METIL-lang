#ifndef EX_H
#define EX_H

#include "config.h"
#include "varargs.h"
#include "splittedvec.h"

struct Op;
struct Thread;

struct Ex {
    typedef Rationnal T;
    
    // --------------------------
    Ex() { init(); }
    Ex( Op *ex ) { init( ex ); }
    Ex( const Op * ex ) { init( const_cast<Op *>( ex ) ); }
    Ex( const Ex &ex ) { init( ex ); }
    Ex( const T &v ) { init( v ); }
    Ex( double   v ) { init( T( v ) ); }
    Ex( unsigned v ) { init( T( v ) ); }
    Ex( int      v ) { init( T( v ) ); }
    Ex( const char *cpp_name_str, unsigned cpp_name_si, const char *tex_name_str, unsigned tex_name_si ) { init( cpp_name_str, cpp_name_si, tex_name_str, tex_name_si ); }
    
    void operator=(const Ex &c) { reassign( c ); }
    
    Ex &operator+=(const Ex &c);
    Ex &operator*=(const Ex &c);
    
    ~Ex() { destroy(); }
    
    // --------------------------
    void init();
    void init( Op *ex );
    void init( const Ex &ex );
    void init( const T &v );
    void init( int v );
    void init( const char *cpp_name_str, unsigned cpp_name_si, const char *tex_name_str, unsigned tex_name_si );
    
    void reassign( const Ex &c );
    
    void destroy();
    
    // --------------------------
    bool known_at_compile_time() const;
    std::string graphviz_repr() const;
    std::string cpp_repr() const;
    std::string tex_repr() const;
    T value() const;
    
    unsigned node_count() const;
    unsigned nb_sub_symbols() const;

    
    Ex diff( Thread *th, const void *tok, const Ex &a ) const;
    Ex subs( Thread *th, const void *tok, const VarArgs &a, const VarArgs &b ) const;
    Ex subs( Thread *th, const void *tok, const Ex &a, const Ex &b ) const;
    bool depends_on( const Ex &a ) const;
    Rationnal subs_numerical( Thread *th, const void *tok, const Rationnal &a ) const;
    
    Ex linearize_discontinuity_children( Thread *th, const void *tok, const VarArgs &a, const VarArgs &b ) const;
    
    Ex expand( Thread *th, const void *tok ) const;
    
    void set_beg_value( T b, bool inclusive );
    void set_end_value( T e, bool inclusive );
    
    bool beg_value_valid() const;
    bool end_value_valid() const;
    bool beg_value_inclusive() const;
    bool end_value_inclusive() const;
    Rationnal beg_value() const;
    Rationnal end_value() const;
    
    // --------------------------
    Op *op;
};

typedef SplittedVec<Ex,8,8,true> SEX;

std::ostream &operator<<( std::ostream &os, const Ex &ex );

Ex integration( Thread *th, const void *tok, Ex expr, Ex var, const Ex &beg, const Ex &end, Int32 deg_poly_max );

Ex make_poly_fit( Thread *th, const void *tok, const Ex &expr, const Ex &var, const Ex &beg, const Ex &end, Int32 deg_poly );

Ex a_posteriori_simplification( const Ex &a );
Ex add_a_posteriori_simplification( const Ex &a );

void polynomial_expansion( Thread *th, const void *tok, const SEX &expressions, const Ex &var, int order, SEX &res );
void polynomial_expansion( Thread *th, const void *tok, const VarArgs &expressions, const Ex &var, int order, VarArgs &res );

void quadratic_expansion( Thread *th, const void *tok, const SEX &expressions, const SEX &variables, SEX &res );
void quadratic_expansion( Thread *th, const void *tok, const VarArgs &expressions, const VarArgs &vars, VarArgs &res );

// ------------------------------------------------------------------------------------------------------------------

Ex operator+( const Ex &a, const Ex &b );
Ex operator-( const Ex &a, const Ex &b );
Ex operator-( const Ex &a        );
Ex operator*( const Ex &a, const Ex &b );
Ex operator/( const Ex &a, const Ex &b );

Ex mod  ( const Ex &a, const Ex &b );
Ex pow  ( const Ex &a, const Ex &b );
Ex atan2( const Ex &a, const Ex &b );
Ex min  ( const Ex &a, const Ex &b );
Ex max  ( const Ex &a, const Ex &b );

Ex abs      ( const Ex &a );
Ex log      ( const Ex &a );
Ex heaviside( const Ex &a );
Ex pos_part ( const Ex &a );
Ex eqz      ( const Ex &a );
Ex exp      ( const Ex &a );
Ex sin      ( Ex a );
Ex cos      ( const Ex &a );
Ex tan      ( const Ex &a );
Ex asin     ( const Ex &a );
Ex acos     ( const Ex &a );
Ex atan     ( const Ex &a );
Ex sinh     ( const Ex &a );
Ex cosh     ( const Ex &a );
Ex tanh     ( const Ex &a );

inline Ex sqrt( const Ex &a ) { return pow( a, Rationnal(1,2) ); }
inline Ex rsqrt( const Ex &a ) { return pow( a, Rationnal(-1,2) ); }

#define PRINT( A ) \
    std::cout << "  " << __STRING(A) << std::flush << " -> " << (A) << std::endl

#endif // EX_H
