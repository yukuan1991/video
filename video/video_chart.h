#ifndef VIDEO_CHART_H
#define VIDEO_CHART_H

#include <QWidget>

namespace Ui {
class video_chart;
}

class video_chart : public QWidget
{
    Q_OBJECT

public:
    explicit video_chart(QWidget *parent = 0);
    ~video_chart();
private:
    Ui::video_chart *ui;
};

#endif // VIDEO_CHART_H
