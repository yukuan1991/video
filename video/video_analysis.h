#pragma once

#include <QWidget>
#include "form_widget.h"
#include "video/first_dlg.h"
#include "video_player.hpp"
#include <memory>
#include <vector>
#include "json.hpp"
#include "video/utils.hpp"
#include <gsl/span>

namespace QtCharts {
class QPieSeries;
}

namespace Ui {
class video_analysis;
}

class video_analysis : public QWidget
{
    Q_OBJECT
signals:
    void marked(long long int);

public:
    explicit video_analysis(QWidget *parent = 0);
    ~video_analysis();

    void push_reaction (const QString& data);
    void load (const json& data);

    void set_video_file (const QString &video);
    void modify_invalid();
    void set_task_count ();

    void on_paste ();

    void on_copy ();

    void on_cut ();

    void on_del ();

    void refresh_chart (action_ratio ratio);

    void refresh_stats (overall_stats stats);

    void update_box (gsl::span<qreal> data);

    void set_measure_date (const QDate & date);
    QString measure_date () const;

    void set_measure_man (const QString & data);
    QString measure_man () const;

    void set_task_man (const QString & data);
    QString task_man () const;

    json dump ();

    void set_example_cycle (int cycle);

    int example_cycle() const noexcept;

private slots:
    void on_combo_second_activated(int index);

    void on_video_player_state_changed(video_player::state_enum state);

    void on_button_sec_backward_clicked();

    void on_spinbox_rate_valueChanged(double arg1);

    void on_button_sec_forward_clicked();

    void on_video_player_stepped_into_invalid(qint64 pos_in, qint64 pos_out);

    void on_button_mark_clicked();

    void on_marked (long long int);

    void on_button_setting_rows_clicked();

private:

    void init_video_widget (const json& video_detail);
    void init_chart ();

protected:
    bool eventFilter (QObject* obj, QEvent* event) override;
private:
    Ui::video_analysis *ui;

    std::vector<qint64> invalid_data_;

    std::vector<std::pair<std::string, std::string>> product_data_;
    const std::shared_ptr<bool> alive_ = std::make_shared<bool> (true);

    std::unique_ptr<first_dlg> dlg_ = std::make_unique<first_dlg> (nullptr);

    bool file_opening_;

    QtCharts::QPieSeries* operation_type_;
    QtCharts::QPieSeries* efficiency_;
};
