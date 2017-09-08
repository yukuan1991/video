#pragma once

#include <QWidget>
#include <base/lang/not_null.h>
#include <string>

namespace Ui {
class VideoMainTrial;
}

class QMdiArea;
class QMdiSubWindow;

class VideoAnalysis;
class VideoMainTrial final : public QWidget
{
    Q_OBJECT
signals:
    void mdi_active (bool);
public:
    explicit VideoMainTrial(QWidget *parent = 0);
    ~VideoMainTrial();
    QMdiArea * area ();
private:
    using analysis_slot = void (VideoAnalysis::*) ();
    not_null<VideoAnalysis*> create_window (const QString & title);
    void create_analysis ();
    VideoAnalysis * current_sub_window ();

    void mdi_changed (QMdiSubWindow *window);

    void apply_to_current (analysis_slot);
    void invalid_timespan ();

    void on_measure_date ();
    void on_measure_man ();
    void on_task_man ();

    void video_import ();
    void init_conn ();
    void change_task_count ();

    void exportXlsx();

    void on_save ();
    void on_open ();
    void on_save_as ();
private:
    Ui::VideoMainTrial *ui;
};

