#include "op.h"
#include "basic_string_manip.h"
#include <usual_strings.h>
#include "nstring.h"
#include "thread.h"
#include "varargs.h"
#include "globaldata.h"

Op &Op::new_number( const Rationnal &val ) {
    Op *res = (Op *)malloc( sizeof(Op) + sizeof(NumberData) ); res->cpt_use = 0; res->type = NUMBER; res->parents.init(); res->cleared_id = 0;
    
    init_arithmetic( res->number_data()->val, val );
    return *res;
}

Op &Op::new_symbol( const char *cpp_name_str, unsigned cpp_name_si, const char *tex_name_str, unsigned tex_name_si ) {
    Op *res = (Op *)malloc( sizeof(Op) + sizeof(SymbolData) ); res->cpt_use = 0; res->type = SYMBOL; res->parents.init(); res->cleared_id = 0;
    
    SymbolData *ds = res->symbol_data();
    ds->cpp_name_str = strdupp0( cpp_name_str, cpp_name_si );
    ds->tex_name_str = strdupp0( tex_name_str, tex_name_si );
    return *res;
}
/// @see new_function 
bool same_op( Op *a, Op *b ) {
    if ( a->type == Op::NUMBER )
        return b->type == Op::NUMBER and a->number_data()->val == b->number_data()->val;
    return a == b;
}
Op &Op::new_function( int type, Op &a ) {
    // look in parents of a or b if function already created somewhere
    for(unsigned i=0;i<a.parents.size();++i) {
        Op *p = a.parents[ i ];
        if ( p->type == type and same_op(&a,p->func_data()->children[0]) ) {
            p->inc_ref();
            return *p;
        }
    }
    //
    Op *res = (Op *)malloc( sizeof(Op) + sizeof(FuncData) ); res->cpt_use = 0; res->type = type; res->parents.init(); res->cleared_id = 0;
    
    FuncData *ds = res->func_data();
    ds->children[0] = &a.inc_ref();
    ds->children[1] = NULL;
    
    if ( a.type != Op::NUMBER ) a.parents.push_back( res );
    return *res;
}
Op &Op::new_function( int type, Op &a, Op &b ) {
    // look in parents of a or b if function already created somewhere
    Op *c = ( a.type != Op::NUMBER ? &a : &b );
    for(unsigned i=0;i<c->parents.size();++i) {
        Op *p = c->parents[ i ];
        if ( p->type == type and same_op(&a,p->func_data()->children[0]) and same_op(&b,p->func_data()->children[1]) ) {
            p->inc_ref();
            return *p;
        }
    }
    //
    Op *res = (Op *)malloc( sizeof(Op) + sizeof(FuncData) ); res->cpt_use = 0; res->type = type; res->parents.init(); res->cleared_id = 0;
    
    FuncData *ds = res->func_data();
    ds->children[0] = &a.inc_ref();
    ds->children[1] = &b.inc_ref();
    
    if ( a.type != Op::NUMBER ) a.parents.push_back( res );
    if ( b.type != Op::NUMBER ) b.parents.push_back( res );
    return *res;
}

bool same_sumseq( Op::SumSeqData *a, Op::SumSeqData *b ) {
    if ( a->sum != b->sum )
        return false;
    if ( a->data.size() != b->data.size() )
        return false;
    // 
    for(unsigned i=0;i<a->data.size();++i) {
        if ( a->data[i].coeff != b->data[i].coeff or a->data[i].items.size() != b->data[i].items.size() )
            return false;
        for(unsigned j=0;j<a->data[i].items.size();++j)
            if ( a->data[i].items[j].val != b->data[i].items[j].val or a->data[i].items[j].expo != b->data[i].items[j].expo or a->data[i].items[j].may_need_abs != b->data[i].items[j].may_need_abs )
                return false;
    }
    return true;
}

struct SortSumSeqItems {
    bool operator()( const Op::SumSeqData::MulSeq &a, const Op::SumSeqData::MulSeq &b ) const {
        if ( a.items.size() != b.items.size() )
            return a.items.size() < b.items.size();
        for(unsigned i=0;i<a.items.size();++i)
            if ( operator()( a.items[i], b.items[i] ) )
                return true;
        return a.coeff < b.coeff;
    }
    bool operator()( const Op::SumSeqData::MulSeq::MulItem &a, const Op::SumSeqData::MulSeq::MulItem &b ) const {
        if ( a.val != b.val )
            return a.val < b.val;
        if ( a.expo != b.expo )
            return a.expo < b.expo;
        return a.may_need_abs < b.may_need_abs;
    }
};

void Op::SumSeqData::sort_data() {
    for(unsigned i=0;i<data.size();++i)
        data[i].items.sort( SortSumSeqItems() );
    data.sort( SortSumSeqItems() );
}

Op &simplified_sum_seq( Op &res ) {
    Op::SumSeqData *ds = res.sumseq_data();
    
    // sum + nothing
    if ( ds->data.size() == 0 ) {
        Op &n_res = Op::new_number( ds->sum );
        dec_ref( &res );
        return n_res;
    }
    //
    if ( ds->data.size() == 1 ) {
        if ( ds->data[0].items.size() == 0 ) { // sum + coeff * (a/a)
            Op &n_res = Op::new_number( ds->sum + ds->data[0].coeff );
            dec_ref( &res );
            return n_res;
        }
        if ( ds->sum.is_zero() and ds->data[0].coeff.is_one() and ds->data[0].items.size() == 1 and ds->data[0].items[0].expo.is_one() ) { // 0 + 1 * a ** 1
            if ( ds->data[0].items[0].may_need_abs ) { // 0 + 1 * abs( a ) ** 1
                Op &n_res = abs( *ds->data[0].items[0].val );
                dec_ref( &res );
                return n_res;
            }
            Op &n_res = ds->data[0].items[0].val->inc_ref();
            dec_ref( &res );
            return n_res;
        }
    }
    
    // generic cases
    // ds->sort_data();
    
    // look if identical sumseq_data in parents
    if ( ds->data.size() and ds->data[0].items.size() ) {
        Op *c = ds->data[0].items[0].val;
        for(unsigned i=0;i<c->parents.size();++i) {
            Op *p = c->parents[ i ];
            if ( p->type == Op::SUMSEQ and same_sumseq( ds, p->sumseq_data() ) ) {
                p->inc_ref();
                dec_ref( &res );
                return *p;
            }
        }
    }
    
    return res;
}
Op &Op::new_sumseq() {
    Op *res = (Op *)malloc( sizeof(Op) + sizeof(SumSeqData) );
    res->cpt_use = 0;
    res->type = Op::SUMSEQ;
    res->parents.init();
    res->cleared_id = 0;
     
    SumSeqData *ds = res->sumseq_data();
    ds->data.init();
    init_arithmetic( ds->sum, 0 );
    return *res;
}

