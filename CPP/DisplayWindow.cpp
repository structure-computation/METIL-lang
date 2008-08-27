#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtCore/QTimer>
#include "DisplayWindow.h"
#include "thread.h"

#ifdef QT4_FOUND

// --------------------------------------------------------------------------------------------------------------------------
DisplayWidget::DisplayWidget( int nb_dim, QWidget *parent ) : QWidget( parent ) {
    paint_function = NULL;
    nb_dim_ = nb_dim;
}

void DisplayWidget::paintEvent( QPaintEvent *qpe ) {
    if ( paint_function )
        paint_function( this, nb_dim_, qpe );
}


// --------------------------------------------------------------------------------------------------------------------------

DisplayWindow::DisplayWindow( int nb_dim, void *paint_function ) {
    disp_widget = new DisplayWidget( nb_dim, this );
    disp_widget->paint_function = reinterpret_cast<DisplayWidget::PaintFunction *>(paint_function);
    setCentralWidget( disp_widget );
}

// --------------------------------------------------------------------------------------------------------------------------

DisplayWindowCreator::DisplayWindowCreator() {
    connect( this, SIGNAL(_make_new_window(DisplayWindow **, int, void *)), this, SLOT(__make_new_window(DisplayWindow **, int, void *)) );
    connect( qApp, SIGNAL(lastWindowClosed()), this, SLOT(lastWindowClosed()) );
    at_least_one_window_was_created = false;
}

void DisplayWindowCreator::make_new_window( DisplayWindow **dw, int nb_dim, void *paint_function ) {
    at_least_one_window_was_created = true;
    emit _make_new_window( dw, nb_dim, paint_function );
}

void DisplayWindowCreator::__make_new_window( DisplayWindow **dw, int nb_dim, void *paint_function ) {
    *dw = new DisplayWindow( nb_dim, paint_function );
    (*dw)->show();
}

void DisplayWindowCreator::lastWindowClosed() {
    at_least_one_window_was_created = false;
    last_window_closed.wakeAll();
}

void DisplayWindowCreator::wait_for_display_windows() {
    if ( at_least_one_window_was_created ) {
        mutex.lock();
        last_window_closed.wait( &mutex );
        mutex.unlock();
    }
}

// --------------------------------------------------------------------------------------------------------------------------
void __wait_for_display_windows__( Thread *th ) {
    th->display_window_creator->wait_for_display_windows();
}

#endif
