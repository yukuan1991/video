#pragma once

#include <QWidget>
#include <base/lang/not_null.h>
#include <string>

namespace Ui {
class video_main;
}

class QMdiArea;
class QMdiSubWindow;

class video_analysis;
class video_main final : public QWidget
{
    Q_OBJECT
signals:
    void mdi_active (bool);
public:
    explicit video_main(QWidget *parent = 0);
    ~video_main();
    QMdiArea * area ();
private:
    using analysis_slot = void (video_analysis::*) ();
    not_null<video_analysis*> create_window (const QString & title);
    void create_analysis ();
    video_analysis * current_sub_window ();

    void mdi_changed (QMdiSubWindow *window);

    void apply_to_current (analysis_slot);
    void invalid_timespan ();

    void on_measure_date ();
    void on_measure_man ();
    void on_task_man ();

    void video_import ();
    void init_conn ();
    void change_task_count ();

    void export_xlsx ();

    void on_save ();
    void on_open ();
    void on_save_as ();
    void on_example_cycle ();
private:
    Ui::video_main *ui;
};

