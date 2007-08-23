#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <vector>
#include <string>
#include <set>
#include <map>
#include <sstream>
#include "config.h"

///
struct PrimitiveClass {
    struct Attr {
        std::string name, type, value;
    };
    PrimitiveClass(std::string s, std::string c ) : sar_name(s), cpp_name(c) {
        reassign_using_mem_copy = true;
    }
    PrimitiveClass &avoid_reassign_using_mem_copy() {
        reassign_using_mem_copy = false;
        return *this;
    }
    void push_attr( std::string name, std::string type, std::string value ) {
        Attr attr;
        attr.name  = name;
        attr.type  = type;
        attr.value = value;
        attributes.push_back( attr );
    }
    
    std::string sar_name;
    std::string cpp_name;
    bool reassign_using_mem_copy;
    std::vector<Attr> attributes;
};

///
struct PrimitiveFunction {
    struct Arg { /// Arg
        Arg() {}
        Arg( const char *t) : type(t) {}
        std::string name;
        std::string type;
        std::string cond_of_modification;
    };
    
    PrimitiveFunction( std::string n, std::string c, std::string r="" ) : name(n), code(c), ret(r) {
        pertinence = 10.0;
        varargs = false;
        dont_mind_about_prop = false;
    }
    
    PrimitiveFunction &A( std::string type, std::string cond_of_modification="", std::string name="" ) {
        Arg arg;
        arg.name = name;
        arg.type = type;
        arg.cond_of_modification = cond_of_modification;
        args.push_back( arg );
        return *this;
    }
        
    PrimitiveFunction &M(const std::string &method_of_,const std::string &cond_of_self_modification_="" ) {
        method_of = method_of_;
        cond_of_self_modification = cond_of_self_modification_;
        return *this;
    }
    PrimitiveFunction &set_pertinence( Float64 p ) {
        pertinence = p;
        return *this;
    }
    
    PrimitiveFunction &has_varargs() {
        varargs = true;
        return *this;
    }
    PrimitiveFunction &set_supplementary_cond( std::string s ) {
        supplementary_cond = s;
        return *this;
    }
    PrimitiveFunction &set_dont_mind_about_prop() {
        dont_mind_about_prop = true;    
        return *this;
    }
    
    std::string decl_name() {
        std::ostringstream ss;
        ss << name << "__" << args.size() << "__";
        for(unsigned i=0;i<args.size();++i)
            ss << args[i].type << "__";
        ss << varargs;
        return ss.str();
    }
    
    std::string name, code, ret;
    std::vector<Arg> args;
    Float64 pertinence;
    std::string method_of;
    std::string cond_of_self_modification;
    bool varargs;
    std::string supplementary_cond;
    bool dont_mind_about_prop;
};

bool float_class( const PrimitiveClass &pc ) {
    return pc.sar_name=="Float32" or
           pc.sar_name=="Float64" or
           pc.sar_name=="Float96";
}
bool signed_arithmetic_class( const PrimitiveClass &pc ) {
    return pc.sar_name=="Int8" or 
           pc.sar_name=="Int16" or 
           pc.sar_name=="Int32" or 
           pc.sar_name=="Int64" or 
           pc.sar_name=="Rationnal" or 
           float_class( pc );
}
bool unsigned_arithmetic_class( const PrimitiveClass &pc ) {
    return pc.sar_name=="Bool" or 
           pc.sar_name=="Unsigned8" or 
           pc.sar_name=="Unsigned16" or 
           pc.sar_name=="Unsigned32" or 
           pc.sar_name=="Unsigned64";
}
bool integer_class( const PrimitiveClass &pc ) {
    return pc.sar_name=="Int8" or 
           pc.sar_name=="Int16" or 
           pc.sar_name=="Int32" or 
           pc.sar_name=="Int64" or 
           pc.sar_name=="Unsigned8" or 
           pc.sar_name=="Unsigned16" or 
           pc.sar_name=="Unsigned32" or 
           pc.sar_name=="Unsigned64";
}
std::string signed_correspondance( const PrimitiveClass &pc ) {
    if ( pc.sar_name=="Unsigned8"  ) return "Int8";
    if ( pc.sar_name=="Unsigned16" ) return "Int16";
    if ( pc.sar_name=="Unsigned32" ) return "Int32";
    if ( pc.sar_name=="Unsigned64" ) return "Int64";
    if ( pc.sar_name=="Bool"       ) return "Int8";
    return pc.sar_name;
}

std::string after_op( const PrimitiveClass &pc ) {
    if ( integer_class( pc ) )
        return "Rationnal";
    return pc.sar_name;
}

bool arithmetic_class( const PrimitiveClass &pc ) {
    return signed_arithmetic_class(pc) or unsigned_arithmetic_class(pc);
}

// -----------------------------------------------------------------------------------------------------------------------
inline void get_primitives_File( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "CFile", "CFile" ) );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self.f = ( a==0 ? stdout : ( a==1 ? stderr : stdin ) );" ).M("CFile").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.f = a.f; self.pipe = a.pipe;" ).M("CFile").A("CFile") );
    primitive_functions.push_back( PrimitiveFunction( "write", "self.write( a, b );" ).M("CFile").A("UntypedPtr").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "read", "self.read( a, b );" ).M("CFile").A("UntypedPtr").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "read", "std::string r=self.read(); assign_string(th,tok,return_var,&r[0],r.size());", /*ret*/"manual" ).M("CFile") );
    primitive_functions.push_back( PrimitiveFunction( "close", "self.close();" ).M("CFile") );
    primitive_functions.push_back( PrimitiveFunction( "flush", "self.flush();" ).M("CFile") );
    primitive_functions.push_back( PrimitiveFunction( "destroy", "self.close();" ).M("CFile") );
    primitive_functions.push_back( PrimitiveFunction( "eof", "ret = self.eof();", /*ret*/"Bool" ).M("CFile") );
    primitive_functions.push_back( PrimitiveFunction( "tell", "ret = self.tell();", /*ret*/"Int32" ).M("CFile") );
    primitive_functions.push_back( PrimitiveFunction( "seek", "self.seek(a);" ).M("CFile").A("Int64") );
    primitive_functions.push_back( PrimitiveFunction( "get_size", "ret = self.size();", /*ret*/"Int32" ).M("CFile") );
    
    primitive_functions.push_back( PrimitiveFunction( "open", "ret = self.open_with_str( a, b, false );", /*ret*/"Int32" ).M("CFile").A("any").A("any").set_supplementary_cond("args[0]->type == global_data.String and args[1]->type == global_data.String") );
    
    primitive_functions.push_back( PrimitiveFunction( "popen", "ret = self.open_with_str( a, b, true );", /*ret*/"Int32" ).M("CFile").A("any").A("any").set_supplementary_cond("args[0]->type == global_data.String and args[1]->type == global_data.String") );
    
    primitive_functions.push_back( PrimitiveFunction( "last_modification_time_or_zero_of_file_named", "ret = last_modification_time_or_zero_of_file_named( a );", /*ret*/"Int64" ).A("any").set_supplementary_cond("args[0]->type == global_data.String") );
    
    primitive_functions.push_back( PrimitiveFunction( "get_next_line_", "get_next_line_( th, tok, a, return_var );", /*ret*/"manual" ).A("CFile") );
}

