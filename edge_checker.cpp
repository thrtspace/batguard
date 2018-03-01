#include "edge_checker.h"

#include <vector>







edge_checker::~edge_checker()
{
    if(m_mark_bits)
        free(m_mark_bits);
}

edge_checker::edge_checker(QPixmap& dst, uchar* data, int width, int height, check_setting setting)
    :m_image(data, width, height, QImage::Format_Grayscale8)
{
    //cv::Mat src = cv::imread("/home/user/dst3.bmp");
    //cv::Mat mark;
    //mark.create(src.size(), src.type());
    //mark = cv::Scalar::all(0);

    m_x_stride = setting.x_stride;
    m_y_stride = setting.y_stride;
    m_min_x = setting.x0;
    m_max_x = setting.x1;
    m_min_y = setting.y0;
    m_max_y = setting.y1;

    QPainter p(&dst);



    int x;
    int y;
    int w = dst.width();
    int h = dst.height();
    m_mark_bits = (uchar*)malloc(w*h);
    memset(m_mark_bits, 255, w*h);
    QImage mark(m_mark_bits, w, h, QImage::Format_Grayscale8);
    m_mark.swap(mark);

    m_black_brush.setColor(Qt::black);
    m_black_brush.setStyle(Qt::SolidPattern);


    //memset(m_black_bits, 0, sizeof(m_black_bits));
    //qDebug()<<"checking...";


    QMargins margin(0,0,m_x_stride/2, m_y_stride/2);
    std::vector<QRect> results;
    for(x=m_min_x; x< m_max_x; x+= m_x_stride)
    {
        for(y= m_min_y; y< m_max_y; y+= m_y_stride)
        {
            //qDebug()<<"check block"<<x<<y;

            QRect area = area_rect(x,y);
            if(check_block(area))
            {


                area+=margin;

                results.push_back(area);

            }




        } //end-for y
    }//-end-for x
    QList<size_t> labels;

//    QBrush b;
//    b.setColor(QColor(255,0,0,40));
//    b.setStyle(Qt::SolidPattern);
//    p.setBrush(b);

    QPen pen;
    pen.setColor(QColor(255,0,0,255));
    pen.setWidth(10);
    p.setPen(pen);

    QFont font;
    font.setPixelSize(64);
    p.setFont(font);


    //qDebug()<<results.size();

    QMargins m(20,20,0,0);


    for(size_t i=0; i<results.size(); ++i)
    {
        QRect o = results[i];
        if(labels.contains(i))
            continue;

        for(size_t j=i+1; j<results.size(); ++j)
        {
            QRect o2 = results[j];
            if(o.intersects(o2) && !labels.contains(j) )
            {
                o = o.united(o2);
                labels<<j;
            }
        }
        o+=m;
        if(o.left()<0)
            o.setLeft(0);
        if(o.top() <0)
            o.setTop(0);
        p.drawRect(o);
        p.drawText(o.left(),o.top() - 40, "缺陷");
        //qDebug()<<o;
        m_results.push_back(o);

    }


}

void edge_checker::process(std::vector<QRect> &result)
{
    result = m_results;
}


