#include <QtGui/QApplication>
#include "DisplayWindow.h"
#include "thread.h"

#ifdef QT4_FOUND

DisplayWindow::DisplayWindow( DisplayWindowCreator *dw, int nb_dim ) : QPushButton("pouet"), dw( dw ) {
}

DisplayWindow::~DisplayWindow() {
}

// --------------------------------------------------------------------------------------------------------------------------

DisplayWindowCreator::DisplayWindowCreator() {
    connect( this, SIGNAL(_make_new_window(DisplayWindow **, int)), this, SLOT(__make_new_window(DisplayWindow **, int)) );
    connect( qApp, SIGNAL(lastWindowClosed()), this, SLOT(lastWindowClosed()) );
    last_window_closed = true;
}

void DisplayWindowCreator::make_new_window( DisplayWindow **dw, int nb_dim ) {
    emit _make_new_window( dw, nb_dim );
}

void DisplayWindowCreator::__make_new_window( DisplayWindow **dw, int nb_dim ) {
    *dw = NULL;
    *dw = new DisplayWindow( this, nb_dim );
    (*dw)->show();
    last_window_closed = false;
}

void DisplayWindowCreator::lastWindowClosed() {
    std::cout << "close" << std::endl;
    last_window_closed = true;
}

// --------------------------------------------------------------------------------------------------------------------------
void __wait_for_display_windows__( Thread *th ) {
    std::cout << "Waint" << std::endl;
    while ( not (volatile bool &)th->display_window_creator->last_window_closed );
    std::cout << "Evc" << std::endl;
}

#endif
