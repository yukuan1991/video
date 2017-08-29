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

    connect (src_model_.get (), &video_form_model::dataChanged, this, &form_widget::data_changed);

    model_des_->setSourceModel (src_model_.get ());
    model_des_->set_range (0, 2);
    ui->table_des->setModel (model_des_.get ());

    model_data_->setSourceModel (src_model_.get ());
    model_data_->set_range (2, 2 + VideoFormModel::dataCol_);
    ui->table_data->setModel (model_data_.get ());

    model_result_->setSourceModel (src_model_.get ());
    model_result_->set_range (2 + VideoFormModel::dataCol_, src_model_->columnCount ());
    ui->table_result->setModel (model_result_.get ());



    ui->table_des->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->table_des->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);

    ui->table_data->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->table_data->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);

    ui->table_result->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->table_result->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);
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
    //ui->table_data->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    //ui->table_data->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);

    //assert (VideoFormModel::dataCol_ % 2 == 0);

    //ui->table_data->setItemDelegate (des_delegate_.get ());
}

void form_widget::set_result_view()
{
    //ui->table_result->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    //ui->table_result->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);
    //ui->table_result->setItemDelegate (des_delegate_.get ());
}

optional<QModelIndex> form_widget::get_next_index(const QModelIndex & index) const
{
    return {};
}

void form_widget::set_models()
{
    //set_des_model ();
    //set_data_model ();
    //set_result_model ();
}

void form_widget::set_des_model()
{
    //ui->table_des->setModel (nullptr);

    //model_des_ = make_unique<video_form_split> ();
    //model_des_->setSourceModel (src_model_.get ());
    //model_des_->set_range (0, 2);

    //ui->table_des->setModel (model_des_.get ());
}

void form_widget::set_data_model()
{
    //ui->table_data->setModel (nullptr);

    //model_data_ = make_unique<video_form_split> ();
    //model_data_->setSourceModel (src_model_.get ());
    //model_data_->set_range (2, VideoFormModel::dataCol_ + 2);

    //ui->table_data->setModel (model_data_.get ());
}

void form_widget::set_result_model()
{
    //ui->table_result->setModel (nullptr);

    //model_result_ = make_unique<video_form_split> ();
    //model_result_->setSourceModel (src_model_.get ());
    //model_result_->set_range (VideoFormModel::dataCol_ + 2, src_model_->columnCount ());

    //ui->table_result->setModel (model_result_.get ());
}

void form_widget::table_clicked(const QModelIndex&)
{

}

json form_widget::task_data()
{
    return {};
}

json form_widget::observation_time()
{
    return {};
}

json form_widget::result_data()
{
    return {};
}

json form_widget::map_to_json(const std::map<QString, QString> &map)
{
    return {};
}

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
    //ui->table_data->setModel (nullptr);
    //model_data_->setSourceModel (nullptr);
    //ui->table_des->setModel (nullptr);
    //model_des_->setSourceModel (nullptr);
    //ui->table_result->setModel (nullptr);
    //model_result_->setSourceModel (nullptr);

    //src_model_->clear ();

    //model_data_->setSourceModel (src_model_.get ());
    //ui->table_data->setModel (model_data_.get ());

    //model_des_->setSourceModel (src_model_.get ());
    //ui->table_des->setModel (model_des_.get ());

    //model_result_->setSourceModel (src_model_.get ());
    //ui->table_result->setModel (model_result_.get ());
}


void form_widget::load_task(const json &task)
{
    //assert (task.is_array ());
    //int max_rows = static_cast<int> (task.size ());
    //QModelIndex index;
    //QVariant val;
    //std::string task_str;

    //for (int i = 0; i < max_rows; ++ i)
    //{
    //    index = src_model_->index (i,1);
    //    auto& task_index_ref = task.at (static_cast<size_t> (i));
    //    assert (task_index_ref.is_string ());
    //    task_str = task_index_ref;
    //    val.setValue (QString (task_str.data ()));
    //    src_model_->setData (index, val);
    //}
}

void form_widget::load_data(const json &data)
{
    //assert (data.is_array ());
    //QModelIndex index;
    //QVariant vat;
    //double T;

    //for (unsigned i = 0; i<data.size (); ++i)
    //{
    //    auto& cir = data [i];
    //    assert (cir.is_array ());
    //    for (unsigned j=0; j < cir.size (); ++j)
    //    {
    //        auto row_data = cir [j];
    //        assert (row_data.is_object ());

    //        index = src_model_->index (j, 2 + i*2);
    //        auto iter = row_data.find ("T");
    //        assert (iter->is_number ());
    //        T = *iter;
    //        if (T == 0)
    //        {
    //            src_model_->setData (index, QVariant {});
    //        }
    //        else
    //        {
    //            vat.setValue (T);
    //            src_model_->setData (index, vat);
    //        }
    //    }
    //}
}

void form_widget::load_result(const json &result)
{
    //assert (result.is_array ());
    //QModelIndex index;
    //QVariant vat;
    //int base_colume = 2+ 2*10;

    //for (unsigned i=0; i<result.size (); ++i)
    //{
    //    auto& row_data = result [i];
    //    assert (row_data.is_object ());

    //    auto iter_rate = row_data.find ("评比系数");
    //    assert (iter_rate != row_data.end () and iter_rate->is_number ());
    //    double rate = *iter_rate;
    //    index = src_model_->index (i, base_colume + 1);
    //    vat.setValue (rate);
    //    src_model_->setData (index, vat);

    //    auto iter_allowance = row_data.find ("宽放率");
    //    assert (iter_allowance != row_data.end () and iter_allowance->is_string ());
    //    std::string allowance_str = *iter_allowance;
    //    auto pos = allowance_str.find_first_of (" ");
    //    allowance_str.erase (pos);
    //    double allowance;
    //    try
    //    {
    //        allowance = boost::lexical_cast<double> (allowance_str.data ());
    //    }
    //    catch (std::exception&)
    //    {
    //        assert (false);
    //    }

    //    index = src_model_->index (i, base_colume + 3);
    //    vat.setValue (allowance);
    //    src_model_->setData (index, vat);

    //    auto iter_opt_type = row_data.find ("操作分类");
    //    assert (iter_opt_type != row_data.end () and iter_opt_type->is_string ());
    //    std::string opt_type = *iter_opt_type;
    //    index = src_model_->index (i, base_colume + 6);
    //    vat.setValue (QString (opt_type.data ()));
    //    src_model_->setData (index, vat);
    //}
}

void form_widget::set_editable(bool b)
{
    //if (b == true)
    //{
    //    ui->table_data->setEditTriggers (QAbstractItemView::AllEditTriggers);
    //    ui->table_des->setEditTriggers (QAbstractItemView::AllEditTriggers);
    //    ui->table_result->setEditTriggers (QAbstractItemView::AllEditTriggers);
    //}
    //else
    //{
    //    ui->table_data->setEditTriggers (QAbstractItemView::NoEditTriggers);
    //    ui->table_des->setEditTriggers (QAbstractItemView::NoEditTriggers);
    //    ui->table_result->setEditTriggers (QAbstractItemView::NoEditTriggers);
    //}
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


void form_widget::set_row(int num)
{
//    emit line_exists (num != 0);

//    src_model_->resize (num);
    set_models();
    set_views ();
    src_model_->setRowCount(num);
//    auto sum = src_model_->get_std_sum ();
//    emit total_time_changed (sum);
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
