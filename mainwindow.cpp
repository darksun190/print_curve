#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <math.h>
#include <qmath.h>

bool compare_X(const point a,const point b)
{
    return a.x<b.x;
}

bool compare_Y(const point a,const point b)
{
    return a.y<b.y;
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //init the printer paper size and type
    //this is very important for the display and save file.


    printer.setPaperSize(QPrinter::A4); //A4 in pixel is 1122.52 * 793.701
    printer.setOrientation(QPrinter::Landscape);
    printer.setOutputFormat(QPrinter::PdfFormat);
    //printer.setFullPage(true);

    printer.setPageMargins(50.0,40.0,80.0,44.0,QPrinter::DevicePixel);
    printer.getPageMargins(&left_margin,&top_margin,&right_margin,&bottom_margin,QPrinter::DevicePixel);
    papersize.setRect(0,0,printer.width(),printer.height());
    //qDebug()<<left_margin<<top_margin<<right_margin<<bottom_margin;
    this->setFixedSize(printer.width(),printer.height());

    painter_area.setRect(left_margin,top_margin,papersize.width()-left_margin-right_margin,papersize.height()-top_margin-bottom_margin);

    const qreal eff1=0.48;
    const qreal eff2=0.37;
    const qreal eff3=0.15;
    const qreal gap=10; //for size of each part & gap between the areas.

    curve_area.setRect(painter_area.x(),painter_area.y(),painter_area.width(),painter_area.height()*eff1-gap);
    table_area.setRect(painter_area.x(),painter_area.height()*eff1,painter_area.width(),painter_area.height()*eff2-gap);
    header_area.setRect(painter_area.x(),painter_area.y()+painter_area.height()*(eff1+eff2),painter_area.width(),painter_area.height()*eff3);


    ui->pushButton->resize(right_margin-10,100);
    ui->pushButton->move(papersize.width()-right_margin+5,papersize.height()-200);

    //get the data from calypso

    Sp_xmlread xml_info("../tmp/");

    QString fileName = xml_info.names.at(0).Identifier;
    size = xml_info.names.at(0).act_points.size();

    //set the log file
    logo = new QPixmap(":/logo.bmp");
    // get the origin data from special program
    nom_data = new QVector<point> (xml_info.names.at(0).nom_points);
    act_data = new QVector<point> (xml_info.names.at(0).act_points);
   //calculate the range of the curve
    {
        qreal max_x,max_y,min_x,min_y;
        max_x = qMax((* std::max_element(nom_data->begin(),nom_data->end(),compare_X)).x,(*std::max_element(act_data->begin(),act_data->end(),compare_X)).x);
        min_x = qMin((* std::min_element(nom_data->begin(),nom_data->end(),compare_X)).x,(*std::min_element(act_data->begin(),act_data->end(),compare_X)).x);
        max_y = qMax((* std::max_element(nom_data->begin(),nom_data->end(),compare_Y)).y,(*std::max_element(act_data->begin(),act_data->end(),compare_Y)).y);
        min_y = qMin((* std::min_element(nom_data->begin(),nom_data->end(),compare_Y)).y,(*std::min_element(act_data->begin(),act_data->end(),compare_Y)).y);
        curve_range = QRectF(min_x,min_y,max_x-min_x,max_y-min_y);
        qDebug()<<max_x<<min_x<<max_y<<min_y;
    }

    //get the translate date from the origin data;

    curve_translate =  QPointF(-curve_range.x(),-curve_range.center().y());
    scale_x = (curve_area.width()-gap*2)/curve_range.width();
    scale_y = (curve_area.height()-gap*2)/curve_range.height();

    //as the painter scale didn't work well,
    //modify all the curve data to adapting the moniter und pdf file.
    QVector <QPair <double,double> > nom_transfer_data;
    QVector <QPair <double,double> > act_transfer_data;

    //start translate
    for(int i=0;i<size;++i)
    {
        nom_transfer_data.push_back(QPair<double,double> ((nom_data->at(i).x+curve_translate.x())*scale_x,
                                                          -(nom_data->at(i).y+curve_translate.y())*scale_y));
        act_transfer_data.push_back(QPair<double,double> ((act_data->at(i).x+curve_translate.x())*scale_x,
                                                          -(act_data->at(i).y+curve_translate.y())*scale_y));
    }


    // make this data to a group to display by painter.drawPoints

    for(int i=0;i<size;++i)
    {
        nom_points.push_back(QPointF(nom_transfer_data.at(i).first,-nom_transfer_data.at(i).second));
    }

    //start to generate bezier control Points
    {
        //the first end last point was delete, just for calc the vector

        //nom_control_points
        {
            nom_control_points = new QVector <QPair <double,double> >();
            int index;
            for(index = 1;index<size-2;index++)
            {
                QPair <double,double> Qi0,Qi,Qi1,Qi2;
                QPair <double,double> Pi0,Pi1,Pi2,Pi3;
                QPair <double,double> Ti0,Ti1;

                Qi0 = QPair<double,double> (nom_transfer_data.at(index-1).first,nom_transfer_data.at(index-1).second);
                Qi =  QPair<double,double> (nom_transfer_data.at(index).first,nom_transfer_data.at(index).second);
                Qi1 =  QPair<double,double> (nom_transfer_data.at(index+1).first,nom_transfer_data.at(index+1).second);
                Qi2 =  QPair<double,double> (nom_transfer_data.at(index+2).first,nom_transfer_data.at(index+2).second);
                Ti0.first = (Qi1.first-Qi0.first)/2;
                Ti0.second = (Qi1.second-Qi0.second)/2;
                Ti1.first = -(Qi2.first-Qi.first)/2;
                Ti1.second = -(Qi2.second-Qi.second)/2;
                Pi0 =  QPair<double,double> (Qi);
                Pi3 =  QPair<double,double> (Qi1);
                Pi1.first = Qi.first+Ti0.first/3;
                Pi1.second = Qi.second+Ti0.second/3;
                Pi2.first = Qi1.first+Ti1.first/3;
                Pi2.second = Qi1.second+Ti1.second/3;

                //insert the Pi0
                nom_control_points->push_back(Pi0);
                //insert the Pi1 Pi2 Pi3
                nom_control_points->push_back(Pi1);
                nom_control_points->push_back(Pi2);
                nom_control_points->push_back(Pi3);
            }
        }

        //act_control_points
        {
            act_control_points = new QVector <QPair <double,double> >();
            int index;
            for(index = 1;index<size-2;index++)
            {
                QPair <double,double> Qi0,Qi,Qi1,Qi2;
                QPair <double,double> Pi0,Pi1,Pi2,Pi3;
                QPair <double,double> Ti0,Ti1;

                Qi0 = QPair<double,double> (act_transfer_data.at(index-1).first,act_transfer_data.at(index-1).second);
                Qi =  QPair<double,double> (act_transfer_data.at(index).first,act_transfer_data.at(index).second);
                Qi1 =  QPair<double,double> (act_transfer_data.at(index+1).first,act_transfer_data.at(index+1).second);
                Qi2 =  QPair<double,double> (act_transfer_data.at(index+2).first,act_transfer_data.at(index+2).second);
                Ti0.first = (Qi1.first-Qi0.first)/2;
                Ti0.second = (Qi1.second-Qi0.second)/2;
                Ti1.first = -(Qi2.first-Qi.first)/2;
                Ti1.second = -(Qi2.second-Qi.second)/2;
                Pi0 =  QPair<double,double> (Qi);
                Pi3 =  QPair<double,double> (Qi1);
                Pi1.first = Qi.first+Ti0.first/3;
                Pi1.second = Qi.second+Ti0.second/3;
                Pi2.first = Qi1.first+Ti1.first/3;
                Pi2.second = Qi1.second+Ti1.second/3;

                //insert the Pi0
                act_control_points->push_back(Pi0);
                //insert the Pi1 Pi2 Pi3
                act_control_points->push_back(Pi1);
                act_control_points->push_back(Pi2);
                act_control_points->push_back(Pi3);
            }
        }
    }
    //start to make path , it will be render in paintEvent, not here
    //notice the qt use a mirror direction in Y axis with calypso.
    //minus every y!!!!

    //nominal path first
    {
        nom_path = new QPainterPath();
        nom_path->moveTo(nom_control_points->at(0).first,-nom_control_points->at(0).second);
        {
            //loop for control points
            int loop_size = nom_control_points->size()/4;
            int index;
            for(index = 0;index<loop_size;++index)
            {
                nom_path->cubicTo(
                            nom_control_points->at(index*4+1).first,-nom_control_points->at(index*4+1).second,
                            nom_control_points->at(index*4+2).first,-nom_control_points->at(index*4+2).second,
                            nom_control_points->at(index*4+3).first,-nom_control_points->at(index*4+3).second
                            );

            }
        }
    }
    //then the actual path
    {
        act_path = new QPainterPath();
        act_path->moveTo(act_control_points->at(0).first,-act_control_points->at(0).second);
       {
            //loop for control points
            int loop_size = act_control_points->size()/4;
            int index;
            for(index = 0;index<loop_size;++index)
            {
                act_path->cubicTo(
                            act_control_points->at(index*4+1).first,-act_control_points->at(index*4+1).second,
                            act_control_points->at(index*4+2).first,-act_control_points->at(index*4+2).second,
                            act_control_points->at(index*4+3).first,-act_control_points->at(index*4+3).second
                            );

            }
        }
    }


}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();
    //fist, draw a rounded rect for header & graphic area;
    painter.drawRoundedRect(header_area,15,15);
    painter.drawRoundedRect(curve_area.x(),curve_area.y(),curve_area.width(),table_area.y()+table_area.height(),15,15);
    painter.drawRoundedRect(curve_area,15,15);

    painter.setPen(Qt::SolidLine);

    QPen nom_line_pen(Qt::black, 2, Qt::SolidLine);
    QPen act_line_pen(Qt::blue, 2 ,Qt::SolidLine);

    //move the painter to the left mid position
    painter.translate(curve_area.x(),curve_area.center().y());

    //draw coordinate axis
    QPen axis_pen(Qt::blue,1,Qt::SolidLine);
    painter.setPen(axis_pen);
    painter.drawLine(0,0,curve_area.width(),0);
    painter.drawLine(curve_area.width()/2,curve_area.height()/2,curve_area.width()/2,-curve_area.height()/2);

    //draw nominal und actual curves
    painter.setPen(nom_line_pen);
    painter.drawPath(*nom_path);
    painter.setPen(act_line_pen);
    painter.drawPath(*act_path);
    //painter.drawPoints(nom_points,data->size());

    //draw nominal points as single points
    for(int i=0;i<size;++i)
    {
        painter.drawEllipse(nom_points[i],1.2,1.2);
    }

    //draw data tables
    painter.restore();
    painter.save();
    painter.translate(table_area.x(),table_area.y());
    painter.setFont(QFont("Times New Roman",10));
    for (int i=0;i<14;++i)  //just for test, offical release need auto adjust
    {
        painter.drawText(10+i*60,10,50,150,Qt::AlignCenter,
                         QString("%1\n%2\n%3\n%4")
                            .arg(i+1)
                         .arg(nom_data->at(i).x)
                         .arg(nom_data->at(i).y)
                            .arg( QString::number(act_data->at(i).y,'f',4)));
    }
    painter.translate(0,90);
    for (int i=14;i<28;++i)  //just for test, offical release need auto adjust
    {
        painter.drawText(10+(i-14)*60,10,50,150,Qt::AlignCenter,
                         QString("%1\n%2\n%3\n%4")
                            .arg(i+1)
                            .arg(nom_data->at(i).x)
                            .arg(nom_data->at(i).y)
                            .arg( QString::number(act_data->at(i).y,'f',4)));
    }

    //draw a header and LOGO
    painter.restore();
    painter.drawPixmap(header_area.x()+20,header_area.y()+1,header_area.height()-2,header_area.height()-2,*logo);
    painter.translate(header_area.center().x()-150,header_area.y());        //just for test

    QFont header_font("Blackadder ITC",20);
    painter.setFont(header_font);
    painter.drawText(0,0,300,header_area.height(),Qt::AlignCenter,QString("Just test, Curve Report by Sean"));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{

    QFileDialog::Options options;
    options |= QFileDialog::DontUseNativeDialog;
    QString selectedFilter;

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("set the PDF name"),
                                                    "",
                                                    tr("PDF Files (*.pdf)"),
                                                    &selectedFilter,
                                                    options);
    fileName.append(".pdf");
    printer.setOutputFileName(fileName);
    QPainter painter;
    if (! painter.begin(&printer)) { // failed to open file
        qWarning("failed to open file, is it writable?");
        return ;
    }
    painter.save();
    //fist, draw a rounded rect for header & graphic area;
    painter.drawRoundedRect(header_area,15,15);
    painter.drawRoundedRect(curve_area.x(),curve_area.y(),curve_area.width(),table_area.y()+table_area.height(),15,15);
    painter.drawRoundedRect(curve_area,15,15);

    painter.setPen(Qt::SolidLine);

    QPen nom_line_pen(Qt::black, 2, Qt::SolidLine);
    QPen act_line_pen(Qt::blue, 2 ,Qt::SolidLine);

    //move the painter to the left mid position
    painter.translate(curve_area.x(),curve_area.center().y());

    //draw coordinate axis
    QPen axis_pen(Qt::blue,1,Qt::SolidLine);
    painter.setPen(axis_pen);
    painter.drawLine(0,0,curve_area.width(),0);
    painter.drawLine(curve_area.width()/2,curve_area.height()/2,curve_area.width()/2,-curve_area.height()/2);

    //draw nominal und actual curves
    painter.setPen(nom_line_pen);
    painter.drawPath(*nom_path);
    painter.setPen(act_line_pen);
    painter.drawPath(*act_path);
    //painter.drawPoints(nom_points,data->size());

    //draw nominal points as single points
    for(int i=0;i<size;++i)
    {
        painter.drawEllipse(nom_points[i],1.2,1.2);
    }

    //draw data tables
    painter.restore();
    painter.save();
    painter.translate(table_area.x(),table_area.y());
    painter.setFont(QFont("Times New Roman",10));
    for (int i=0;i<14;++i)  //just for test, offical release need auto adjust
    {
        painter.drawText(10+i*60,10,50,150,Qt::AlignCenter,
                         QString("%1\n%2\n%3\n%4")
                            .arg(i+1)
                         .arg(nom_data->at(i).x)
                         .arg(nom_data->at(i).y)
                            .arg( QString::number(act_data->at(i).y,'f',4)));
    }
    painter.translate(0,90);
    for (int i=14;i<28;++i)  //just for test, offical release need auto adjust
    {
        painter.drawText(10+(i-14)*60,10,50,150,Qt::AlignCenter,
                         QString("%1\n%2\n%3\n%4")
                            .arg(i+1)
                         .arg(nom_data->at(i).x)
                         .arg(nom_data->at(i).y)
                         .arg( QString::number(act_data->at(i).y,'f',4)));
    }

    //draw a header and LOGO
    painter.restore();
    painter.drawPixmap(header_area.x()+20,header_area.y()+1,header_area.height()-2,header_area.height()-2,*logo);
    painter.translate(header_area.center().x()-150,header_area.y());        //just for test

    QFont header_font("Blackadder ITC",20);
    painter.setFont(header_font);
    painter.drawText(0,0,300,header_area.height(),Qt::AlignCenter,QString("Just test, Curve Report by Sean"));

}
