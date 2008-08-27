#ifndef DISPLAY_WINDOW_H
#define DISPLAY_WINDOW_H

#include "metil_qt_config.h"

#ifdef QT4_FOUND
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>

class DisplayWindow : public QPushButton {
    Q_OBJECT
public:
    DisplayWindow( struct DisplayWindowCreator *dw, int nb_dim );
    ~DisplayWindow();
protected:
    int nb_dim_;
    struct DisplayWindowCreator *dw;
};

/** */
class DisplayWindowCreator : public QObject {
    Q_OBJECT
public:
    DisplayWindowCreator();
    void make_new_window( DisplayWindow **dw, int nb_dim );
signals:
    void _make_new_window( DisplayWindow **dw, int nb_dim );
    void end_of_the_beans();
public slots:
    void __make_new_window( DisplayWindow **dw, int nb_dim );
    void lastWindowClosed();
public:
    bool last_window_closed;
};

void __wait_for_display_windows__( struct Thread *th );

#else // QT4_FOUND

class DisplayWindow {
public:
};

class DisplayWindowCreator {
public:
    void make_new_window( DisplayWindow **dw, int nb_dim ) { *dw = NULL; }
}

void __wait_for_display_windows__( Thread *th ) {}

#endif

typedef DisplayWindow *DisplayWindowPtr;

#endif // DISPLAY_WINDOW_H
