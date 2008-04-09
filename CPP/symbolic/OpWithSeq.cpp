#include "config.h"
#include "OpWithSeq.h"
#include "basic_string_manip.h"
#include "usual_strings.h"
#include "op.h"
#include "nstring.h"

unsigned OpWithSeq::current_id = 0;

OpWithSeq::OpWithSeq( const Rationnal &n ) : type( NUMBER ) {
    init_gen();
    num = n.num;
    den = n.den;
}

OpWithSeq::OpWithSeq( int t ) : type( t ) {
    init_gen();
}

OpWithSeq::OpWithSeq( const char *cpp_name ) : type( SYMBOL ) {
    init_gen();
    cpp_name_str = strdup( cpp_name );
}

OpWithSeq::OpWithSeq( int method, const char *name, OpWithSeq *ch ) { // WRITE_...
    init_gen();
    switch ( method ) {
        case STRING_add_NUM :       type = WRITE_ADD;      break;
        case STRING_init_NUM:       type = WRITE_INIT;     break;
        case STRING_reassign_NUM:   type = WRITE_REASSIGN; break;
        case STRING___return___NUM: type = WRITE_RET;      break;
        default: assert(0);
    }
    cpp_name_str = strdup( name );
    add_child( ch );
}

void OpWithSeq::init_gen() {
    reg = -1;
    ordering = -1;
    id = 0;
}

OpWithSeq::~OpWithSeq() {
    for(unsigned i=0;i<children.size();++i) {
        OpWithSeq *ch = children[i];
        ch->parents.erase( std::find( ch->parents.begin(), ch->parents.end(), this ) );
        if ( ch->parents.size() == 0 )
            delete ch;
    }
}

void OpWithSeq::add_child( OpWithSeq *c ) {
    children.push_back( c );
    c->parents.push_back( this );
}

OpWithSeq *make_OpWithSeq_rec( const Op *op ) {
    if ( op->op_id == Op::current_op )
        return reinterpret_cast<OpWithSeq *>( op->additional_info );
    op->op_id = Op::current_op;
    //
    if ( op->type == Op::SYMBOL ) {
        OpWithSeq *res = new OpWithSeq( op->symbol_data()->cpp_name_str );
        op->additional_info = reinterpret_cast<Op *>( res );
        return res;
    }
    //
    if ( op->type == Op::NUMBER ) {
        OpWithSeq *res = new OpWithSeq( op->number_data()->val );
        op->additional_info = reinterpret_cast<Op *>( res );
        return res;
    }
    //
    if ( op->type == STRING_add_NUM ) {
        SplittedVec<const Op *,32> sum;
        get_child_not_of_type_add( op, sum );
        OpWithSeq *res = new OpWithSeq( op->type );
        for(unsigned i=0;i<sum.size();++i)
            res->add_child( make_OpWithSeq_rec( sum[i] ) );
        op->additional_info = reinterpret_cast<Op *>( res );
        return res;
    }
    //
    if ( op->type == STRING_mul_NUM ) {
        SplittedVec<const Op *,32> mul;
        get_child_not_of_type_mul( op, mul );
        OpWithSeq *res = new OpWithSeq( op->type );
        for(unsigned i=0;i<mul.size();++i)
            res->add_child( make_OpWithSeq_rec( mul[i] ) );
        op->additional_info = reinterpret_cast<Op *>( res );
        return res;
    }
    //
    OpWithSeq *res = new OpWithSeq( op->type );
    for(unsigned i=0;i<Op::FuncData::max_nb_children and op->func_data()->children[i];++i)
        res->add_child( make_OpWithSeq_rec( op->func_data()->children[i] ) );
    op->additional_info = reinterpret_cast<Op *>( res );
    return res;
}

OpWithSeq *new_inv( OpWithSeq *ch ) {
    for(unsigned i=0;i<ch->parents.size();++i)
        if ( ch->parents[i]->type == OpWithSeq::INV and ch->parents[i]->children[0] == ch )
            return ch->parents[i];
    //
    OpWithSeq *res = new OpWithSeq( OpWithSeq::INV );
    res->add_child( ch );
    return res;
}

OpWithSeq *new_sub( OpWithSeq *a, OpWithSeq *b ) {
    OpWithSeq *res = new OpWithSeq( STRING_sub_NUM );
    res->add_child( a );
    res->add_child( b );
    return res;
}

