#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <vector>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void reset_slide();

protected:
    void resizeEvent(QResizeEvent *event);

private slots:
    void on_verticalSlider_valueChanged(int value);

    void on_actionStop_triggered();

    void on_actionStart_triggered();

    void on_verticalSlider_2_valueChanged(int value);

    void on_actionCheck_triggered();

    void process_batcheck();

    void adjust_view();
    void process_camera();

    void on_verticalSlider_2_sliderReleased();

    void on_actionCheckTop_triggered();

    void on_actionCheckCover_triggered();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *m_scene;
    std::vector<QGraphicsPixmapItem*> m_items;
    int m_pic_width;
    int m_pic_height;
    QSize m_view_size;
    int m_check ;
    uchar* m_image_gray;
    uchar* m_image_mono;
    int m_image_width;
    int m_image_height;

    uchar* m_camera_mono;
    std::vector<QRect> m_results;

};

#endif // MAINWINDOW_H
