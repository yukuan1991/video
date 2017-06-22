#pragma once

#include <QWidget>
#include <base/lang/not_null.h>
#include <string>

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
    using analysis_slot = void (video_analysis::*) ();
    not_null<video_analysis*> create_window ();
    void create_analysis ();
    video_analysis * current_sub_window ();

    void apply_to_current (analysis_slot);
    void invalid_timespan ();

    void video_import ();
    void init_conn ();
    void change_task_count ();

    void on_save ();
private:
    Ui::video_main *ui;
};

