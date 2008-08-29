#include <QtGui/QPaintEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QResizeEvent>
#include <cmath>
#include <iostream>
#include "DisplayWidget.h"

#ifdef QT4_FOUND

DisplayWidget::DisplayWidget( QWidget *parent ) : QWidget( parent ), nb_dim( 0 ) {
    setAutoFillBackground( true );
    
    x_screen_off           = 0;
    y_screen_off           = 0;
    zoom                   = 1;
    anti_aliasing          = false;
    shrink                 = 1;
    zoom_should_be_updated = false;
}

void DisplayWidget::paintEvent( QPaintEvent *qpe ) {
    if ( zoom_should_be_updated ) {
        double x0 = std::numeric_limits<double>::max();
        double y0 = std::numeric_limits<double>::max();
        double x1 = std::numeric_limits<double>::min();
        double y1 = std::numeric_limits<double>::min();
        for(int i=0;i<paint_functions.size();++i)
            paint_functions[i].bounding_box_function( this, paint_functions[i].data, x0, y0, x1, y1 );
        zoom = 0.9 * std::min(
            width () / ( x1 - x0 ),
            height() / ( y1 - y0 )
        );
        zoom_should_be_updated = false;
    }
    //
    QPainter painter( this );
    painter.setBrush( Qt::white );
    painter.setPen  ( Qt::NoPen );
    painter.drawRect( 0, 0, width(), height() );
    for(int i=0;i<paint_functions.size();++i)
        paint_functions[i].paint_function( painter, this, paint_functions[i].data, qpe );
}

void DisplayWidget::wheelEvent( QWheelEvent *event ) {
    if ( event->modifiers() & Qt::ShiftModifier ) {
        shrink += event->delta() / 3600.0;
        shrink = std::max( 0.1, std::min( shrink, 1.0 ) );
    } else {
        double fact = std::pow( 2.0, event->delta() / 360.0 );
        double x_m = event->x() - width () / 2.0;
        double y_m = event->y() - height() / 2.0;
        x_screen_off = x_m + fact * ( x_screen_off - x_m );
        y_screen_off = y_m + fact * ( y_screen_off - y_m );
        zoom *= fact;
    }
    update();
}

void DisplayWidget::mousePressEvent  ( QMouseEvent *event ) {
    x_during_last_mouse_event = event->x();
    y_during_last_mouse_event = event->y();
}

void DisplayWidget::mouseMoveEvent   ( QMouseEvent *event ) {
    if ( event->buttons() & Qt::MidButton ) {
        x_screen_off += event->x() - x_during_last_mouse_event;
        y_screen_off += event->y() - y_during_last_mouse_event;
        update();
    }
    x_during_last_mouse_event = event->x();
    y_during_last_mouse_event = event->y();
}

void DisplayWidget::mouseReleaseEvent( QMouseEvent *event ) {
}

void DisplayWidget::resizeEvent( QResizeEvent *event ) {
    double old_diag = std::sqrt( pow( event->oldSize().width(), 2 ) + pow( event->oldSize().height(), 2 ) );
    double new_diag = std::sqrt( pow(                  width(), 2 ) + pow(                  height(), 2 ) );
    double fact = new_diag / old_diag;
    x_screen_off *= fact;
    y_screen_off *= fact;
    zoom *= fact;
}

#endif // QT4_FOUND

