#ifndef OPWITHSEQASMGENERATOR_H
#define OPWITHSEQASMGENERATOR_H

#include "splittedvec.h"

struct OpWithSeqAsmGenerator {
    typedef enum AsmType {
        X86,
        X86_64
    };
    
    OpWithSeqAsmGenerator( AsmType asm_type ) : asm_type( asm_type ) {
        num_instr = 0;
        nb_ops = 0;
    }
    
    void write_ret_instr() {
        os.push_back( 0xB8 ); os.push_back( 0x01 ); os.push_back( 0x00 ); os.push_back( 0x00 ); os.push_back( 0x00 );
        os.push_back( 0xBB ); os.push_back( 0x01 ); os.push_back( 0x00 ); os.push_back( 0x00 ); os.push_back( 0x00 );
        os.push_back( 0xCD ); os.push_back( 0x80 );
        os.push_back( 0xC3 );
    }
    
    void write_instr( OpWithSeq *op ) {
        if ( op->type == OpWithSeq::SEQ )
            return;
        //if ( op->type == OpWithSeq::NUMBER )
        //    return;
            
        ++num_instr;
        nb_ops += op->nb_instr();
        
        //
//         if ( op->type == OpWithSeq::WRITE_ADD      ) { os                               << op->cpp_name_str << " += R" << op->children[0]->reg << "; "; return; }
//         if ( op->type == OpWithSeq::WRITE_REASSIGN ) { os                               << op->cpp_name_str <<  " = R" << op->children[0]->reg << "; "; return; }
//         if ( op->type == OpWithSeq::WRITE_RET      ) { os << "return R"                                                << op->children[0]->reg << "; "; return; }
//         if ( op->type == OpWithSeq::WRITE_INIT     ) { os << ty(op->children[0]) << " " << op->cpp_name_str <<  " = R" << op->children[0]->reg << "; "; return; }
//         
//         // we will need a register
//         op->reg = find_reg();
//         os << ty( op ) << " R" << op->reg << " = ";
//         
//         //
//         if ( op->type == OpWithSeq::NUMBER ) {
//             os.precision(16);
//             os << op->num / op->den;
//         } else if ( op->type == OpWithSeq::SYMBOL ) {
//             os << op->cpp_name_str;
//         } else if ( op->type == OpWithSeq::INV ) {
//             os << "1/R" << op->children[0]->reg;
//         } else if ( op->type == OpWithSeq::NEG ) {
//             os << "-R" << op->children[0]->reg;
//         } else if ( op->type == STRING_add_NUM ) {
//             for(unsigned i=0;i<op->children.size();++i)
//                 os << ( i ? "+R" : "R" ) << op->children[i]->reg;
//         } else if ( op->type == STRING_sub_NUM ) {
//             os << "R" << op->children[0]->reg << "-R" << op->children[1]->reg;
//         } else if ( op->type == STRING_div_NUM ) {
//             os << "R" << op->children[0]->reg << "/R" << op->children[1]->reg;
//         } else if ( op->type == STRING_mul_NUM ) {
//             for(unsigned i=0;i<op->children.size();++i)
//                 os << ( i ? "*R" : "R" ) << op->children[i]->reg;
//         } else if ( op->type == STRING_select_symbolic_NUM ) {
//             if ( op->children[1]->nb_simd_terms > 1 ) {
//                 os << S << "(";
//                 for(int i=0;i<op->children[1]->nb_simd_terms;++i)
//                     os << (i?",":"") << op->children[0]->cpp_name_str << "[R" << op->children[1]->reg << "[" << i << "]]";
//                 os << ")";
//             } else
//                 os << op->children[0]->cpp_name_str << "[R" << op->children[1]->reg << "]";
//         } else {
//             os << Nstring( op->type );
//             os << "(";
//             for(unsigned i=0;i<op->children.size();++i)
//                 os << ( i ? ",R" : "R" ) << op->children[i]->reg;
//             os << ")";
//         }
//         os << "; ";
    }
    
    AsmType asm_type;
    SplittedVec<unsigned char,1024,1024> os;
    unsigned num_instr, nb_ops;
};

#endif // OPWITHSEQASMGENERATOR_H