inline void get_primitives_UntypedPtr( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "UntypedPtr", "UntypedPtr" ) );
    
    primitive_functions.push_back( PrimitiveFunction( "untyped_ptr_on", "ret = a->data;", /*ret*/"UntypedPtr" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "untyped_ptr_on_type_of", "ret = (void *)a->type;", /*ret*/"UntypedPtr" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "ptr_on_type_to_def", "return get_def_from_type( th, tok, sp, ret, (Type *)a );", /*ret*/"Def" ).A("UntypedPtr") );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "init_arithmetic( self, bool(a) );" ).M("Bool").A("UntypedPtr") );
    
    primitive_functions.push_back( PrimitiveFunction( "free", "if ( self ) free( self );" ).M("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self  = NULL;" ).M("UntypedPtr",/*modify*/"true") );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self = a;" ).M("UntypedPtr",/*modify*/"true").A("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "self = a;" ).M("UntypedPtr",/*modify*/"true").A("UntypedPtr") );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self = (void *)a;" ).M("UntypedPtr",/*modify*/"true").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "self = (void *)a;" ).M("UntypedPtr",/*modify*/"true").A("Int32") );
    
    primitive_functions.push_back( PrimitiveFunction( "equal", "ret = (a==b);", /*ret*/"Bool" ).A("UntypedPtr").A("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "inferior", "ret = (a<b);", /*ret*/"Bool" ).A("UntypedPtr").A("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "superior", "ret = (a>b);", /*ret*/"Bool" ).A("UntypedPtr").A("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "inferior_equal", "ret = (a<=b);", /*ret*/"Bool" ).A("UntypedPtr").A("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "superior_equal", "ret = (a>=b);", /*ret*/"Bool" ).A("UntypedPtr").A("UntypedPtr") );
    
    primitive_functions.push_back( PrimitiveFunction( "__get_pointed_value__", "return __get_data_in_dyn_vec__( th, tok, sp, return_var, a, args[0], b, 0 );", /*ret*/"manual" ).A("UntypedPtr").A("any") );
    primitive_functions.push_back( PrimitiveFunction( "__get_data_in_dyn_vec__", "return __get_data_in_dyn_vec__( th, tok, sp, return_var, a, args[0], b, c );", /*ret*/"manual" ).A("UntypedPtr").A("any").A("Int32") );
    
    primitive_functions.push_back( PrimitiveFunction( "add", "ret = (char *)a + b;", /*ret*/"UntypedPtr" ).A("UntypedPtr").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "sub", "ret = (const char *)a - (const char *)b;", /*ret*/"Int32" ).A("UntypedPtr").A("UntypedPtr") );
    
    // static vec
    primitive_functions.push_back( PrimitiveFunction( "__uninitialised_vec_using_partial_instanciation__", "uninitialised_vec_using_partial_instanciation( th, tok, sp, return_var, a, b );", /*ret*/"manual" ).A("any").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "__get_data_in_static_vec__", "return __get_data_in_static_vec__( th, tok, sp, return_var, a, b );", /*ret*/"manual" ).A("any").A("Int32") );
    
    primitive_functions.push_back( PrimitiveFunction( "allocate", "ret = malloc( (a+7)/8 * b );", /*ret*/"UntypedPtr" ).A("Int32").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "memcpy", "memcpy( a, b, (c+7)/8 * d );" ).A("UntypedPtr").A("UntypedPtr").A("Int32").A("Int32") );
    
    primitive_functions.push_back( PrimitiveFunction( "conversion_to", "a = (Int32)self;" ).M("UntypedPtr").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "conversion_to", "a = (Int64)self;" ).M("UntypedPtr").A("Int64") );
}

inline void get_primitives_Property( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "Property", "Property" ).avoid_reassign_using_mem_copy() );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "return property_init_or_reassign( self_var, self, th, tok, args, sp, self.init_(), STRING_init_NUM );" ).M("Property").has_varargs() );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "return property_init_or_reassign( self_var, self, th, tok, args, sp, self.reassign_(), STRING_reassign_NUM );" ).M("Property").has_varargs() );
    
    primitive_functions.push_back( PrimitiveFunction( "destroy", "return destroy( self, th, sp );" ).M("Property") );
    
    primitive_functions.push_back( PrimitiveFunction( "__has_type_of_function__", "ret = (self.type_of_().v!=0);", /*ret*/"Bool" ).M("Property") );
    primitive_functions.push_back( PrimitiveFunction( "__get_type_of_function__", "__get_type_of_function__( th, tok, self, return_var );", /*ret*/"manual" ).M("Property") );
    
    primitive_functions.push_back( PrimitiveFunction( "__is_a_Property__", "ret = ( a->type == global_data.Property );", /*ret*/"Bool" ).A("any").set_dont_mind_about_prop() );
}

// VarArgs
inline void get_primitives_VarArgs( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "VarArgs", "VarArgs" ).avoid_reassign_using_mem_copy() );
    
    primitive_functions.push_back( PrimitiveFunction( "destroy", "return destroy( self, th, sp );" ).M("VarArgs") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "destroy( self, th, sp ); self.copy_data_from( &a );" ).A("VarArgs").M("VarArgs",/*modify*/"true") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init();" ).M("VarArgs",/*modify*/"true") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init( &a );" ).A("VarArgs").M("VarArgs",/*modify*/"true") );
    
    primitive_functions.push_back( PrimitiveFunction( "contains_only_named_variables_included_in_self_non_static_variables",
                                   "return varargs_contains_only_named_variables_included_in_self_non_static_variables(th,tok,a,self,ret,sp);", "Bool" ).M("VarArgs").A("any") );
    primitive_functions.push_back( PrimitiveFunction( "init_using_varargs", "return init_using_varargs(th,tok,a,b,sp);" ).A("any").A("VarArgs") );
    
    primitive_functions.push_back( PrimitiveFunction( "call", "return call_using_varargs(th,tok,function,b,sp,NULL);" ).A("any","","function").A("VarArgs").set_dont_mind_about_prop() );
    primitive_functions.push_back( PrimitiveFunction( "call_with_return", "return call_using_varargs(th,tok,function,b,sp,return_var);", /*ret*/"manual" ).A("any","","function").A("VarArgs").set_dont_mind_about_prop() );
    primitive_functions.push_back( PrimitiveFunction( "call_select", "return call_select_using_varargs(th,tok,a,b,sp,return_var);", /*ret*/"manual" ).A("any").A("VarArgs").set_dont_mind_about_prop() );

    primitive_functions.push_back( PrimitiveFunction( "init", "self = (bool)a;" ).A("VarArgs").M("Bool",/*modify*/"true") );
    primitive_functions.push_back( PrimitiveFunction( "nb_unnamed_arguments", "ret = self.nb_uargs();", "Int32" ).M("VarArgs") );
    primitive_functions.push_back( PrimitiveFunction( "nb_named_arguments"  , "ret = self.nb_nargs();", "Int32" ).M("VarArgs") );
    primitive_functions.push_back( PrimitiveFunction( "get_unnamed_argument_nb", "self.get_ref_on_uarg(th,tok,a,return_var);", /*ret*/"manual" ).A("Int32").M("VarArgs") );
    primitive_functions.push_back( PrimitiveFunction( "get_named_argument_nb"  , "self.get_ref_on_narg(th,tok,a,return_var);", /*ret*/"manual" ).A("Int32").M("VarArgs") );
    primitive_functions.push_back( PrimitiveFunction( "get_name_of_named_argument_nb", "if ( return_var ) { std::string s(self.name(a)); assign_string( th, tok, return_var, &s[0], s.size() ); }", /*ret*/"manual" ).A("Int32").M("VarArgs") );
    primitive_functions.push_back( PrimitiveFunction( "push_unnamed"  , "self.push_unnamed(a);" ).A("any").M("VarArgs").set_dont_mind_about_prop() );
    primitive_functions.push_back( PrimitiveFunction( "push_named"  , "self.push_named( *reinterpret_cast<const char **>(a->data), *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1), b ); " ).
            A("any").A("any").M("VarArgs").set_supplementary_cond("args[0]->type == global_data.String").set_dont_mind_about_prop() );
    
    primitive_functions.push_back( PrimitiveFunction( "pointed_value", "if ( return_var ) assign_ref_on( return_var, self.uarg(0) );", /*ret*/"manual" ).M("VarArgs") );
    primitive_functions.push_back( PrimitiveFunction( "select", "self.get_ref_on_uarg(th,tok,a,return_var);", /*ret*/"manual" ).M("VarArgs").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "select", "self.get_ref_on_uarg(th,tok,a,return_var);", /*ret*/"manual" ).M("VarArgs").A("Bool") );
    primitive_functions.push_back( PrimitiveFunction( "get_size", "ret = self.variables.size();", /*ret*/"Int32" ).M("VarArgs") );

    primitive_functions.push_back( PrimitiveFunction( "contains_data_of_same_type_only", "ret = self.contains_data_of_same_type_only(a);", /*ret*/"Bool" ).M("VarArgs").A("Int32") );
    
    primitive_functions.push_back( PrimitiveFunction( "add", "ret.init_from( a, b );", /*ret*/"VarArgs" ).A("VarArgs").A("VarArgs") );
}

