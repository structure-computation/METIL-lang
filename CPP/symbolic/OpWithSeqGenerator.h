#ifndef OPWITHSEQGENERATOR_H
#define OPWITHSEQGENERATOR_H

struct OpWithSeqGenerator {
    OpWithSeqGenerator( unsigned nb_sp, const char *T, const char *S ) : beg_line( nb_sp, ' ' ), T( T ), S( S ) {
        num_instr = 0;
        nb_ops = 0;
        nb_reg = 0;
    }
    
    //     std::string R( OpWithSeq *op ) {
    //         std::ostringstream ss;
    //         //         if ( op->type == OpWithSeq::NUMBER ) {
    //         //             if ( strcmp(T,"Ex") == 0 ) {
    //         //                 bool neg = op->val() < 0;
    //         //                 if ( op->den != 1 )
    //         //                     ss << "Rationnal(" << op->num << "," << op->den << ")";
    //         //                 else
    //         //                     ss << ( neg ? "(" : "" ) << op->val() << ( neg ? ")" : "" );
    //         //             }
    //         //             else {
    //         //                 bool neg = op->val() < 0, den = ( op->den != 1 );
    //         //                 if ( den )
    //         //                     ss << ( neg ? "(" : "" ) << op->num << "/" << T << "(" << op->den << ")" << ( neg ? ")" : "" );
    //         //                 else
    //         //                     ss << ( neg ? "(" : "" ) << op->num << ( neg ? ")" : "" );
    //         //             }
    //         //         }
    //         //         else
    //         ss << "R" << op->reg;
    //         return ss.str();
    //     }
    
    void write_instr( OpWithSeq *op ) {
        if ( op->type == OpWithSeq::SEQ )
            return;
        //if ( op->type == OpWithSeq::NUMBER )
        //    return;
            
        //
        if ( num_instr % 6 == 0 ) {
            if ( num_instr )
                os << "\n";
            os << beg_line;
        }
        ++num_instr;
        nb_ops += op->nb_instr();
        
        //
        if ( op->type == OpWithSeq::WRITE_ADD      ) { os                                     << op->cpp_name_str << " += R" << op->children[0]->reg << "; "; return; }
        if ( op->type == OpWithSeq::WRITE_INIT     ) { os << (op->nb_simd_terms>1?S:T) << " " << op->cpp_name_str <<  " = R" << op->children[0]->reg << "; "; return; }
        if ( op->type == OpWithSeq::WRITE_REASSIGN ) { os                                     << op->cpp_name_str <<  " = R" << op->children[0]->reg << "; "; return; }
        if ( op->type == OpWithSeq::WRITE_RET      ) { os << "return R"                                                      << op->children[0]->reg << "; "; return; }
        
        // we will need a register
        op->reg = find_reg();
        os << ( op->nb_simd_terms > 1 ? S : T ) << " R" << op->reg << " = ";
        
        //
        if ( op->type == OpWithSeq::NUMBER ) {
            os.precision(16);
            os << op->num / op->den;
        } else if ( op->type == OpWithSeq::SYMBOL ) {
            os << op->cpp_name_str;
        } else if ( op->type == OpWithSeq::INV ) {
            os << "1/R" << op->children[0]->reg;
        } else if ( op->type == OpWithSeq::NEG ) {
            os << "-R" << op->children[0]->reg;
        } else if ( op->type == STRING_add_NUM ) {
            for(unsigned i=0;i<op->children.size();++i)
                os << ( i ? "+R" : "R" ) << op->children[i]->reg;
        } else if ( op->type == STRING_sub_NUM ) {
            os << "R" << op->children[0]->reg << "-R" << op->children[1]->reg;
        } else if ( op->type == STRING_div_NUM ) {
            os << "R" << op->children[0]->reg << "/R" << op->children[1]->reg;
        } else if ( op->type == STRING_mul_NUM ) {
            for(unsigned i=0;i<op->children.size();++i)
                os << ( i ? "*R" : "R" ) << op->children[i]->reg;
        } else {
            os << Nstring( op->type );
            os << "(";
            for(unsigned i=0;i<op->children.size();++i)
                os << ( i ? ",R" : "R" ) << op->children[i]->reg;
            os << ")";
        }
        os << "; ";
    }
    
    int find_reg() { return nb_reg++; }
    
    std::ostringstream os;
    std::string beg_line;
    const char *T, *S;
    unsigned num_instr, nb_ops;
    int nb_reg;
};

#endif // OPWITHSEQGENERATOR_H

