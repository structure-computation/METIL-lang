#ifndef DISPLAY_WINDOW_H
#define DISPLAY_WINDOW_H

#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtGui/QMainWindow>

/** */
class DisplayWindow : public QMainWindow {
    Q_OBJECT
public:
    DisplayWindow( struct DisplayPainter *display_painter );
    
    class DisplayWidget *disp_widget;
protected:
    void keyPressEvent( QKeyEvent *event );
};

#endif // QT4_FOUND

#endif // DISPLAY_WINDOW_H
