#ifndef DISPLAY_PAINTER_H
#define DISPLAY_PAINTER_H

#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtCore/QVector>
#include <QtGui/QColor>

struct DisplayPainter {
    struct DisplayPainterBackgroundImg {
        struct Pix {
            Pix( double c = 0, double l = 1, double a = 1, double z = 0 ) : c(c), l(l), a(a), z(z) {}
            double c; // field value
            double l; // luminosity
            double a; // alpha chanel
            double z; // z
        };
        
        DisplayPainterBackgroundImg( int w, int h, double orig_x0, double orig_y0, double orig_x1, double orig_y1 ) : w(w), h(h) {
            double scale_x = w / ( orig_x1 - orig_x0 );
            double scale_y = h / ( orig_y1 - orig_y0 );
            if ( scale_x > scale_y ) {
                x0 = 0.5 * ( orig_x0 + orig_x1 ) - ( orig_x1 - orig_x0 ) * scale_x / scale_y;
                x1 = 0.5 * ( orig_x0 + orig_x1 ) + ( orig_x1 - orig_x0 ) * scale_x / scale_y;
                y0 = orig_y0;
                y1 = orig_y1;
            } else {
                x0 = orig_x0;
                x1 = orig_x1;
                y0 = 0.5 * ( orig_y0 + orig_y1 ) - ( orig_y1 - orig_y0 ) * scale_y / scale_x;
                y1 = 0.5 * ( orig_y0 + orig_y1 ) + ( orig_y1 - orig_y0 ) * scale_y / scale_x;
            }
            sh = ( h - 1 ) * w;
            iw = w;
            data = new Pix[ w * h ];
        }
        ~DisplayPainterBackgroundImg() { delete [] data; }
        Pix &operator()( int x, int y ) { return data[ sh - y * iw + x ]; }
        
        double local_x( double x ) const { return ( x - x0 ) * w / ( x1 - x0 ); }
        double local_y( double y ) const { return ( y - y0 ) * h / ( y1 - y0 ); }
        
        double local_x_sat( double x ) const { return std::max( 0.0, std::min( w - 1, local_x( x ) ) ); }
        double local_y_sat( double y ) const { return std::max( 0.0, std::min( h - 1, local_y( y ) ) ); }
        
        double w, h, x0, y0, x1, y1;
        int sh, iw;
        Pix *data;
    };

    typedef void MakeTexFunction( DisplayPainterBackgroundImg &img, DisplayPainter *, void * );
    typedef void PaintFunction( class QPainter &, DisplayPainter *, void * );
    typedef void BoundingBoxFunction( DisplayPainter *, void *, double &, double &, double &, double & );
    
    struct DispFun {
        MakeTexFunction     *make_tex_function;
        PaintFunction       *paint_function;
        BoundingBoxFunction *bounding_box_function;
        void                *data;
    };
    
    struct SimpleGradient {
        struct Stop { double p, r, g, b, a; };
        void append( double p, double r, double g, double b, double a = 1 );
        QColor color_at( double p );
        QColor color_at( double p, double lum, double alpha );
        
        QVector<Stop> data;
    };
    
    struct ColorSet {
        ColorSet() : fg_color(0,0,0), bg_color(255,255,255), bg_color_0(255,255,255), bg_color_1(255,255,255) {}
        QColor fg_color;
        QColor bg_color;
        QColor bg_color_0;
        QColor bg_color_1;
    };
    
    DisplayPainter(); ///
    void load_settings( class QSettings *settings );
    void save_settings( class QSettings *settings );
    
    void add_paint_function( void *make_tex_function, void *paint_function, void *bounding_box_function, void *data );
    void rm_paint_functions();
    void set_anti_aliasing( bool val );
    void set_shrink( double val );
    void save_as( const QString &filename, int w, int h );
    void change_color_mode( QString cm );
    
    void paint( class QPainter &painter, int w = 0, int h = 0 ); ///
    double get_scale_r( double w, double h ) const; // in pixel / real_dim
    void zoom( double fact, double x, double y, double w, double h );
    void pan( double dx, double dy, double w, double h );
    void set_min_max( double mi, double ma );
    double scaled_val( double val ) const { return mi + val / ( ma - mi + ( ma == mi ) ) * ( ma != mi ); }
    
    double xm() const { return 0.5 * ( x0 + x1 ); }
    double ym() const { return 0.5 * ( y0 + y1 ); }
    
    double real_x0( double w, double h ) const { return xm() - 0.5 * w / get_scale_r(w,h); }
    double real_y0( double w, double h ) const { return ym() - 0.5 * h / get_scale_r(w,h); }
    double real_x1( double w, double h ) const { return xm() + 0.5 * w / get_scale_r(w,h); }
    double real_y1( double w, double h ) const { return ym() + 0.5 * h / get_scale_r(w,h); }
    
    
    QVector<DispFun> paint_functions;
    int nb_dim;
    double x0, y0, x1, y1; // "minimum" box -> what we want to see
    double shrink, mi, ma;
    bool anti_aliasing, borders, zoom_should_be_updated;
    SimpleGradient color_bar_gradient;
    QMap<QString,ColorSet> color_sets;
    QString cur_color_mode;
    ColorSet cur_color_set;
};

#endif // QT4_FOUND

#endif // DISPLAY_PAINTER_H
