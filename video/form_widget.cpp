#include "form_widget.h"
#include "ui_form_widget.h"
#include <QMessageBox>
#include <memory>
#include <QDebug>
#include <QScrollBar>
#include <QDir>
#include <boost/lexical_cast.hpp>
#include <QHeaderView>
#include <QPainter>

using namespace std;

form_widget::form_widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::form_widget)
{
    ui->setupUi(this);

    set_views ();
    set_scrolls ();

    connect (src_model_.get (), &video_form_model::dataChanged, [this]{
        auto sum = src_model_->get_std_sum ();
        emit total_time_changed (sum);
    });
    connect (src_model_.get (), &video_form_model::dataChanged, this, &form_widget::data_changed);
}

form_widget::~form_widget()
{
    delete ui;
}

void form_widget::mark(long long time_val)
{
    if (current_view_ != ui->table_data)
    {
        QMessageBox::information (this, "标记", "请选择一个操作内容");
        return;
    }

    auto model = current_view_->selectionModel ();
    auto list = model->selectedIndexes ();

    if (list.empty ())
    {
        QMessageBox::information (this, "标记", "请选择一个操作内容");
        return;
    }

    int min_row = 9999, min_col = 9999;
    for (const auto& iter : list)
    {
        if (iter.row () < min_row)
        {
            min_row = iter.row ();
        }
        if (iter.column () < min_col)
        {
            min_col = iter.column ();
        }
    }

    auto data_model = current_view_->model (); assert (data_model);
    auto index = data_model->index (min_row, min_col);

    current_view_->model ()->setData (index, static_cast<double>(time_val) / 1000, Qt::EditRole);

    current_view_->clearSelection ();
    auto op_next_index = get_next_index (index);

    if (op_next_index)
    {
        model->select (*op_next_index, QItemSelectionModel::Select);
        scroll_to_index (*op_next_index);
    }
    else
    {
        current_view_ = nullptr;
    }
}

void form_widget::set_views()
{
    views_ = {ui->table_des, ui->table_data, ui->table_result};
    set_des_view ();
    set_data_view ();
    set_result_view ();

    for (const auto & iter : views_)
    {
        //iter->horizontalHeader ()->setSectionResizeMode (QHeaderView::Interactive);
        connect (iter, &QTableView::pressed, this, &form_widget::table_clicked);
    }
}

void form_widget::set_des_view()
{
    ui->table_des->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->table_des->horizontalHeader ()->setSectionResizeMode (QHeaderView::Fixed);


    ui->table_des->setItemDelegate (des_delegate_.get ());
}

void form_widget::set_data_view()
{
    ui->table_data->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->table_data->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);

    assert (video_form_model::data_col % 2 == 0);

    ui->table_data->setItemDelegate (des_delegate_.get ());
}

void form_widget::set_result_view()
{
    ui->table_result->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->table_result->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);
    ui->table_result->setItemDelegate (des_delegate_.get ());
}

optional<QModelIndex> form_widget::get_next_index(const QModelIndex & index) const
{
    if (!index.isValid())
    {
        QMessageBox::information (nullptr, "标记", "请选择一个操作内容");
        return {};
    }
    auto model = index.model (); assert (model);

    int row = index.row ();
    int col = index.column ();
    int next_row = -1;
    int next_col = -1;

    if (row == model->rowCount () - 1 and col >= model->columnCount () - 2)
    {
        return {};
    }

    if (col % 2 != 0)
    {
        col --;
    }

    if (row == model->rowCount () - 1)
    {
        next_col = col + 2;
        next_row = 0;
    }
    else
    {
        next_col = col;
        next_row = row + 1;
    }

    return model->index (next_row, next_col);
}

void form_widget::set_models()
{
    set_des_model ();
    set_data_model ();
    set_result_model ();
}

void form_widget::set_des_model()
{
    ui->table_des->setModel (nullptr);

    model_des_ = make_unique<video_form_split> ();
    model_des_->setSourceModel (src_model_.get ());
    model_des_->set_range (0, 2);

    ui->table_des->setModel (model_des_.get ());
}

