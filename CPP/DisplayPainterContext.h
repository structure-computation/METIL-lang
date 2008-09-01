#ifndef DISPLAY_PAINTER_CONTEXT_H
#define DISPLAY_PAINTER_CONTEXT_H

///
struct DisplayPainterContext {
    ///
    struct Point {
        Point( double x, double y, double z = 0 ) : x(x), y(y), z(z) {}
        Point( double x, double y, double z, double w ) : x(x/w), y(y/w), z(z/w) {}
        Point operator+( const Point &p ) const { return Point( x + p.x, y + p.y, z + p.z ); }
        Point operator/( double w ) const { return Point( x / w, y / w, z / w ); }
        double x, y, z;
    };
    struct Matrix {
        Matrix();
        Matrix operator*( const Matrix &m );
        double &operator[]( int i )       { return data[ i ]; }
        double  operator[]( int i ) const { return data[ i ]; }
        double &operator()( int r, int c )       { return data[ 4 * r + c ]; }
        double  operator()( int r, int c ) const { return data[ 4 * r + c ]; }
        Point   operator()( const Point &p ) const;
        Matrix inverse() const;
        
        double data[ 4 * 4 ];
    };
    ///
    struct BackgroundImg {
        struct Pix {
            Pix( double c = 0, double l = 1, double a = 1, double z = 0 ) : c(c), l(l), a(a), z(z) {}
            double c; // field value
            double l; // luminosity
            double a; // alpha chanel
            double z; // depth
        };
        
        BackgroundImg( int w, int h ) : w( w ), h( h ) { data = new Pix[ w * h ]; }
        ~BackgroundImg() { delete [] data; }
        Pix &operator()( int x, int y ) { return data[ y * w + x ]; }
        
        int w, h;
        Pix *data;
    };
    
    ///
    void init_bb();
    ///
    void add_to_bb( const Point &p );
    ///
    void update_bb();
    
    ///
    BackgroundImg img;
    /// "minimum" box -> what we want to see
    double x0, y0, x1, y1;
    double pan_x, pan_y, zoom;
};

#endif // DISPLAY_PAINTER_CONTEXT_H
