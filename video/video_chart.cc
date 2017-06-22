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


