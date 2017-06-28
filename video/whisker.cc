#include "whisker.h"
#include <QPaintEvent>
#include <math.h>
#include <QPoint>
#include <QPainter>
#include <QFontMetrics>

using namespace std;

whisker::whisker(QWidget *parent)
    : QWidget(parent)
{
}


whisker::~whisker()
{

}

void whisker::paintEvent(QPaintEvent *)
{
    QPainter painter (this);
    painter.setBrush (Qt::transparent);

    const auto & data = data_;
    if (fabs (data.top) < 0.00000001 or
        fabs (data.top_quarter) < 0.00000001 or
        fabs (data.mid) < 0.00000001 or
        fabs (data.bottom_quarter) < 0.00000001 or
        fabs (data.bottom) < 0.00000001 or
        fabs (data.top - data.bottom) < 0.00000001)
    {
        return;
    }

    const auto top_height = static_cast<int> (0.2 * height ());
    const auto bottom_height = static_cast<int> (0.8 * height ());
    const auto left = static_cast<int> (0.35 * width ());
    const auto right = static_cast<int> (0.65 * width  ());

    const auto top_line = QLine (QPoint (left, top_height), QPoint (right, top_height));
    const auto bottom_line = QLine (QPoint (left, bottom_height), QPoint (right, bottom_height));

    painter.drawLine (top_line);
    painter.drawLine (bottom_line);

    const auto top_quarter_percent = (data.top_quarter - data.bottom) / (data.top - data.bottom);
    const auto top_height_ratio = 0.8 - top_quarter_percent * (0.8 - 0.2);
    const auto top_quarter_height = static_cast<int> (top_height_ratio * height ());

    const auto bottom_quarter_percent = (data.bottom_quarter - data.bottom) / (data.top - data.bottom);
    const auto bottom_height_ratio = 0.8 - bottom_quarter_percent * (0.8 - 0.2);
    const auto bottom_quarter_height = static_cast<int> (bottom_height_ratio * height ());

    const auto mid_percent = (data.mid - data.bottom) / (data.top - data.bottom);
    const auto mid_ratio = 0.8 - mid_percent * (0.8 - 0.2);
    const auto mid_height = static_cast<int> (mid_ratio * height ());

    const auto mid_left = QPoint (static_cast<int> (0.3 * width ()), mid_height);
    const auto mid_right = QPoint (static_cast<int> (0.7 * width ()), mid_height);
    const auto box_top_left = QPoint (static_cast<int> (0.3 * width ()), top_quarter_height);
    const auto box_bottom_right = QPoint (static_cast<int> (0.7 * width ()), bottom_quarter_height);

    painter.drawRect (QRect (box_top_left, box_bottom_right));

    painter.save ();
    {
        auto pen = painter.pen ();
        pen.setWidth (3);
        painter.setPen (pen);
    }

    painter.drawLine (mid_left, mid_right);
    painter.restore ();

    painter.save ();
    {
        auto pen = painter.pen ();
        pen.setStyle (Qt::DashLine);
        painter.setPen (pen);
    }

    const auto upper_dotline = QLine (QPoint (width () / 2, top_height), QPoint (width () / 2, top_quarter_height));
    const auto lower_dotline = QLine (QPoint (width () / 2, bottom_height), QPoint (width () / 2, bottom_quarter_height));

    painter.drawLine (upper_dotline);
    painter.drawLine (lower_dotline);
    painter.restore ();
}