void form_widget::set_data_model()
{
    ui->table_data->setModel (nullptr);

    model_data_ = make_unique<video_form_split> ();
    model_data_->setSourceModel (src_model_.get ());
    model_data_->set_range (2, video_form_model::data_col + 2);

    ui->table_data->setModel (model_data_.get ());
}

void form_widget::set_result_model()
{
    ui->table_result->setModel (nullptr);

    model_result_ = make_unique<video_form_split> ();
    model_result_->setSourceModel (src_model_.get ());
    model_result_->set_range (video_form_model::data_col + 2, src_model_->columnCount ());

    ui->table_result->setModel (model_result_.get ());
}

void form_widget::table_clicked(const QModelIndex&)
{
    auto src = sender (); assert (src);
    current_view_ = dynamic_cast<table_view*>(src); assert (current_view_);

    for (const auto& iter : views_)
    {
        if (current_view_ != iter)
        {
            iter->clearSelection ();
        }
    }
}

json form_widget::task_data()
{
    json task = json::array ();
    QModelIndex index;
    QVariant vat;

    for (int i = 0; i < src_model_->rowCount (); ++i)
    {
        index = src_model_->index (i,1);
        vat = index.data ();
        task.push_back (vat.toString().toStdString ());
    }
    return task;
}

json form_widget::observation_time()
{
    json observation = json::array ();
    QModelIndex index;
    QVariant vat;
    int base_colume = 2;
    bool is_ok = false;

    for (int row = base_colume; row < base_colume + 10 * 2; row += 2)
    {
        json tr_row = json::array ();
        for (int i = 0; i < src_model_->rowCount (); ++i)
        {
            json tr = json::object ();
            index = src_model_->index (i,row);
            vat = index.data ();
            if (vat.isNull())
            {
                tr ["T"] = (double)0;
            }
            else
            {
                tr ["T"] = vat.toDouble (&is_ok);
                assert (is_ok);
            }

            index = src_model_->index (i,row+1);
            vat = index.data ();
            if (vat.isNull())
            {
                tr ["R"] = (double)0;
            }
            else
            {
                tr ["R"] = vat.toDouble (&is_ok);
                assert (is_ok);
            }

            try
            {
                if (row == base_colume)
                {
                    double R = tr["R"];
                    double T = tr["T"];
                    (void)R;
                    (void)T;
                    assert (R == R);
                }
            }
            catch (...)
            {
                assert (false);
            }
            tr_row.push_back (tr);
        }
        observation.push_back (tr_row);
    }
    return observation;
}

json form_widget::result_data()
{
    json result = json::array ();
    QModelIndex index;
    QVariant vat;
    int base_colume = 1 + 20;
    bool is_ok = false;

    for (int i = 0; i < src_model_->rowCount (); ++i)
    {
        json row_data = json::object ();
        index = src_model_->index (i,base_colume + 1);
        vat = index.data ();
        if (!vat.isNull ())
        {
            row_data ["平均时间"] = vat.toDouble (&is_ok); assert (is_ok);
        }
        else
        {
            row_data ["平均时间"] = (double)0;
        }

        index = src_model_->index (i,base_colume + 2);
        vat = index.data ();
        row_data ["评比系数"] = vat.toDouble (&is_ok); assert (is_ok);

        index = src_model_->index (i,base_colume + 3);
        vat = index.data ();
        if (!vat.isNull ())
        {
            row_data ["基本时间"] = vat.toDouble (&is_ok); assert (is_ok);
        }
        else
        {
            row_data ["基本时间"] = (double)0;
        }

        index = src_model_->index (i,base_colume + 4);
        vat = index.data ();
        row_data ["宽放率"] = vat.toString ().toStdString ();

        index = src_model_->index (i,base_colume + 5);
        vat = index.data ();
        if (!vat.isNull ())
        {
            row_data ["标准时间"] = vat.toDouble (&is_ok); assert (is_ok);
        }
        else
        {
            row_data ["标准时间"] = (double)0;
        }

        index = src_model_->index (i,base_colume + 6);
        vat = index.data ();
        row_data ["增值/非增值"] = vat.toString ().toStdString ();

        index = src_model_->index (i,base_colume + 7);
        vat = index.data ();
        row_data ["操作分类"] = vat.toString ().toStdString ();

        result.push_back (row_data);
    }
    return result;
}

