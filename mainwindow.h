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
#include <QPrinter>
#include <QFileDialog>
#include <QtGlobal>

class point{
public:
    point();
    point(double,double,double,double);

   double x_nom,x_act,y_nom,y_act;
};


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
    QVector <point> *data;
    Ui::MainWindow *ui;
    void paintEvent(QPaintEvent *);
    QVector <QPair <double,double> > *nom_control_points;
    QVector <QPair <double,double> > *act_control_points;

    QRectF *area_graphic;
    QRectF *area_table;
    QRectF *area_header;

    QPainterPath *nom_path;
    QPainterPath *act_path;
    QPointF *nom_points;

    //value in pixel for printer margin

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

};

#endif // MAINWINDOW_H
