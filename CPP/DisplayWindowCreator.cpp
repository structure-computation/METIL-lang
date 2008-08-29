#include <QtGui/QApplication>
#include "DisplayWindowCreator.h"
#include "DisplayWindow.h"
#include "thread.h"

#ifdef QT4_FOUND

DisplayWindowCreator::DisplayWindowCreator() {
    connect( qApp, SIGNAL(lastWindowClosed()), this, SLOT(lastWindowClosed()) );
    connect( this, SIGNAL(sig_add_paint_function( int, void *, void *, void * )), this, SLOT(add_paint_function( int, void *, void *, void * )) );
    connect( this, SIGNAL(sig_rm_paint_functions( int                         )), this, SLOT(rm_paint_functions( int                         )) );
    connect( this, SIGNAL(sig_update_disp_widget( int                         )), this, SLOT(update_disp_widget( int                         )) );
    at_least_one_window_was_created = false;
}

void DisplayWindowCreator::call_add_paint_function( int num_display_window, void *paint_function, void *bounding_box_function, void *data ) {
    at_least_one_window_was_created = true;
    emit sig_add_paint_function( num_display_window, paint_function, bounding_box_function, data );
}

void DisplayWindowCreator::call_rm_paint_functions( int num_display_window ) {
    emit sig_rm_paint_functions( num_display_window );
}

void DisplayWindowCreator::call_update_disp_widget( int num_display_window ) {
    emit sig_update_disp_widget( num_display_window );
}

void DisplayWindowCreator::set_anti_aliasing( int num_display_window, bool val ) {
    if ( display_windows.contains( num_display_window ) )
        display_windows[ num_display_window ]->set_anti_aliasing( val );
}

void DisplayWindowCreator::set_shrink( int num_display_window, double val ) {
    if ( display_windows.contains( num_display_window ) )
        display_windows[ num_display_window ]->set_shrink( val );
}

void DisplayWindowCreator::add_paint_function( int num_display_window, void *paint_function, void *bounding_box_function, void *data ) {
    if ( not display_windows.contains( num_display_window ) ) {
        DisplayWindow *res = new DisplayWindow();
        display_windows[ num_display_window ] = res;
        res->show();
    }
    display_windows[ num_display_window ]->add_paint_function( paint_function, bounding_box_function, data );
}

void DisplayWindowCreator::rm_paint_functions( int num_display_window ) {
    if ( display_windows.contains( num_display_window ) )
        display_windows[ num_display_window ]->rm_paint_functions();
}

void DisplayWindowCreator::update_disp_widget( int num_display_window ) {
    if ( display_windows.contains( num_display_window ) )
        display_windows[ num_display_window ]->update_disp_widget();
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

#endif // QT4_FOUND
