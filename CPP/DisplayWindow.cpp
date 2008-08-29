#include <QtGui/QApplication>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include "DisplayWindow.h"
#include "DisplayWidget.h"

#ifdef QT4_FOUND

// --------------------------------------------------------------------------------------------------------------------------

DisplayWindow::DisplayWindow() {
    disp_widget = new DisplayWidget( this );
    setCentralWidget( disp_widget );
}

void DisplayWindow::add_paint_function( void *paint_function, void *bounding_box_function, void *data ) {
    DisplayWidget::DispFun df;
    df.paint_function = reinterpret_cast<DisplayWidget::PaintFunction *>(paint_function);
    df.bounding_box_function = reinterpret_cast<DisplayWidget::BoundingBoxFunction *>(bounding_box_function);
    df.data = data;

    disp_widget->paint_functions.push_back( df );
    disp_widget->zoom_should_be_updated = true;
}

void DisplayWindow::rm_paint_functions() {
    disp_widget->paint_functions.clear();
}

void DisplayWindow::update_disp_widget() {
    disp_widget->update();
}
    
void DisplayWindow::set_anti_aliasing( bool val ) {
    disp_widget->anti_aliasing = val;
}

void DisplayWindow::set_shrink( double val ) {
    disp_widget->shrink = val;
}

void DisplayWindow::keyPressEvent( QKeyEvent *event ) {
    if ( event->key() == Qt::Key_Q or event->key() == Qt::Key_Escape ) {
        close();
    } else if ( event->key() == Qt::Key_A ) {
        disp_widget->anti_aliasing = not disp_widget->anti_aliasing;
        QTimer::singleShot( 0, disp_widget, SLOT(update()) );
    }
}

#endif
