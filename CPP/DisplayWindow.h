#ifndef DISPLAY_WINDOW_H
#define DISPLAY_WINDOW_H

#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtGui/QMainWindow>

/** */
class DisplayWindow : public QMainWindow {
    Q_OBJECT
public:
    DisplayWindow();
    void add_paint_function( void *paint_function, void *data );
    void rm_paint_functions();
    void update_disp_widget();
protected:
    void keyPressEvent( QKeyEvent *event );

    class DisplayWidget *disp_widget;
};

#endif // QT4_FOUND

#endif // DISPLAY_WINDOW_H
