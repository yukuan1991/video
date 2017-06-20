#ifndef FIRST_DLG_H
#define FIRST_DLG_H

#include <QDialog>
#include "video_player.hpp"

namespace Ui {
class first_dlg;
}

class first_dlg : public QDialog
{
    Q_OBJECT

public:
    explicit first_dlg(QWidget *parent = 0);
    ~first_dlg();

    void set_current_video(const QString &file);

    const std::vector<qint64> &retrieve_invalid();
    void set_invalid(std::vector<qint64> &v);

private slots:
    void on_button_start_clicked();
    void on_widget_state_changed(const video_player::state_enum& state);

    void on_button_end_clicked();

    void on_button_confirm_clicked();

    void on_first_dlg_finished(int result);

private:
    Ui::first_dlg *ui;
    unsigned long long start_mark_time_;
    unsigned long long end_mark_time_;
    int start_mark_=0;
    int end_mark_=0;
    std::vector<int> invalid_data;
};

#endif // FIRST_DLG_H
