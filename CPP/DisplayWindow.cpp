#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtCore/QTimer>
#include "DisplayWindow.h"
#include "DisplayWidget.h"

#ifdef QT4_FOUND

// --------------------------------------------------------------------------------------------------------------------------

DisplayWindow::DisplayWindow() {
    disp_widget = new DisplayWidget( this );
    setCentralWidget( disp_widget );
}

void DisplayWindow::add_paint_function( void *paint_function, void *data ) {
    disp_widget->paint_functions.push_back( qMakePair( reinterpret_cast<DisplayWidget::PaintFunction *>(paint_function), data ) );
}

void DisplayWindow::rm_paint_functions() {
    disp_widget->paint_functions.clear();
}

void DisplayWindow::update_disp_widget() {
    disp_widget->update();
}

void DisplayWindow::keyPressEvent( QKeyEvent *event ) {
    if ( event->key() == Qt::Key_Q or event->key() == Qt::Key_Escape ) {
        close();
    }
}

#endif
