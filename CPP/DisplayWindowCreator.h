#ifndef DISPLAY_WINDOW_CREATOR_H
#define DISPLAY_WINDOW_CREATOR_H

#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtCore/QObject>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QMap>

/** */
class DisplayWindowCreator : public QObject {
    Q_OBJECT
public:
    DisplayWindowCreator();
    void call_add_paint_function( int num_display_window, void *paint_function, void *bounding_box_function, void *data );
    void call_rm_paint_functions( int num_display_window );
    void call_update_disp_widget( int num_display_window );
    void set_anti_aliasing      ( int num_display_window, bool   val );
    void set_shrink             ( int num_display_window, double val );
    void wait_for_display_windows();
signals:
    void sig_add_paint_function( int, void *, void *, void * );
    void sig_rm_paint_functions( int );
    void sig_update_disp_widget( int );
public slots: // must be called in main_thread
    void add_paint_function( int num_display_window, void *paint_function, void *bounding_box_function, void *data );
    void rm_paint_functions( int num_display_window );
    void update_disp_widget( int num_display_window );
    void lastWindowClosed();
private:
    QMutex mutex; // for the wait condition
    QWaitCondition last_window_closed;
    bool at_least_one_window_was_created;
    QMap<int,class DisplayWindow *> display_windows;
};

void __wait_for_display_windows__( struct Thread *th );

#else // QT4_FOUND

class DisplayWindowCreator {
public:
    void call_add_paint_plugin( int num_display_window, char *filename, int size_filename ) {}
    void call_clr_paint_plugin( int num_display_window ) {}
};

inline void __wait_for_display_windows__( struct Thread *th ) {
}

#endif // QT4_FOUND

#endif // DISPLAY_WINDOW_CREATOR_H
