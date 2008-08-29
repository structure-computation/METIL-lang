#ifndef DISPLAY_WIDGET_H
#define DISPLAY_WIDGET_H

#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtGui/QWidget>
#include <QtGui/QPainter>
#include <QtCore/QVector>
#include <QtCore/QPair>
 
/** */
class DisplayWidget : public QWidget {
public:
    typedef void PaintFunction( QPainter &, DisplayWidget *, void *, QPaintEvent * );
    typedef void BoundingBoxFunction( DisplayWidget *, void *, double &, double &, double &, double & );
    
    struct DispFun {
        PaintFunction       *paint_function;
        BoundingBoxFunction *bounding_box_function;
        void                *data;
    };

    DisplayWidget( QWidget *parent );
    
    QVector<DispFun> paint_functions;
    int nb_dim;
    double zoom;
    double x_screen_off, y_screen_off;
    bool anti_aliasing;
    double shrink;
    bool zoom_should_be_updated;
protected:
    void paintEvent       ( QPaintEvent  *event );
    void wheelEvent       ( QWheelEvent  *event );
    void mousePressEvent  ( QMouseEvent  *event );
    void mouseMoveEvent   ( QMouseEvent  *event );
    void mouseReleaseEvent( QMouseEvent  *event );
    void resizeEvent      ( QResizeEvent *event );
    
    int x_during_last_mouse_event;
    int y_during_last_mouse_event;
};

#endif // QT4_FOUND

#endif // DISPLAY_WIDGET_H

