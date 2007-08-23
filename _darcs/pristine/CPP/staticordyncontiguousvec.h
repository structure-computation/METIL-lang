#ifndef STATICORDYNCONTIGUOUSVEC_H
#define STATICORDYNCONTIGUOUSVEC_H

/**
 */
template<class T,unsigned nb_in_stack>
struct StaticOrDynContiguousVec {
    StaticOrDynContiguousVec( unsigned s ) {
        size_ = s;
        if ( size_ > nb_in_stack )
            dyn_data = new T[ size_ ];
    }
    
    ~StaticOrDynContiguousVec() {
        if ( size_ > nb_in_stack )
            delete [] dyn_data;
    }
    
    const T *ptr() const { if ( size_ > nb_in_stack ) return dyn_data; return sta_data; }
    T *ptr() { if ( size_ > nb_in_stack ) return dyn_data; return sta_data; }
    
    const T &operator[]( unsigned i ) const { return ptr()[ i ]; }
    T &operator[]( unsigned i ) { return ptr()[ i ]; }
    
    unsigned size() const { return size_; }
    
    unsigned size_;
    T *dyn_data;
    T sta_data[ nb_in_stack ];
};

#endif // STATICORDYNCONTIGUOUSVEC_H
