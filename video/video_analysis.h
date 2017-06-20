#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "form_widget.h"
#include "video/first_dlg.h"
#include "video_player.hpp"
#include <memory>
#include <vector>
#include "json.hpp"


namespace Ui {
class video_analysis;
}

class video_analysis : public QWidget
{
    Q_OBJECT

public:
    explicit video_analysis(QWidget *parent = 0);
    ~video_analysis();

    void load_config(const std::map<std::string,std::string>& map_config);
    void config (std::map<std::string, std::string>& config);
    void push_reaction (const QString& data);
    void save_widget_config (std::map<std::string,std::string>& map);
    void load_json (const json& data);

signals:
    void marked(long long int);

private slots:
    void on_combo_second_activated(int index);

    void on_video_player_state_changed(video_player::state_enum state);

    void on_button_sec_backward_clicked();

    void on_spinbox_rate_valueChanged(double arg1);

    void on_button_sec_forward_clicked();

    void on_button_modify_clicked();

    void on_video_player_stepped_into_invalid(qint64 pos_in, qint64 pos_out);

    void on_button_mark_clicked();

    void on_marked (long long int);

    void on_button_setting_rows_clicked();

    void on_button_paste_clicked();

    void on_new_construction_clicked ();
    void on_copy ();

    void on_cut ();

    void on_del ();
private:
    void on_video_open (const QString &video);

    void init_video_widget (const json& video_detail);

protected:
    bool eventFilter (QObject* obj, QEvent* event) override;


private:
    Ui::video_analysis *ui;

    QString current_video_dir_ = ".";
    QString current_excel_dir_ = ".";
    QString current_excel_;
    QString current_video_;
    std::vector<qint64> invalid_data_;
    int chart_size_ = form_widget::max_row;

    std::vector<std::pair<std::string, std::string>> product_data_;
    const std::shared_ptr<bool> alive_ = std::make_shared<bool> (true);

    QAction * play_open_action_;
    QAction * pause_marked_action_;
    std::unique_ptr<first_dlg> dlg_ = std::make_unique<first_dlg> (nullptr);
    json current_file_data_;
    QString current_file_path_;

    bool file_opening_;
};


#endif // WIDGET_H
