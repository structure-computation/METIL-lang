#ifndef OPWITHSEQASMGENERATOR_H
#define OPWITHSEQASMGENERATOR_H

#include "splittedvec.h"
#include <sys/mman.h>
#define DEBUG_ASM

struct OpWithSeqAsmGenerator {
    enum AsmType {
        X86,
        X86_64
    };
    static const int T_size = sizeof( Float64 ); //
    
    OpWithSeqAsmGenerator( AsmType asm_type ) : asm_type( asm_type ) {
        num_instr = 0;
        nb_ops = 0;
        nb_regs = 8 + 8 * ( asm_type == X86_64 );
        for(unsigned i=0;i<nb_regs;++i)
            regs[ i ] = NULL;
        stack_size = NULL;
    }
    
    void push_header() {
        write_save_registers_from_stack();
        stack_size = write_sub_stack_pointer();
    }

    void push_footer() {
        #ifdef DEBUG_ASM
            std::cout << "    ; stack size = " << stack.size() * T_size << std::endl;
        #endif
        
        *stack_size = stack.size() * T_size;
        *write_add_stack_pointer() = stack.size() * T_size;
        
        write_load_registers_from_stack();
        write_ret_instr();
    }
    
    int get_free_room_in_stack() {
        int room_in_stack = -1;
        for(unsigned i=0;i<stack.size();++i) {
            if ( not stack[i] ) {
                room_in_stack = i;
                break;
            }
        }
        if ( room_in_stack < 0 ) {
            room_in_stack = stack.size();
            stack.push_back( NULL );
        }
        return room_in_stack;
    }
    
    int get_free_reg( OpWithSeq *op, int forbidden_register = -1 ) {
        // if there's already a free reg in children of op
        if ( op ) {
            if ( op->type == STRING_sub_NUM or op->type == STRING_div_NUM ) {
                if ( not regs[ op->children[0]->reg ] )
                    return op->children[0]->reg;
                forbidden_register = op->children[1]->reg;
            } else if ( op->children.size() >= 1 ) {
                for(unsigned i=0;i<op->children.size();++i)
                    if ( not regs[ op->children[i]->reg ] )
                        return op->children[i]->reg;
            }
        }
    
        // if there's already a free reg
        for(unsigned i=0;i<nb_regs;++i)
            if ( regs[ i ] == NULL and i != forbidden_register )
                return i;
        
        // else, if there's already a reg saved in stack
        int best_parent_date = -1, best_reg = -1;
        for(unsigned i=0;i<nb_regs;++i) {
            if ( regs[ i ]->stack >= 0 and best_parent_date < regs[ i ]->min_parent_date() ) {
                best_parent_date = regs[ i ]->min_parent_date();
                best_reg = i;
            }
        }
        if ( best_reg >= 0 )  {
            regs[ best_reg ]->reg = -1;
            return best_reg;
        }
        
        // else, select a given register to be saved in stack
        best_parent_date = regs[ 0 ]->min_parent_date();
        best_reg = 0;
        for(unsigned i=1;i<nb_regs;++i) {
            if ( best_parent_date < regs[ i ]->min_parent_date() ) {
                best_parent_date = regs[ i ]->min_parent_date();
                best_reg = i;
            }
        }
        // get a room to save it, and save it
        int room_in_stack = get_free_room_in_stack();
        stack[ room_in_stack ] = regs[ best_reg ];
        regs[ best_reg ]->stack = room_in_stack;
        regs[ best_reg ]->reg = -1;
        write_save_xmm_in_stack_plus_offset( best_reg, room_in_stack * T_size );
        
        return best_reg;
    }
    
