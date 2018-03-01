#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <unistd.h>

#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QObject>
#include <QFile>
#include <QDebug>

#include <opencv2/opencv.hpp>

#include "edge_checker.h"

typedef struct _acq_image {
    size_t width;
    size_t height;
    int type;
    int ready;
    size_t  size;
    char data[16*1024*1024];
} acq_image;

extern int camera_main();
extern acq_image* g_images;
extern int g_images_count;
extern int g_quit_flag;
extern int g_camera_work;
size_t bstart = 0;
int level = 100;

int candy_level = 20;

int check_style = 0;
enum e_check_style {
    style_huahen=0,
    style_top,
    style_skin
};


check_setting g_settings[4]={
    {style_huahen,  1250,   400,    1600,   1500,   35, 35, 20, 100},
    {style_top,     1350,   1000,   1450,   1100,   35, 35, 21, 180},
    {style_skin,    600,    500,    900,    1500,   35, 35, 20, 100},
    {style_skin,    1900,   500,    2100,   1500,   35, 35, 20, 100}

};
// proc image
static inline void proc_image(const QImage& img, QPixmap& out, const int num = 0, std::vector<QRect>* pvec = 0)
{
    using namespace cv;
    cv::Mat src;
    cv::Size size(img.width(), img.height());
    uchar* data = (uchar*)malloc(img.byteCount());

    memcpy(data, img.bits(), img.byteCount());
    src = cv::Mat(size, CV_8UC1, data);

    cv::Mat grey = src;
    cv::Mat edge = grey;
    blur(grey, edge, cv::Size(5,5));

    cv::Canny(edge, edge, candy_level, candy_level*3, 3);

    cv::Mat dst;
    dst.create(src.size(), src.type());
    dst = cv::Scalar::all(0);
    src.copyTo(dst, edge);

    QString name = QString("/home/user/dst%1.bmp").arg(num);
    cv::imwrite(name.toStdString(), edge);

    //convert dst to QImage
    QImage image(dst.data, dst.size().width, dst.size().height, QImage::Format_Grayscale8);


    out.convertFromImage(image);//.scaled(800,600));

    if(pvec)
    {
        pvec->clear();
    }

    for(int i=0; i<sizeof(g_settings)/sizeof(check_setting); ++i)
    {
        check_setting& setting = g_settings[i];
        if(setting.style == check_style)
        {
            edge_checker checker(out, dst.data, dst.size().width, dst.size().height, setting);
            if(pvec)
            {
                pvec->insert(pvec->begin(), checker.m_results.begin(), checker.m_results.end());
            }
        }
    }

    free(data);

    return;

}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_scene = new QGraphicsScene(this);
    QGraphicsPixmapItem * pic = new QGraphicsPixmapItem();
    QGraphicsPixmapItem * pic2 = new QGraphicsPixmapItem();
    QGraphicsPixmapItem * pic3 = new QGraphicsPixmapItem();
    QGraphicsPixmapItem * pic4 = new QGraphicsPixmapItem();
    m_scene->addItem(pic);
    m_scene->addItem(pic2);
    m_scene->addItem(pic3);
    m_scene->addItem(pic4);
    //pic->scale(0.5);
    pic->setOffset(-400, -300);
    pic2->setOffset(400, -300);
    pic3->setOffset(-400, 300);
    pic4->setOffset(400, 300);
    m_pic_width = 800;
    m_pic_height = 600;

    m_items.push_back(pic);
    m_items.push_back(pic2);
    m_items.push_back(pic3);
    m_items.push_back(pic4);
    //pic2->scale(0.5, 0.5);
    ui->graphicsView->setScene(m_scene);
    ui->graphicsView->centerOn(0,0);
    ui->graphicsView->ensureVisible(pic);
    ui->graphicsView->ensureVisible(pic2);
    ui->graphicsView->ensureVisible(pic3);
    ui->graphicsView->ensureVisible(pic4);

    m_image_gray = NULL;
    m_image_mono = NULL;
    m_image_width = 0;
    m_image_height = 0;

    m_camera_mono = NULL;

    //Start Camera
    camera_main();
    ui->actionStart->setEnabled(false);

    // resize Graphics View
    QTimer *size_timer = new QTimer(this);
    size_timer->setInterval(1000);
    size_timer->start();
    connect(size_timer, SIGNAL(timeout()), this, SLOT(adjust_view()));

    // display result
    QTimer *camera_timer = new QTimer(this);
    camera_timer->setInterval(150);
    camera_timer->start();
    connect(camera_timer, SIGNAL(timeout()), this, SLOT(process_camera()));

    setMaximumSize(980,900);
    showMaximized();

}

MainWindow::~MainWindow()
{
    qDebug()<<"quitting...\n";
    g_quit_flag = 1;
    do {
        usleep(15000);
    } while(g_camera_work == 1);
    qDebug()<<"quitted\n";
    delete ui;
}

