#ifndef EDGE_CHECKER_H
#define EDGE_CHECKER_H

#include <QImage>
#include <QRect>
#include <QPainter>
#include <QBrush>
#include <QDebug>

#include <vector>

typedef struct _check_setting {
    int style;
    int x0;
    int y0;
    int x1;
    int y1;
    int x_stride;
    int y_stride;
    int candy_level;
    int mono_level;

} check_setting;


class edge_checker
{
    int m_x_stride;
    int m_y_stride;
    QImage m_image;
    QImage m_mark;
    QBrush m_black_brush;
    std::vector<QRect> m_areas;
    uchar* m_mark_bits;
    //uchar m_black_bits[4096];


    int m_max_x;
    int m_max_y;
    int m_min_x;
    int m_min_y;
public:
    inline QRect area_rect(int x, int y)
    {
        return QRect(x,y, m_x_stride, m_y_stride);
    }

inline bool check_color(QImage& img)
{
    uchar* bits = img.bits();
    //qDebug()<<img.size()<<img.byteCount();
//    int i = memcmp(m_black_bits, bits, img.byteCount());
//    if(i == 0)
//        return false;
//    return true;

    int cnt =0;

    for(int i=0; i<img.byteCount(); ++i)
    {
        uchar c = bits[i];
        if(c == 255)
            cnt ++;
        if(cnt >= 10)
            return true;
    }
    return false;
}


inline bool check_block(QRect& a)
{

    //qDebug()<<a;
    QImage slice = m_image.copy(a);
    QImage smark = m_mark.copy(a);
    if(check_color(slice) && check_color(smark) )
    {


        QPainter p;
        p.begin(&m_mark);
        p.setBrush(m_black_brush);
        p.fillRect(a, m_black_brush);
        p.end();

        //smark = m_mark.copy(x,y, x_scale, y_scale);
        //qDebug()<<"block:"<<a<<check_color(slice)<<check_color(smark);
        //print_color(smark);

        m_areas.push_back(a);
        return true;
    }
    return false;
}
inline bool check_proc(QRect a)
{
    int x = a.left() + m_x_stride;
    int y = a.top() + m_y_stride;

    int cr=0;
    int cd = 0;
    int cl = 0;
    if(x > m_max_x)
    {
        x = m_max_x;
        cr = 1;
    }
    if(y > m_max_y)
    {
        y=m_max_y;
        cd = 1;
    }

    int xl = a.left() - m_x_stride;
    if(xl <m_min_x)
    {
        xl = m_min_x;
        cl = 1;
    }
    //qDebug()<<x<<y;
    QRect right=        area_rect(  x,      a.top());
    QRect right_down =  area_rect(  x,      y);
    QRect down =        area_rect(  a.left(),   y);
    QRect left_down =   area_rect(  xl,     y);

    bool rr = check_block(right);
    bool rrd = check_block(right_down);
    bool rd = check_block(down);
    bool rld= check_block(left_down);

    if(rr && cr == 0)
    {
        check_proc(right);
    }
    if(rrd && cd == 0 && cr == 0)
    {
        check_proc(right_down);
    }
    if(rd && cd == 0)
    {
        check_proc(down);
    }
    if(rld && cl == 0)
    {
        check_proc(left_down);
    }
    return true;

}

~edge_checker();
edge_checker(QPixmap& dst, uchar* data, int width, int height, check_setting setting);
void process(std::vector<QRect>& result);
std::vector<QRect> m_results;
}; //end-class edge-checker




#endif // EDGE_CHECKER_H
