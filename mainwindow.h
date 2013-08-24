#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QDir>
#include "sp_xmlread.h"
#include <QString>
#include <QRectF>
#include <QPainter>
#include <QPair>
#include <QtPrintSupport/QPrinter>
#include <QFileDialog>
#include <QtGlobal>
#include <QPixmap>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_pushButton_clicked();

private:


    Ui::MainWindow *ui;
    void paintEvent(QPaintEvent *);
    QVector <QPair <double,double> > *nom_control_points;
    QVector <QPair <double,double> > *act_control_points;
    QVector <QPair <double,double> > *ut_control_points;
    QVector <QPair <double,double> > *lt_control_points;

    QVector <point> *nom_data;
    QVector <point> *act_data;

    QRectF *area_graphic;
    QRectF *area_table;
    QRectF *area_header;

    QPainterPath *nom_path;
    QPainterPath *act_path;
    QPainterPath *ut_path;
    QPainterPath *lt_path;

    QVector <QPointF> nom_points;

    int size;       //point size
    //value in pixel for printer margin
    Sp_xmlread *xml_info;

    qreal left_margin;
    qreal right_margin;
    qreal top_margin;
    qreal bottom_margin;
    QRectF papersize;
    QPrinter printer;
    QRectF painter_area;
    QRectF curve_area;
    QRectF table_area;
    QRectF header_area;

    QRectF curve_range;
    QPointF curve_translate;
    qreal scale_x,scale_y;
    QPixmap *logo;
};

#endif // MAINWINDOW_H