OpWithSeq *replace_op_by( OpWithSeq *o, OpWithSeq *n ) {
    if ( o == n )
        return o;
    //
    for(unsigned i=0;i<o->parents.size();++i) {
        OpWithSeq *p = o->parents[i];
        for(unsigned j=0;j<p->children.size();++j)
            if ( p->children[j] == o )
                p->children[j] = n;
        n->parents.push_back( p );
    }
    o->parents.resize( 0 );
    delete o;
    return n;
}

void factorisation( OpWithSeq *op ) {
    //     std::map<OpWithSeq *op,int> nb_ops;
}

bool OpWithSeq::is_mul_minus_one() const {
    return type == STRING_mul_NUM and children[0]->is_minus_one();
}

void simple_simplifications_rec( OpWithSeq *op ) {
    if ( op->id == OpWithSeq::current_id )
        return;
    op->id = OpWithSeq::current_id;
    // recursivity
    for(unsigned i=0;i<op->children.size();++i)
        simple_simplifications_rec( op->children[i] );
    
    // a ^ -1
    if ( op->type == STRING_pow_NUM and op->children[1]->type == OpWithSeq::NUMBER and op->children[1]->val() == -1 )
        op = replace_op_by( op, new_inv( op->children[0] ) );
    
    // a + ( -1 ) * b + c + ( - 1 ) * d -> ( a + c ) - ( b + d )
    if ( op->type == STRING_add_NUM ) {
        unsigned nb_mul_minus_one = 0;
        for(unsigned i=0;i<op->children.size();++i)
            nb_mul_minus_one += op->children[i]->is_mul_minus_one();
        if ( nb_mul_minus_one and nb_mul_minus_one < op->children.size() ) {
            OpWithSeq *p = new OpWithSeq( STRING_add_NUM );
            OpWithSeq *n = new OpWithSeq( STRING_add_NUM );
            for(unsigned i=0;i<op->children.size();++i) {
                if ( op->children[i]->is_mul_minus_one() ) {
                    if ( op->children[i]->children.size() == 2 ) // (-1) * b
                        n->add_child( op->children[i]->children[1] ); // b
                    else { // ( -1 ) * b * c * ...
                        OpWithSeq *m = new OpWithSeq( STRING_mul_NUM );
                        for(unsigned k=1;k<op->children[i]->children.size();++k)
                            m->add_child( op->children[i]->children[k] );
                        n->add_child( m ); // b * c * ...
                    }
                } else
                    p->add_child( op->children[i] );
            }
            //
            OpWithSeq *r = new OpWithSeq( STRING_sub_NUM );
            // a
            if ( p->children.size() == 1 ) { // a - ( ... )
                r->add_child( p->children[0] );
                delete p;
            }
            else
                r->add_child( p );
            // b
            if ( n->children.size() == 1 ) { // a - b
                r->add_child( n->children[0] );
                delete n;
            } else
                r->add_child( n );
            //
            op = replace_op_by( op, r );
        }
    }

    // a + c * b + 5 * b
    if ( op->type == STRING_add_NUM )
        factorisation( op );
}

void simplifications( OpWithSeq *op ) {
    ++OpWithSeq::current_id;
    simple_simplifications_rec( op );
}


std::ostream &operator<<( std::ostream &os, const OpWithSeq &op ) {
    if ( op.type == OpWithSeq::SEQ ) {
        for(unsigned i=0;i<op.children.size();++i)
            os << ( i ? ", " : "" ) << *op.children[i];
    }
    else if ( op.type == OpWithSeq::INV ) {
        os << "inv(" << *op.children[0] << ")";
    }
    else if ( op.type == OpWithSeq::WRITE_ADD ) {
        for(unsigned i=0;i<op.children.size();++i)
            os << "\n -> " << *op.children[i];
    }
    else if ( op.type == OpWithSeq::NUMBER ) {
        os << op.num / op.den;
    }
    else if ( op.type == OpWithSeq::SYMBOL ) {
        os << op.cpp_name_str;
    }
    else {
        os << Nstring( op.type );
        os << "(";
        for(unsigned i=0;i<op.children.size();++i)
            os << ( i ? "," : "" ) << *op.children[i];
        os << ")";
    }
    return os;
}