void MainWindow::reset_slide()
{
    for(int i=0; i<sizeof(g_settings)/sizeof(check_setting); ++i)
    {
        check_setting& setting = g_settings[i];
        if(setting.style == check_style)
        {
            ui->verticalSlider->setValue(setting.mono_level);
            ui->verticalSlider_2->setValue(setting.candy_level);
            break;
        }

    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    (void)event;
    //setWindowTitle(tr("%1, %1").arg(event->size().width()).arg(event->size().height()));

}

void MainWindow::on_verticalSlider_valueChanged(int value)
{
    level = value % 250;
    ui->verticalSlider->setToolTip(tr("%1").arg(value));
}

void MainWindow::on_actionStop_triggered()
{
    g_quit_flag = 1;
    do {
        usleep(15000);
    } while(g_camera_work == 1);
    ui->actionStart->setEnabled(true);
}

void MainWindow::on_actionStart_triggered()
{
    g_quit_flag = 0;
    g_camera_work = 0;
    camera_main();
    ui->actionStart->setEnabled(false);
}

void MainWindow::on_verticalSlider_2_valueChanged(int value)
{
    candy_level = value;
    ui->verticalSlider_2->setToolTip(tr("%1").arg(value));
    process_batcheck();

}

void MainWindow::on_actionCheck_triggered()
{
    check_style = style_huahen;

    reset_slide();
    m_check = 1;
}

void MainWindow::process_batcheck()
{
    if(m_image_gray == NULL)
        return;

    QGraphicsPixmapItem* pic3;
    QGraphicsPixmapItem *pic4;
    QImage img3(m_image_gray, m_image_width,m_image_height, QImage::Format_Grayscale8);
    QImage img4(m_image_mono, m_image_width,m_image_height, QImage::Format_Grayscale8);

    QPixmap pix3;
    pic3 = m_items[2];

    proc_image(img3, pix3, 3, &m_results);
    pic3->setPixmap(pix3.scaled(m_pic_width,m_pic_height));


    QPixmap pix4=QPixmap::fromImage(img3);

    {
        QPainter p(&pix4);
        QPen pen;
        pen.setColor(QColor(255,0,0,255));
        pen.setWidth(10);
        p.setPen(pen);
        for(size_t i=0; i<m_results.size(); ++i)
        {
            QRect o = m_results[i];
            p.drawRect(o);
        }

    }
    pic4 = m_items[3];
    pic4->setPixmap(pix4.scaled(m_pic_width,m_pic_height));

//    QPixmap pix4;
//    pic4 = m_items[3];
//    proc_image(img4, pix4, 4);
//    pic4->setPixmap(pix4.scaled(m_pic_width,m_pic_height));

}

void MainWindow::adjust_view()
{
    QSize ssize = ui->graphicsView->viewport()->size();

    if(m_view_size == ssize)
        return;
    m_view_size = ssize;
    //double dw = m_pic_width*2+10;
    //double dh = m_pic_height*2+10;


    double dw = m_pic_width*2 + 10;
    double dh = m_pic_height*2 + 10;

    double sw = m_view_size.width()/dw;
    double sh = m_view_size.height()/dh;

    QMatrix mat = ui->graphicsView->matrix();
    mat.reset();
    mat = mat.scale(sw, sh);
    ui->graphicsView->setMatrix(mat);

}

void MainWindow::process_camera()
{
    if(g_images == NULL)
        return;

    acq_image* image = g_images;
    for(int i=0; i<g_images_count-1; ++i)
    {
        if(image->ready == 1)
        {
            break;
        }
        image++;
    }
    if(image->ready == 0)
        return;

    QGraphicsPixmapItem* pic;
    QGraphicsPixmapItem *pic2;

    pic = m_items[0];
    pic2 = m_items[1];

    if(m_camera_mono == NULL)
    {
        m_camera_mono = (uchar*)malloc(image->size);
    }

    QImage img((uchar*)image->data, (int)image->width, (int)image->height, QImage::Format_Grayscale8);
    //QImage img2;// = img.convertToFormat(QImage::Format_MonoLSB, Qt::MonoOnly);
    uchar* dt = img.bits();
    uchar* dt2 = m_camera_mono;


    //convert to mono
    for(qsizetype i=0; i<img.byteCount(); i++)
    {
        uchar c = 0;
        if( dt[i] > level)
                c = 255;
        dt2[i] = c;
    }
    QImage img2(dt2, (int)image->width, (int)image->height, QImage::Format_Grayscale8);


//    QPixmap pix=QPixmap::fromImage(img);

//    {
//        QPainter p(&pix);
//        QPen pen;
//        pen.setColor(QColor(255,0,0,255));
//        pen.setWidth(10);
//        p.setPen(pen);
//        for(size_t i=0; i<m_results.size(); ++i)
//        {
//            QRect o = m_results[i];
//            p.drawRect(o);
//        }

//    }
    QPixmap pix = QPixmap::fromImage(img);
    pic->setPixmap(pix.scaled(m_pic_width,m_pic_height));

    QPixmap pix2;
    pix2.convertFromImage(img2.scaled(m_pic_width,m_pic_height));
    pic2->setPixmap(pix2);

    if(m_check == 1)
    {
        if (m_image_gray == NULL)
        {
            m_image_gray = (uchar*)malloc( img.byteCount() );
            m_image_mono = (uchar*)malloc(img.byteCount());
            m_image_height = img.height();
            m_image_width = img.width();
        }
        memcpy(m_image_gray, img.bits(), img.byteCount());
        memcpy(m_image_mono, img2.bits(), img2.byteCount());
        process_batcheck();

        m_check = 0;
    }

    image->ready = 0;

}

void MainWindow::on_verticalSlider_2_sliderReleased()
{

}

void MainWindow::on_actionCheckTop_triggered()
{
    check_style = style_top;

    reset_slide();
    m_check = 1;

}

void MainWindow::on_actionCheckCover_triggered()
{
    check_style = style_skin;


    reset_slide();
    m_check = 1;
}