Op &Op::new_sumseq_sum( Op &a, const Rationnal &ma, Op &b, const Rationnal &mb ) {
    Op &res = Op::new_sumseq();
    SumSeqData *ds = res.sumseq_data();
    ds->add_( a, ma, &res );
    ds->add_( b, mb, &res );
    return simplified_sum_seq( res );
}

Op &Op::new_sumseq_mul( Op &a, const Rationnal &ea, Op &b, const Rationnal &eb ) {
    // 0 * b or a * 0
    if ( (a.is_zero() and ea) or (b.is_zero() and eb) )
        return Op::new_number( 0 );
    // 1 * b
    if ( a.is_one() ) {
        if ( eb.is_one() )
            return b.inc_ref();
        return pow( b, Op::new_number( eb ) );
    }
    // a * 1
    if ( b.is_one() ) {
        if ( ea.is_one() )
            return a.inc_ref();
        return pow( a, Op::new_number( ea ) );
    }
        
    // a * b
    Op &res = Op::new_sumseq();
    SumSeqData *ds = res.sumseq_data();
    
    SumSeqData::MulSeq *ms = ds->data.new_elem(); ms->items.init(); init_arithmetic( ms->coeff, 1 );
    ms->mul_( a, ea, &res );
    ms->mul_( b, eb, &res );
    return simplified_sum_seq( res );
}

/// return true of a and b are indentical (don't take the coefficient into account)
bool identical_modulo_coeff( Op::SumSeqData::MulSeq &a, Op::SumSeqData::MulSeq &b ) {
    if ( a.items.size() != b.items.size() )
        return false;
    for(unsigned i=0;i<a.items.size();++i) {
        for(unsigned j=0;;++j) {
            if ( j == b.items.size() ) return false; // haven't found identical...
            if ( a.items[i] == b.items[j] ) break;
        }
    }
    return true;
}

void Op::SumSeqData::add_( Op &a, const Rationnal &ma, Op *res_op ) {
    if ( ma and not a.is_zero() ) {
        if ( a.type == Op::NUMBER ) {
            // std::cout << "sum " << sum << std::endl; std::cout << ma << std::endl; std::cout << a.number_data()->val << std::endl;
            sum = sum + ma * a.number_data()->val;
        }
        else if ( a.type == Op::SUMSEQ ) { // [...] + ma * [ 1 + toto + tata ]
            SumSeqData *sd = a.sumseq_data();
            sum = sum + ma * sd->sum;
            for(unsigned i=0;i<sd->data.size();++i) { // look if already present
                for(unsigned j=0;;++j) {
                    if ( j == data.size() ) { // neither toto nor tata was in this->data -> create a new mulseq
                        MulSeq *ms = data.new_elem();
                        init_arithmetic( ms->coeff, ma * sd->data[i].coeff );
                        ms->items.init();
                        for(unsigned k=0;k<sd->data[i].items.size();++k) { // 
                            MulSeq::MulItem *mi = ms->items.new_elem();
                            mi->init( sd->data[i].items[k] );
                            mi->val->parents.push_back( res_op );
                        }
                        break;
                    }
                    if ( identical_modulo_coeff( data[j], sd->data[i] ) ) { // already in [...]
                        data[j].coeff = data[j].coeff + ma * sd->data[i].coeff;
                        if ( data[j].coeff.num.val == 0 ) {
                            for(unsigned k=0;k<data[j].items.size();++k) {
                                data[j].items[k].val->parents.erase_elem_in_unordered_list( res_op );
                                dec_ref( data[j].items[k].val );
                                data[j].items[k].expo.destroy();
                            }
                            data.erase_elem_nb(j);
                        }
                        break;
                    }
                }
            }
        } else { // sumseq + something
            // look if something in sumseq
            for(unsigned j=0;;++j) {
                if ( j == data.size() ) { // not found
                    MulSeq *ms = data.new_elem(); ms->items.init();
                    MulSeq::MulItem *mi = ms->items.new_elem();
                    mi->init( &a, 1 );
                    a.parents.push_back( res_op );
                    init_arithmetic( ms->coeff, ma );
                    break;
                }
                if ( data[j].items.size()==1 and data[j].items[0].expo==Rationnal(1) and data[j].items[0].val == &a ) { // already in [...]
                    data[j].coeff = data[j].coeff + ma;
                    if ( data[j].coeff.num.val == 0 ) {
                        data[j].items[0].val->parents.erase_elem_in_unordered_list( res_op );
                        dec_ref( data[j].items[0].val );
                        // data[j].items[0].expo.destroy(); nothing to destroy if == 1
                        data.erase_elem_nb( j );
                    }
                    break;
                }
            }
        } 
    }
}
void Op::SumSeqData::MulSeq::mul_( Op &a, const Rationnal &ea, Op *res_op ) {
    //
    if ( ea and not a.is_one() ) {
        if ( a.type == Op::NUMBER ) {
            coeff = coeff * pow_96( a.number_data()->val, ea );
        } else if ( a.type == Op::SUMSEQ and a.sumseq_data()->sum.num.val==0 and a.sumseq_data()->data.size() == 1 ) { // [...] * [ C * toto**1.2 * tata ] ** ea
            SumSeqData::MulSeq &ams = a.sumseq_data()->data[0];
            coeff = coeff * pow_96( ams.coeff, ea );
            for(unsigned i=0;i<ams.items.size();++i) { // look if already present
                for(unsigned j=0;;++j) {
                    if ( j == items.size() ) { // neither toto nor tata was in this->data -> create a new mulseq
                        MulItem *mi = items.new_elem();
                        mi->init( ams.items[i] );
                        mi->expo = mi->expo * ea; // ea is an integer
                        ams.items[i].val->parents.push_back( res_op );
                        
                        if ( ea.is_integer()==false ) { // () ** 0.75
                            if ( ams.items[i].expo.is_even() ) // ( a**6 ) ** 0.75 -> abs is mandatory (unless a can be assumed to be positive_or_null)
                                mi->may_need_abs |= mi->assume_val_pos==false;
                            else if ( ams.items[i].expo.is_odd() ) // ( a**5 ) ** 0.75 -> a can be assumed to be positive_or_null (unless there's already an abs)
                                mi->assume_val_pos |= mi->may_need_abs==false;
                        }
                        if ( ea.is_even() and ams.items.size()==1 )
                            mi->may_need_abs = false;
                        break;
                    }
                    if ( ams.items[i].val == items[j].val ) { // already in [...]
                        Rationnal new_expo = items[j].expo + ea * ams.items[i].expo;
                        if ( ams.items[i].may_need_abs == items[j].may_need_abs or new_expo.is_even() ) { // abs(a)**x * abs(a)**y -> ok; abs(a) * a -> no; abs(a) * a**5 -> a**6
                            items[j].expo = new_expo;
                            items[j].assume_val_pos |= ams.items[i].assume_val_pos;
                            items[j].may_need_abs |= ams.items[i].may_need_abs;
                            if ( new_expo.is_even() )
                                items[j].may_need_abs = false;
                            if ( items[j].expo.num.val == 0 ) {
                                items[j].val->parents.erase_elem_in_unordered_list( res_op );
                                dec_ref( items[j].val );
                                items[j].expo.destroy();
                                items.erase_elem_nb( j );
                            }
                        }
                        break;
                    }
                }
            }
        } else {
            // look if similar val
            for(unsigned j=0;;++j) {
                if ( j == items.size() ) { // not found
                    MulSeq::MulItem *mi = items.new_elem();
                    mi->init( &a, ea );
                    a.parents.push_back( res_op );
                    break;
                }
                if ( items[j].val == &a ) {
                    items[j].expo = items[j].expo + ea;
                    if ( items[j].expo.num.val == 0 ) {
                        items[j].val->parents.erase_elem_in_unordered_list( res_op );
                        dec_ref( items[j].val );
                        items[j].expo.destroy();
                        items.erase_elem_nb( j );
                    }
                    break;
                }
            }
        }
    }
}