    void write_instr( Thread *th, const void *tok, OpWithSeq *op ) {
        if ( op->type == OpWithSeq::SEQ )
            return;
        
        ++num_instr;
        nb_ops += op->nb_instr();
        
        // free operands with only one remaining parent
        for(unsigned i=0;i<op->children.size();++i) {
            if ( ++op->children[i]->nb_times_used == op->children[i]->parents.size() ) {
                if ( op->children[i]->stack >= 0 )
                    stack[ op->children[i]->stack ] = NULL;
                if ( op->children[i]->reg >= 0 )
                    regs[ op->children[i]->reg ] = NULL;
            }
        }
        
        //
        if ( op->type == OpWithSeq::WRITE_REASSIGN ) {
            write_save_xmm_in_memory( op->children[0]->reg, op->ptr_res );
            return;
        }
        if ( op->type == OpWithSeq::WRITE_ADD      ) {
            assert( 0 /*TODO*/ );
            return;
        }
        if ( op->type == OpWithSeq::WRITE_RET      ) {
            assert( 0 /*TODO*/ );
            return;
        }
        if ( op->type == OpWithSeq::WRITE_INIT     ) {
            assert( 0 /*TODO*/ );
            return;
        }
        
        #ifdef DEBUG_ASM
            if ( op->type == OpWithSeq::NUMBER )
                std::cout << "    ; " << op->num / op->den << std::endl;
            else if ( op->type == OpWithSeq::NUMBER_M1 )
                std::cout << "    ; ~( 1L << 63 )" << std::endl;
            else if ( op->type == OpWithSeq::SYMBOL )
                std::cout << "    ; " << op->cpp_name_str << std::endl;
            else
                std::cout << "    ; " << Nstring( op->type ) << std::endl;
        #endif
        
        // find a free register
        op->reg = get_free_reg( op );
        regs[ op->reg ] = op;
        
        //
        if ( op->ptr_val ) { // somewhere in memory
            write_save_memory_in_xmm( op->reg, op->ptr_val );
        } else if ( op->type == OpWithSeq::NUMBER ) {
            int room_in_stack = get_free_room_in_stack();
            write_assign_number_to_offset_from_stack_pointer( room_in_stack * T_size, op->num / op->den );
            write_assign_to_reg_from_offset_from_stack_pointer( op->reg, room_in_stack * T_size );
        } else if ( op->type == OpWithSeq::NUMBER_M1 ) {
            int room_in_stack = get_free_room_in_stack();
            write_assign_number_M1_to_offset_from_stack_pointer( room_in_stack * T_size );
            write_assign_to_reg_from_offset_from_stack_pointer( op->reg, room_in_stack * T_size );
        } else if ( op->type == OpWithSeq::SYMBOL ) {
            th->add_error( "There's a remaining non associated symbol (named '" + std::string( op->cpp_name_str ) + "') in expression.", tok );
        } else if ( op->type == STRING_add_NUM or op->type == STRING_mul_NUM ) {
            unsigned char sse2_op = ( op->type == STRING_add_NUM ? 0x58 : 0x59 );
            for(unsigned i=0;;++i) {
                // -> res not in children
                if ( i == op->children.size() ) {
                    write_save_op_in_xmm( op->reg, op->children[ 0 ] );
                    for(unsigned j=1;j<op->children.size();++j)
                        write_self_sse2_op_xmm( op->reg, op->children[ j ], sse2_op );
                    break;
                }
                //
                if ( op->children[i]->reg == op->reg ) {
                    for(unsigned j=0;j<op->children.size();++j)
                        if ( i != j )
                            write_self_sse2_op_xmm( op->reg, op->children[ j ], sse2_op );
                    break;
                }
            }
        } else if ( op->type == STRING_sub_NUM or op->type == STRING_div_NUM ) {
            unsigned char sse2_op = ( op->type == STRING_sub_NUM ? 0x5c : 0x5e );
            write_save_op_in_xmm( op->reg, op->children[ 0 ] ); // if necessary
            write_self_sse2_op_xmm( op->reg, op->children[ 1 ], sse2_op );
        } else if ( op->type == STRING_select_symbolic_NUM ) {
            write_select_symbolic( op->reg, op );
        } else if ( op->type == STRING_pow_NUM ) {
            write_pow_instr( op );
        } else if ( op->type == STRING_sqrt_NUM ) {
            write_self_sse2_op_xmm( op->reg, op->children[ 0 ], 0x51 ); // particular case : op->reg does not have to be initialized
        } else if ( op->type == STRING_heaviside_NUM ) {
            write_heaviside_or_eqz( op->reg, op, 5 ); // >= 0
        } else if ( op->type == STRING_eqz_NUM ) {
            write_heaviside_or_eqz( op->reg, op, 0 ); // == 0
        } else if ( op->type == STRING_pos_part_NUM ) {
            write_pos_part( op->reg, op ); // x_+
        } else if ( op->type == STRING_abs_NUM ) {
            write_save_op_in_xmm( op->reg, op->children[ 0 ] );
            write_andsd( op->reg, op->children[ 1 ] );
        } else {
            std::cout << "TODO: " << Nstring( op->type ) << std::endl;
            assert( 0 );
        }
    }
    
