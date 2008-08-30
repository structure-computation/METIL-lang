#ifndef DISPLAY_PAINTER_H
#define DISPLAY_PAINTER_H

#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtCore/QVector>
#include <QtGui/QColor>

struct DisplayPainter {
    typedef void PaintFunction( class QPainter &, DisplayPainter *, void * );
    typedef void BoundingBoxFunction( DisplayPainter *, void *, double &, double &, double &, double & );
    
    struct DispFun {
        PaintFunction       *paint_function;
        BoundingBoxFunction *bounding_box_function;
        void                *data;
    };
    
    struct SimpleGradient {
        struct Stop { double p, r, g, b, a; };
        void append( double p, double r, double g, double b, double a = 1 );
        QColor color_at( double p );
        
        QVector<Stop> data;
    };
    
    
    DisplayPainter(); ///
    
    void add_paint_function( void *paint_function, void *bounding_box_function, void *data );
    void rm_paint_functions();
    void set_anti_aliasing( bool val );
    void set_shrink( double val );
    void save_as( const QString &filename, int w, int h );
    
    void paint( class QPainter &painter, int w = 0, int h = 0 ); ///
    double get_scale_r( double w, double h ) const;
    void zoom( double fact, double x, double y, double w, double h );
    void pan( double dx, double dy, double w, double h );

    
    double xm() const { return 0.5 * ( x0 + x1 ); }
    double ym() const { return 0.5 * ( y0 + y1 ); }
    
    
    QVector<DispFun> paint_functions;
    int nb_dim;
    double x0, y0, x1, y1, shrink;
    bool anti_aliasing, borders, zoom_should_be_updated;
    SimpleGradient color_bar_gradient;
};

#endif // QT4_FOUND

#endif // DISPLAY_PAINTER_H
