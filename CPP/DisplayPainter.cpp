#include "metil_qt_config.h"
#ifdef QT4_FOUND

#include <QtGui/QPixmap>
#include <QtGui/QImageWriter>
#include <QtGui/QPrinter>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QMessageBox>
#include <QtSvg/QSvgGenerator>
#include <QtCore/QSettings>

#include "DisplayPainter.h"
#include <iostream>

QLinearGradient to_QLinearGradient( const DisplayPainter::SimpleGradient &sg, double x0, double y0, double x1, double y1 ) {
    QLinearGradient res( x0, y0, x1, y1 );
    for( int i=0; i < sg.data.size(); ++i )
        res.setColorAt( sg.data[i].p, QColor( 255 * sg.data[i].r, 255 * sg.data[i].g, 255 * sg.data[i].b, 255 * sg.data[i].a ) );
    return res;
}
void DisplayPainter::SimpleGradient::append( double p, double r, double g, double b, double a ) {
    Stop s; s.p = p; s.r = r; s.g = g; s.b = b; s.a = a;
    data.push_back( s );
}
QColor DisplayPainter::SimpleGradient::color_at( double p ) {
    p = std::min( 1.0, std::max( 0.0, p ) );
    for( int i=0; i < data.size(); ++i ) {
        if ( data[i+1].p >= p ) {
            double d = data[i+1].p - data[i].p; d += not d;
            return QColor( 
                255 * ( data[i].r + ( p - data[i].p ) / d * ( data[i+1].r - data[i].r ) ),
                255 * ( data[i].g + ( p - data[i].p ) / d * ( data[i+1].g - data[i].g ) ),
                255 * ( data[i].b + ( p - data[i].p ) / d * ( data[i+1].b - data[i].b ) ),
                255 * ( data[i].a + ( p - data[i].p ) / d * ( data[i+1].a - data[i].a ) )
            );
        }
    }
    return QColor( 0, 0, 0 );
}


DisplayPainter::DisplayPainter() {
    x0 = 0; y0 = 0;
    x1 = 1; y1 = 1;
    anti_aliasing          = 0;
    borders                = 0;
    shrink                 = 1;
    zoom_should_be_updated = 0;
    
    color_bar_gradient.append( 0.0, 0.0, 0.0, 1.0 );
    color_bar_gradient.append( 0.3, 0.0, 1.0, 0.0 );
    color_bar_gradient.append( 0.6, 1.0, 0.0, 0.0 );
    color_bar_gradient.append( 1.0, 1.0, 1.0, 1.0 );
}

void DisplayPainter::load_settings( QSettings *settings ) {
    settings->beginGroup("DisplayPainter");
     anti_aliasing = settings->value( "anti_aliasing", false ).toBool();
     borders       = settings->value( "borders"      , false ).toBool();
     shrink        = settings->value( "shrink"       , 1 ).toDouble();
    settings->endGroup();
}

void DisplayPainter::save_settings( QSettings *settings ) {
    settings->beginGroup("DisplayPainter");
     settings->setValue( "anti_aliasing", anti_aliasing );
     settings->setValue( "borders"      , borders       );
     settings->setValue( "shrink"       , shrink        );
    settings->endGroup();
}

void DisplayPainter::add_paint_function( void *paint_function, void *bounding_box_function, void *data ) {
    DispFun df;
    df.paint_function        = reinterpret_cast<PaintFunction       *>(paint_function       );
    df.bounding_box_function = reinterpret_cast<BoundingBoxFunction *>(bounding_box_function);
    df.data                  = data;

    paint_functions.push_back( df );
    zoom_should_be_updated = true;
}

void DisplayPainter::rm_paint_functions() {
    paint_functions.clear();
}

void DisplayPainter::set_anti_aliasing( bool val ) {
    anti_aliasing = val;
}

void DisplayPainter::set_shrink( double val ) {
    shrink = val;
}

