#ifndef DISPLAY_WINDOW_CREATOR_H
#define DISPLAY_WINDOW_CREATOR_H

#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtCore/QObject>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QMap>
#include "DisplayPainter.h"

/** */
class DisplayWindowCreator : public QObject {
    Q_OBJECT
public:
    struct Display {
        Display() : window( NULL ) {}
        class DisplayWindow *window;
        DisplayPainter painter;
    };
    
    DisplayWindowCreator();
    void call_add_paint_function( int num_display_window, void *paint_function, void *bounding_box_function, void *data );
    void call_rm_paint_functions( int num_display_window );
    void call_save_as           ( int num_display_window, const char *s, int si, int w, int h );
    void call_set_anti_aliasing ( int num_display_window, bool   val );
    void call_set_shrink        ( int num_display_window, double val );
    void call_update_disp_widget( int num_display_window );
    
    void wait_for_display_windows();
signals:
    void sig_add_paint_function( int, void *, void *, void * );
    void sig_rm_paint_functions( int );
    void sig_update_disp_widget( int );
    void sig_save_as           ( int, QString, int, int );
    void sig_set_anti_aliasing ( int, bool    );
    void sig_set_shrink        ( int, double  );
public slots: // must be called in main_thread
    void add_paint_function( int num_display_window, void *paint_function, void *bounding_box_function, void *data );
    void rm_paint_functions( int num_display_window );
    void update_disp_widget( int num_display_window );
    void save_as           ( int num_display_window, QString f, int w, int h );
    void set_anti_aliasing ( int num_display_window, bool   val );
    void set_shrink        ( int num_display_window, double val );
    void lastWindowClosed();
private:
    void make_num_display_window_if_necessary( int num_display_window );
    QMutex mutex; // for the wait condition
    QWaitCondition last_window_closed;
    bool at_least_one_window_was_created;
    QMap<int,Display> displays;
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
