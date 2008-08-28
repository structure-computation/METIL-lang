#include <QtGui/QPaintEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <cmath>
#include <iostream>
#include "DisplayWidget.h"

#ifdef QT4_FOUND

DisplayWidget::DisplayWidget( QWidget *parent ) : QWidget( parent ), nb_dim( 0 ) {
    setAutoFillBackground( true );
    
    x_screen_off = 0;
    y_screen_off = 0;
    zoom         = 1;
}

void DisplayWidget::paintEvent( QPaintEvent *qpe ) {
    QPainter painter( this );
    painter.setBrush( Qt::white );
    painter.setPen  ( Qt::NoPen );
    painter.drawRect( 0, 0, width(), height() );
    for(int i=0;i<paint_functions.size();++i)
        paint_functions[i].first( painter, this, paint_functions[i].second, qpe );
}

void DisplayWidget::wheelEvent( QWheelEvent *event ) {
    double fact = std::pow( 2.0, event->delta() / 360.0 );
    double x_m = event->x() - width () / 2.0;
    double y_m = event->y() - height() / 2.0;
    x_screen_off = x_m + fact * ( x_screen_off - x_m );
    y_screen_off = y_m + fact * ( y_screen_off - y_m );
    zoom *= fact;
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

#endif // QT4_FOUND