void Op::destroy() {
    assert( parents.size() == 0 );
    parents.destroy();
    if ( type == SYMBOL ) {
        free( symbol_data()->cpp_name_str );
        free( symbol_data()->tex_name_str );
    } else if ( type == NUMBER )
        number_data()->val.destroy();
    else if ( type == SUMSEQ ) {
        for(unsigned i=0;i<sumseq_data()->data.size();++i) {
            for(unsigned j=0;j<sumseq_data()->data[i].items.size();++j) {
                if ( sumseq_data()->data[i].items[j].val->type != Op::NUMBER )
                    sumseq_data()->data[i].items[j].val->parents.erase_elem_in_unordered_list( this );
                dec_ref( sumseq_data()->data[i].items[j].val );
                sumseq_data()->data[i].items[j].expo.destroy();
            }
            sumseq_data()->data[i].items.destroy();
            sumseq_data()->data[i].coeff.destroy();
        }
        sumseq_data()->data.destroy();
        sumseq_data()->sum.destroy();
    } else { // function
        for(unsigned i=0;i<Op::FuncData::max_nb_children and func_data()->children[i];++i) {
            if ( func_data()->children[i]->type != Op::NUMBER )
                func_data()->children[i]->parents.erase_elem_in_unordered_list( this );
            dec_ref( func_data()->children[i] );
        }
    }
}
    
bool Op::is_zero     () { return type==NUMBER and number_data()->val.is_zero(); }
bool Op::is_one      () { return type==NUMBER and number_data()->val.is_one(); }
bool Op::is_minus_one() { return type==NUMBER and number_data()->val.is_minus_one(); }

void Op::clear_additional_info_rec() {
    static unsigned clear_id = 0;
    clear_additional_info_rec( ++clear_id );
}

void Op::clear_additional_info_rec( unsigned clear_id ) {
    if ( cleared_id == clear_id )
        return;
        
    cleared_id = clear_id;
    additional_info = NULL;
    if ( type == Op::SUMSEQ ) {
        for(unsigned i=0;i<sumseq_data()->data.size();++i)
            for(unsigned j=0;j<sumseq_data()->data[i].items.size();++j)
                sumseq_data()->data[i].items[j].val->clear_additional_info_rec( clear_id );
    } else if ( type > 0 )
        for(unsigned i=0;i<FuncData::max_nb_children and func_data()->children[i];++i)
            func_data()->children[i]->clear_additional_info_rec( clear_id );
}

void graphviz_repr_rec( std::ostream &os, Op *op ) {
    if ( op->additional_info ) return; // already outputted ?
    op->additional_info = op;
    
    if ( op->type == Op::NUMBER )
        os << "    node" << op << " [label=\"" << op->number_data()->val << "\"];\n";
    else if ( op->type == Op::SYMBOL )
        os << "    node" << op << " [label=\"" << op->symbol_data()->cpp_name_str << "\"];\n";
    else if ( op->type == Op::SUMSEQ ) {
        os << "    node" << op << " [label=\"+\"];\n";
        for(unsigned i=0;i<op->sumseq_data()->data.size();++i) {
            
            if ( op->sumseq_data()->data[i].items.size() == 1 and op->sumseq_data()->data[i].items[0].expo.is_one() ) {
                os << "    node" << op << " -> node" << op->sumseq_data()->data[i].items[0].val << " [color=green];\n";
                graphviz_repr_rec( os, op->sumseq_data()->data[i].items[0].val );
            }
            else {
                os << "    node_m" << op << "_" << i << " [label=\"* " << op->sumseq_data()->data[i].coeff << "\"];\n";
                os << "    node" << op << " -> node_m" << op << "_" << i << " [color=green];\n";
                for(unsigned j=0;j<op->sumseq_data()->data[i].items.size();++j) {
                    if ( op->sumseq_data()->data[i].items[j].expo.is_one() ) {
                        os << "    node_m" << op << "_" << i << " -> node" << op->sumseq_data()->data[i].items[j].val << " [color=green];\n";
                    }
                    else {
                        os << "    node_p" << op << "_" << i << "_" << j << " [label=\"^ " << op->sumseq_data()->data[i].items[j].expo << "\"];\n";
                        os << "    node_m" << op << "_" << i << " -> node_p" << op << "_" << i << "_" << j << " [color=green];\n";
                        os << "    node_p" << op << "_" << i << "_" << j << " -> node" << op->sumseq_data()->data[i].items[j].val << " [color=green];\n";
                    }
                    graphviz_repr_rec( os, op->sumseq_data()->data[i].items[j].val );
                }
            }
        }
        if ( op->sumseq_data()->sum ) {
            os << "    node_nu" << op << " [label=\"" << op->sumseq_data()->sum << "\"];\n";
            os << "    node" << op << " -> node_nu" << op << " [color=blue];\n";
        }
    } else {
        os << "    node" << op << " [label=\"" << Nstring( op->type ) << "\"];\n";
        for(unsigned i=0;i<Op::FuncData::max_nb_children and op->func_data()->children[i];++i) {
            os << "    node" << op << " -> node" << op->func_data()->children[i] << " [color=black];\n";
            graphviz_repr_rec( os, op->func_data()->children[i] );
        }
    }
}
void graphviz_repr( std::ostream &os, Op *op ) {
    op->clear_additional_info_rec();
    graphviz_repr_rec( os, op );
}

std::string tex_repr( const Rationnal &val, bool want_par = false, bool assume_par_not_needed = false ) {
    std::ostringstream os;
//     want_par |= ( val.den != Rationnal::BI(1) );
    if ( want_par and assume_par_not_needed==false ) os << "(";
    if ( val.den == Rationnal::BI(1) ) os << val.num;
    else os << "\\frac{" << val.num << "}{" << val.den << "}";
    if ( want_par and assume_par_not_needed==false ) os << ")";
    return os.str();
}

