#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QMessageBox>
#include <QtGui/QPrintDialog>
#include <QtGui/QPrinter>
#include <QtGui/QPainter>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include "DisplayWindow.h"
#include "DisplayWidget.h"
#include "DisplayPainter.h"

// --------------------------------------------------------------------------------------------------------------------------

DisplayWindow::DisplayWindow( DisplayPainter *display_painter ) {
    disp_widget = new DisplayWidget( this, display_painter );
    setCentralWidget( disp_widget );
}
    
void DisplayWindow::keyPressEvent( QKeyEvent *event ) {
    if ( event->key() == Qt::Key_Q or event->key() == Qt::Key_Escape ) {
        close();
    } else if ( event->key() == Qt::Key_A ) {
        disp_widget->display_painter->anti_aliasing = not disp_widget->display_painter->anti_aliasing;
        QTimer::singleShot( 0, disp_widget, SLOT(update()) );
    } else if ( event->key() == Qt::Key_B ) {
        disp_widget->display_painter->borders = not disp_widget->display_painter->borders;
        QTimer::singleShot( 0, disp_widget, SLOT(update()) );
    } else if ( event->key() == Qt::Key_F ) {
        disp_widget->display_painter->zoom_should_be_updated = true;
        QTimer::singleShot( 0, disp_widget, SLOT(update()) );
    } else if ( event->key() == Qt::Key_P ) {
        QPrinter printer;
        QPrintDialog printDialog( &printer, NULL );
        if ( printDialog.exec() == QDialog::Accepted ) {
            QPainter painter( &printer );
            QRectF pr( printer.pageRect() );
            disp_widget->display_painter->paint( painter, pr.width(), pr.height() );
        }
    } else if ( event->key() == Qt::Key_H ) {
        QMessageBox::information( NULL, "Help", tr(
            "<strong>Bindings</strong> :" \
            "<table><tbody>" \
            "<tr><td> <em>Q or Esc     </em> </td><td></td> quit </tr>" \
            "<tr><td> <em>A            </em> </td><td></td> toggle anti-aliasing </tr>" \
            "<tr><td> <em>B            </em> </td><td></td> toggle surface borders </tr>" \
            "<tr><td> <em>F            </em> </td><td></td> fit in window </tr>" \
            "<tr><td> <em>H            </em> </td><td></td> help </tr>" \
            "<tr><td> <em>P            </em> </td><td></td> print </tr>" \
            "<tr><td> <em>wheel        </em> </td><td></td> zoom </tr>" \
            "<tr><td> <em>shift-wheel  </em> </td><td></td> surface shrink ratio </tr>" \
            "<tr><td> <em>middle button</em> </td><td></td> pan </tr>" \
            "</tbody>" \
            "</table>" \
        ) );
    }
}

#endif
