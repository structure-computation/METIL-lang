#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtGui/QApplication>
#include <QtCore/QDebug>
#include "DisplayWindowCreator.h"
#include "DisplayWindow.h"
#include "DisplayWidget.h"
#include "thread.h"

DisplayWindowCreator::DisplayWindowCreator() {
    connect( qApp, SIGNAL(lastWindowClosed()), this, SLOT(lastWindowClosed()) );
    connect( this, SIGNAL(sig_add_paint_function( int, void *, void *, void * )), this, SLOT(add_paint_function( int, void *, void *, void * )) );
    connect( this, SIGNAL(sig_rm_paint_functions( int                         )), this, SLOT(rm_paint_functions( int                         )) );
    connect( this, SIGNAL(sig_update_disp_widget( int                         )), this, SLOT(update_disp_widget( int                         )) );
    connect( this, SIGNAL(sig_save_as           ( int, QString, int , int     )), this, SLOT(save_as           ( int, QString, int , int     )) );
    connect( this, SIGNAL(sig_set_anti_aliasing ( int, bool                   )), this, SLOT(set_anti_aliasing ( int, bool                   )) );
    connect( this, SIGNAL(sig_set_shrink        ( int, double                 )), this, SLOT(set_shrink        ( int, double                 )) );

    at_least_one_window_was_created = false;
}

void DisplayWindowCreator::call_add_paint_function( int num_display_window, void *paint_function, void *bounding_box_function, void *data ) {
    emit sig_add_paint_function( num_display_window, paint_function, bounding_box_function, data );
}

void DisplayWindowCreator::call_rm_paint_functions( int num_display_window ) {
    emit sig_rm_paint_functions( num_display_window );
}

void DisplayWindowCreator::call_update_disp_widget( int num_display_window ) {
    at_least_one_window_was_created = true;
    emit sig_update_disp_widget( num_display_window );
}
    
void DisplayWindowCreator::call_save_as( int num_display_window, const char *s, int si, int w, int h ) {
    std::string st( s, si );
    emit sig_save_as( num_display_window, QString( st.c_str() ), w, h );
}

void DisplayWindowCreator::call_set_anti_aliasing( int num_display_window, bool val ) {
    emit sig_set_anti_aliasing( num_display_window, val );
}

void DisplayWindowCreator::call_set_shrink( int num_display_window, double val ) {
    emit sig_set_shrink( num_display_window, val );
}





void DisplayWindowCreator::add_paint_function( int num_display_window, void *paint_function, void *bounding_box_function, void *data ) {
    displays[ num_display_window ].painter.add_paint_function( paint_function, bounding_box_function, data );
}

void DisplayWindowCreator::rm_paint_functions( int num_display_window ) {
    if ( displays.contains( num_display_window ) )
        displays[ num_display_window ].painter.rm_paint_functions();
}

void DisplayWindowCreator::save_as( int num_display_window, QString qs, int w, int h ) {
    if ( displays.contains( num_display_window ) )
        displays[ num_display_window ].painter.save_as( qs, w, h );
}

void DisplayWindowCreator::set_anti_aliasing( int num_display_window, bool val ) {
    displays[ num_display_window ].painter.set_anti_aliasing( val );
}

void DisplayWindowCreator::set_shrink( int num_display_window, double val ) {
    displays[ num_display_window ].painter.set_shrink( val );
}

DisplayWindow *new_disp_win_if_nec( DisplayWindowCreator::Display &d ) {
    if ( not d.window ) {
        d.window = new DisplayWindow( &d.painter );
        d.window->show();
    }
    return d.window;
}

void DisplayWindowCreator::update_disp_widget( int num_display_window ) {
    if ( displays.contains( num_display_window ) )
        new_disp_win_if_nec( displays[ num_display_window ] )->disp_widget->update();
}



void DisplayWindowCreator::wait_for_display_windows() {
    if ( at_least_one_window_was_created ) {
        mutex.lock();
        last_window_closed.wait( &mutex );
        mutex.unlock();
    }
}

void DisplayWindowCreator::lastWindowClosed() {
    at_least_one_window_was_created = false;
    last_window_closed.wakeAll();
}

// --------------------------------------------------------------------------------------------------------------------------
void __wait_for_display_windows__( Thread *th ) {
    th->display_window_creator->wait_for_display_windows();
}

#endif // QT4_FOUND