void tex_repr_rec( std::ostream &os, Op *op, int type_parent ) {
    if ( op->type == Op::NUMBER )
        os << tex_repr( op->number_data()->val );
    else if ( op->type == Op::SYMBOL )
        os << op->symbol_data()->tex_name_str;
    else if ( op->type == Op::SUMSEQ ) {
        // os << "[";
        Op::SumSeqData *sd = op->sumseq_data();
        int exposed_type = STRING_pow_NUM;
        if ( sd->sum or sd->data.size() > 1 )
            exposed_type = STRING_add_NUM;
        else if ( sd->data[0].coeff or sd->data[0].items.size() > 1 )
            exposed_type = STRING_mul_NUM;
        bool np = type_parent > exposed_type;
        if ( np ) os << "(";
        if ( sd->sum ) os << tex_repr( sd->sum );
        for(unsigned i=0;i<op->sumseq_data()->data.size();++i) {
            Rationnal c = op->sumseq_data()->data[i].coeff;
            if ( i or sd->sum ) {
                os << ( c >= 0 ? "+" : "-" );
                c = abs(c);
            }
            if ( c != Rationnal(1) )
                os << tex_repr( c, c.num.val < 0 );
            for(unsigned j=0;j<op->sumseq_data()->data[i].items.size();++j) {
                Rationnal e = op->sumseq_data()->data[i].items[j].expo;
                if ( j or c != Rationnal(1) ) {
                    os << ( e.num.val >= 0 ? " " : "/" );
                    e = abs(e);
                }
                if ( e != Rationnal(1) ) {
                    os <<  "{";
                    if ( op->sumseq_data()->data[i].items[j].need_abs() ) os << "abs(";
                    tex_repr_rec( os, op->sumseq_data()->data[i].items[j].val, STRING_pow_NUM );
                    if ( op->sumseq_data()->data[i].items[j].need_abs() ) os << ")";
                    
                    os <<  "}^{" << tex_repr( e, false, true ) << "}";
                } else {
                    int ntype_par = STRING_mul_NUM;
                    bool need_abs = op->sumseq_data()->data[i].items[j].need_abs();
                    if ( op->sumseq_data()->data[i].items.size()>1 or c ) ntype_par = STRING_mul_NUM;
                    else if ( op->sumseq_data()->data.size()>1 or sd->sum ) ntype_par = STRING_add_NUM;
                    
                    if ( need_abs ) os << "abs(";
                    tex_repr_rec( os, op->sumseq_data()->data[i].items[j].val, ntype_par * (need_abs==false) );
                    if ( need_abs ) os << ")";
                }
            }
        }
        if ( np ) os << ")";
        // os << "]";
    } else {
        os << Nstring(op->type) << "(";
        for(unsigned i=0;i<Op::FuncData::max_nb_children and op->func_data()->children[i];++i) {
            if ( i ) os << ",";
            tex_repr_rec( os, op->func_data()->children[i], 0 );
        }
        os << ")";
    }
}

void tex_repr( std::ostream &os, Op *op ) {
    tex_repr_rec( os, op, 0 );
}


std::string cpp_repr( const Rationnal &val, bool want_par = false, bool assume_par_not_needed = false ) {
    std::ostringstream os;
    want_par |= ( val.den != Rationnal::BI(1) );
    if ( want_par and assume_par_not_needed==false ) os << "(";
    if ( val.den == Rationnal::BI(1) ) os << val.num;
    else os << val.num << ".0/" << val.den << ".0";
    if ( want_par and assume_par_not_needed==false ) os << ")";
    return os.str();
}

void cpp_repr_rec( std::ostream &os, Op *op, int type_parent ) {
    if ( op->type == Op::NUMBER )
        os << cpp_repr( op->number_data()->val );
    else if ( op->type == Op::SYMBOL )
        os << op->symbol_data()->cpp_name_str;
    else if ( op->type == Op::SUMSEQ ) {
        // os << "[";
        Op::SumSeqData *sd = op->sumseq_data();
        int exposed_type = STRING_pow_NUM;
        if ( sd->sum or sd->data.size() > 1 )
            exposed_type = STRING_add_NUM;
        else if ( sd->data.size() and (sd->data[0].coeff or sd->data[0].items.size() > 1) )
            exposed_type = STRING_mul_NUM;
        bool np = type_parent > exposed_type;
        if ( np ) os << "(";
        if ( sd->sum ) os << cpp_repr( sd->sum );
        for(unsigned i=0;i<op->sumseq_data()->data.size();++i) {
            Rationnal c = op->sumseq_data()->data[i].coeff;
            if ( i or sd->sum ) {
                os << ( c >= 0 ? "+" : "-" );
                c = abs(c);
            }
            if ( c != Rationnal(1) )
                os << cpp_repr( c, c.num.val < 0 );
            for(unsigned j=0;j<op->sumseq_data()->data[i].items.size();++j) {
                Rationnal e = op->sumseq_data()->data[i].items[j].expo;
                if ( j or c != Rationnal(1) ) {
                    os << ( e.num.val >= 0 ? "*" : "/" );
                    e = abs(e);
                }
                if ( e != Rationnal(1) ) {
                    os << "pow(";
                    
                    if ( op->sumseq_data()->data[i].items[j].need_abs() ) os << "abs(";
                    cpp_repr_rec( os, op->sumseq_data()->data[i].items[j].val, 0 );
                    if ( op->sumseq_data()->data[i].items[j].need_abs() ) os << ")";
                    
                    os <<  "," << cpp_repr( e, false, true ) << ")";
                } else {
                    int ntype_par = STRING_mul_NUM;
                    bool need_abs = op->sumseq_data()->data[i].items[j].need_abs();
                    if ( op->sumseq_data()->data[i].items.size()>1 or c ) ntype_par = STRING_mul_NUM;
                    else if ( op->sumseq_data()->data.size()>1 or sd->sum ) ntype_par = STRING_add_NUM;
                    
                    if ( need_abs ) os << "abs(";
                    cpp_repr_rec( os, op->sumseq_data()->data[i].items[j].val, ntype_par * (need_abs==false) );
                    if ( need_abs ) os << ")";
                }
            }
        }
        if ( np ) os << ")";
        // os << "]";
    } else {
        os << Nstring(op->type) << "(";
        for(unsigned i=0;i<Op::FuncData::max_nb_children and op->func_data()->children[i];++i) {
            if ( i ) os << ",";
            cpp_repr_rec( os, op->func_data()->children[i], 0 );
        }
        os << ")";
    }
}

void cpp_repr( std::ostream &os, Op *op ) {
    cpp_repr_rec( os, op, 0 );
}

bool Op::SumSeqData::necessary_positive_or_null() const {
    if ( not sum.positive_or_null() )
        return false;
    for(unsigned i=0;i<data.size();++i) {
        if ( not data[i].coeff.positive_or_null() )
            return false;
        for(unsigned j=0;j<data[i].items.size();++j)
            if ( data[i].items[j].assume_val_pos==false and data[i].items[j].expo.is_odd() )
                return false;
    }
    return true;
}

