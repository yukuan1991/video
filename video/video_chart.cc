#include "video_chart.h"
#include "ui_video_chart.h"
#include <QPainter>

video_chart::video_chart(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::video_chart)
{
    ui->setupUi(this);
}

video_chart::~video_chart()
{
    delete ui;
}

void video_chart::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
