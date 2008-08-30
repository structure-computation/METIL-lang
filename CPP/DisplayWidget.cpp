#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QResizeEvent>
#include <cmath>

#include "DisplayWidget.h"
#include "DisplayPainter.h"

DisplayWidget::DisplayWidget( QWidget *parent, DisplayPainter *display_painter ) : QWidget( parent ), display_painter( display_painter ) {
    setAutoFillBackground( true );
}

void DisplayWidget::paintEvent( QPaintEvent *event ) {
    QPainter painter( this );
    display_painter->paint( painter, width(), height() );
}

void DisplayWidget::wheelEvent( QWheelEvent *event ) {
    if ( event->modifiers() & Qt::ShiftModifier )
        display_painter->shrink = std::max( 0.1, std::min( display_painter->shrink + event->delta() / 3600.0, 1.0 ) );
    else
        display_painter->zoom( std::pow( 2.0, - event->delta() / 720.0 ), event->x(), event->y(), width(), height() );
    update();
}

void DisplayWidget::mousePressEvent( QMouseEvent *event ) {
    x_during_last_mouse_event = event->x();
    y_during_last_mouse_event = event->y();
}

void DisplayWidget::mouseMoveEvent( QMouseEvent *event ) {
    if ( event->buttons() & Qt::MidButton ) {
        display_painter->pan( x_during_last_mouse_event - event->x(), event->y() - y_during_last_mouse_event, width(), height() );
        update();
    }
    x_during_last_mouse_event = event->x();
    y_during_last_mouse_event = event->y();
}

void DisplayWidget::mouseReleaseEvent( QMouseEvent *event ) {
}

void DisplayWidget::resizeEvent( QResizeEvent *event ) {
}

#endif // QT4_FOUND