bool Op::necessary_positive_or_null() {
    return ( type == NUMBER and number_data()->val.positive_or_null() ) or
           ( type == SUMSEQ and sumseq_data()->necessary_positive_or_null() ) or
             type == STRING_abs_NUM or
             type == STRING_eqz_NUM or
             type == STRING_cosh_NUM or
             type == STRING_heaviside_NUM;
}


Op &operator+( Op &a, Op &b ) { return Op::new_sumseq_sum( a,  1, b,  1 ); }
Op &operator-( Op &a, Op &b ) { return Op::new_sumseq_sum( a,  1, b, -1 ); }
Op &operator-( Op &a        ) { return Op::new_sumseq_sum( a, -1, a,  0 ); }
Op &operator*( Op &a, Op &b ) { return Op::new_sumseq_mul( a,  1, b,  1 ); }
Op &operator/( Op &a, Op &b ) { return Op::new_sumseq_mul( a,  1, b, -1 ); }
Op &pow      ( Op &a, Op &b ) {
    if ( b.type == Op::NUMBER ) {
        // a ** 1
        if ( b.number_data()->val.is_one() )
            return a.inc_ref();
        // a ** 0
        if ( b.number_data()->val.is_zero() )
            return Op::new_number( 1 );
        //
        Rationnal expo = b.number_data()->val;
        // -> pow( 2.3, 5.2 )
        if ( a.type == Op::NUMBER )
            return Op::new_number( pow_96( a.number_data()->val, expo ) );
        // pow( something, Rationnal ) -> sumseq_data
        Op &res = Op::new_sumseq();
        Op::SumSeqData *res_sd = res.sumseq_data();
        // -> pow( sumseq_data, 5.2 )
        if ( a.type == Op::SUMSEQ ) {
            Op::SumSeqData *sd = a.sumseq_data();
            if ( sd->data.size() == 1 and not sd->sum ) { // 0 + C*a*b + nothing
                Op::SumSeqData::MulSeq *ms = res_sd->data.new_elem();
                init_arithmetic( ms->coeff, pow_96( sd->data[0].coeff, expo ) ); ms->items.init();
                for(unsigned i=0;i<sd->data[0].items.size();++i) {
                    Op::SumSeqData::MulSeq::MulItem *mi = ms->items.new_elem();
                    mi->init( sd->data[0].items[i] );
                    sd->data[0].items[i].val->parents.push_back( &res );
                    mi->expo = mi->expo * expo;
                    if ( expo.is_integer()==false ) { // () ** 0.75
                        if ( sd->data[0].items[i].expo.is_even() ) // ( a**6 ) ** 0.75 -> abs is mandatory (unless a can be assumed to be positive_or_null)
                            mi->may_need_abs |= mi->assume_val_pos==false;
                        else if ( sd->data[0].items[i].expo.is_odd() ) // ( a**5 ) ** 0.75 -> a can be assumed to be positive_or_null (unless there's already an abs)
                            mi->assume_val_pos |= mi->may_need_abs==false;
                    }
                    if ( expo.is_even() and sd->data[0].items.size()==1 )
                        mi->may_need_abs = false;
                }
                return simplified_sum_seq( res );
            }
        }
        // -> pow( ..., 5.2 )
        Op::SumSeqData::MulSeq *ms = res_sd->data.new_elem();
        init_arithmetic( ms->coeff, 1 ); ms->items.init();
        Op::SumSeqData::MulSeq::MulItem *mi = ms->items.new_elem();
        if ( a.type == STRING_abs_NUM and expo.is_even() ) { // abs(a)**4 -> a**4
            mi->init( a.func_data()->children[0], expo );
            a.func_data()->children[0]->parents.push_back( &res );
        }
        else {
            mi->init( &a, expo );
            a.parents.push_back( &res );
        }
        return res;
    }
    // pow( ..., ... )
    return Op::new_function( STRING_pow_NUM, a, b );
}

inline bool are_numbers( const Op &a, const Op &b ) { return a.type == Op::NUMBER and b.type == Op::NUMBER; }


std::string graphviz_repr( Op *op ) { std::ostringstream res; graphviz_repr( res, op ); return res.str(); }
std::string cpp_repr     ( Op *op ) { std::ostringstream res; cpp_repr     ( res, op ); return res.str(); }
std::string tex_repr     ( Op *op ) { std::ostringstream res; tex_repr     ( res, op ); return res.str(); }

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
Op &diff_rec( Thread *th, const void *tok, Op &a, SplittedVec<Op *,1024,4096> &of, Op &z, Op &o );

Op &diff_sumseq_rec( Thread *th, const void *tok, Op &op, SplittedVec<Op *,1024,4096> &of, Op &z, Op &o ) {
    Op::SumSeqData *sd = op.sumseq_data();
    for(unsigned i=0;i<sd->data.size();++i)
        for(unsigned j=0;j<sd->data[i].items.size();++j)
            diff_rec( th, tok, *sd->data[i].items[j].val, of, z, o );
    //
    Op &res = Op::new_sumseq();
    Op::SumSeqData *nsd = res.sumseq_data();
    init_arithmetic( nsd->sum, 0 );
    nsd->data.init();
    for(unsigned i=0;i<sd->data.size();++i) {
        for(unsigned j=0;j<sd->data[i].items.size();++j) { // ... * a_k**(e_k-1) * ...
            Op &da_j = *sd->data[i].items[j].val->additional_info;
            if ( da_j.type != Op::NUMBER or not da_j.number_data()->val.is_zero() ) {
                Op::SumSeqData::MulSeq *ms = nsd->data.new_elem();
                ms->items.init();
                init_arithmetic( ms->coeff, sd->data[i].coeff * sd->data[i].items[j].expo );
                for(unsigned k=0;k<sd->data[i].items.size();++k) { // 
                    if ( k == j ) {
                        if ( not sd->data[i].items[k].expo.is_one() ) {
                            Op::SumSeqData::MulSeq::MulItem *mi = ms->items.new_elem();
                            mi->init( sd->data[i].items[k] );
                            mi->val->parents.push_back( &res );
                            mi->expo = sd->data[i].items[k].expo - Rationnal( 1 );
                        }
                    } else {
                        Op::SumSeqData::MulSeq::MulItem *mi = ms->items.new_elem();
                        mi->init( sd->data[i].items[k] );
                        mi->val->parents.push_back( &res );
                    }
                }
                ms->mul_( da_j, 1, &res ); // * da_j
                if ( ms->items.size() == 0 ) {
                    nsd->sum = nsd->sum + ms->coeff;
                    nsd->data.pop_back();
                } else if ( ms->items.size() == 1 and ms->items[0].expo.is_one() ) { // ms = a**1 
                    Op *cp = ms->items[0].val;
                    cp->parents.erase_elem_in_unordered_list( &res );
                    Rationnal coeff = ms->coeff;
                    nsd->data.pop_back();
                    nsd->add_( *cp, coeff, &res );
                }
            }
        }
    }
    //
    Op &r = simplified_sum_seq( res );
    of.push_back( &r );
    return r;
}