// Ref
inline void get_primitives_Ref( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "InternalVariable", "Variable" ).avoid_reassign_using_mem_copy() );
    
    primitive_functions.push_back( PrimitiveFunction( "destroy", "if (self.type) {th->set_current_sourcefile( NULL ); const void *tok_del = destroy_var( th, tok, tok_end_def_block, sp, &self ); if ( tok_del ) return tok_del;}" ).M("InternalVariable") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.type = NULL;" ).M("InternalVariable") );
    primitive_functions.push_back( PrimitiveFunction( "init", "if (a.type) assign_ref_on( &self, &a ); else self.type = NULL;" ).A("InternalVariable").M("InternalVariable",/*modify*/"true") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "if (self.type) {th->check_for_room_in_variable_stack(sp,1); assign_ref_on(sp++,&self);} if (a.type) assign_ref_on(&self,&a); else self.type = NULL;" ).A("InternalVariable").M("InternalVariable",/*modify*/"true") );
    
    primitive_functions.push_back( PrimitiveFunction( "assign_var", "if (self.type) {th->check_for_room_in_variable_stack(sp,1); assign_ref_on(sp++,&self);} assign_ref_on(&self,a);" ).A("any").M("InternalVariable",/*modify*/"true").set_dont_mind_about_prop() );
    primitive_functions.push_back( PrimitiveFunction( "init_using_var", "assign_ref_on(&self,function);" ).A("any","","function").M("InternalVariable",/*modify*/"true").set_dont_mind_about_prop() );
    
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self = (bool)a.type;" ).A("InternalVariable").M("Bool",/*modify*/"true") );
    
    primitive_functions.push_back( PrimitiveFunction( "pointed_value", "if ( return_var ) { if (self.type) assign_ref_on( return_var, &self); else { th->add_error(\"NULL pointer\",tok); assign_error_var(return_var); } }", /*ret*/"manual" ).M("InternalVariable") );
}

// Functionnal
inline void get_primitives_Functionnal( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "Functionnal", "Functionnal" ).avoid_reassign_using_mem_copy() );
    
    primitive_functions.push_back( PrimitiveFunction( "destroy", "return destroy( self, th, sp );" ).M("Functionnal") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "destroy( self, th, sp ); self.copy_data_from( &a );" ).A("Functionnal").M("Functionnal",/*modify*/"true") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init();" ).M("Functionnal") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init( &a );" ).A("Functionnal").M("Functionnal",/*modify*/"true") );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self = (bool)a;" ).A("Functionnal").M("Bool",/*modify*/"true") );
    
    std::string cond = "reinterpret_cast<Variable *>(args[0]->data)->type == global_data.Functionnal";
    primitive_functions.push_back( PrimitiveFunction( "comes_from_an_apply_with_functionnal_arg", "ret = (bool)reinterpret_cast<Functionnal *>(a.data)->comes_from_an_apply_with_functionnal_arg();", /*ret*/"Bool" ).A("InternalVariable"). set_supplementary_cond(cond) );
    primitive_functions.push_back( PrimitiveFunction( "comes_from_a_get_attr_with_functionnal_lhs", "ret = (bool)reinterpret_cast<Functionnal *>(a.data)->comes_from_a_get_attr_with_functionnal_lhs();", /*ret*/"Bool" ).A("InternalVariable"). set_supplementary_cond(cond) );
    primitive_functions.push_back( PrimitiveFunction( "comes_from_a_predefined_functionnal", "ret = (bool)reinterpret_cast<Functionnal *>(a.data)->comes_from_a_predefined_functionnal();", /*ret*/"Bool" ).A("InternalVariable"). set_supplementary_cond(cond) );
    
    primitive_functions.push_back( PrimitiveFunction( "extract_arg_defined_in_functionnal", "if ( return_var ) extract_arg_defined_in_functionnal( th, tok, return_var, th->current_def_trial->old_current_self, reinterpret_cast<Functionnal *>(a.data), &b );",
        /*ret*/"manual" ).A("InternalVariable").A("VarArgs").set_supplementary_cond(cond) );
    primitive_functions.push_back( PrimitiveFunction( "extract_var_defined_in_functionnal", "if ( return_var ) extract_var_defined_in_functionnal( return_var, reinterpret_cast<Functionnal *>(a.data) );",
        /*ret*/"manual" ).A("InternalVariable").set_supplementary_cond(cond) );
    primitive_functions.push_back( PrimitiveFunction( "extract_var_nb_defined_in_functionnal", "if ( return_var ) extract_var_nb_defined_in_functionnal( return_var, reinterpret_cast<Functionnal *>(a.data), b );",
        /*ret*/"manual" ).A("InternalVariable").A("Int32").set_supplementary_cond(cond) );
    primitive_functions.push_back( PrimitiveFunction( "extract_attr_defined_in_functionnal", "std::string s(reinterpret_cast<Functionnal *>(a.data)->attribute); if (return_var) assign_string( th, tok, return_var, &s[0], s.size() );", /*ret*/"manual" ).A("InternalVariable").set_supplementary_cond(cond) );
    
    primitive_functions.push_back( PrimitiveFunction( "nb_variables_in_functionnal", "ret = reinterpret_cast<Functionnal *>(a.data)->variables.size();", /*ret*/"Int32" ).A("InternalVariable").set_supplementary_cond(cond) );
    primitive_functions.push_back( PrimitiveFunction( "variables_num_is_functionnal_in_functionnal", "ret = ( reinterpret_cast<Functionnal *>(a.data)->variables[b].type == global_data.Functionnal );", /*ret*/"Bool" ).A("InternalVariable").A("Int32").set_supplementary_cond(cond) );

    primitive_functions.push_back( PrimitiveFunction( "is_a_Ref_on_Functionnal", "ret = ( a.type == global_data.Functionnal );", /*ret*/"Bool" ).A("InternalVariable") );
}
void get_primitives_Symbol( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "Op", "Op *" ).avoid_reassign_using_mem_copy() );
    
    primitive_functions.push_back( PrimitiveFunction( "destroy", "dec_ref( self );" ).M("Op") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self = &Op::new_number(0);" ).M("Op") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self = &a->inc_ref();" ).A("Op").M("Op",/*modify*/"true") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "Op *old = self; self = &a->inc_ref(); dec_ref(old);" ).A("Op").M("Op",/*modify*/"self != a") );
    
    for(unsigned i=0;i<primitive_classes.size();++i) {
        if ( arithmetic_class(primitive_classes[i]) ) {
            primitive_functions.push_back( PrimitiveFunction( "init", "self = &Op::new_number( Rationnal(a) );" ).A(primitive_classes[i].sar_name).M("Op",/*modify*/"true") );
            primitive_functions.push_back( PrimitiveFunction( "reassign", "dec_ref(self); self = &Op::new_number( Rationnal(a) );" ).A(primitive_classes[i].sar_name).M("Op",/*modify*/"true") );
        }
    }
    
    //
    primitive_functions.push_back( PrimitiveFunction( "add", "ret = &(*a + *b);", /*ret*/"Op" ).A("Op").A("Op") );
    primitive_functions.push_back( PrimitiveFunction( "sub", "ret = &(*a - *b);", /*ret*/"Op" ).A("Op").A("Op") );
    primitive_functions.push_back( PrimitiveFunction( "sub", "ret = &(   - *a);", /*ret*/"Op" ).A("Op") );
    primitive_functions.push_back( PrimitiveFunction( "mul", "ret = &(*a * *b);", /*ret*/"Op" ).A("Op").A("Op") );
    primitive_functions.push_back( PrimitiveFunction( "div", "ret = &(*a / *b);", /*ret*/"Op" ).A("Op").A("Op") );
    
    primitive_functions.push_back( PrimitiveFunction( "pow", "ret = &pow(*a, *b);", /*ret*/"Op" ).A("Op").A("Op") );
    primitive_functions.push_back( PrimitiveFunction( "mod", "ret = &mod(*a, *b);", /*ret*/"Op" ).A("Op").A("Op") );
    
    static const char *f[] = { "log", "heaviside", "abs", "eqz", "exp", "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", NULL };
    for(unsigned i=0;f[i];++i)
        primitive_functions.push_back( PrimitiveFunction( f[i], "ret = &"+std::string( f[i] )+"(*a);", /*ret*/"Op" ).A("Op") );
    
    primitive_functions.push_back( PrimitiveFunction( "known_at_compile_time", "ret = ( a->type == Op::NUMBER );", /*ret*/"Bool" ).A("Op") );
    
    primitive_functions.push_back( PrimitiveFunction( "symbol", "ret = &Op::new_symbol( *(const char **)a->data, *(Int32 *)((const char **)a->data+1), *(const char **)b->data, *(Int32 *)((const char **)b->data+1) );", /*ret*/"Op" ).A("any").A("any").
        set_supplementary_cond("args[0]->type == global_data.String").set_supplementary_cond("args[1]->type == global_data.String") );
        
    primitive_functions.push_back( PrimitiveFunction( "graphviz_string", "if (return_var) {std::string s=graphviz_repr(self); assign_string( th, tok, return_var, &s[0], s.size() );}", /*ret*/"manual" ). M("Op") );
    primitive_functions.push_back( PrimitiveFunction( "cpp_string", "if (return_var) {std::string s=cpp_repr(self); assign_string( th, tok, return_var, &s[0], s.size() );}", /*ret*/"manual" ). M("Op") );
    primitive_functions.push_back( PrimitiveFunction( "tex_string", "if (return_var) {std::string s=tex_repr(self); assign_string( th, tok, return_var, &s[0], s.size() );}", /*ret*/"manual" ). M("Op") );
    
    primitive_functions.push_back( PrimitiveFunction( "value", "if (self->type==Op::NUMBER) init_arithmetic( ret, self->number_data()->val ); else { init_arithmetic( ret, 0 ); th->add_error(\"method value works only with expressions which do not depend on any symbol.\",tok); }",
        /*ret*/"Rationnal" ). M("Op") );

    primitive_functions.push_back( PrimitiveFunction( "diff", "ret = diff( th, tok, self, a );", /*ret*/"Op" ).M("Op").A("Op") );
    
    primitive_functions.push_back( PrimitiveFunction( "subs", "ret = subs( th, tok, self, a, b );", /*ret*/"Op" ).M("Op").A("VarArgs").A("VarArgs").
        set_supplementary_cond("reinterpret_cast<VarArgs *>(args[0]->data)->contains_only_Ops() and reinterpret_cast<VarArgs *>(args[1]->data)->contains_only_Ops()") );
    
    primitive_functions.push_back( PrimitiveFunction( "depends_on", "ret = depends_on_rec( self, a );", /*ret*/"Bool" ).M("Op").A("Op") );
        
    for(unsigned i=0;i<primitive_classes.size();++i)
        if ( arithmetic_class( primitive_classes[i] ) )
            primitive_functions.push_back( PrimitiveFunction( "apply", "init_arithmetic( ret, subs_numerical( th, tok, self, Rationnal(a) ) );", /*ret*/after_op(primitive_classes[i]) ).M("Op").A(primitive_classes[i].sar_name) );
        
    primitive_functions.push_back( PrimitiveFunction( "discontinuities_separation", "discontinuities_separation( th, tok, return_var, a, b );", /*ret*/"manual" ).A("Op").A("VarArgs") );
        
}

