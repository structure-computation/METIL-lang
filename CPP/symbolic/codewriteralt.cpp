#include "codewriteralt.h"
#include "definition.h"
#include "definitiondata.h"
#include "transientdata.h"
#include "globaldata.h"
#include "op.h"
#include <usual_strings.h>
#include "basic_string_manip.h"
#include "OpWithSeq.h"
#include "OpWithSeqGenerator.h"
#include <iostream>

void CodeWriterAlt::init( const char *s, Int32 si ) {
    if ( si )
        basic_type = strdupp0( s, si );
    else
        basic_type = NULL;
    
    //
    op_to_write.init();
    nb_to_write.init();
    has_init_methods = false;    
    
    want_float = ( basic_type ? strcmp( basic_type, "float" ) == 0 : 0 );
}

void CodeWriterAlt::init( CodeWriterAlt &c ) {
    std::cout << "TODO " << __FILE__ << " " << __LINE__ << std::endl;
}

void CodeWriterAlt::reassign(  Thread *th, const void *tok, CodeWriterAlt &c ) {
    std::cout << "TODO " << __FILE__ << " " << __LINE__ << std::endl;
}

void CodeWriterAlt::add_expr( Thread *th, const void *tok, Variable *str, const Ex &expr, Definition &b ) {
    const char *s = *reinterpret_cast<const char **>(str->data);
    Int32 si = *reinterpret_cast<const Int32 *>( reinterpret_cast<const char **>(str->data) + 1 );
    if ( b.def_data->name == STRING_init_NUM )
        has_init_methods = true;
        
    if ( expr.op->type == Op::NUMBER ) {
        NumberToWrite *ow = nb_to_write.new_elem();
        init_arithmetic( ow->n, expr.op->number_data()->val );
        ow->method = b.def_data->name;
        ow->name = strdupp0( s, si );
    } else {
        OpToWrite *ow = op_to_write.new_elem();
        ow->ex.init( expr );
        ow->method = b.def_data->name;
        ow->name = strdupp0( s, si );
    }
}

void CodeWriterAlt::add_expr( const Ex &expr, Nstring method, char *name ) {
    if ( expr.op->type == Op::NUMBER ) {
        NumberToWrite *ow = nb_to_write.new_elem();
        init_arithmetic( ow->n, expr.op->number_data()->val );
        ow->method = method;
        ow->name = strdup( name );
    } else {
        OpToWrite *ow = op_to_write.new_elem();
        ow->ex = expr;
        ow->method = method;
        ow->name = strdup( name );
    }
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string CodeWriterAlt::invariant( Thread *th, const void *tok, Int32 nb_spaces, const VarArgs &variables ) {
    return "";
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
void make_OpWithSeq_simple_ordering( OpWithSeq *seq, std::vector<OpWithSeq *> &ordering ) {
    if ( seq->ordering >= 0 )
        return;
    for(unsigned i=0;i<seq->children.size();++i)
        make_OpWithSeq_simple_ordering( seq->children[i], ordering );
    seq->ordering = ordering.size();
    ordering.push_back( seq );
}

std::string CodeWriterAlt::to_string( Thread *th, const void *tok, Int32 nb_spaces ) {
    if ( op_to_write.size() + nb_to_write.size() == 0 )
        return "";
    
    //
    OpWithSeq *seq = new OpWithSeq( OpWithSeq::SEQ );
    ++Op::current_op;
    for(unsigned i=0;i<op_to_write.size();++i) {
        seq->add_child( 
            new OpWithSeq( 
                op_to_write[i].method.v,
                op_to_write[i].name,
                make_OpWithSeq_rec( op_to_write[i].ex.op )
            )
        );
    }
    for(unsigned i=0;i<nb_to_write.size();++i) {
        seq->add_child( 
            new OpWithSeq( 
                nb_to_write[i].method.v,
                nb_to_write[i].name,
                new OpWithSeq( nb_to_write[i].n )
            )
        );
    }
    
    //
    //     std::cout << *seq << std::endl;
    simplifications( seq );
    //     std::cout << *seq << std::endl;
    
    //
    std::vector<OpWithSeq *> ordering;
    make_OpWithSeq_simple_ordering( seq, ordering );
    
    //
    OpWithSeqGenerator g( nb_spaces, basic_type );
    for(unsigned i=0;i<ordering.size();++i)
        g.write_instr( ordering[i] );
    g.os << " /* " << g.num_instr << " instructions */";
    
    //
    delete seq;
    return g.os.str();
}

void destroy( Thread *th, const void *tok, CodeWriterAlt &c ) {
    for(unsigned i=0;i<c.op_to_write.size();++i) {
        free( c.op_to_write[i].name );
        c.op_to_write[i].ex.destroy();
    }
    
    for(unsigned i=0;i<c.nb_to_write.size();++i) {
        free( c.nb_to_write[i].name );
        c.nb_to_write[i].n.destroy();
    }
    
    c.op_to_write.destroy();
    c.nb_to_write.destroy();
    if ( c.basic_type )
        free( c.basic_type );
}
    