Op &diff_rec( Thread *th, const void *tok, Op &a, SplittedVec<Op *,1024,4096> &of, Op &z, Op &o ) {
    if ( a.additional_info )
        return *a.additional_info; // 
    //
    #define GET_D0 Op &c0 = *a.func_data()->children[0]; Op &da = diff_rec( th, tok, c0, of, z, o ); if (&da==&z) { a.additional_info = &z; return z; }
    // 
    switch ( a.type ) {
        case Op::NUMBER:
        case Op::SYMBOL:
            a.additional_info = &z;
            return z;
        case Op::SUMSEQ: // a +  3 * b**2 * c
            a.additional_info = &diff_sumseq_rec( th, tok, a, of, z, o );
            return *a.additional_info;
        //
        case STRING_heaviside_NUM: a.additional_info = &z; return z;
        case STRING_eqz_NUM:       a.additional_info = &z; return z;
        //
        case STRING_log_NUM:       { GET_D0; Op &n = o / c0; Op &r = da * n; of.push_back(&n); of.push_back(&r); a.additional_info = &r; return r; }
        case STRING_abs_NUM:       { GET_D0; Op &h = heaviside(c0); Op &_2 = Op::new_number(2); Op &h2 = h * _2; Op &h2m1 = h2 - o; Op &r = da * h2m1;
                                        of.push_back(&h); of.push_back(&_2); of.push_back(&h2); of.push_back(&h2m1); of.push_back(&r); a.additional_info = &r; return r; }
        case STRING_exp_NUM:       { GET_D0; Op &r = da * a; of.push_back(&r); a.additional_info = &r; return r; }
        case STRING_sin_NUM:       { GET_D0; Op &cos_c0 = cos(c0); Op &r = da * cos_c0; of.push_back(&cos_c0); of.push_back(&r); a.additional_info = &r; return r; }
        case STRING_cos_NUM:       { GET_D0; Op &sin_c0 = sin(c0); Op &msin_c0 = - sin_c0; Op &r = da * msin_c0; of.push_back(&sin_c0); of.push_back(&msin_c0); of.push_back(&r); a.additional_info = &r; return r; }
        case STRING_tan_NUM:       { GET_D0; Op &_2 = Op::new_number(2); Op &pow_a_2 = pow(a,_2); Op &_1_p_pow_a_2 = o+pow_a_2; Op &r = da / _1_p_pow_a_2;
                                        of.push_back(&_2); of.push_back(&pow_a_2); of.push_back(&_1_p_pow_a_2); of.push_back(&r); a.additional_info = &r; return r; }
        case STRING_asin_NUM:      { GET_D0; Op &_2 = Op::new_number(2); Op &pow_c0_2 = pow(c0,_2); Op &_1_s_pow_c0_2 = o-pow_c0_2; Op &z5 = Op::new_number(Rationnal(-1,2)); Op &pow_1_s_pow_c0_2_z5 = pow( _1_s_pow_c0_2, z5 );
                                        Op &r = da * pow_1_s_pow_c0_2_z5; of.push_back(&_2); of.push_back(&pow_c0_2); of.push_back(&_1_s_pow_c0_2); of.push_back(&z5); of.push_back(&pow_1_s_pow_c0_2_z5); of.push_back(&r); a.additional_info = &r; return r; }
        case STRING_acos_NUM:      { GET_D0; Op &_2 = Op::new_number(2); Op &pow_c0_2 = pow(c0,_2); Op &_1_s_pow_c0_2 = o-pow_c0_2; Op &z5 = Op::new_number(Rationnal(-1,2)); Op &pow_1_s_pow_c0_2_z5 = pow( _1_s_pow_c0_2, z5 );
                                        Op &mr = da * pow_1_s_pow_c0_2_z5; Op &r = -mr; of.push_back(&_2); of.push_back(&pow_c0_2); of.push_back(&_1_s_pow_c0_2); of.push_back(&z5); of.push_back(&pow_1_s_pow_c0_2_z5); of.push_back(&mr); 
                                        of.push_back(&r); a.additional_info = &r; return r; }
        case STRING_atan_NUM:      { GET_D0; Op &_2 = Op::new_number(2); Op &pow_c0_2 = pow(c0,_2); Op &_1_p_pow_c0_2 = o+pow_c0_2; Op &r = da / _1_p_pow_c0_2;
                                        of.push_back(&_2); of.push_back(&pow_c0_2); of.push_back(&_1_p_pow_c0_2); of.push_back(&r); a.additional_info = &r; return r; }
        default:
            th->add_error( "for now, no rules to differentiate function '"+std::string(Nstring(a.type))+"'.", tok );
    }
    #undef GET_D0
    return z;
}

