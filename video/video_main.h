#pragma once

#include <QWidget>
#include <base/lang/not_null.h>

namespace Ui {
class video_main;
}

class QMdiArea;

class video_analysis;
class video_main final : public QWidget
{
    Q_OBJECT

public:
    explicit video_main(QWidget *parent = 0);
    ~video_main();
    QMdiArea * area ();
private:

    not_null<video_analysis*> create_window ();
    void create_analysis ();

    void video_import ();
    void init_conn ();
private:
    Ui::video_main *ui;
};

