#ifndef DISPLAY_PAINTER_H
#define DISPLAY_PAINTER_H

#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtCore/QVector>
#include <QtGui/QColor>

#include "DisplayPainterContext.h"

/** */
struct DisplayPainter {
    typedef void MakeTexFunction( DisplayPainter_NS::BackgroundImg &img, DisplayPainter *, void * );
    typedef void PaintFunction( class QPainter &, DisplayPainter_NS::BackgroundImg &img, DisplayPainter *, void * );
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
    double shrink, mi, ma;
    bool anti_aliasing, borders, zoom_should_be_updated;
    SimpleGradient color_bar_gradient;
    QMap<QString,ColorSet> color_sets;
    QString cur_color_mode;
    ColorSet cur_color_set;
};

#endif // QT4_FOUND

#endif // DISPLAY_PAINTER_H