    void *add_ret_and_make_code_as_new_contiguous_data() const {
        //
        unsigned code_size = os.size();
        void *res = malloc( code_size );
        
        os.copy_binary_data_to( (char *)res );
        
        size_t gm1 = getpagesize() - 1;
        size_t beg = (size_t)res & ~gm1;
        size_t end = ( (size_t)res + code_size + gm1 ) & ~gm1;
        
        mprotect( (void *)beg, end - beg, PROT_READ | PROT_WRITE | PROT_EXEC );
        return res;
    }
    
    AsmType asm_type;
    SplittedVec<unsigned char,1024,1024> os;
    unsigned num_instr, nb_ops;
    
    OpWithSeq *regs[16];
    unsigned nb_regs;
    
    SplittedVec<OpWithSeq *,1024,1024> stack;
private:
    void write_pow_instr( OpWithSeq *op ) {
        std::cout << "TODO : pow(x,y)... in asm" << std::endl;
        assert( 0 );
    }
    
    void write_select_symbolic( int reg, OpWithSeq *op ) {
        assert( op->children[0]->ptr_val );
        assert( op->children[1]->type == Op::NUMBER );
        #ifdef DEBUG_ASM
            std::cout << "    mov   rax, " << op->children[0]->ptr_val << std::endl;
            if ( op->children[1]->type == Op::NUMBER )
                std::cout << "    add   rax, " << op->children[1]->val() * T_size << std::endl;
            std::cout << "    mov   xmm" << reg << ", [ rax ]" << std::endl;
        #endif
        os.push_back( 0x48 );
        os.push_back( 0xb8 );
        *reinterpret_cast<void **>( os.get_room_for( sizeof(void *) ) ) = op->children[0]->ptr_val;
        
        if ( op->children[1]->type == Op::NUMBER ) {
            os.push_back( 0x48 );
            os.push_back( 0x05 );
            *reinterpret_cast<size_t *>( os.get_room_for( sizeof(size_t) ) ) = size_t(op->children[1]->val()) * T_size;
        } else {
            assert( 0 );
        }
        
        os.push_back( 0xf2 );
        if ( reg >= 8 )
            os.push_back( 0x44 );
        os.push_back( 0x0f );
        os.push_back( 0x10 );
        os.push_back( 0x00 + 8 * ( reg % 8 ) );
    }
    
    void write_heaviside_or_eqz( int reg, OpWithSeq *op, unsigned char cmp_op ) {
        write_save_op_in_xmm( reg, op->children[ 0 ] );
        write_cmpsd( reg, op->children[1], cmp_op );
        write_andsd( reg, op->children[2] );
    }
    
    void write_pos_part( int reg, OpWithSeq *op ) {
        write_save_op_in_xmm( reg, op->children[ 0 ] );
        OpWithSeq *ch = op->children[ 1 ];
        //
        #ifdef DEBUG_ASM
            std::cout << "    maxsd   xmm" << reg << ", " << ch->asm_str() << std::endl;
        #endif
        os.push_back( 0xf2 );
        if ( reg >= 8 or ch->reg >= 8 )
            os.push_back( 0x40 | 4 * ( reg >= 8 ) | ( ch->reg >= 8 ) );
        os.push_back( 0x0f );
        os.push_back( 0x5f );
        if ( ch->reg >= 0 ) {
            os.push_back( 0xc0 | 8 * ( reg % 8 ) | ( ch->reg % 8 ) );
        } else {
            assert( ch->stack >= 0 );
            os.push_back( 0x84 | 8 * ( reg % 8 ) );
            os.push_back( 0x24 );
            *reinterpret_cast<int *>( os.get_room_for( 4 ) ) = ch->stack * T_size;
        }
    }
    
