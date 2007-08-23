#ifndef MET_CFILE_H
#define MET_CFILE_H

#include <stdio.h>
#include <errno.h>

struct CFile {
    void init() {
        f = NULL;
        pipe = false;
    }
    
    int write( const void *p, Int32 si ) {
        return fwrite( (const char *)p, si, 1, f );
    }
    int read( void *p, Int32 si ) {
        return fread( (char *)p, si, 1, f );
    }
    
    std::string read() { // hum
        std::string res;
        char buf[256];
        while ( true ) {
            int n = fread( buf, 1, 256, f );
            if ( n <= 0 ) break;
            res += std::string( buf, buf + n );
        }
        return res;
    }
    
    void close() {
        if ( f and f != stdout and f != stderr and f != stdin ) {
            if ( pipe ) pclose( f );
            else        fclose( f );
            f = NULL;
        }
    }
    void flush() {
        fflush( f );
    }
    
    int open_with_str( Variable *a, Variable *b, bool want_pipe ) {
        const char *file_name_str = *reinterpret_cast<const char **>(a->data);
        Int32 file_name_si = *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(a->data)+1);
        const char *mode_name_str = *reinterpret_cast<const char **>(b->data);
        Int32 mode_name_si = *reinterpret_cast<Int32 *>(reinterpret_cast<const char **>(b->data)+1);
        
        std::string file_name( file_name_str, file_name_str + file_name_si );
        std::string mode_name( mode_name_str, mode_name_str + mode_name_si );
        
        pipe = want_pipe;
        if ( want_pipe )
            f = popen( file_name.c_str(), mode_name.c_str() );
        else
            f = fopen( file_name.c_str(), mode_name.c_str() );
        
        if ( not f )
            return errno;
        return 0;
    }
    
    operator bool() const { return not feof(f); }
    
    bool eof() const { return feof(f); }
    Int32 tell() const { return ftell(f); }
    void seek( Int64 v ) const { fseek( f, v, SEEK_SET ); }
    
    Int32 size() const {
        Int32 t = tell();
        fseek( f, 0, SEEK_END );
        Int32 res = tell() - t;
        fseek( f, t, SEEK_SET );
        return res;
    }
    
    bool pipe;
    FILE *f;
};

inline bool operator==(const CFile &f0,const CFile &f1) { return f0.f==f1.f; }

#endif // MET_FILE_H