json form_widget::map_to_json(const std::map<QString, QString> &map)
{
    try
    {
        json info = json::object();
        const auto iter_product = map.find ("产品");
        assert (iter_product != map.end ());
        info ["产品"] = iter_product->second.toStdString ();

        const auto iter_station = map.find ("工站号");
        assert (iter_station != map.end ());
        info ["工站号"] = iter_station->second.toStdString ();

        const auto iter_man = map.find ("作业员");
        assert (iter_man != map.end());
        info ["作业员"] = iter_man->second.toStdString ();

        const auto iter_date = map.find ("测量日期");
        assert (iter_date != map.end());
        info ["测量日期"] = iter_date->second.toStdString ();

        const auto iter_mesure_man = map.find ("测量人");
        assert (iter_mesure_man != map.end());
        info ["测量人"] = iter_mesure_man->second.toStdString ();

        info ["测量方法"] = "视频分析法";

        const auto iter_unit = map.find ("数据单位");
        assert (iter_unit != map.end());
        info ["数据单位"] = iter_unit->second.toStdString ();

        return info;
    }
    catch (std::exception &)
    {
        return {};
    }
}

json form_widget::info_pandect(const json& data, const std::string& video_path)
{
    try
    {
        auto iter_result = data.find ("结果");
        assert (iter_result != data.end ());
        assert (iter_result->is_array ());
        auto iter_task = data.find ("作业内容");
        assert (iter_task != data.end ());
        assert (iter_task->is_array ());


        json pandect = json::array ();

        for (unsigned i = 0; i < iter_result->size (); ++i)
        {
            auto & row_object = (*iter_result)[i];
            assert (row_object.is_object ());
            json content = json::object ();

            std::string task = (*iter_task)[i];
            content ["作业内容"] = task;

            auto iter_rate = row_object.find ("评比系数"); assert (iter_rate != row_object.end ());
            content ["评比系数"] = *iter_rate;
            auto iter_base_time = row_object.find ("基本时间"); assert (iter_base_time != row_object.end ());
            content ["基本时间"] = *iter_base_time;
            auto iter_allowance = row_object.find ("宽放率"); assert (iter_allowance != row_object.end ());
            content ["宽放率"] = *iter_allowance;
            auto iter_stdtime = row_object.find ("标准时间"); assert (iter_stdtime != row_object.end ());
            content ["标准时间"] = *iter_stdtime;
            auto iter_added = row_object.find ("增值/非增值"); assert (iter_added != row_object.end ());
            content ["增值/非增值"] = *iter_added;
            auto iter_opt = row_object.find ("操作分类"); assert (iter_opt != row_object.end ());
            content ["操作分类"] = *iter_opt;
            auto iter_average_time = row_object.find ("平均时间"); assert (iter_average_time != row_object.end ());
            content ["测量时间"] = *iter_average_time;
            content ["视频路径"] = video_path;

            pandect.push_back (content);
        }

        return pandect;
    }
    catch (std::exception &e)
    {
        qDebug () << __LINE__ << e.what();
        return {};
    }
    return {};
}

//void form_widget::save_task(const QString &filename, const std::map<QString, QString> &info, const json &data)
//{
//    const auto station_iter = info.find ("产品");
//    assert (station_iter != info.end ());
//    auto station_dir = station_iter->second;
//
//    const auto process_iter = info.find ("工艺");
//    assert (process_iter != info.end ());
//    auto process_dir = process_iter->second;
//
//    QDir dir {PRODUCT_PATH};
//
//    bool is_ok;
//    is_ok = dir.cd (station_dir); assert (is_ok);
//    is_ok = dir.cd (process_dir); assert (is_ok);
//    auto save_path = dir.absoluteFilePath(filename);
//
//    auto transcode_path = unicode_to_system (save_path.toStdString());
//    krys::write_buffer (transcode_path, data.dump (4));
//}