    void write_cmpsd( int reg, OpWithSeq *ch, unsigned char cmp_op ) {
        #ifdef DEBUG_ASM
            std::cout << "    cmpsd   xmm" << reg << ", " << ch->asm_str() << ", " << int(cmp_op) << std::endl;
        #endif
        os.push_back( 0xf2 );
        if ( reg >= 8 or ch->reg >= 8 )
            os.push_back( 0x40 | 4 * ( reg >= 8 ) | ( ch->reg >= 8 ) );
        os.push_back( 0x0f );
        os.push_back( 0xc2 );
        if ( ch->reg >= 0 ) {
            os.push_back( 0xc0 | 8 * ( reg % 8 ) | ( ch->reg % 8 ) );
        } else {
            assert( ch->stack >= 0 );
            os.push_back( 0x84 | 8 * ( reg % 8 ) );
            os.push_back( 0x24 );
            *reinterpret_cast<int *>( os.get_room_for( 4 ) ) = ch->stack * T_size;
        }
        os.push_back( cmp_op );
    }
    
    void write_andsd( int reg, OpWithSeq *ch ) {
        if ( ch->reg < 0 and ch->stack & 1 ) {
            int n_reg = get_free_reg( NULL, reg );
            write_save_op_in_xmm( n_reg, ch );
            ch->reg = n_reg;
        }
        
        //
        #ifdef DEBUG_ASM
            std::cout << "    andpd   xmm" << reg << ", " << ch->asm_str() << std::endl;
        #endif
        
        //
        os.push_back( 0x66 );
        if ( reg >= 8 or ch->reg >= 8 )
            os.push_back( 0x40 | 4 * ( reg >= 8 ) | ( ch->reg >= 8 ) );
        os.push_back( 0x0f );
        os.push_back( 0x54 );
        if ( ch->reg >= 0 ) {
            os.push_back( 0xc0 | 8 * ( reg % 8 ) | ( ch->reg % 8 ) );
        } else {
            assert( ch->stack >= 0 );
            os.push_back( 0x84 | 8 * ( reg % 8 ) );
            os.push_back( 0x24 );
            *reinterpret_cast<int *>( os.get_room_for( 4 ) ) = ch->stack * T_size;
        }
    }
    
    void write_assign_number_M1_to_offset_from_stack_pointer( int offset_in_stack ) {
        #ifdef DEBUG_ASM
            std::cout << "    mov   rax, ~( 1L << 63 )" << std::endl;
            std::cout << "    mov   [ rsp + " << offset_in_stack << " ], rax" << std::endl;
        #endif
        os.push_back( 0x48 );
        os.push_back( 0xb8 );
        *reinterpret_cast<long long *>( os.get_room_for( 8 ) ) = ~( 1L << 63 );
        
        os.push_back( 0x48 );
        os.push_back( 0x89 );
        os.push_back( 0x84 );
        os.push_back( 0x24 );
        *reinterpret_cast<int *>( os.get_room_for( 4 ) ) = offset_in_stack;
    }
    
    void write_assign_number_to_offset_from_stack_pointer( int offset_in_stack, Float64 val ) {
        #ifdef DEBUG_ASM
            std::cout << "    mov   rax, " << val << std::endl;
            std::cout << "    mov   [ rsp + " << offset_in_stack << " ], rax" << std::endl;
        #endif
        os.push_back( 0x48 );
        os.push_back( 0xb8 );
        *reinterpret_cast<Float64 *>( os.get_room_for( sizeof(Float64) ) ) = val;
        
        os.push_back( 0x48 );
        os.push_back( 0x89 );
        os.push_back( 0x84 );
        os.push_back( 0x24 );
        *reinterpret_cast<int *>( os.get_room_for( 4 ) ) = offset_in_stack;
    }
    
    void write_assign_to_reg_from_offset_from_stack_pointer( int reg, int offset_in_stack ) {
        #ifdef DEBUG_ASM
            std::cout << "    movsd xmm" << reg << ", [ rsp + " << offset_in_stack << " ]" << std::endl;
        #endif
        os.push_back( 0xf2 );
        if ( reg >= 8 )
            os.push_back( 0x44 );
        os.push_back( 0x0f );
        os.push_back( 0x10 );
        os.push_back( 0x84 | ( 8 * ( reg % 8 ) ) );
        os.push_back( 0x24 );
        *reinterpret_cast<int *>( os.get_room_for( 4 ) ) = offset_in_stack;
    }
    