void DisplayPainter::save_as( const QString &filename, int w, int h ) {
    if ( filename.endsWith( ".pdf" ) ) {
        QPrinter printer;
        // printer.setPageSize( QPrinter::Custom );
        // printer.setPageSize( QPrinterPageSize( disp_widget->width(), disp_widget->height() ) );
        // printer.setPageMargins   ( 0, 0, 0, 0, QPrinter::DevicePixel );
        printer.setOutputFileName( filename );
        //
        QPainter painter( &printer );
        paint( painter, printer.pageRect().width(), printer.pageRect().height() );
    } else if ( filename.endsWith( ".svg" ) ) {
        QSvgGenerator printer;
        printer.setSize( QSize( w, h ) );
        printer.setFileName( filename );
        //
        QPainter painter( &printer );
        paint( painter, w, h );
    } else {
        QList<QByteArray> lst_ext_pix = QImageWriter::supportedImageFormats();
        bool pix_ok = false;
        for(int i=0;i<lst_ext_pix.size();++i)
            pix_ok |= filename.endsWith( lst_ext_pix[i] );
        if ( pix_ok ) {
            QPixmap printer( w, h );
            //
            QPainter painter( &printer );
            paint( painter, w, h );
            printer.save( filename );
        } else
            QMessageBox::warning( NULL, "save as problem", filename + " does not have an allowed extension. Only png, jpg, svg and pdf are allowed." );
    }
}

double DisplayPainter::get_scale_r( double w, double h ) const {
    double scale_x = w / ( x1 - x0 );
    double scale_y = h / ( y1 - y0 );
    return std::min( scale_x, scale_y ) * 0.95;
}

void DisplayPainter::zoom( double fact, double x, double y, double w, double h ) {
    double xd = ( x - 0.5 * w ) / get_scale_r( w, h ) + xm();
    double yd = ( 0.5 * h - y ) / get_scale_r( w, h ) + ym();
    double ox0 = x0, oy0 = y0, ox1 = x1, oy1 = y1;
    x0 = xd + fact * ( ox0 - xd ); y0 = yd + fact * ( oy0 - yd );
    x1 = xd + fact * ( ox1 - xd ); y1 = yd + fact * ( oy1 - yd );
}

void DisplayPainter::pan( double dx, double dy, double w, double h ) {
    dx /= get_scale_r( w, h );
    dy /= get_scale_r( w, h );
    x0 += dx; y0 += dy;
    x1 += dx; y1 += dy;
}

void DisplayPainter::set_min_max( double mi_, double ma_ ) {
    mi = mi_; ma = ma_;
}

void DisplayPainter::paint( QPainter &painter, int w, int h ) {
    // hints
    painter.setRenderHint( QPainter::Antialiasing, anti_aliasing );
    if ( not w ) w = x1 - x0;
    if ( not h ) h = x1 - x0;
    
    // recompute x0, ... ?
    if ( zoom_should_be_updated ) {
        x0 = std::numeric_limits<double>::max();
        y0 = std::numeric_limits<double>::max();
        x1 = std::numeric_limits<double>::min();
        y1 = std::numeric_limits<double>::min();
        for(int i=0;i<paint_functions.size();++i)
            paint_functions[i].bounding_box_function( this, paint_functions[i].data, x0, y0, x1, y1 );
        zoom_should_be_updated = false;
    }
    
    // background
    QLinearGradient bg_gradient( QPoint( 0, 0 ), QPoint( 0, h ) );
    bg_gradient.setColorAt( 0.0, QColor(  0,  0,  0 ) );
    bg_gradient.setColorAt( 0.5, QColor(  0,  0, 10 ) );
    bg_gradient.setColorAt( 1.0, QColor(  0,  0, 60 ) );
    painter.setBrush( bg_gradient );
    painter.setPen  ( Qt::NoPen   );
    painter.drawRect( 0, 0, w, h );
    
    // data
    painter.setBrush( Qt::white );
    painter.translate(  0.5 * w         ,  0.5 * h          );
    painter.scale    (  get_scale_r(w,h), -get_scale_r(w,h) );
    painter.translate( -xm()            , -ym()             );
    for(int i=0;i<paint_functions.size();++i)
        paint_functions[i].paint_function( painter, this, paint_functions[i].data );
}

#endif // QT4_FOUND
