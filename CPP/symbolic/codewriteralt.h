#ifndef CODE_WRITER_ALT_H
#define CODE_WRITER_ALT_H

#include "typeconfig.h"
#include "splittedvec.h"
#include "variable.h"
#include "symbolic/ex.h"
#include <string>
#include <map>
#include <simplevector.h>

struct Thread;
struct Variable;
struct Definition;

struct CodeWriterAlt {
    void init( const char *s, Int32 si );
    void init( CodeWriterAlt &c );
    void reassign( Thread *th, const void *tok, CodeWriterAlt &c );
    void add_expr( Thread *th, const void *tok, Variable *str, const Ex &expr, Definition &b );
    void add_expr( const Ex &op, Nstring method, char *name );
    std::string to_string( Thread *th, const void *tok, Int32 nb_spaces );
    
    std::string invariant( Thread *th, const void *tok, Int32 nb_spaces, const VarArgs &variables );
    
    operator bool() const { return true; }
    
    struct OpToWrite {
        Ex ex; 
        Nstring method;
        char *name;
    };
    struct NumberToWrite {
        Rationnal n;
        Nstring method;
        char *name;
    };
    
    SplittedVec<OpToWrite    ,32> op_to_write;
    SplittedVec<NumberToWrite,32> nb_to_write;
    
    bool has_init_methods;
    char *basic_type;
private:
    bool want_float;
};
    
    
void destroy( Thread *th, const void *tok, CodeWriterAlt &c );
    
    
#endif // CODE_WRITER_ALT_H
