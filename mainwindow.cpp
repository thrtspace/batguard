#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <unistd.h>

#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QObject>
#include <QFile>
#include <QDebug>

#include <opencv2/opencv.hpp>

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
    style_skip
};

typedef struct _check_setting {
    int style;
    int x0;
    int y0;
    int x1;
    int y1;
} check_setting;
check_setting setting[4]={
    {0, 1250,400,1600,1500},

};
// proc image
static inline void proc_image(const QImage& img, QPixmap& out, const int num = 0)
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

    cv::Canny(edge, edge, candy_level, candy_level*3, 3, false);

    cv::Mat dst;
    dst.create(src.size(), src.type());
    dst = cv::Scalar::all(0);
    src.copyTo(dst, edge);

    QString name = QString("/home/user/dst%1.bmp").arg(num);
    cv::imwrite(name.toStdString(), edge);

    //convert dst to QImage
    QImage image(dst.data, dst.size().width, dst.size().height, QImage::Format_Grayscale8);


    out.convertFromImage(image);//.scaled(800,600));

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

    //Start Camera
    camera_main();
    ui->actionStart->setEnabled(false);

    // resize Graphics View
    QTimer *size_timer = new QTimer(this);
    size_timer->setInterval(1000);
    size_timer->start();
    QObject::connect(size_timer, &QTimer::timeout,
                     [=](){
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
    });

    // display result
    QTimer *timer = new QTimer(this);
    timer->setInterval(30);
    timer->start();
    QObject::connect(timer, &QTimer::timeout,
                     [=](){
        if(g_images == NULL)
            return;

        acq_image* image = g_images;
        for(int i=0; i<g_images_count; ++i)
        {
            image = g_images +i;
            if(image->ready == 1)
            {
                break;
            }
        }
        if(image->ready == 0)
            return;

        QImage img((uchar*)image->data, (int)image->width, (int)image->height, QImage::Format_Grayscale8);
        //QImage img2;// = img.convertToFormat(QImage::Format_MonoLSB, Qt::MonoOnly);
        uchar* dt = img.bits();
        uchar* dt2 = (uchar*)malloc(img.byteCount()/8);

        //convert to mono
        for(qsizetype i=0; i<img.byteCount()/8; i++)
        {
            uchar c = 0;
            for(int j=0; j<8; ++j)
            {
                if( dt[i*8 + j] > level)
                    c += 1<<j;
            }
            dt2[i] = c;

        }
        QImage img2(dt2, (int)image->width, (int)image->height, QImage::Format_MonoLSB);


        QPixmap pix;
        pix.convertFromImage(img.scaled(m_pic_width,m_pic_height));
        pic->setPixmap(pix);

        QPixmap pix2;
        pix2.convertFromImage(img2.scaled(m_pic_width,m_pic_height));
        pic2->setPixmap(pix2);

        if(m_check == 1)
        {
            if (m_image_gray == NULL)
            {
                m_image_gray = (uchar*)malloc( img.byteCount() );
                m_image_mono = (uchar*)malloc(img.byteCount()/8);
                m_image_height = img.height();
                m_image_width = img.width();
            }
            memcpy(m_image_gray, img.bits(), img.byteCount());
            memcpy(m_image_mono, img2.bits(), img2.byteCount());
            process_batcheck();

            m_check = 0;
        }


        image->ready = 0;
        free(dt2);

    });
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    (void)event;

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
    m_check = 1;
}

void MainWindow::process_batcheck()
{
    QGraphicsPixmapItem* pic3;
    QGraphicsPixmapItem *pic4;
    QImage img3(m_image_gray, m_image_width,m_image_height, QImage::Format_Grayscale8);
    QPixmap pix3;
    pic3 = m_items[2];
    proc_image(img3, pix3, 3);
    pic3->setPixmap(pix3.scaled(m_pic_width,m_pic_height));

    QImage img_mono(m_image_mono, m_image_width,m_image_height, QImage::Format_MonoLSB);
    QImage img4=img_mono.convertToFormat(QImage::Format_Grayscale8);

    QPixmap pix4;
    pic4 = m_items[3];
    proc_image(img4, pix4, 4);
    pic4->setPixmap(pix4.scaled(m_pic_width,m_pic_height));

}
