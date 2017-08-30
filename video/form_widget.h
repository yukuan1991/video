#pragma once
#include <QWidget>
#include "video/video_delegate.h"
#include <QTableView>
#include "view/table_view.h"
#include <memory>
#include <QTableWidgetItem>
#include <vector>
#include "video_form_split.h"
#include <memory>
#include <json.hpp>
#include <QModelIndex>
#include <optional>
#include "video/utils.hpp"
#include "video/VideoFormModel.h"

using json = nlohmann::json;

namespace Ui {
class form_widget;
}

class form_widget final : public QWidget
{
    Q_OBJECT
public:
    static constexpr int max_row = 12;


public:
    explicit form_widget(QWidget *parent = 0);
    ~form_widget();
    void mark (long long time_val);
    int row ();
    void set_row (int num);
    void add_row (int num);
    void reduce_row (int num);
    bool task_content_check ();
    void scroll_to_index (const QModelIndex& index);

    void on_paste ();
    void on_copy ();
    void on_cut ();
    void on_del ();

    void load_task (const json& task);
    void load_data (const json& data);
    void load_result (const json& result);
    void set_editable (bool b);

    action_ratio get_ratio () const;
    std::optional<action_ratio> operation_ratio () const;
    std::optional<overall_stats> operation_stats () const;
    std::vector<qreal> cycle_times () const;

private:
    void set_views ();
    void set_des_view ();
    void set_data_view ();
    void set_result_view ();
    std::optional<QModelIndex> get_next_index (const QModelIndex&) const;

    void set_models ();
    void set_des_model ();
    void set_data_model ();
    void set_result_model ();
    void table_clicked (const QModelIndex&);
    json task_data ();
    json observation_time ();
    json result_data ();
    json map_to_json (const std::map <QString,QString>& map);
    json info_pandect (const json& json_data, const std::string& video_path);
    //void save_task (const QString& filename, const std::map <QString,QString>& info,const json &data);
    void set_scrolls ();
    void initConn();
    void initTable();
public:
    json export_data ();
    //void save_file (const std::map <QString, QString>& dlg_info, std::vector <unsigned long long >& invalid_vec);
    void clear ();

signals:
    void line_exists (bool yes_or_no);
    void bar_move (int value);
    void total_time_changed (const QString& sum);
    void data_changed ();

private:
    Ui::form_widget *ui;
    int total_round_ = 10;
    const std::unique_ptr<VideoFormModel> src_model_ = std::make_unique<VideoFormModel> ();
//    std::unique_ptr<video_form_model> src_model_ = std::make_unique<video_form_model> ();
    const std::unique_ptr<video_form_split> model_des_ = std::make_unique<video_form_split> ();
    const std::unique_ptr<video_form_split> model_data_ = std::make_unique<video_form_split> ();
    const std::unique_ptr<video_form_split> model_result_ = std::make_unique<video_form_split> ();
    const std::unique_ptr<video_delegate> des_delegate_ = std::make_unique<video_delegate> ();

    std::vector<QTableView*> views_;
    table_view* current_view_ = nullptr;
};
