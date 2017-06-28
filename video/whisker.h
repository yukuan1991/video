#pragma once
#include <QWidget>

struct whisker_data
{
    qreal top = 0.0;
    qreal top_quarter = 0.0;
    qreal mid = 0.0;
    qreal bottom_quarter = 0.0;
    qreal bottom = 0.0;
};

class whisker : public QWidget
{
    Q_OBJECT
public:
    whisker(QWidget *parent = 0);
    void set_data (const whisker_data & data) noexcept { data_ = data; }
    const whisker_data & data () const noexcept { return data_; }
    ~whisker();
    void paintEvent (QPaintEvent * event) override;
private:
    whisker_data data_;
};

