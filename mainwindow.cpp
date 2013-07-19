#include "mainwindow.h"
#include "ui_mainwindow.h"
point::point(double a,double b,double c,double d)
{
    this->x_nom=a;
    this->y_nom=b;
    this->x_act=c;
    this->y_act=d;
}
point::point()
{}

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
    data = new QVector<point>();
    QDir base_dir ("../../home/om/"); //~/zeiss/home/om dir
    QDir trans_dir ("../tmp/"); //dir for files;
    QFile ele_xml_file (QString("%1/ElementsToSpecialProgram.xml").arg(trans_dir.absolutePath()));
    QFile sys_xml_file (QString("%1/SysParaToSpecialProgram.xml").arg(trans_dir.absolutePath()));

    Sp_xmlread xml_info("../tmp/");

    QString fileName = xml_info.names.at(0);

    QFile nom_file(QString("../tmp/%1_NomPoints.txt").arg(fileName));
    QFile act_file(QString("../tmp/%1_ActPoints.txt").arg(fileName));


    nom_file.open(QFile::ReadOnly|QFile::Text);
    act_file.open(QFile::ReadOnly|QFile::Text);

    QTextStream in_nom(&nom_file);
    QTextStream in_act(&act_file);

    // get the origin data from special program
    while(1)
    {
        QString buf1,buf2;
        buf1 = in_nom.readLine();
        buf2 = in_act.readLine();
        if(buf1.isNull())
            break;
        double xn,xa,yn,ya;
        {
            int index_space;
            index_space = buf1.indexOf(" ");
            xn = buf1.mid(index_space+1,buf1.indexOf(" ",index_space+1)-index_space-1).toDouble();
            index_space = buf1.indexOf(" ",index_space+1);
            yn = buf1.mid(index_space+1,buf1.indexOf(" ",index_space+1)-index_space-1).toDouble();
            index_space = buf2.indexOf(" ");
            xa = buf2.mid(index_space+1,buf2.indexOf(" ",index_space+1)-index_space-1).toDouble();
            index_space = buf2.indexOf(" ",index_space+1);
            ya = buf2.mid(index_space+1,buf2.indexOf(" ",index_space+1)-index_space-1).toDouble();
            data->push_back(point(xn,yn,xa,ya));


       }

    }

    //calculate the range of the curve
    {
        qreal max_x,max_y,min_x,min_y;
        max_x = min_x = data->at(0).x_nom;
        max_y = min_y = data->at(0).y_nom;
        for (int i=0;i<data->size();++i)
        {
            if(data->at(i).x_nom>max_x)
                max_x = data->at(i).x_nom;
            if(data->at(i).y_nom>max_y)
                max_y = data->at(i).y_nom;
            if(data->at(i).x_nom<min_x)
                min_x = data->at(i).x_nom;
            if(data->at(i).y_nom<min_y)
                min_y = data->at(i).y_nom;

            if(data->at(i).x_act>max_x)
                max_x = data->at(i).x_act;
            if(data->at(i).y_act>max_y)
                max_y = data->at(i).y_act;
            if(data->at(i).x_act<min_x)
                min_x = data->at(i).x_act;
            if(data->at(i).y_act<min_y)
                min_y = data->at(i).y_act;
        }
        curve_range = QRectF(min_x,min_y,max_x-min_x,max_y-min_y);
        qDebug()<<curve_range;
    }

    //as the painter scale didn't work well,
    //modify all the curve data to adapting the moniter und pdf file.

    nom_points = new QPointF[data->size()];
    for(int i=0;i<data->size();++i)
    {
        nom_points[i]=QPointF(data->at(i).x_nom,data->at(i).y_nom);
    }
    curve_translate =  QPointF(left_margin-curve_range.x(),
                               curve_area.bottom()+curve_range.bottom());
    scale_x = curve_area.width()/curve_range.width();
    scale_y = curve_area.height()/curve_range.height();

    //start to generate bezier control Points
    {
        //the first end last point was delete, just for calc the vector
        int data_size = data->size();

        //nom_control_points
        {
            nom_control_points = new QVector <QPair <double,double> >();
            int index;
            for(index = 1;index<data_size-2;index++)
            {
                QPair <double,double> Qi0,Qi,Qi1,Qi2;
                QPair <double,double> Pi0,Pi1,Pi2,Pi3;
                QPair <double,double> Ti0,Ti1;

                Qi0 = QPair<double,double> (data->at(index-1).x_nom,data->at(index-1).y_nom);
                Qi =  QPair<double,double> (data->at(index).x_nom,data->at(index).y_nom);
                Qi1 =  QPair<double,double> (data->at(index+1).x_nom,data->at(index+1).y_nom);
                Qi2 =  QPair<double,double> (data->at(index+2).x_nom,data->at(index+2).y_nom);
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
            for(index = 1;index<data_size-2;index++)
            {
                QPair <double,double> Qi0,Qi,Qi1,Qi2;
                QPair <double,double> Pi0,Pi1,Pi2,Pi3;
                QPair <double,double> Ti0,Ti1;

                Qi0 = QPair<double,double> (data->at(index-1).x_act,data->at(index-1).y_act);
                Qi =  QPair<double,double> (data->at(index).x_act,data->at(index).y_act);
                Qi1 =  QPair<double,double> (data->at(index+1).x_act,data->at(index+1).y_act);
                Qi2 =  QPair<double,double> (data->at(index+2).x_act,data->at(index+2).y_act);
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
    //fist, draw a rounded rect for header & graphic area;
    painter.drawRoundedRect(header_area,15,15);
    painter.drawRoundedRect(curve_area.x(),curve_area.y(),curve_area.width(),table_area.y()+table_area.height(),15,15);
    painter.drawRoundedRect(curve_area,15,15);

    painter.setPen(Qt::SolidLine);

    //painter.drawLine(-300,-300,300,300);
    QPen nom_line_pen(Qt::black, 0, Qt::SolidLine);

    painter.translate(curve_translate);
    //painter.scale(scale_x,scale_y);
    painter.scale(5,1);
    painter.setPen(nom_line_pen);
    painter.drawPath(*nom_path);
    painter.setPen(Qt::blue);
    painter.drawPath(*act_path);
    //painter.setPen( QPen(Qt::green, 0.001, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin));
    //painter.drawPoints(nom_points,data->size());
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
    fileName.append("pdf");
    printer.setOutputFileName(fileName);
    QPainter painter;
    if (! painter.begin(&printer)) { // failed to open file
        qWarning("failed to open file, is it writable?");
        return ;
    }
    painter.setPen(Qt::SolidLine);

    //painter.drawLine(-300,-300,300,300);
    QPen nom_line_pen(Qt::black, 0, Qt::SolidLine);
    painter.drawRect(5,5,1113,783);
    painter.translate(curve_translate);
    painter.scale(scale_x,scale_y);


    painter.setPen(nom_line_pen);
    painter.drawPath(*nom_path);
    painter.setPen(Qt::blue);
    painter.drawPath(*act_path);
    painter.end();
}