std::string nb_bits_in_mantissa(std::string class_name) {
    if ( class_name == "Bool"       ) return " 1";
    if ( class_name == "Unsigned8"  ) return " 8";
    if ( class_name == "Unsigned16" ) return "16";
    if ( class_name == "Unsigned32" ) return "32";
    if ( class_name == "Unsigned64" ) return "64";
    if ( class_name == "Int8"       ) return " 7";
    if ( class_name == "Int16"      ) return "15";
    if ( class_name == "Int32"      ) return "32";
    if ( class_name == "Int64"      ) return "63";
    if ( class_name == "Float32"    ) return "23";
    if ( class_name == "Float64"    ) return "52";
    if ( class_name == "Float96"    ) return "63";
    if ( class_name == "Rationnal"  ) return "INT32_MAX_VAL";
    return "pouet";
}
    
void get_primitives_CodeWriter( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "CodeWriter_", "CodeWriter" ).avoid_reassign_using_mem_copy() );
    
    primitive_functions.push_back( PrimitiveFunction( "destroy", "destroy( th, tok, self );" ).M("CodeWriter_") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init(\"double\",6);" ).M("CodeWriter_") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init(*reinterpret_cast<const char **>(a->data),*reinterpret_cast<const Int32 *>( reinterpret_cast<const char **>(a->data) + 1 ));" ).A("any").M("CodeWriter_").set_pertinence(1e5).
             set_supplementary_cond( "args[0]->type == global_data.String" ) );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init( a );" ).A("CodeWriter_").M("CodeWriter_",/*modify*/"true") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "self.reassign( th, tok, a );" ).A("CodeWriter_").M("CodeWriter_",/*modify*/"true") );
    
    primitive_functions.push_back( PrimitiveFunction( "add_expr", "self.add_expr( th, tok, a, b, c );" ).M("CodeWriter_",/*modify*/"true").
        A("any").
        A("Op").
        A("Def").set_supplementary_cond("args[0]->type == global_data.String")
    );
    
    primitive_functions.push_back( PrimitiveFunction( "to_string", "if ( return_var ) { std::string s=self.to_string(th,tok,a); assign_string(th,tok,return_var,&s[0],s.size()); }", /*ret*/"manual" ).M("CodeWriter_").A("Int32") );
}

void get_primitives_Lambda( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "Lambda", "Lambda" ) );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init();" ).M("Lambda") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init(a);" ).M("Lambda").A("Lambda") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "self.reassign(a);" ).M("Lambda").A("Lambda") );
    primitive_functions.push_back( PrimitiveFunction( "destroy", "self.destroy();" ).M("Lambda") );
}

void get_primitives_Label( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "Label", "Label" ) );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init();" ).M("Label") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init(a);" ).M("Label").A("Label") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "self.reassign(a);" ).M("Label").A("Label") );
    primitive_functions.push_back( PrimitiveFunction( "destroy", "self.destroy();" ).M("Label") );
    primitive_functions.push_back( PrimitiveFunction( "go", "return self.go( th, sp );" ).M("Label") );
}

void get_primitives_CompiledFunctionSet( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "CompiledFunctionSet_", "CompiledFunctionSet_" ).avoid_reassign_using_mem_copy() );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init();" ).M("CompiledFunctionSet_") );
    primitive_functions.push_back( PrimitiveFunction( "destroy", "self.destroy();" ).M("CompiledFunctionSet_") );
    
    primitive_functions.push_back( PrimitiveFunction( "call_function", "ret = self.call( th, tok, *(const char **)a->data, *(Int32 *)((const char **)a->data+1), b );", /*ret*/"Int32" ).
        M("CompiledFunctionSet_").A("any").A("VarArgs").set_supplementary_cond("args[0]->type == global_data.String") );
    
    primitive_functions.push_back( PrimitiveFunction( "append_so", "self.append_so( th, tok, *(const char **)a->data, *(Int32 *)((const char **)a->data+1) );" ).
        M("CompiledFunctionSet_").A("any").set_supplementary_cond("args[0]->type == global_data.String") );
}

