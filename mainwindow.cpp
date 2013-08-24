#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <math.h>
#include <cmath>
#include <qmath.h>
const qreal eff1=0.48;
const qreal eff2=0.37;
const qreal eff3=0.15;
const qreal gap=10; //for size of each part & gap between the areas.
qreal max_x,max_y,min_x,min_y;
qreal profil;
qreal act_profil;

bool compare_X(const point a,const point b)
{
    return a.x<b.x;
}

bool compare_Y(const point a,const point b)
{
    return a.y<b.y;
}

bool compare_UT(const point a,const point b)
{
    return a.ut<b.ut;
}bool compare_LT(const point a,const point b)
{
    return a.lt<b.lt;
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
    this->setFixedSize(printer.width(),printer.height());

    printer.setPageMargins(50.0,40.0,80.0,44.0,QPrinter::DevicePixel);
    printer.getPageMargins(&left_margin,&top_margin,&right_margin,&bottom_margin,QPrinter::DevicePixel);
    papersize.setRect(0,0,this->width(),this->height());
    //qDebug()<<left_margin<<top_margin<<right_margin<<bottom_margin;


    painter_area.setRect(left_margin,top_margin,this->width()-left_margin-right_margin,this->height()-top_margin-bottom_margin);

    curve_area.setRect(painter_area.x(),painter_area.y(),painter_area.width(),painter_area.height()*eff1-gap);
    table_area.setRect(painter_area.x(),painter_area.y()+painter_area.height()*eff1,painter_area.width(),painter_area.height()*eff2-gap);
    header_area.setRect(painter_area.x(),painter_area.y()+painter_area.height()*(eff1+eff2),painter_area.width(),painter_area.height()*eff3);

    ui->pushButton->resize(right_margin-10,100);
    ui->pushButton->move(this->width()-right_margin+5,this->height()-200);

    //get the data from calypso

    xml_info = new Sp_xmlread("../tmp/");

    QString fileName = xml_info->names.at(0).Identifier;
    size = xml_info->names.at(0).act_points.size();

    //set the log file
    logo = new QPixmap(":/logo.bmp");
    // get the origin data from special program
    nom_data = new QVector<point> (xml_info->names.at(0).nom_points);
    act_data = new QVector<point> (xml_info->names.at(0).act_points);
   //calculate the range of the curve
    {
        max_x = qMax((* std::max_element(nom_data->begin(),nom_data->end(),compare_X)).x,(*std::max_element(act_data->begin(),act_data->end(),compare_X)).x);
        min_x = qMin((* std::min_element(nom_data->begin(),nom_data->end(),compare_X)).x,(*std::min_element(act_data->begin(),act_data->end(),compare_X)).x);
        max_y = qMax((* std::max_element(nom_data->begin(),nom_data->end(),compare_Y)).y,(*std::max_element(act_data->begin(),act_data->end(),compare_Y)).y);
        min_y = qMin((* std::min_element(nom_data->begin(),nom_data->end(),compare_Y)).y,(*std::min_element(act_data->begin(),act_data->end(),compare_Y)).y);
        curve_range = QRectF(min_x,min_y,max_x-min_x,max_y-min_y);
        qDebug()<<max_x<<min_x<<max_y<<min_y;
    }
    //calculate the profil
    profil = 2* qMax( (* std::max_element(nom_data->begin(),nom_data->end(),compare_UT)).ut,
                      fabs((* std::min_element(nom_data->begin(),nom_data->end(),compare_LT)).lt));
    //qreal maxdev =
    QString maxdev,mindev;
    maxdev = xml_info->names.at(0).curve_paras.value("maxDev");
    mindev = xml_info->names.at(0).curve_paras.value("minDev");
    if (maxdev.endsWith('d')){
        maxdev= maxdev.left(maxdev.size()-1);
    }
    if (mindev.endsWith('d')){
        mindev= mindev.left(mindev.size()-1);
    }
    act_profil = 2* qMax(fabs(maxdev.replace('d','e').toDouble()),fabs(mindev.replace('d','e').toDouble() ));
    //get the translate date from the origin data;

    curve_translate =  QPointF(-curve_range.x(),-curve_range.center().y());
    scale_x = (curve_area.width()-gap*2)/curve_range.width();
    scale_y = (curve_area.height()-gap*2)/curve_range.height()/2;

    //as the painter scale didn't work well,
    //modify all the curve data to adapting the moniter und pdf file.
    QVector <QPair <double,double> > nom_transfer_data;
    QVector <QPair <double,double> > act_transfer_data;
    QVector <QPair <double,double> > ut_transfer_data;
    QVector <QPair <double,double> > lt_transfer_data;

    //start translate
    for(int i=0;i<size;++i)
    {
        nom_transfer_data.push_back(QPair<double,double> ((nom_data->at(i).x+curve_translate.x())*scale_x,
                                                          -(nom_data->at(i).y+curve_translate.y())*scale_y));
        act_transfer_data.push_back(QPair<double,double> ((act_data->at(i).x+curve_translate.x())*scale_x,
                                                          -(act_data->at(i).y+curve_translate.y())*scale_y));
        ut_transfer_data.push_back(
                    QPair<double,double> (
                        (nom_data->at(i).x+nom_data->at(i).u*nom_data->at(i).ut+curve_translate.x())*scale_x,
                        -(nom_data->at(i).y+nom_data->at(i).v*nom_data->at(i).ut+curve_translate.y())*scale_y)
                    );
        lt_transfer_data.push_back(
                    QPair<double,double> (
                        (nom_data->at(i).x+nom_data->at(i).u*nom_data->at(i).lt+curve_translate.x())*scale_x,
                        -(nom_data->at(i).y+nom_data->at(i).v*nom_data->at(i).lt+curve_translate.y())*scale_y)
                    );
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

        nom_transfer_data.push_back(QPair<double,double>(
                    2*nom_transfer_data.at(size-1).first-nom_transfer_data.at(size-2).first,
                    2*nom_transfer_data.at(size-1).second-nom_transfer_data.at(size-2).second));
        nom_transfer_data.push_front(QPair<double,double>(
                    2*nom_transfer_data.at(0).first-nom_transfer_data.at(1).first,
                    2*nom_transfer_data.at(0).second-nom_transfer_data.at(1).second));
        {
            nom_control_points = new QVector <QPair <double,double> >();
            int index;
            for(index = 1;index<size;index++)
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
        act_transfer_data.push_back(QPair<double,double>(
                    2*act_transfer_data.at(size-1).first-act_transfer_data.at(size-2).first,
                    2*act_transfer_data.at(size-1).second-act_transfer_data.at(size-2).second));
        act_transfer_data.push_front(QPair<double,double>(
                    2*act_transfer_data.at(0).first-act_transfer_data.at(1).first,
                    2*act_transfer_data.at(0).second-act_transfer_data.at(1).second));
        {
            act_control_points = new QVector <QPair <double,double> >();
            int index;
            for(index = 1;index<size;index++)
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
        //ut_control_points

        ut_transfer_data.push_back(QPair<double,double>(
                    2*ut_transfer_data.at(size-1).first-ut_transfer_data.at(size-2).first,
                    2*ut_transfer_data.at(size-1).second-ut_transfer_data.at(size-2).second));
        ut_transfer_data.push_front(QPair<double,double>(
                    2*ut_transfer_data.at(0).first-ut_transfer_data.at(1).first,
                    2*ut_transfer_data.at(0).second-ut_transfer_data.at(1).second));
        {
            ut_control_points = new QVector <QPair <double,double> >();
            int index;
            for(index = 1;index<size;index++)
            {
                QPair <double,double> Qi0,Qi,Qi1,Qi2;
                QPair <double,double> Pi0,Pi1,Pi2,Pi3;
                QPair <double,double> Ti0,Ti1;

                Qi0 = QPair<double,double> (ut_transfer_data.at(index-1).first,ut_transfer_data.at(index-1).second);
                Qi =  QPair<double,double> (ut_transfer_data.at(index).first,ut_transfer_data.at(index).second);
                Qi1 =  QPair<double,double> (ut_transfer_data.at(index+1).first,ut_transfer_data.at(index+1).second);
                Qi2 =  QPair<double,double> (ut_transfer_data.at(index+2).first,ut_transfer_data.at(index+2).second);
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
                ut_control_points->push_back(Pi0);
                //insert the Pi1 Pi2 Pi3
                ut_control_points->push_back(Pi1);
                ut_control_points->push_back(Pi2);
                ut_control_points->push_back(Pi3);
            }
        }
        //lt_control_points

        lt_transfer_data.push_back(QPair<double,double>(
                    2*lt_transfer_data.at(size-1).first-lt_transfer_data.at(size-2).first,
                    2*lt_transfer_data.at(size-1).second-lt_transfer_data.at(size-2).second));
        lt_transfer_data.push_front(QPair<double,double>(
                    2*lt_transfer_data.at(0).first-lt_transfer_data.at(1).first,
                    2*lt_transfer_data.at(0).second-lt_transfer_data.at(1).second));
        {
            lt_control_points = new QVector <QPair <double,double> >();
            int index;
            for(index = 1;index<size;index++)
            {
                QPair <double,double> Qi0,Qi,Qi1,Qi2;
                QPair <double,double> Pi0,Pi1,Pi2,Pi3;
                QPair <double,double> Ti0,Ti1;

                Qi0 = QPair<double,double> (lt_transfer_data.at(index-1).first,lt_transfer_data.at(index-1).second);
                Qi =  QPair<double,double> (lt_transfer_data.at(index).first,lt_transfer_data.at(index).second);
                Qi1 =  QPair<double,double> (lt_transfer_data.at(index+1).first,lt_transfer_data.at(index+1).second);
                Qi2 =  QPair<double,double> (lt_transfer_data.at(index+2).first,lt_transfer_data.at(index+2).second);
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
                lt_control_points->push_back(Pi0);
                //insert the Pi1 Pi2 Pi3
                lt_control_points->push_back(Pi1);
                lt_control_points->push_back(Pi2);
                lt_control_points->push_back(Pi3);
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

    //und ut path
    {
        ut_path = new QPainterPath();
        ut_path->moveTo(ut_control_points->at(0).first,-ut_control_points->at(0).second);
       {
            //loop for control points
            int loop_size = ut_control_points->size()/4;
            int index;
            for(index = 0;index<loop_size;++index)
            {
                ut_path->cubicTo(
                            ut_control_points->at(index*4+1).first,-ut_control_points->at(index*4+1).second,
                            ut_control_points->at(index*4+2).first,-ut_control_points->at(index*4+2).second,
                            ut_control_points->at(index*4+3).first,-ut_control_points->at(index*4+3).second
                            );

            }
        }
    }
    //finally the lt path
    {
        lt_path = new QPainterPath();
        lt_path->moveTo(lt_control_points->at(0).first,-lt_control_points->at(0).second);
       {
            //loop for control points
            int loop_size = lt_control_points->size()/4;
            int index;
            for(index = 0;index<loop_size;++index)
            {
                lt_path->cubicTo(
                            lt_control_points->at(index*4+1).first,-lt_control_points->at(index*4+1).second,
                            lt_control_points->at(index*4+2).first,-lt_control_points->at(index*4+2).second,
                            lt_control_points->at(index*4+3).first,-lt_control_points->at(index*4+3).second
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

    painter.drawRoundedRect(curve_area.x(),curve_area.y(),curve_area.width(),table_area.y()+table_area.height()-curve_area.y(),15,15);
    //painter.drawRoundedRect(curve_area,15,15);

    painter.setPen(Qt::SolidLine);

    QFont Nr_font("Arial",8);
    QPen nom_line_pen(Qt::black, 1.5, Qt::SolidLine);
    QPen act_line_pen(Qt::red, 1.5 ,Qt::SolidLine);
    QPen line_pen(Qt::blue, 1 , Qt::SolidLine);
    QPen tol_line_pen(Qt::green, 1 , Qt::SolidLine);
    //move the painter to the left mid position
    painter.translate(curve_area.x()+gap,curve_area.center().y());

    //draw coordinate axis
    QPen axis_pen(Qt::blue,1,Qt::SolidLine);
    painter.setPen(axis_pen);
    painter.drawLine(0,curve_area.height()/2-10,curve_area.width()-gap*2,curve_area.height()/2-10);
    painter.drawLine(curve_area.width()/2-gap,curve_area.height()/2,curve_area.width()/2-gap,-curve_area.height()/2);
    //draw X axis sign & number
    painter.setFont(Nr_font);
    int be,en;
    be = std::ceil(min_x)+1;
    en = std::floor(max_x)-1;
    for(int i=be;i<=en;++i)
    {
        painter.drawLine((i-min_x)*scale_x,curve_area.height()/2-5,(i-min_x)*scale_x,curve_area.height()/2-15);
        painter.drawText((i-min_x)*scale_x+3,curve_area.height()/2,QString("%1").arg(i));
    }
    //draw Y axis sign & number
    be = std::ceil(min_y*1000);
    en = std::floor(max_y*1000);
    be-=3;
    en+=4;

    //draw 10e-3
    painter.drawText(curve_area.width()/2-gap-20,-curve_area.height()/2+10,
                     10,10,
                     Qt::AlignRight|Qt::AlignVCenter,QString("10"));
    painter.setFont(QFont("Arial",6));
    painter.drawText(curve_area.width()/2-gap-12,-curve_area.height()/2+7,
                     10,10,
                     Qt::AlignRight|Qt::AlignVCenter,QString("-3"));

    for(int i=be;i<=en;i++)
    {
        if (i%2==0)
        {
            painter.drawLine(curve_area.width()/2-gap-5,(double(i)/1000-(min_y+max_y)/2)*scale_y,
                             curve_area.width()/2-gap+5,(double(i)/1000-(min_y+max_y)/2)*scale_y);
            painter.drawText(curve_area.width()/2-gap-15,(double(i)/1000-(min_y+max_y)/2)*scale_y-5,
                             10,10,
                             Qt::AlignRight|Qt::AlignVCenter,QString("%1").arg(i));
        }
        else
        {
            painter.drawLine(curve_area.width()/2-gap-2,(double(i)/1000-(min_y+max_y)/2)*scale_y,
                             curve_area.width()/2-gap+2,(double(i)/1000-(min_y+max_y)/2)*scale_y);
        }
    }
    //draw nominal und actual curves
    painter.setPen(nom_line_pen);
    painter.drawPath(*nom_path);
    painter.setPen(act_line_pen);
    painter.drawPath(*act_path);

    painter.setPen(tol_line_pen);
    painter.drawPath(*ut_path);
    painter.drawPath(*lt_path);

    //painter.drawPoints(nom_points,data->size());

    //draw nominal points as single points
    painter.setPen(line_pen);
    for(int i=0;i<size;++i)
    {
        painter.drawEllipse(nom_points[i],1.2,1.2);
    }
    //draw nominal points number
    for(int i=0;i<28;i++)
    {
        painter.drawLine(nom_points[i],QPointF(nom_points[i].x(),-curve_area.height()/2+50));
        painter.save();
        painter.translate(nom_points[i].x()-2,-curve_area.height()/2+50);
        painter.rotate(-40);
        painter.setFont(Nr_font);
        painter.drawText(0,0,QString("%1").arg(i+1));
        painter.restore();

    }

    //draw profile value
    painter.restore();
    painter.save();
    painter.translate(table_area.center().x(),table_area.y());
    for(int i=0;i<3;++i)
    {
        painter.drawRect(-90+i*60,5,60,20);
    }
    painter.drawArc(-70,10,20,20,0,180*16);
    painter.drawText(-30,5,60,20,Qt::AlignCenter,QString("%1").arg(profil));
    painter.drawText(30,5,60,20,Qt::AlignCenter,QString("%1").arg(QString::number(act_profil,'f',4)));
    //draw data tables
    painter.restore();
    painter.save();

    painter.translate(table_area.x(),table_area.y());

    //set draw paras for the table lines & fonts
    painter.setPen(axis_pen);
    QRectF one_cell(0,0,50,15);
    QRectF title_cell(0,0,80,15);
    QPointF start_pos(60,40);
    QFont word_font("Times New Roman",10);
    QFont num_font("Times New Roman",10);
    num_font.setLetterSpacing(QFont::AbsoluteSpacing,1.5);


    //draw first line table and data
    painter.drawRect(start_pos.x(),start_pos.y(),title_cell.width(),title_cell.height());
    for(int i=1;i<5;++i)
    {
        painter.drawRect(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height());
    }
    for(int j=1;j<15;++j)
    {
        painter.drawRect(start_pos.x()+title_cell.width()+gap+(j-1)*one_cell.width(),start_pos.y(),one_cell.width(),one_cell.height());
        for(int i=1;i<5;++i)
        {
            painter.drawRect(start_pos.x()+title_cell.width()+gap+(j-1)*one_cell.width(),start_pos.y()+one_cell.height()*i+5,one_cell.width(),one_cell.height());
        }
    }
    //draw text for titles
    painter.setFont(word_font);
    painter.drawText(start_pos.x(),start_pos.y(),title_cell.width(),title_cell.height(),Qt::AlignHCenter|Qt::AlignBottom,
                     QString("number"));
    {
        int i;
        i=1;
        painter.drawText(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("X   [mm]"));
        i=2;
        painter.drawText(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("Ys  [um]"));
        i=3;
        painter.drawText(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("Yi   [um]"));
        i=4;
        painter.drawText(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("dev.[um]"));
    }

    //draw the data
    painter.setFont(num_font);
    for (int i=0;i<14;++i)
    {
        painter.drawText(start_pos.x()+title_cell.width()+gap+(i)*one_cell.width(),start_pos.y(),one_cell.width(),one_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("%1").arg(i+1));

        {
            int j=1;
            painter.drawText(start_pos.x()+title_cell.width()+gap+(i)*one_cell.width(),start_pos.y()+one_cell.height()*j+5,
                             one_cell.width()-2,one_cell.height(),
                             Qt::AlignVCenter|Qt::AlignRight,
                             QString("%1").arg(QString::number(nom_data->at(i).x,'f',2)));
            j=2;
            painter.drawText(start_pos.x()+title_cell.width()+gap+(i)*one_cell.width(),start_pos.y()+one_cell.height()*j+5,
                             one_cell.width()-2,one_cell.height(),
                             Qt::AlignVCenter|Qt::AlignRight,
                             QString("%1").arg(QString::number(nom_data->at(i).y*1000,'f',2)));
            j=3;
            painter.drawText(start_pos.x()+title_cell.width()+gap+(i)*one_cell.width(),start_pos.y()+one_cell.height()*j+5,
                             one_cell.width()-2,one_cell.height(),
                             Qt::AlignVCenter|Qt::AlignRight,
                             QString("%1").arg(QString::number(act_data->at(i).y*1000,'f',2)));
            j=4;
            painter.drawText(start_pos.x()+title_cell.width()+gap+(i)*one_cell.width(),start_pos.y()+one_cell.height()*j+5,
                             one_cell.width()-2,one_cell.height(),
                             Qt::AlignVCenter|Qt::AlignRight,
                             QString("%1").arg(QString::number((act_data->at(i).y-nom_data->at(i).y)*1000,'f',2)));

        }

    }
    painter.translate(0,90);
    painter.drawRect(start_pos.x(),start_pos.y(),title_cell.width(),title_cell.height());
    for(int i=1;i<5;++i)
    {
        painter.drawRect(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height());
    }
    for(int j=1;j<15;++j)
    {
        painter.drawRect(start_pos.x()+title_cell.width()+gap+(j-1)*one_cell.width(),start_pos.y(),one_cell.width(),one_cell.height());
        for(int i=1;i<5;++i)
        {
            painter.drawRect(start_pos.x()+title_cell.width()+gap+(j-1)*one_cell.width(),start_pos.y()+one_cell.height()*i+5,one_cell.width(),one_cell.height());
        }
    }
    //draw text for titles
    painter.setFont(word_font);
    painter.drawText(start_pos.x(),start_pos.y(),title_cell.width(),title_cell.height(),Qt::AlignHCenter|Qt::AlignBottom,
                     QString("number"));
    {
        int i;
        i=1;
        painter.drawText(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("X   [mm]"));
        i=2;
        painter.drawText(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("Ys  [um]"));
        i=3;
        painter.drawText(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("Yi   [um]"));
        i=4;
        painter.drawText(start_pos.x(),start_pos.y()+title_cell.height()*i+5,title_cell.width(),title_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("dev.[um]"));
    }

    //draw the data
    painter.setFont(num_font);
    for (int i=14;i<28;++i)
    {
        painter.drawText(start_pos.x()+title_cell.width()+gap+(i-14)*one_cell.width(),start_pos.y(),one_cell.width(),one_cell.height(),
                         Qt::AlignHCenter|Qt::AlignBottom,QString("%1").arg(i+1));

        {
            int j=1;
            painter.drawText(start_pos.x()+title_cell.width()+gap+(i-14)*one_cell.width(),start_pos.y()+one_cell.height()*j+5,
                             one_cell.width()-2,one_cell.height(),
                             Qt::AlignVCenter|Qt::AlignRight,
                             QString("%1").arg(QString::number(nom_data->at(i).x,'f',2)));
            j=2;
            painter.drawText(start_pos.x()+title_cell.width()+gap+(i-14)*one_cell.width(),start_pos.y()+one_cell.height()*j+5,
                             one_cell.width()-2,one_cell.height(),
                             Qt::AlignVCenter|Qt::AlignRight,
                             QString("%1").arg(QString::number(nom_data->at(i).y*1000,'f',2)));
            j=3;
            painter.drawText(start_pos.x()+title_cell.width()+gap+(i-14)*one_cell.width(),start_pos.y()+one_cell.height()*j+5,
                             one_cell.width()-2,one_cell.height(),
                             Qt::AlignVCenter|Qt::AlignRight,
                             QString("%1").arg(QString::number(act_data->at(i).y*1000,'f',2)));
            j=4;
            painter.drawText(start_pos.x()+title_cell.width()+gap+(i-14)*one_cell.width(),start_pos.y()+one_cell.height()*j+5,
                             one_cell.width()-2,one_cell.height(),
                             Qt::AlignVCenter|Qt::AlignRight,
                             QString("%1").arg(QString::number((act_data->at(i).y-nom_data->at(i).y)*1000,'f',2)));

        }

    }


    //draw a header and LOGO
    painter.restore();
    painter.drawPixmap(header_area.x()+20,header_area.y()+1,header_area.height()-2,header_area.height()-2,*logo);
    painter.translate(header_area.x(),header_area.y());

    QFont header_font("Arial",header_area.height()*3/10,10,true);
    painter.setFont(header_font);
    painter.drawText(0,0,header_area.width(),header_area.height()*6/10,Qt::AlignHCenter|Qt::AlignBottom,QString("PROFILFORM"));
    header_font.setPointSize(header_area.height()*2/10);
    painter.setFont(header_font);
    painter.drawText(0,0,header_area.width(),header_area.height()*9/10,Qt::AlignHCenter|Qt::AlignBottom,QString("ISO 1101"));

    header_font.setItalic(false);
    header_font.setPointSize(9);
    header_font.setFixedPitch(true);
    painter.setFont(header_font);
    painter.drawText(header_area.width()*8/10,header_area.height()*2/10,header_area.width()*2/10,header_area.height()*0.2,
                     Qt::AlignLeft|Qt::AlignVCenter,
                     QString("Inspector  "));
    painter.drawText(header_area.width()*8/10,header_area.height()*4/10,header_area.width()*2/10,header_area.height()*0.2,
                     Qt::AlignLeft|Qt::AlignVCenter,
                     QString("Date  "));
    painter.drawText(header_area.width()*8/10,header_area.height()*6/10,header_area.width()*2/10,header_area.height()*0.2,
                     Qt::AlignLeft|Qt::AlignVCenter,
                     QString("Time "));
    painter.drawText(header_area.width()*9/10,header_area.height()*2/10,header_area.width()*1/10,header_area.height()*0.2,
                     Qt::AlignLeft|Qt::AlignVCenter,
                     QString(":   %1").arg(xml_info->paras.value("operid")));
    painter.drawText(header_area.width()*9/10,header_area.height()*4/10,header_area.width()*1/10,header_area.height()*0.2,
                     Qt::AlignLeft|Qt::AlignVCenter,
                     QString(":   %1").arg(xml_info->paras.value("date")));
    painter.drawText(header_area.width()*9/10,header_area.height()*6/10,header_area.width()*1/10,header_area.height()*0.2,
                     Qt::AlignLeft|Qt::AlignVCenter,
                     QString(":   %1").arg(xml_info->paras.value("time")));
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