    void write_save_registers_from_stack() {
        static const unsigned char data[] = {
            0x48, 0x81, 0xec, 0x88, 0x00, 0x00, 0x00,      // sub   rsp, 8 * 16
            0xf2, 0x0f, 0x11, 0x04, 0x24,                  // movsd [ rsp + 0 * 8 ], xmm0
            0xf2, 0x0f, 0x11, 0x4c, 0x24, 0x08,            // movsd [ rsp + 1 * 8 ], xmm1
            0xf2, 0x0f, 0x11, 0x54, 0x24, 0x10,            // movsd [ rsp + 2 * 8 ], xmm2
            0xf2, 0x0f, 0x11, 0x5c, 0x24, 0x18,            // movsd [ rsp + 3 * 8 ], xmm3
            0xf2, 0x0f, 0x11, 0x64, 0x24, 0x20,            // movsd [ rsp + 4 * 8 ], xmm4
            0xf2, 0x0f, 0x11, 0x6c, 0x24, 0x28,            // movsd [ rsp + 5 * 8 ], xmm5
            0xf2, 0x0f, 0x11, 0x74, 0x24, 0x30,            // movsd [ rsp + 6 * 8 ], xmm6
            0xf2, 0x0f, 0x11, 0x7c, 0x24, 0x38,            // movsd [ rsp + 7 * 8 ], xmm7
            0xf2, 0x44, 0x0f, 0x11, 0x44, 0x24, 0x40,      // movsd [ rsp + 8 * 8 ], xmm8
            0xf2, 0x44, 0x0f, 0x11, 0x4c, 0x24, 0x48,      // movsd [ rsp + 9 * 8 ], xmm9
            0xf2, 0x44, 0x0f, 0x11, 0x54, 0x24, 0x50,      // movsd [ rsp + 10* 8 ], xmm10
            0xf2, 0x44, 0x0f, 0x11, 0x5c, 0x24, 0x58,      // movsd [ rsp + 11* 8 ], xmm11
            0xf2, 0x44, 0x0f, 0x11, 0x64, 0x24, 0x60,      // movsd [ rsp + 12* 8 ], xmm12
            0xf2, 0x44, 0x0f, 0x11, 0x6c, 0x24, 0x68,      // movsd [ rsp + 13* 8 ], xmm13
            0xf2, 0x44, 0x0f, 0x11, 0x74, 0x24, 0x70,      // movsd [ rsp + 14* 8 ], xmm14
            0xf2, 0x44, 0x0f, 0x11, 0x7c, 0x24, 0x78,      // movsd [ rsp + 15* 8 ], xmm15
            0x48, 0x89, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00 // mov    %rax,0x80(%rsp)
        };
        const unsigned data_size = 0x76;
        for(unsigned i=0;i<data_size;++i)
            os.push_back( data[ i ] );
    }
    void write_load_registers_from_stack() {
        static const unsigned char data[] = {
            0xf2, 0x0f, 0x10, 0x04, 0x24,                   // movsd xmm0 , [ rsp + 0 * 8 ]
            0xf2, 0x0f, 0x10, 0x4c, 0x24, 0x08,             // movsd xmm1 , [ rsp + 1 * 8 ]
            0xf2, 0x0f, 0x10, 0x54, 0x24, 0x10,             // movsd xmm2 , [ rsp + 2 * 8 ]
            0xf2, 0x0f, 0x10, 0x5c, 0x24, 0x18,             // movsd xmm3 , [ rsp + 3 * 8 ]
            0xf2, 0x0f, 0x10, 0x64, 0x24, 0x20,             // movsd xmm4 , [ rsp + 4 * 8 ]
            0xf2, 0x0f, 0x10, 0x6c, 0x24, 0x28,             // movsd xmm5 , [ rsp + 5 * 8 ]
            0xf2, 0x0f, 0x10, 0x74, 0x24, 0x30,             // movsd xmm6 , [ rsp + 6 * 8 ]
            0xf2, 0x0f, 0x10, 0x7c, 0x24, 0x38,             // movsd xmm7 , [ rsp + 7 * 8 ]
            0xf2, 0x44, 0x0f, 0x10, 0x44, 0x24, 0x40,       // movsd xmm8 , [ rsp + 8 * 8 ]
            0xf2, 0x44, 0x0f, 0x10, 0x4c, 0x24, 0x48,       // movsd xmm9 , [ rsp + 9 * 8 ]
            0xf2, 0x44, 0x0f, 0x10, 0x54, 0x24, 0x50,       // movsd xmm10, [ rsp + 10* 8 ]
            0xf2, 0x44, 0x0f, 0x10, 0x5c, 0x24, 0x58,       // movsd xmm11, [ rsp + 11* 8 ]
            0xf2, 0x44, 0x0f, 0x10, 0x64, 0x24, 0x60,       // movsd xmm12, [ rsp + 12* 8 ]
            0xf2, 0x44, 0x0f, 0x10, 0x6c, 0x24, 0x68,       // movsd xmm13, [ rsp + 13* 8 ]
            0xf2, 0x44, 0x0f, 0x10, 0x74, 0x24, 0x70,       // movsd xmm14, [ rsp + 14* 8 ]
            0xf2, 0x44, 0x0f, 0x10, 0x7c, 0x24, 0x78,       // movsd xmm15, [ rsp + 15* 8 ]
            0x48, 0x8b, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00, // mov    0x80(%rsp),%rax
            0x48, 0x81, 0xc4, 0x88, 0x00, 0x00, 0x00        // add   rsp, 8 * 16
        };
        const unsigned data_size = 0x76;
        for(unsigned i=0;i<data_size;++i)
            os.push_back( data[ i ] );
    }
    
