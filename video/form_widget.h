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

private:
    std::optional<QModelIndex> get_next_index (const QModelIndex&) const;

    void table_clicked (const QModelIndex&);
    json task_data ();
    json observation_time ();
    json result_data ();
    json map_to_json (const std::map <QString,QString>& map);
    json info_pandect (const json& json_data, const std::string& video_path);
    void set_scrolls ();

    void initConn();
    void initTable();
    void setTable();
public:
    json export_data ();
    void clear ();

signals:
    void total_time_changed (const QString& sum);
    void data_changed ();

private:
    Ui::form_widget *ui;
    const std::unique_ptr<VideoFormModel> src_model_ = std::make_unique<VideoFormModel> ();
    const std::unique_ptr<video_form_split> model_des_ = std::make_unique<video_form_split> ();
    const std::unique_ptr<video_form_split> model_data_ = std::make_unique<video_form_split> ();
    const std::unique_ptr<video_form_split> model_result_ = std::make_unique<video_form_split> ();
    const std::unique_ptr<video_delegate> des_delegate_ = std::make_unique<video_delegate> ();

    std::vector<QTableView*> views_;
    table_view* current_view_ = nullptr;
};