void form_widget::set_scrolls()
{
    for (auto& item : views_)
    {
        auto scroll = make_unique<QScrollBar> ();
        item->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOn);
        connect (scroll.get (), &QScrollBar::valueChanged, [=] (int value)
        {
            for (auto iter : views_)
            {
                iter->verticalScrollBar ()->setValue (value);
            }
        });

        item->setVerticalScrollBar (scroll.release ());
    }
}

void form_widget::clear()
{
    ui->table_data->setModel (nullptr);
    model_data_->setSourceModel (nullptr);
    ui->table_des->setModel (nullptr);
    model_des_->setSourceModel (nullptr);
    ui->table_result->setModel (nullptr);
    model_result_->setSourceModel (nullptr);

    src_model_->clear ();

    model_data_->setSourceModel (src_model_.get ());
    ui->table_data->setModel (model_data_.get ());

    model_des_->setSourceModel (src_model_.get ());
    ui->table_des->setModel (model_des_.get ());

    model_result_->setSourceModel (src_model_.get ());
    ui->table_result->setModel (model_result_.get ());
}


void form_widget::load_task(const json &task)
{
    assert (task.is_array ());
    int max_rows = static_cast<int> (task.size ());
    QModelIndex index;
    QVariant val;
    std::string task_str;

    for (int i = 0; i < max_rows; ++ i)
    {
        index = src_model_->index (i,1);
        auto& task_index_ref = task.at (static_cast<size_t> (i));
        assert (task_index_ref.is_string ());
        task_str = task_index_ref;
        val.setValue (QString (task_str.data ()));
        src_model_->setData (index, val);
    }
}

void form_widget::load_data(const json &data)
{
    assert (data.is_array ());
    QModelIndex index;
    QVariant vat;
    double T;

    for (unsigned i = 0; i<data.size (); ++i)
    {
        auto& cir = data [i];
        assert (cir.is_array ());
        for (unsigned j=0; j < cir.size (); ++j)
        {
            auto row_data = cir [j];
            assert (row_data.is_object ());

            index = src_model_->index (j, 2 + i*2);
            auto iter = row_data.find ("T");
            assert (iter->is_number ());
            T = *iter;
            if (T == 0)
            {
                src_model_->setData (index, QVariant {});
            }
            else
            {
                vat.setValue (T);
                src_model_->setData (index, vat);
            }
        }
    }
}

void form_widget::load_result(const json &result)
{
    assert (result.is_array ());
    QModelIndex index;
    QVariant vat;
    int base_colume = 2+ 2*10;

    for (unsigned i=0; i<result.size (); ++i)
    {
        auto& row_data = result [i];
        assert (row_data.is_object ());

        auto iter_rate = row_data.find ("评比系数");
        assert (iter_rate != row_data.end () and iter_rate->is_number ());
        double rate = *iter_rate;
        index = src_model_->index (i, base_colume + 1);
        vat.setValue (rate);
        src_model_->setData (index, vat);

        auto iter_allowance = row_data.find ("宽放率");
        assert (iter_allowance != row_data.end () and iter_allowance->is_string ());
        std::string allowance_str = *iter_allowance;
        auto pos = allowance_str.find_first_of (" ");
        allowance_str.erase (pos);
        double allowance;
        try
        {
            allowance = boost::lexical_cast<double> (allowance_str.data ());
        }
        catch (std::exception&)
        {
            assert (false);
        }

        index = src_model_->index (i, base_colume + 3);
        vat.setValue (allowance);
        src_model_->setData (index, vat);

        auto iter_opt_type = row_data.find ("操作分类");
        assert (iter_opt_type != row_data.end () and iter_opt_type->is_string ());
        std::string opt_type = *iter_opt_type;
        index = src_model_->index (i, base_colume + 6);
        vat.setValue (QString (opt_type.data ()));
        src_model_->setData (index, vat);
    }
}