    int *write_sub_stack_pointer() {
        // sub rsp, stack_size
        os.push_back( 0x48 ); os.push_back( 0x81 ); os.push_back( 0xec );
        return reinterpret_cast<int *>( os.get_room_for( 4 ) );
    }
    
    int *write_add_stack_pointer() {
        // add rsp, ...
        os.push_back( 0x48 ); os.push_back( 0x81 ); os.push_back( 0xc4 );
        return reinterpret_cast<int *>( os.get_room_for( 4 ) );
    }
    
    void write_ret_instr() {
        os.push_back( 0xc3 );
    }
    
    void write_exit_code() {
        os.push_back( 0x31 ); os.push_back( 0xff );
        os.push_back( 0xb8 ); os.push_back( 0x3c ); os.push_back( 0x00 ); os.push_back( 0x00 ); os.push_back( 0x00 );
        os.push_back( 0x0f ); os.push_back( 0x05 );
    }
    
    
    void write_save_xmm_in_memory( int reg, void *ptr_res ) {
        #ifdef DEBUG_ASM
            std::cout << "    mov   rax, " << ptr_res << std::endl;
            std::cout << "    movsd [ rax ], xmm" << reg << std::endl;
        #endif
        
        os.push_back( 0x48 );
        os.push_back( 0xb8 );
        *reinterpret_cast<void **>( os.get_room_for( sizeof(void *) ) ) = ptr_res;
        
        os.push_back( 0xf2 );
        if ( reg >= 8 )
            os.push_back( 0x44 );
        os.push_back( 0x0f );
        os.push_back( 0x11 );
        os.push_back( 8 * ( reg % 8 ) );
    }
    
    void write_save_xmm_in_stack_plus_offset( int reg, int offset ) {
        #ifdef DEBUG_ASM
            std::cout << "    movsd  [ rsp + " << offset << " ], xmm" << reg << std::endl;
        #endif
        os.push_back( 0xf2 );
        if ( reg >= 8 )
            os.push_back( 0x44 );
        os.push_back( 0x0f );
        os.push_back( 0x11 );
        os.push_back( 0x84 | ( 8 * ( reg % 8 ) ) );
        os.push_back( 0x24 );
        *reinterpret_cast<int *>( os.get_room_for( 4 ) ) = offset;
    }
    
    void write_save_memory_in_xmm( int reg, void *ptr_val ) {
        #ifdef DEBUG_ASM
            std::cout << "    mov   rax, " << ptr_val << std::endl;
            std::cout << "    movsd xmm" << reg << ", [ rax ]" << std::endl;
        #endif
        
        os.push_back( 0x48 );
        os.push_back( 0xb8 );
        *reinterpret_cast<void **>( os.get_room_for( sizeof(void *) ) ) = ptr_val;
        
        os.push_back( 0xf2 );
        if ( reg >= 8 )
            os.push_back( 0x44 );
        os.push_back( 0x0f );
        os.push_back( 0x10 );
        os.push_back( 8 * ( reg % 8 ) );
    }
    