Op *diff( Thread *th, const void *tok, Op *a, Op *b ) {
    SplittedVec<Op *,1024,4096> of;
    
    a->clear_additional_info_rec();
    b->additional_info = &Op::new_number( 1 );
    
    Op *z = &Op::new_number( 0 );
    Op *o = &Op::new_number( 1 );
    of.push_back( b->additional_info );
    of.push_back( z );
    of.push_back( o );
    
    //
    Op &res = diff_rec( th, tok, *a, of, *z, *o );
    res.inc_ref();
    for(unsigned i=0;i<of.size();++i)
        dec_ref( of[i] );
    return &res;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
Op &subs_rec( Thread *th, const void *tok, Op &a, SplittedVec<Op *,1024,4096> &of );

Op &subs_sumseq_rec( Thread *th, const void *tok, Op &op, SplittedVec<Op *,1024,4096> &of ) {
    // get subs if children
    Op::SumSeqData *sd = op.sumseq_data();
    bool at_least_one_different_child = false;
    for(unsigned i=0;i<sd->data.size();++i) {
        for(unsigned j=0;j<sd->data[i].items.size();++j) {
            Op *c = sd->data[i].items[j].val;
            Op &nc = subs_rec( th, tok, *c, of );
            at_least_one_different_child |= ( &nc != c );
        }
    }
    // if no changes in children -> return op
    //     std::cout << "at_least_one_different_child " << at_least_one_different_child << std::endl;
    if ( at_least_one_different_child == false ) {
        op.additional_info = &op;
        return op;
    }
    // else, create a new sumseq
    Op &res = Op::new_sumseq();
    Op::SumSeqData *nsd = res.sumseq_data();
    init_arithmetic( nsd->sum, sd->sum );
    nsd->data.init();
    for(unsigned i=0;i<sd->data.size();++i) {
        if ( sd->data[i].items.size() == 1 and sd->data[i].items[0].expo.is_one() and sd->data[i].items[0].may_need_abs==false ) {
            nsd->add_( *sd->data[i].items[0].val->additional_info, sd->data[i].coeff, &res );
        } else {
            // look if there's a 0 in additional_info
            bool has_z = false;
            for(unsigned j=0;j<sd->data[i].items.size();++j) // ... * ...
                has_z |= ( sd->data[i].items[j].val->additional_info->is_zero() );
            // else -> new mulseq with *
            if ( not has_z ) {
                Op &mulseq = Op::new_sumseq();
                Op::SumSeqData *ds_mulseq = mulseq.sumseq_data();
                Op::SumSeqData::MulSeq *ms = ds_mulseq->data.new_elem();
                ms->items.init();
                init_arithmetic( ms->coeff, sd->data[i].coeff );
                for(unsigned j=0;j<sd->data[i].items.size();++j) // ... * ...
                    ms->mul_( *sd->data[i].items[j].val->additional_info, sd->data[i].items[j].expo, &mulseq );
                Op &simplified_mulseq = simplified_sum_seq( mulseq );
                of.push_back( &simplified_mulseq );
                // add new mulseq
                nsd->add_( simplified_mulseq, 1, &res );
            }
        }
    }
    //
    Op &r = simplified_sum_seq( res );
    of.push_back( &r );
    //     op.additional_info = &r;
    return r;
}

Op &subs_rec( Thread *th, const void *tok, Op &a, SplittedVec<Op *,1024,4096> &of ) {
    if ( a.additional_info )
        return *a.additional_info;
    // 
    #define GET_S0 Op &c0 = *a.func_data()->children[0]; Op &s0 = subs_rec( th, tok, c0, of ); if (&s0==&c0) { a.additional_info = &a; return a; }
    switch ( a.type ) {
        case Op::NUMBER:
        case Op::SYMBOL:
            a.additional_info = &a;
            return *a.additional_info;
        case Op::SUMSEQ: // a +  3 * b**2 * c
            a.additional_info = &subs_sumseq_rec( th, tok, a, of );
            return *a.additional_info;
        //
        case STRING_heaviside_NUM: { GET_S0; Op &r = heaviside( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_eqz_NUM:       { GET_S0; Op &r = eqz      ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_log_NUM:       { GET_S0; Op &r = log      ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_abs_NUM:       { GET_S0; Op &r = abs      ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_exp_NUM:       { GET_S0; Op &r = exp      ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_sin_NUM:       { GET_S0; Op &r = sin      ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_cos_NUM:       { GET_S0; Op &r = cos      ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_tan_NUM:       { GET_S0; Op &r = tan      ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_asin_NUM:      { GET_S0; Op &r = asin     ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_acos_NUM:      { GET_S0; Op &r = acos     ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        case STRING_atan_NUM:      { GET_S0; Op &r = atan     ( s0 ); a.additional_info = &r; of.push_back(&r); return r; }
        default:
            th->add_error( "for now, no rules to substitute function '"+std::string(Nstring(a.type))+"'.", tok );
    }
    return *a.additional_info;
}

Op *subs( Thread *th, const void *tok, Op *self, VarArgs &a, VarArgs &b ) {
    if ( a.variables.size() != b.variables.size() ) {
        std::ostringstream ss;
        ss << "attempting to make 'op.subs(a,b)' with a and b of different sizes (respectively " << a.variables.size() << " and " << b.variables.size() << ").";
        th->add_error(ss.str(),tok);
        return &self->inc_ref();
    }
    
    SplittedVec<Op *,1024,4096> of;
    self->clear_additional_info_rec();
    for(unsigned i=0;i<a.variables.size();++i)
        (*reinterpret_cast<Op **>(a.variables[i].data))->additional_info = *reinterpret_cast<Op **>(b.variables[i].data);
    //
    Op &res = subs_rec( th, tok, *self, of ).inc_ref();
    for(unsigned i=0;i<of.size();++i) dec_ref( of[i] );
    return &res;
}

Rationnal subs_numerical( Thread *th, const void *tok, Op *op, const Rationnal &a ) {
    SplittedVec<Op *,32> symbols;
    get_sub_symbols( op, symbols );
    if ( symbols.size() > 1 ) {
        th->add_error("subs_numerical works only with expressions which contains at most 1 variable.",tok);
        return 0;
    }
    //
    SplittedVec<Op *,1024,4096> of;
    op->clear_additional_info_rec();
    if ( symbols.size() ) {
        Op &n = Op::new_number( a );
        of.push_back( &n );
        symbols[0]->additional_info = &n;
    }
    //
    Op &res = subs_rec( th, tok, *op, of ).inc_ref();
    for(unsigned i=0;i<of.size();++i) dec_ref( of[i] );
    assert( res.type == Op::NUMBER );
    Rationnal n_res = res.number_data()->val; dec_ref( &res );
    return n_res;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
void get_sub_symbols( Op *op, SplittedVec<Op *,32> &symbols ) {
    if ( op->type == Op::SYMBOL ) {
        for(unsigned i=0;;++i) {
            if ( i == symbols.size() ) {
                symbols.push_back( op );
                return;
            }
            if ( symbols[i] == op )
                return;
        }
    }
    if ( op->type == Op::SUMSEQ ) {
        Op::SumSeqData *sd = op->sumseq_data();
        for(unsigned i=0;i<sd->data.size();++i)
            for(unsigned j=0;j<sd->data[i].items.size();++j)
                get_sub_symbols( sd->data[i].items[j].val, symbols );
    } else if ( op->type > 0 ) {
        Op::FuncData *fd = op->func_data();
        if ( fd->children[0] ) get_sub_symbols( fd->children[0], symbols );
        if ( fd->children[1] ) get_sub_symbols( fd->children[1], symbols );
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool depends_on_rec( Op *op, Op *var ) {
    if ( op == var )
        return true;
    if ( op->type >= 0 ) {
        for(unsigned i=0;i<Op::FuncData::max_nb_children and op->func_data()->children[i];++i)
            if ( depends_on_rec( op->func_data()->children[i], var ) )
                return true;
    } else if ( op->type == Op::SUMSEQ ) {
        Op::SumSeqData *sd = op->sumseq_data();
        for(unsigned i=0;i<sd->data.size();++i)
            for(unsigned j=0;j<sd->data[i].items.size();++j)
                if ( depends_on_rec( sd->data[i].items[j].val, var ) )
                    return true;
    }
    return false;
}

bool op_depends_on_symbols( Op *op, VarArgs &symbols ) {
    if ( symbols.variables.size() == 0 )
        return true;
    for(unsigned i=0;i<symbols.variables.size();++i)
        if ( symbols.variables[i].type==global_data.Op and depends_on_rec( op, *reinterpret_cast<Op **>(symbols.variables[i].data) ) )
            return true;
    return false;
}

Op *find_deepest_disc_func_which_depends_on( Op *op, VarArgs &symbols ) {
    if ( op->type >= 0 ) {
        // look in children
        for(unsigned i=0;i<Op::FuncData::max_nb_children and op->func_data()->children[i];++i) {
            Op *c = find_deepest_disc_func_which_depends_on( op->func_data()->children[i], symbols );
            if ( c ) 
                return c;
        }
        // abs, heaviside, ... ?
        if ( ( op->type == STRING_heaviside_NUM or op->type == STRING_abs_NUM ) and op_depends_on_symbols( op, symbols ) )
            return op;
    }
    else if ( op->type == Op::SUMSEQ ) {
        Op::SumSeqData *sd = op->sumseq_data();
        // look in children
        for(unsigned i=0;i<sd->data.size();++i) {
            for(unsigned j=0;j<sd->data[i].items.size();++j) {
                Op *c = find_deepest_disc_func_which_depends_on( sd->data[i].items[j].val, symbols );
                if ( c )
                    return c;
            }
        }
        // abs, heaviside, ... ?
        for(unsigned i=0;i<sd->data.size();++i)
            for(unsigned j=0;j<sd->data[i].items.size();++j)
                if ( sd->data[i].items[j].may_need_abs and op_depends_on_symbols( sd->data[i].items[j].val, symbols ) )
                    return sd->data[i].items[j].val;
    }
    return NULL;
}

void get_discontinuities_rec( Thread *th, const void *tok, Op *op, SplittedVec<Op*,32> &res, VarArgs &symbols, SplittedVec<Op *,1024,4096> &of, Op *z, Op *o ) {
    Op *h = find_deepest_disc_func_which_depends_on( op, symbols );
    //
    if ( h ) {
        Op *old = res.back();
        // NULL
        res.back() = NULL;
        // condition
        Op *c = h->func_data()->children[0];
        res.push_back( c );
        
        // substitutions
        for(unsigned val_cond=0;val_cond<2;++val_cond) {
            old->clear_additional_info_rec();
            //
            if ( h->type == STRING_heaviside_NUM )
                h->additional_info = ( val_cond ? o : z );
            else if ( h->type == STRING_abs_NUM ) {
                if ( val_cond ) {
                    h->additional_info = c;
                } else {
                    h->additional_info = &operator-( *c );
                    of.push_back( h->additional_info );
                }
            }
            else {
                std::cout << "unmanaged " << Nstring( h->type ) << std::endl;
                assert(0);
            }
            //
            Op *new_op = &subs_rec( th, tok, *old, of );
            res.push_back( new_op );
            get_discontinuities_rec( th, tok, new_op, res, symbols, of, z, o );
        }
    }
}
    
void make_varargs_from_disc_desc( Variable *return_var, SplittedVec<Op *,32> res, unsigned &n ) {
    Op *op = res[n];
    if ( op == NULL ) { // -> ...
        return_var->init( global_data.VarArgs, 0 );
        VarArgs *va = reinterpret_cast<VarArgs *>( return_var->data ); va->init();
        make_varargs_from_disc_desc( va->variables.new_elem(), res, ++n ); // cond
        make_varargs_from_disc_desc( va->variables.new_elem(), res, ++n ); // subs 0
        make_varargs_from_disc_desc( va->variables.new_elem(), res, ++n ); // subs 1
    }
    else {
        return_var->init( global_data.Op, 0 );
        *reinterpret_cast<Op **>(return_var->data) = &op->inc_ref();
    }
}

void discontinuities_separation( Thread *th, const void *tok, Variable *return_var, Op *expr, VarArgs &symbols ) {
    if ( not return_var ) return;
    
    // res -> [ NULL cond1, a, NULL, cond2, b, c ]
    SplittedVec<Op *,1024,4096> of;
    Op *z = &Op::new_number( 0 ); of.push_back( z );
    Op *o = &Op::new_number( 1 ); of.push_back( o );
    
    SplittedVec<Op *,32> res;
    res.push_back( expr );
    get_discontinuities_rec( th, tok, expr, res, symbols, of, z, o );
        
    // return_var -> [ [...] ]
    unsigned n = 0;
    make_varargs_from_disc_desc( return_var, res, n );
    
    // free
    for(unsigned i=0;i<of.size();++i) dec_ref( of[i] );
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------
Op &mod( Op &a, Op &b ) {
    if ( are_numbers(a,b) ) return Op::new_number( mod( a.number_data()->val, b.number_data()->val ) );
    if ( a.is_zero() ) return a.inc_ref();
    return Op::new_function( STRING_mod_NUM, a, b );
}

Op &atan2( Op &a, Op &b ) {
    if ( are_numbers(a,b) ) return Op::new_number( Rationnal( atan2( Float96(a.number_data()->val), Float96(b.number_data()->val) ) ) );
    return Op::new_function( STRING_atan2_NUM, a );
}

Op &abs( Op &a ) {
    if ( a.type == Op::NUMBER ) return Op::new_number( abs( a.number_data()->val ) );
    if ( a.necessary_positive_or_null() ) return a.inc_ref();
    return Op::new_function( STRING_abs_NUM, a );
}

Op &log      ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( log_96   ( a.number_data()->val ) ); return Op::new_function( STRING_log_NUM      , a ); }
Op &heaviside( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( heaviside( a.number_data()->val ) ); if ( a.necessary_positive_or_null() ) return Op::new_number(1); return Op::new_function( STRING_heaviside_NUM, a ); }
Op &eqz      ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( eqz      ( a.number_data()->val ) ); return Op::new_function( STRING_eqz_NUM      , a ); }
Op &exp      ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( exp_96   ( a.number_data()->val ) ); return Op::new_function( STRING_exp_NUM      , a ); }
Op &sin      ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( sin_96   ( a.number_data()->val ) ); return Op::new_function( STRING_sin_NUM      , a ); }
Op &cos      ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( cos_96   ( a.number_data()->val ) ); return Op::new_function( STRING_cos_NUM      , a ); }
Op &tan      ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( tan_96   ( a.number_data()->val ) ); return Op::new_function( STRING_tan_NUM      , a ); }
Op &asin     ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( asin_96  ( a.number_data()->val ) ); return Op::new_function( STRING_asin_NUM     , a ); }
Op &acos     ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( acos_96  ( a.number_data()->val ) ); return Op::new_function( STRING_acos_NUM     , a ); }
Op &atan     ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( atan_96  ( a.number_data()->val ) ); return Op::new_function( STRING_atan_NUM     , a ); }
Op &sinh     ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( sinh_96  ( a.number_data()->val ) ); return Op::new_function( STRING_sinh_NUM     , a ); }
Op &cosh     ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( cosh_96  ( a.number_data()->val ) ); return Op::new_function( STRING_cosh_NUM     , a ); }
Op &tanh     ( Op &a ) { if ( a.type == Op::NUMBER ) return Op::new_number( tanh_96  ( a.number_data()->val ) ); return Op::new_function( STRING_tanh_NUM     , a ); }