void form_widget::set_editable(bool b)
{
    if (b == true)
    {
        ui->table_data->setEditTriggers (QAbstractItemView::AllEditTriggers);
        ui->table_des->setEditTriggers (QAbstractItemView::AllEditTriggers);
        ui->table_result->setEditTriggers (QAbstractItemView::AllEditTriggers);
    }
    else
    {
        ui->table_data->setEditTriggers (QAbstractItemView::NoEditTriggers);
        ui->table_des->setEditTriggers (QAbstractItemView::NoEditTriggers);
        ui->table_result->setEditTriggers (QAbstractItemView::NoEditTriggers);
    }
}


std::optional<action_ratio> form_widget::operation_ratio () const
{
    return src_model_->operation_ratio ();
}

std::optional<overall_stats> form_widget::operation_stats() const
{
    return src_model_->operation_stats ();
}

std::vector<qreal> form_widget::cycle_times() const
{
    return src_model_->cycle_times ();
}

json form_widget::export_data()
{
    json video_data = json::object ();

    json task = task_data ();
    json obs = observation_time ();
    json result = result_data ();

    video_data ["作业内容"] = task;
    video_data ["观测时间"] = obs;
    video_data ["结果"] = result;

    return video_data;
}

//void form_widget::save_file(const std::map<QString, QString> &dlg_info, std::vector <unsigned long long >& invalid_vec)
//{
//    try
//    {
//        auto iter = dlg_info.find ("作业内容");
//        assert (iter!=dlg_info.end());
//        auto filename = iter->second;
//        json extra_info = map_to_json (dlg_info);
//
//        auto video_table_data = export_data ();
//
//        json sheet = json::object();
//        sheet ["视频分析法详细信息"] = video_table_data;
//        json& detail = sheet ["视频分析法详细信息"];
//        assert (detail.is_object ());
//
//        const auto iter_path = dlg_info.find ("视频路径"); assert (iter_path!=dlg_info.end ());
//        auto path = iter_path->second.toStdString ();
//        detail ["视频路径"] = path;
//        detail ["无效时间段"] = invalid_vec;
//        sheet ["信息总览"] = info_pandect (video_table_data, path);
//
//        const auto iter_circulation = dlg_info.find ("循环"); assert (iter_circulation!=dlg_info.end ());
//        bool is_ok = false;
//        detail ["循环"] = iter_circulation->second.toInt (&is_ok); assert (is_ok);
//        json working_procedure = json::object ();
//        working_procedure ["表"] = sheet;
//        working_procedure ["附加信息"] = extra_info;
//        working_procedure ["文件来源"] = "产品工时";
//
//        //save_task (filename, dlg_info, working_procedure);
//
//    }
//    catch (std::exception &e)
//    {
//        qDebug () << __LINE__ << e.what();
//    }
//}


void form_widget::set_row(int num)
{
    emit line_exists (num != 0);

    src_model_->resize (num);
    set_models();
    set_views ();
    auto sum = src_model_->get_std_sum ();
    emit total_time_changed (sum);
}

void form_widget::add_row(int num)
{
    auto current_row = row ();
    set_row (num+current_row);
}

void form_widget::reduce_row(int num)
{
    auto current_row = row ();
    set_row (num-current_row);
}

bool form_widget::task_content_check()
{
    auto rows = src_model_->rowCount ();
    for (int i = 0; i < rows; ++i)
    {
        const auto index =src_model_->index (i,1);
        const auto content = index.data ().toString ().trimmed ();
        if (content.isEmpty ())
        {
            return false;
        }
    }
    return true;
}

void form_widget::scroll_to_index(const QModelIndex& index)
{
    assert (index.isValid ());
    ui->table_data->scrollTo (index);
}

void form_widget::on_paste()
{
    if (current_view_ != nullptr)
    {
        current_view_->on_paste ();
    }
}

void form_widget::on_copy()
{
    if (current_view_ != nullptr)
    {
        current_view_->on_copy_del(table_view::OPERATION_COPY);
    }
}

void form_widget::on_cut()
{
    if (current_view_ != nullptr)
    {
        current_view_->on_copy_del(table_view::OPERATION_COPY | table_view::OPERATION_DEL);
    }
}

void form_widget::on_del()
{
    if (current_view_ != nullptr)
    {
        current_view_->on_copy_del (table_view::OPERATION_DEL);
    }
}

int form_widget::row()
{
    return src_model_->rowCount ();
}