    void write_copy_xmm_regs( int reg0, int reg1 ) {
        #ifdef DEBUG_ASM
            std::cout << "    movsd   xmm" << reg0 << ", xmm" << reg1 << std::endl;
        #endif
        /*
            f2 0f 10 c0             movsd  xmm0, xmm0
            f2 41 0f 10 c0          movsd  xmm0, xmm8
            f2 44 0f 10 c0          movsd  xmm8, xmm0
            f2 45 0f 10 c0          movsd  xmm8, xmm8
        */
        os.push_back( 0xf2 );
        if ( reg0 >= 8 or reg1 >= 8 )
            os.push_back( 0x40 | 4 * ( reg0 >= 8 ) | ( reg1 >= 8 ) );
        os.push_back( 0x0f );
        os.push_back( 0x10 );
        os.push_back( 0xc0 | 8 * ( reg0 % 8 ) | ( reg1 % 8 ) );
    }
    
    void write_save_stack_in_xmm( int reg, int stack_offset ) {
        #ifdef DEBUG_ASM
            std::cout << "    movsd   xmm" << reg << ", [ rsp + " << stack_offset << " ]" << std::endl;
        #endif
        os.push_back( 0xf2 );
        if ( reg >= 8 )
            os.push_back( 0x44 );
        os.push_back( 0x0f );
        os.push_back( 0x10 );
        os.push_back( 0x84 | ( 8 * ( reg % 8 ) ) );
        os.push_back( 0x24 );
        *reinterpret_cast<int *>( os.get_room_for( 4 ) ) = stack_offset;
    }
    
    void write_save_op_in_xmm( int reg, OpWithSeq *ch ) {
        if ( reg == ch->reg )
            return;
        //
        if ( ch->reg >= 0 )
            write_copy_xmm_regs( reg, ch->reg );
        else
            write_save_stack_in_xmm( reg, ch->stack * T_size );
    }

    void write_self_sse2_op_xmm( int reg, OpWithSeq *ch, unsigned char sse2_op ) {
        #ifdef DEBUG_ASM
            switch ( sse2_op ) {
                case 0x58: std::cout << "    add"; break;
                case 0x59: std::cout << "    mul"; break;
                case 0x5c: std::cout << "    sub"; break;
                case 0x5e: std::cout << "    div"; break;
                case 0x51: std::cout << "    sqrt"; break;
            }
            std::cout << "sd xmm" << reg << ", ";
            if ( ch->reg >= 0 ) std::cout << "xmm" << ch->reg << std::endl;
            else                std::cout << "[ rsp + " << ch->stack * T_size << " ]" << std::endl;
        #endif
        /*
             f2 0f 58 c0                 addsd  xmm0, xmm0
             f2 0f 58 c1                 addsd  xmm0, xmm1
             f2 0f 58 c9                 addsd  xmm1, xmm1
             f2 41 0f 58 c0              addsd  xmm0, xmm8
             f2 44 0f 58 c0              addsd  xmm8, xmm0
             f2 45 0f 58 c0              addsd  xmm8, xmm8
             f2 0f 58 c0                 addsd  xmm0, xmm0
             f2 0f 58 84 24 11 11 00 00  addsd  xmm0, [ rsp + 0x1111 ]
             f2 0f 58 8c 24 11 11 00 00  addsd  xmm1, [ rsp + 0x1111 ]
             f2 44 0f 58 84 24 11        addsd  xmm8, [ rsp + 0x1111 ]
        */
        os.push_back( 0xf2 );
        if ( reg >= 8 or ch->reg >= 8 )
            os.push_back( 0x40 | 4 * ( reg >= 8 ) | ( ch->reg >= 8 ) );
        os.push_back( 0x0f );
        os.push_back( sse2_op );
        if ( ch->reg >= 0 )
            os.push_back( 0xc0 | 8 * ( reg % 8 ) | ( ch->reg % 8 ) );
        else {
            assert( ch->stack >= 0 );
            os.push_back( 0x84 | 8 * ( reg % 8 ) );
            os.push_back( 0x24 );
            *reinterpret_cast<int *>( os.get_room_for( 4 ) ) = ch->stack * T_size;
        }
    }
    
    int *stack_size; // sub rsp, $stack_size at the beginning of the code
};

#endif // OPWITHSEQASMGENERATOR_H