void get_primitives_DlLoader( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "DlLoader", "DlLoader" ).avoid_reassign_using_mem_copy() );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init();" ).M("DlLoader") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init( *(const char **)a->data, *(Int32 *)((const char **)a->data+1) );" ).M("DlLoader").A("any").
        set_supplementary_cond("args[0]->type == global_data.String") );
    primitive_functions.push_back( PrimitiveFunction( "destroy", "self.destroy();" ).M("DlLoader") );
    
    primitive_functions.push_back( PrimitiveFunction( "find_symbol", "ret = self.find_symbol( *(const char **)a->data, *(Int32 *)((const char **)a->data+1) );", /*ret*/"UntypedPtr" ).
        M("DlLoader").A("any").set_supplementary_cond("args[0]->type == global_data.String") );
    
    primitive_functions.push_back( PrimitiveFunction( "get_last_error", "std::string res(self.last_error()); if (return_var) assign_string( th, tok, return_var, &res[0], res.size() );",
        /*ret*/"manual" ).M("DlLoader") );
    
    // void exec_ccode_function( CcodeFunction *function, void *sp, void *th, void *tok, void *self_var );
    //  -> function( sp, th, tok, self_var );
    primitive_functions.push_back( PrimitiveFunction( "exec_ccode_function", "exec_ccode_function( a );" ).A("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "exec_ccode_function", "exec_ccode_function( a, b );" ).A("UntypedPtr").A("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "exec_ccode_function", "exec_ccode_function( a, b, c );" ).A("UntypedPtr").A("UntypedPtr").A("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "exec_ccode_function", "exec_ccode_function( a, b, c, d );" ).A("UntypedPtr").A("UntypedPtr").A("UntypedPtr").A("UntypedPtr") );
    primitive_functions.push_back( PrimitiveFunction( "exec_ccode_function", "exec_ccode_function( a, b, c, d, e );" ).A("UntypedPtr").A("UntypedPtr").A("UntypedPtr").A("UntypedPtr").A("UntypedPtr") );
}
    
void get_primitives_Pthread( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    primitive_classes.push_back( PrimitiveClass( "Pthread", "Pthread" ) );
    primitive_classes.push_back( PrimitiveClass( "Semaphore", "Semaphore" ) );
    
    primitive_functions.push_back( PrimitiveFunction( "pthread_create_", "pthread_create_( th, tok, sp, ret, function, b );", /*ret*/"Pthread" ).A("any","","function").A("VarArgs") );
    primitive_functions.push_back( PrimitiveFunction( "pthread_join", "pthread_join_( a );" ).A("Pthread") );
    primitive_functions.push_back( PrimitiveFunction( "join", "pthread_join_( self );" ).M("Pthread") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self = a;" ).M("Pthread").A("Pthread") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "self = a;" ).M("Pthread").A("Pthread") );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "sem_init_( th, tok, self, 0 );" ).M("Semaphore") );
    primitive_functions.push_back( PrimitiveFunction( "init", "sem_init_( th, tok, self, a );" ).M("Semaphore").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self = a;" ).M("Semaphore").A("Semaphore") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "self = a;" ).M("Semaphore").A("Semaphore") );
    
    primitive_functions.push_back( PrimitiveFunction( "wait", "sem_wait_( th, tok, self );" ).M("Semaphore") );
    primitive_functions.push_back( PrimitiveFunction( "post", "sem_post_( th, tok, self );" ).M("Semaphore") );
    primitive_functions.push_back( PrimitiveFunction( "destroy", "sem_destroy_( th, tok, self );" ).M("Semaphore") );

    primitive_functions.push_back( PrimitiveFunction( "save_and_post", "save_and_post( th, tok, self, a );" ).M("Semaphore").A("VarArgs") );
}

// void get_primitives_Type( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
//     primitive_classes.push_back( PrimitiveClass( "Type", "TypePtr" ) );
//     
//     primitive_functions.push_back( PrimitiveFunction( "init", "self = a;" ).M("Type").A("Type") );
//     primitive_functions.push_back( PrimitiveFunction( "reassign", "self = a;" ).M("Type").A("Type") );
// }

// -----------------------------------------------------------------------------------------------------------------------
inline void get_primitives( std::vector<PrimitiveClass> &primitive_classes, std::vector<PrimitiveFunction> &primitive_functions ) {
    //
    primitive_classes.push_back( PrimitiveClass( "Def", "Definition" ).avoid_reassign_using_mem_copy() );
    primitive_classes.push_back( PrimitiveClass( "Error", "int" ) );
    primitive_classes.push_back( PrimitiveClass( "Bool", "Bool" ).avoid_reassign_using_mem_copy() );
    
    primitive_classes.push_back( PrimitiveClass( "Unsigned8", "Unsigned8" ) );
    primitive_classes.push_back( PrimitiveClass( "Unsigned16", "Unsigned16" ) );
    primitive_classes.push_back( PrimitiveClass( "Unsigned32", "Unsigned32" ) );
    primitive_classes.push_back( PrimitiveClass( "Unsigned64", "Unsigned64" ) );
    
    primitive_classes.push_back( PrimitiveClass( "Int8", "Int8" ) );
    primitive_classes.push_back( PrimitiveClass( "Int16", "Int16" ) );
    primitive_classes.push_back( PrimitiveClass( "Int32", "Int32" ) );
    primitive_classes.push_back( PrimitiveClass( "Int64", "Int64" ) );
    
    primitive_classes.push_back( PrimitiveClass( "Rationnal", "Rationnal" ).avoid_reassign_using_mem_copy() );
    
    primitive_classes.push_back( PrimitiveClass( "Float32", "Float32" ) );
    primitive_classes.push_back( PrimitiveClass( "Float64", "Float64" ) );
    primitive_classes.push_back( PrimitiveClass( "Float96", "Float96" ) );
    
    primitive_classes.push_back( PrimitiveClass( "Void", "int" ) );
    primitive_classes.push_back( PrimitiveClass( "BlockOfTok", "Block" ) );
    
    //     get_primitives_Type( primitive_classes, primitive_functions );
    get_primitives_File( primitive_classes, primitive_functions );
    get_primitives_UntypedPtr( primitive_classes, primitive_functions );
    get_primitives_Property( primitive_classes, primitive_functions );
    get_primitives_VarArgs( primitive_classes, primitive_functions );
    get_primitives_Ref( primitive_classes, primitive_functions );
    get_primitives_Functionnal( primitive_classes, primitive_functions );
    get_primitives_Symbol( primitive_classes, primitive_functions );
    get_primitives_CodeWriter( primitive_classes, primitive_functions );
    get_primitives_Lambda( primitive_classes, primitive_functions );
    get_primitives_Label( primitive_classes, primitive_functions );
    get_primitives_CompiledFunctionSet( primitive_classes, primitive_functions );
    get_primitives_DlLoader( primitive_classes, primitive_functions );
    get_primitives_Pthread( primitive_classes, primitive_functions );
    
    // need destroy
    
    // attributes
    for(unsigned i=0;i<primitive_classes.size();++i) {
        if ( primitive_classes[i].sar_name != "Property" ) {
            primitive_classes[i].push_attr( "is_a_number", "Bool", ( arithmetic_class(primitive_classes[i]) or primitive_classes[i].sar_name=="Op" ? "true" : "false" ) );
            primitive_classes[i].push_attr( "is_a_float_number", "Bool", ( float_class(primitive_classes[i]) or primitive_classes[i].sar_name=="Op" ? "true" : "false" ) );
            primitive_classes[i].push_attr( "is_an_integer_number", "Bool", ( integer_class(primitive_classes[i]) ? "true" : "false" ) );
            primitive_classes[i].push_attr( "is_a_signed_number", "Bool", ( signed_arithmetic_class(primitive_classes[i]) ? "true" : "false" ) );
            primitive_classes[i].push_attr( "is_a_rationnal_number", "Bool", ( primitive_classes[i].sar_name=="Rationnal" or primitive_classes[i].sar_name=="Op" ? "true" : "false" ) );
            primitive_classes[i].push_attr( "is_a_vector", "Bool", ( primitive_classes[i].sar_name == "VarArgs" ? "true" : "false" ) );
            primitive_classes[i].push_attr( "is_an_array", "Bool", "false" );
            primitive_classes[i].push_attr( "is_a_matrix", "Bool", "false" );
            primitive_classes[i].push_attr( "tensor_order", "Int32", ( primitive_classes[i].sar_name == "VarArgs" ? "1" : "-1" ) );
            primitive_classes[i].push_attr( "is_a_function", "Bool", ( primitive_classes[i].sar_name=="Functionnal" or primitive_classes[i].sar_name=="Lambda" ? "true" : "false" ) );
            
            if ( arithmetic_class( primitive_classes[i] ) ) {
                if ( primitive_classes[i].sar_name != "Rationnal" ) {
                    primitive_classes[i].push_attr( "max_representable", primitive_classes[i].sar_name, "std::numeric_limits<"+primitive_classes[i].cpp_name+">::max()" );
                    primitive_classes[i].push_attr( "min_representable", primitive_classes[i].sar_name, "std::numeric_limits<"+primitive_classes[i].cpp_name+">::min()" );
                }
                primitive_classes[i].push_attr( "nb_bits_in_mantissa", "Int32", nb_bits_in_mantissa(primitive_classes[i].sar_name) );
                primitive_classes[i].push_attr( "nb_bits_in_denominator_mantissa", "Int32", ( primitive_classes[i].sar_name=="Rationnal" ? "INT32_MAX_VAL" : "0" ) );
            }
            
            primitive_classes[i].push_attr( "does_not_need_destroy", "Bool", primitive_classes[i].reassign_using_mem_copy ? "true" : "false" );
        }
        primitive_classes[i].push_attr( "is_a_pointer", "Bool", ( primitive_classes[i].sar_name=="UntypedPtr" ? "true" : "false" ) );
        primitive_classes[i].push_attr( "is_a_Property", "Bool", ( primitive_classes[i].sar_name=="Property" ? "true" : "false" ) );
        if ( arithmetic_class( primitive_classes[i] ) and primitive_classes[i].reassign_using_mem_copy )
            primitive_functions.push_back( PrimitiveFunction( "init", "fake_func(self);" ).M(primitive_classes[i].sar_name) );
    }
    primitive_functions.push_back( PrimitiveFunction( "get_epsilon", "ret = std::numeric_limits<Float32>::epsilon();", /*ret*/"Float32" ).M("Float32") );
    primitive_functions.push_back( PrimitiveFunction( "get_epsilon", "ret = std::numeric_limits<Float64>::epsilon();", /*ret*/"Float64" ).M("Float64") );
    primitive_functions.push_back( PrimitiveFunction( "get_epsilon", "ret = std::numeric_limits<Float96>::epsilon();", /*ret*/"Float96" ).M("Float96") );
    

    
    primitive_functions.push_back( PrimitiveFunction( "to_string", "std::ostringstream ss; ss << self; if (return_var) assign_string( th, tok, return_var, &ss.str()[0], ss.str().size() );", /*ret*/"manual" ).M("Rationnal") );
    for(unsigned i=0;i<primitive_classes.size();++i) {
        if ( integer_class( primitive_classes[i] ) or float_class( primitive_classes[i] ) or primitive_classes[i].sar_name=="UntypedPtr" )
            primitive_functions.push_back( PrimitiveFunction( "to_string", "std::ostringstream ss; ss << self; std::string s = ss.str(); if (return_var) assign_string( th, tok, return_var, &s[0], s.size() );", /*ret*/"manual" ).
                    M(primitive_classes[i].sar_name) );
        if ( float_class( primitive_classes[i] ) )
            primitive_functions.push_back( PrimitiveFunction( "to_string", "std::ostringstream ss; ss.precision(a); ss << self; std::string s = ss.str(); if (return_var) assign_string( th, tok, return_var, &s[0], s.size() );", /*ret*/"manual" ).
                    M(primitive_classes[i].sar_name).A("Int32") );
    }
    
    primitive_functions.push_back( PrimitiveFunction( "system"  , "ret = system_( *reinterpret_cast<const char **>(a->data), *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1) );", /*ret*/"Int32" ).
            A("any").set_supplementary_cond("args[0]->type == global_data.String") );
    
    // is_of_type_...
    for(unsigned i=0;i<primitive_classes.size();++i)
        primitive_functions.push_back( PrimitiveFunction( "is_of_type_"+primitive_classes[i].sar_name, "ret = (a->type == global_data."+primitive_classes[i].sar_name+");", /*ret*/"Bool" ).A("any") );

    
    // destroy
    primitive_functions.push_back( PrimitiveFunction( "destroy_instanciated_attributes", "return destroy_instanciated_attributes( th, tok, a, sp );" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "destroy", "return destroy( self, th, sp );" ).M("Def") );
    
    primitive_functions.push_back( PrimitiveFunction( "destroy", "return destroy( self, th, sp );" ).M("BlockOfTok") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "self.reassign( &a, th, sp );" ).A("BlockOfTok").M("BlockOfTok",/*modify*/"true") );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init();" ).M("Rationnal") );
    primitive_functions.push_back( PrimitiveFunction( "destroy", "self.destroy();" ).M("Rationnal") );
 
 
    primitive_functions.push_back( PrimitiveFunction( "to_string", "fake_func(self); if (return_var) assign_string( th, tok, return_var, NULL, 0 );", /*ret*/"manual" ).M("Error") );
    primitive_functions.push_back( PrimitiveFunction( "apply", "fake_func(self); if (return_var) assign_string( th, tok, return_var, NULL, 0 );", /*ret*/"manual" ).M("Error").has_varargs() );
    primitive_functions.push_back( PrimitiveFunction( "select", "fake_func(self); fake_func(a); if (return_var) assign_string( th, tok, return_var, NULL, 0 );", /*ret*/"manual" ).M("Error").A("any") );
    primitive_functions.push_back( PrimitiveFunction( "to_string", "std::ostringstream ss; ss << self; if (return_var) assign_string( th, tok, return_var, &ss.str()[0], ss.str().size() );", /*ret*/"manual" ).M("Def") );
    
    primitive_functions.push_back( PrimitiveFunction( "to_string", "const char *s( self ? \"true\" : \"false\" ); if (return_var) assign_string( th, tok, return_var, s, strlen(s) );", /*ret*/"manual" ).M("Bool") );

    primitive_functions.push_back( PrimitiveFunction( "strlen", "ret = strlen((char *)a);", /*ret*/"Int32" ).A("UntypedPtr") );
    
    
    //
    primitive_functions.push_back( PrimitiveFunction( "__init_using_default_values__", "return init_using_default_values(th,tok,a,sp);" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "init_and_return_false_if_same_types", "return init_and_return_false_if_same_types(th,tok,a,b,ret,sp);", "Bool" ).A("any").A("any") );
    primitive_functions.push_back( PrimitiveFunction( "reassign_and_return_false_if_same_types", "return reassign_and_return_false_if_same_types(th,tok,a,b,ret,sp);", "Bool" ).A("any").A("any") );
    
    // reassign
    for(unsigned i=0;i<primitive_classes.size();++i) {
        std::string type_conv = ", global_data." + primitive_classes[i].sar_name;
        for(unsigned j=0;j<=i;++j) // reassign from types with less information
            if ( arithmetic_class(primitive_classes[i]) and arithmetic_class(primitive_classes[j]) )
                primitive_functions.push_back( PrimitiveFunction( "reassign", "init_arithmetic( self, a ); reassign_transient_data( th, self_var->transient_data, args[0]->transient_data );" ).
                    A(primitive_classes[j].sar_name). M(primitive_classes[i].sar_name,/*modify*/"are_different(self,a)") );
    }
                    
    primitive_functions.push_back( PrimitiveFunction( "type_promote", "fake_func(a); fake_func(b); fake_func(c); fake_func(ret);", /*ret*/"Error" )
            .A("Error")
            .A("Error")
            .A("any") );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "fake_func(self); fake_func(a);" ).A("any").M("Error",/*modify*/"").set_dont_mind_about_prop() );
    primitive_functions.push_back( PrimitiveFunction( "reassign", "self.reassign( &a, th, sp );" ).A("Def").M("Def",/*modify*/"true") );
   
    // init    
    for(unsigned i=0;i<primitive_classes.size();++i) {
        std::string conv_to = ( primitive_classes[i].cpp_name=="Bool" ? "bool" : primitive_classes[i].cpp_name );
        std::string type_conv = ", global_data." + primitive_classes[i].sar_name;
        for(unsigned j=0;j<primitive_classes.size();++j)
            if ( arithmetic_class(primitive_classes[i]) and arithmetic_class(primitive_classes[j]) )
                primitive_functions.push_back( PrimitiveFunction( "init", "init_arithmetic( self, ("+conv_to+")a ); reassign_transient_data( th, self_var->transient_data, args[0]->transient_data );" ).
                        A(primitive_classes[j].sar_name).M(primitive_classes[i].sar_name,/*modify*/"true") );
    }
    
    primitive_functions.push_back( PrimitiveFunction( "init", "fake_func(a); fake_func(self);" ).A("any").
        M("Error",/*modify*/"").set_dont_mind_about_prop() );
    primitive_functions.push_back( PrimitiveFunction( "init", "fake_func(self);" ).M("Error",/*modify*/"true").set_dont_mind_about_prop() );
    primitive_functions.push_back( PrimitiveFunction( "write_to_stream", "fake_func(a); fake_func(self);" ).A("any").
        M("Error",/*modify*/"").set_dont_mind_about_prop() );
    primitive_functions.push_back( PrimitiveFunction( "equal", "fake_func(ret);fake_func(a);fake_func(b);", /*ret*/"Error" ).
        A("Error","","").A("any").set_pertinence(2e20) );
    primitive_functions.push_back( PrimitiveFunction( "equal", "fake_func(ret);fake_func(a);fake_func(b);", /*ret*/"Error" ).
        A("any","","").A("Error").set_pertinence(1e20) );
    primitive_functions.push_back( PrimitiveFunction( "not_equal", "fake_func(ret);fake_func(a);fake_func(b);", /*ret*/"Error" ).
        A("Error","","").A("any").set_pertinence(2e20) );
    primitive_functions.push_back( PrimitiveFunction( "not_equal", "fake_func(ret);fake_func(a);fake_func(b);", /*ret*/"Error" ).
        A("any","","").A("Error").set_pertinence(1e20) );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self = false; fake_func(a);" ).A("Error").M("Bool",/*modify*/"true") );
    
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init();" ).M("Def",/*modify*/"true") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init( &a );" ).A("Def").M("Def",/*modify*/"true") );
    primitive_functions.push_back( PrimitiveFunction( "init", "self.init( &a );" ).A("BlockOfTok").M("BlockOfTok",/*modify*/"true") );
    
    // +, -, / ...
    const char *arithmetic_operations[] = {"add","sub","mul",NULL};
    for( const char **a=arithmetic_operations; *a; ++a )
        for(unsigned i=0;i<primitive_classes.size();++i)
            if ( arithmetic_class(primitive_classes[i]) and primitive_classes[i].sar_name != "Bool" )
                primitive_functions.push_back( PrimitiveFunction( *a, "init_arithmetic( ret, METIL_NS::"+std::string(*a)+"(a,b) );", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name).A(primitive_classes[i].sar_name) );
    primitive_functions.push_back( PrimitiveFunction( "mul", "ret = a * b;", /*ret*/"Bool" ).A("Bool").A("Bool") );
    primitive_functions.push_back( PrimitiveFunction( "add", "ret = Int32(a) + Int32(b);", /*ret*/"Int32" ).A("Bool").A("Bool") );
    primitive_functions.push_back( PrimitiveFunction( "sub", "ret = Int32(a) - Int32(b);", /*ret*/"Int32" ).A("Bool").A("Bool") );
    // pow
    for(unsigned i=0;i<primitive_classes.size();++i) {
        if ( float_class(primitive_classes[i]) ) {
            primitive_functions.push_back( PrimitiveFunction( "pow", "init_arithmetic( ret, METIL_NS::pow(a,b) );", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name).A(primitive_classes[i].sar_name) );
            primitive_functions.push_back( PrimitiveFunction( "pow", "init_arithmetic( ret, METIL_NS::pow(a,b) );", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name).A("Rationnal") );
        }
        if ( integer_class( primitive_classes[i] ) )
            primitive_functions.push_back( PrimitiveFunction( "pow", "init_arithmetic( ret, METIL_NS::pow(a,b) );", /*ret*/"Rationnal" ).A("Rationnal").A(primitive_classes[i].sar_name) );
    }
    // div
    for(unsigned i=0;i<primitive_classes.size();++i)
        if ( float_class(primitive_classes[i]) or primitive_classes[i].sar_name=="Rationnal" )
            primitive_functions.push_back( PrimitiveFunction( "div", "init_arithmetic( ret, a / b );", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name).A(primitive_classes[i].sar_name) );
    for(unsigned i=0;i<primitive_classes.size();++i) {
        if ( integer_class(primitive_classes[i]) ) {
            primitive_functions.push_back( PrimitiveFunction( "div_int", "init_arithmetic( ret, a / b );", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name).A(primitive_classes[i].sar_name) );
            primitive_functions.push_back( PrimitiveFunction( "div", "ret.init(); ret.num = a; ret.den = b; ret.cannonicalize();", /*ret*/"Rationnal" ).A(primitive_classes[i].sar_name).A(primitive_classes[i].sar_name) );
        }
    }
    // exp, log, ...
    const char *fps_arithmetic_operations[] = {"exp","log","sin","cos","tan","asin","acos","atan","sinh","cosh","tanh",NULL};
    for( const char **a=fps_arithmetic_operations; *a; ++a ) {
        for(unsigned i=0;i<primitive_classes.size();++i)
            if ( float_class(primitive_classes[i]) )
                primitive_functions.push_back( PrimitiveFunction( *a, "init_arithmetic( ret, "+std::string(*a)+"( a ) );", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name) );
    }
    for(unsigned i=0;i<primitive_classes.size();++i)
        if ( arithmetic_class(primitive_classes[i]) )
            primitive_functions.push_back( PrimitiveFunction( "sub", "init_arithmetic( ret, -a );", /*ret*/signed_correspondance(primitive_classes[i]) ).A(primitive_classes[i].sar_name) );
    for(unsigned i=0;i<primitive_classes.size();++i) {
        if ( arithmetic_class(primitive_classes[i]) and primitive_classes[i].sar_name != "Bool" ) {
            primitive_functions.push_back( PrimitiveFunction( "mod", "init_arithmetic( ret, METIL_NS::mod( a, b ) );", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name).A(primitive_classes[i].sar_name) );
            primitive_functions.push_back( PrimitiveFunction( "abs", "init_arithmetic( ret, METIL_NS::abs( a ) );", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name) );
        }
    }
    primitive_functions.push_back( PrimitiveFunction( "atan2", "ret = atan2(a,b);", /*ret*/"Float32" ).A("Float32").A("Float32") );
    primitive_functions.push_back( PrimitiveFunction( "atan2", "ret = atan2(a,b);", /*ret*/"Float64" ).A("Float64").A("Float64") );
    primitive_functions.push_back( PrimitiveFunction( "atan2", "ret = atan2(a,b);", /*ret*/"Float96" ).A("Float96").A("Float96") );
    
    // heaviside, eqz
    for(unsigned i=0;i<primitive_classes.size();++i) {
        if ( unsigned_arithmetic_class(primitive_classes[i]) )
            primitive_functions.push_back( PrimitiveFunction( "heaviside", "ret = true; fake_func( a );", /*ret*/"Bool" ).A(primitive_classes[i].sar_name) );
        else if ( arithmetic_class(primitive_classes[i]) )
            primitive_functions.push_back( PrimitiveFunction( "heaviside", "ret = ( a >= 0 );", /*ret*/"Bool" ).A(primitive_classes[i].sar_name) );
        if ( arithmetic_class(primitive_classes[i]) )
            primitive_functions.push_back( PrimitiveFunction( "eqz", "ret = ( not a );", /*ret*/"Bool" ).A(primitive_classes[i].sar_name) );
    }
    
    // eq, ineq
    const char *eq_ineq_operations[] = {"equal","inferior","superior","inferior_equal","superior_equal","not_equal",NULL};
    const char *eq_ineq_operators[]  = {"=="   ,"<"       ,">"       ,"<="            ,">="            ,"!="       ,NULL};
    for( const char **a=eq_ineq_operations, **o=eq_ineq_operators; *a; ++a,++o ) {
        for(unsigned i=0;i<primitive_classes.size();++i)
            if ( arithmetic_class(primitive_classes[i]) )
                primitive_functions.push_back( PrimitiveFunction( *a, "ret = (a "+std::string(*o)+" b);", /*ret*/"Bool" ).A(primitive_classes[i].sar_name).A(primitive_classes[i].sar_name) );
    }
    // &, |, ...
    const char *bb_operations[] = {"and_bitwise","or_bitwise","xor_bitwise",NULL};
    const char *bb_operators[]  = {"&"          ,"|"          ,"^"         ,NULL};
    for( const char **a=bb_operations, **o=bb_operators; *a; ++a,++o ) {
        for(unsigned i=0;i<primitive_classes.size();++i)
            if ( integer_class(primitive_classes[i]) )
                primitive_functions.push_back( PrimitiveFunction( *a, "ret = (a "+std::string(*o)+" b);", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name).A(primitive_classes[i].sar_name) );
    }
    // ~, ...
    const char *bnb_operations[] = {"not_bitwise",NULL};
    const char *bnb_operators[]  = {"~"          ,NULL};
    for( const char **a=bnb_operations, **o=bnb_operators; *a; ++a,++o ) {
        for(unsigned i=0;i<primitive_classes.size();++i)
            if ( integer_class(primitive_classes[i]) )
                primitive_functions.push_back( PrimitiveFunction( *a, "ret = "+std::string(*o)+" a;", /*ret*/primitive_classes[i].sar_name ).A(primitive_classes[i].sar_name) );
    }
    
    primitive_functions.push_back( PrimitiveFunction( "are_of_same_type", "ret = ( a->type == b->type );", /*ret*/"Bool" ).A("any").A("any") );
    primitive_functions.push_back( PrimitiveFunction( "are_of_different_type", "ret = ( a->type != b->type );", /*ret*/"Bool" ).A("any").A("any") );
    primitive_functions.push_back( PrimitiveFunction( "are_of_type_Def", "ret = ( a->type == global_data.Def and b->type == global_data.Def );", /*ret*/"Bool" ).A("any").A("any") );
    primitive_functions.push_back( PrimitiveFunction( "are_radically_different", "ret = are_radically_different( a, b );", /*ret*/"Bool" ).A("Def").A("Def") );
    primitive_functions.push_back( PrimitiveFunction( "are_similar_for_sure", "ret = are_similar_for_sure( a, b );", /*ret*/"Bool" ).A("Def").A("Def") );
    primitive_functions.push_back( PrimitiveFunction( "is_a_class", "ret = self.def_data->is_class();", /*ret*/"Bool" ).M("Def") );
    primitive_functions.push_back( PrimitiveFunction( "have_same_name", "ret = ( a.def_data->name == b.def_data->name );", /*ret*/"Bool" ).A("Def").A("Def") );
    primitive_functions.push_back( PrimitiveFunction( "base_name", "std::string s(self.def_data->name); if (return_var) assign_string( th, tok, return_var, &s[0], s.size() );", /*ret*/"manual" ).M("Def") );
    
    // miscelaneous
    primitive_functions.push_back( PrimitiveFunction( "__print__", "std::cout << a << std::endl;" ).A("Def") );
    primitive_functions.push_back( PrimitiveFunction( "__print__", "std::cout << a << std::endl;" ).A("BlockOfTok") );
    primitive_functions.push_back( PrimitiveFunction( "__print__", "std::cout << a << std::endl;" ).A("UntypedPtr") );
    
    primitive_functions.push_back( PrimitiveFunction( "has_size_and_select", "ret = (a->type->find_var(NULL,STRING_size_NUM) or a->type->find_var(NULL,STRING_get_size_NUM)) and a->type->find_var(NULL,STRING_select_NUM);", /*ret*/"Bool" ).
        A("any") );
        
    primitive_functions.push_back( PrimitiveFunction( "nb_attributes_of", "ret = a->type->nb_attributes_in_inst();", /*ret*/"Int32" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "name_attribute_nb_of", "std::string s(a->type->name_attributes_in_inst(b)); if ( return_var ) assign_string( th, tok, return_var, &s[0], s.size() );", /*ret*/"manual" ).
        A("any").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "attribute_nb_of", "if ( return_var ) { assign_ref_on( return_var, a ); return_var->replace_by_non_static_attr(a->type->attr_in_inst_nb(b)); }", /*ret*/"manual" ).
        A("any").A("Int32") );
    
    primitive_functions.push_back( PrimitiveFunction( "nb_static_attributes_of", "ret = a->type->nb_static_attributes_in_inst();", /*ret*/"Int32" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "name_static_attribute_nb_of", "std::string s(a->type->name_static_attributes_in_inst(b)); if ( return_var ) assign_string( th, tok, return_var, &s[0], s.size() );", /*ret*/"manual" ).
        A("any").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "static_attribute_nb_of", "if ( return_var ) { assign_ref_on( return_var, &a->type->static_attr_in_inst_nb(b)->v ); }", /*ret*/"manual" ).
        A("any").A("Int32") );
        
    primitive_functions.push_back( PrimitiveFunction( "nb_template_parameters_of", "ret = a->type->nb_template_parameters();", /*ret*/"Int32" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "name_template_parameter_nb_of", "std::string s(a->type->name_template_parameter_nb(b)); if ( return_var ) assign_string( th, tok, return_var, &s[0], s.size() );", /*ret*/"manual" ).
        A("any").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "template_parameter_nb_of", "if ( return_var ) { assign_ref_on( return_var, a->type->template_parameter_nb(b) ); }", /*ret*/"manual" ).
        A("any").A("Int32") );
    
    primitive_functions.push_back( PrimitiveFunction( "type_of", "return get_def_from_type( th, tok, sp, ret, a->type );", /*ret*/"Def" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "size_of", "ret = a->type->data_size_in_bits;", /*ret*/"Int32" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "size_of_if_in_vec", "ret = a->type->size_of_in_in_vec();", /*ret*/"Int32" ).A("any") );
    
    primitive_functions.push_back( PrimitiveFunction( "has___for___method", "ret = bool( a->type->find_var( NULL, STRING___for___NUM ) );", /*ret*/"Bool" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "rm_spaces_at_beg_of_lines", "std::string s=rm_spaces_at_beg_of_lines( a, b ); assign_string(th,tok,return_var,&s[0],s.size());",
        /*ret*/"manual" ).A("Int32").A("any").set_supplementary_cond("args[1]->type == global_data.String") );
    
    primitive_functions.push_back( PrimitiveFunction( "absolute_filename", "std::string s = absolute_filename( *reinterpret_cast<const char **>(a->data), *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1) ); assign_string(th,tok,return_var,&s[0],s.size());", /*ret*/"manual" ).A("any").set_supplementary_cond("args[0]->type == global_data.String") );
    primitive_functions.push_back( PrimitiveFunction( "get_includes_of_cpp_named", "bool cc=false,cg=false; get_includes_of_cpp_named( th, tok, ret, *reinterpret_cast<const char **>(a->data), *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1), cc, cg ); b=cc; c=cg;", /*ret*/"VarArgs" ).A("any").A("Bool").A("Bool").set_supplementary_cond("args[0]->type == global_data.String") );

    // stack exploration
    primitive_functions.push_back( PrimitiveFunction( "__nb_stack_level__", "ret = __nb_stack_level__( th, tok );", /*ret*/"Int32" ) );
    primitive_functions.push_back( PrimitiveFunction( "__file_at_stack_level__", "__file_at_stack_level__( th, tok, a, return_var );", /*ret*/"manual" ).A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "__line_at_stack_level__", "ret = __line_at_stack_level__( th, tok, a );", /*ret*/"Int32" ).A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "__col_at_stack_level__", "ret = __col_at_stack_level__( th, tok, a );", /*ret*/"Int32" ).A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "__line_str_at_stack_level__", "__line_str_at_stack_level__( th, tok, a, b, return_var );", /*ret*/"manual" ).A("Int32").A("Int32") );
    
    primitive_functions.push_back( PrimitiveFunction( "calc_name", "calc_name( th, tok, return_var, *reinterpret_cast<const char **>(a->data), *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1) );", /*ret*/"manual" ).A("any").set_supplementary_cond("args[0]->type == global_data.String") );

    primitive_functions.push_back( PrimitiveFunction( "md5_str", "if (return_var) {std::string s=md5_str( *reinterpret_cast<const char **>(a->data), *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1) ); assign_string(th,tok,return_var,&s[0],s.size());}", /*ret*/"manual" ).A("any").set_supplementary_cond("args[0]->type == global_data.String") );
    primitive_functions.push_back( PrimitiveFunction( "time_of_day_in_sec", "ret = time_of_day_in_sec();", /*ret*/"Float64" ) );
    primitive_functions.push_back( PrimitiveFunction( "directory_of", "if (return_var) {std::string s=directory_of( *reinterpret_cast<const char **>(a->data), *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1) ); assign_string(th,tok,return_var,&s[0],s.size());}", /*ret*/"manual" ).A("any").set_supplementary_cond("args[0]->type == global_data.String") );
    
    primitive_functions.push_back( PrimitiveFunction( "equal", "ret = str_eq(a,b);", /*ret*/"Bool" ).A("any").A("any").set_supplementary_cond("args[0]->type == global_data.String and args[1]->type == global_data.String") );
    
    primitive_functions.push_back( PrimitiveFunction( "__argv__size__", "ret = th->argc;", /*ret*/"Int32" ) );
    primitive_functions.push_back( PrimitiveFunction( "__argv__item__", "if ( return_var ) { std::string s(th->argv[a]); assign_string( th, tok, return_var, &s[0], s.size() ); }", /*ret*/"manual" ).A("Int32") );

    primitive_functions.push_back( PrimitiveFunction( "split_dir", "split_dir( th, tok, *reinterpret_cast<const char **>(a->data), *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1), ret );", /*ret*/"VarArgs" ).A("any").set_supplementary_cond("args[0]->type == global_data.String") );
    primitive_functions.push_back( PrimitiveFunction( "getenv", "getenv( th, tok, *reinterpret_cast<const char **>(a->data), *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1), return_var );", /*ret*/"manual" ).A("any").set_supplementary_cond("args[0]->type == global_data.String") );
    primitive_functions.push_back( PrimitiveFunction( "getcwd", "getcwd( th, tok, return_var );", /*ret*/"manual" ) );
    primitive_functions.push_back( PrimitiveFunction( "get_os_type", "get_os_type_( th, tok, return_var );", /*ret*/"manual" ) );
    primitive_functions.push_back( PrimitiveFunction( "get_cpu_type", "get_cpu_type_( th, tok, return_var );", /*ret*/"manual" ) );
    primitive_functions.push_back( PrimitiveFunction( "get_type_md5_sum", "get_type_md5_sum( th, tok, a, return_var );", /*ret*/"manual" ).A("any") );
    primitive_functions.push_back( PrimitiveFunction( "int32_display", "std::cout << a << std::endl;" ).A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "set_thread_return_value", "th->return_value = a;" ).A("Int32") );

    primitive_functions.push_back( PrimitiveFunction( "rand", "ret = rand();", /*ret*/"Int32" ) );
    primitive_functions.push_back( PrimitiveFunction( "randf", "ret = rand() / Float64(RAND_MAX);", /*ret*/"Float64" ) );


    // mpi
    primitive_functions.push_back( PrimitiveFunction( "mpi_size", "ret = metil_mpi_size();", /*ret*/"Int32" ) );
    primitive_functions.push_back( PrimitiveFunction( "mpi_rank", "ret = metil_mpi_rank();", /*ret*/"Int32" ) );
    primitive_functions.push_back( PrimitiveFunction( "mpi_send", "ret = metil_mpi_send( a, b, c, d );", /*ret*/"Int32" ).A("UntypedPtr").A("Int32").A("Int32").A("Int32") );
    primitive_functions.push_back( PrimitiveFunction( "mpi_recv", "ret = metil_mpi_recv( a, b, c, d );", /*ret*/"Int32" ).A("UntypedPtr").A("Int32").A("Int32").A("Int32") );
    
    //
    primitive_functions.push_back( PrimitiveFunction( "inheritance", "ret = inheritance( a->type, b->type );", /*ret*/"Bool" ).A("any").A("any") );
    
    
}

#endif // PRIMITIVE_H

