﻿#include "form_widget.h"
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

    views_ = {ui->table_des, ui->table_data, ui->table_result};
    set_scrolls ();
    initConn();
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

void form_widget::table_clicked(const QModelIndex &)
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

QVariant form_widget::taskData()
{
    QVariantList taskList;
    const auto col = src_model_->getHorizontalHeaderCol("作业内容");
    for(int row = 0; row < src_model_->rowCount(); row++)
    {
        const auto index = src_model_->index(row, col);
        const auto var = index.data();
        taskList.push_back(var);
    }

    return taskList;

}

QVariant form_widget::observationTime()
{
    QVariantList observationList;
    const auto startCol = src_model_->getHorizontalHeaderCol("1T");
    const auto endCol = src_model_->getHorizontalHeaderCol("10R");

    for(int col = startCol; col <= endCol; col += 2)
    {
        QVariantList TRList;
        for(int row = 0; row < src_model_->rowCount(); row++)
        {
            const auto index = src_model_->index(row, col);
            const auto var = index.data();
            QVariantMap map;
            bool isOk = false;
            if(var.isNull())
            {
                map["T"] = static_cast<double>(0);
            }
            else
            {
                map["T"] = var.toDouble(&isOk);
                assert(isOk);
            }

            const auto indexR = src_model_->index(row, col + 1);
            const auto varR = indexR.data();
            bool isOkR = false;
            if(varR.isNull())
            {
                map["R"] = static_cast<double>(0);
            }
            else
            {
                map["R"] = varR.toDouble(&isOkR);
                assert(isOkR);
            }

            TRList.push_back(map);
        }
        observationList.push_back(TRList);
    }

    return observationList;
}

QVariant form_widget::resultData()
{
    QVariantList resultList;
    const auto averageTimeCol = src_model_->getHorizontalHeaderCol("平均时间");
    const auto comparsionCol = src_model_->getHorizontalHeaderCol("评比系数");
    const auto basicTimeCol = src_model_->getHorizontalHeaderCol("基本时间");
    const auto rateCol = src_model_->getHorizontalHeaderCol("宽放率");
    const auto stdTimeCol = src_model_->getHorizontalHeaderCol("标准工时");
    const auto appreciationCol = src_model_->getHorizontalHeaderCol("增值/非增值");
    const auto typeCol = src_model_->getHorizontalHeaderCol("操作类型");

    bool isOk = false;

    for(int row = 0; row < src_model_->rowCount(); row++)
    {
        QVariantMap map;
        {
            const auto index = src_model_->index(row, averageTimeCol);
            const auto var = index.data();
            if(!var.isNull())
            {
                map["平均时间"] = var.toDouble(&isOk); assert(isOk);
            }
            else
            {
                map["平均时间"] = static_cast<double>(0);
            }
        }

        {
            const auto index = src_model_->index(row, comparsionCol);
            const auto var = index.data();

            map["评比系数"] = var.toDouble(&isOk); assert(isOk);
        }

        {
            const auto index = src_model_->index(row, basicTimeCol);
            const auto var = index.data();
            if(!var.isNull())
            {
                map["基本时间"] = var.toDouble(&isOk); assert(isOk);
            }
            else
            {
                map["基本时间"] = static_cast<double>(0);
            }
        }

        {
            const auto index = src_model_->index(row, rateCol);
            const auto var = index.data();

            map["宽放率"] = var.toString();
        }

        {
            const auto index = src_model_->index(row, stdTimeCol);
            const auto var = index.data();
            if(!var.isNull())
            {
                map["标准时间"] = var.toDouble(&isOk); assert(isOk);
            }
            else
            {
                map["标准时间"] = static_cast<double>(0);
            }
        }

        {
            const auto index = src_model_->index(row, appreciationCol);
            const auto var = index.data();

            map["增值/非增值"] = var.toString();
        }

        {
            const auto index = src_model_->index(row, typeCol);
            const auto var = index.data();

            map["操作分类"] = var.toString();
        }

        resultList.push_back(map);
    }

    return resultList;
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

void form_widget::initConn()
{
    connect(src_model_.get(), &QStandardItemModel::rowsInserted, this, &form_widget::initTable);
    connect(src_model_.get(), &QStandardItemModel::rowsRemoved, this, &form_widget::initTable);

    connect (src_model_.get (), &video_form_model::dataChanged,
             this, &form_widget::data_changed);

    connect (src_model_.get (), &video_form_model::dataChanged, [this]{
        auto sum = src_model_->getStdSum ();
        emit total_time_changed (sum);
    });

    for (const auto & iter : views_)
    {
        connect (iter, &QTableView::pressed, this, &form_widget::table_clicked);
    }
}

void form_widget::initTable()
{
    const auto rows = src_model_->rowCount();
    const auto cols = src_model_->columnCount();
    {
        for(int row = 0; row < rows; row++)
        {
            for(int col = 0; col < cols; col++)
            {
                src_model_->setItem(row, col, new QStandardItem);
                src_model_->item(row, col)->setTextAlignment(Qt::AlignCenter);
            }
        }
    }


    {
        const auto col = src_model_->getHorizontalHeaderCol("评比系数");
        for(int row = 0; row < rows; row++)
        {
            const auto index = src_model_->index(row, col);
            src_model_->setData(index, double{1});
        }
    }

    {
        const auto col = src_model_->getHorizontalHeaderCol("宽放率");
        for(int row = 0; row < rows; row++)
        {
            const auto index = src_model_->index(row, col);
            src_model_->setData(index, "0.00%");
        }
    }

    {
        const auto col = src_model_->getHorizontalHeaderCol("操作类型");
        for(int row = 0; row < rows; row++)
        {
            const auto index = src_model_->index(row, col);
            src_model_->setData(index, "加工");
        }
    }
}

void form_widget::setTable()
{
    model_des_->setSourceModel (src_model_.get ());
    model_des_->set_range (0, 2);
    ui->table_des->setModel (model_des_.get ());
    ui->table_des->setItemDelegate (des_delegate_.get ());

    model_data_->setSourceModel (src_model_.get ());
    model_data_->set_range (2, 2 + VideoFormModel::dataCol_);
    ui->table_data->setModel (model_data_.get ());
    ui->table_data->setItemDelegate (des_delegate_.get ());

    model_result_->setSourceModel (src_model_.get ());
    model_result_->set_range (2 + VideoFormModel::dataCol_, src_model_->columnCount ());
    ui->table_result->setModel (model_result_.get ());
    ui->table_result->setItemDelegate (des_delegate_.get ());

    ui->table_des->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->table_des->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);

    ui->table_data->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->table_data->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);

    ui->table_result->verticalHeader()->setSectionResizeMode (QHeaderView::Fixed);
    ui->table_result->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);
}

//void form_widget::clear()
//{
//    ui->table_data->setModel (nullptr);
//    model_data_->setSourceModel (nullptr);
//    ui->table_des->setModel (nullptr);
//    model_des_->setSourceModel (nullptr);
//    ui->table_result->setModel (nullptr);
//    model_result_->setSourceModel (nullptr);

//    src_model_->clear ();

//    model_data_->setSourceModel (src_model_.get ());
//    ui->table_data->setModel (model_data_.get ());

//    model_des_->setSourceModel (src_model_.get ());
//    ui->table_des->setModel (model_des_.get ());

//    model_result_->setSourceModel (src_model_.get ());
//    ui->table_result->setModel (model_result_.get ());
//}


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

void form_widget::loadTask(const QVariant &task)
{
    if(task.isNull())
    {
        return;
    }

    const auto list = task.toList();

    if(list.size() == 0)
    {
        return;
    }

    const auto rows = list.size();
    const auto col = src_model_->getHorizontalHeaderCol("作业内容");

    for(int row = 0; row < rows; row++)
    {
        const auto index = src_model_->index(row, col);
        src_model_->setData(index, list.at(row));
    }
}

void form_widget::loadData(const QVariant &data)
{
    if(data.isNull())
    {
        return;
    }

    const auto list = data.toList();

    if(list.size() == 0)
    {
        return;
    }

    for(int i = 0; i < list.size(); i++)
    {

        const auto groupList = list.at(i).toList();

        for(int j = 0; j < groupList.size(); j++)
        {
            bool isOk = false;
            const auto startCol = src_model_->getHorizontalHeaderCol("1T");
            const auto index = src_model_->index(j, startCol + 2 * j);
            const auto data = groupList.at(j).toMap()["T"].toDouble(&isOk);
            if(!isOk)
            {
                continue;
            }

            if(data > 0)
            {
                src_model_->setData(index, data);
            }
            else
            {
                src_model_->setData(index, QVariant{});
            }
        }
    }
}

void form_widget::loadResult(const QVariant &result)
{
    if(result.isNull())
    {
        return;
    }

    const auto list = result.toList();

    if(list.size() == 0)
    {
        return;
    }

    const auto comparsionCol = src_model_->getHorizontalHeaderCol("评比系数");
    const auto rateCol = src_model_->getHorizontalHeaderCol("宽放率");
    const auto typeCol = src_model_->getHorizontalHeaderCol("操作类型");

    for(int row = 0; row < list.size(); row++)
    {
        const auto comparsionIndex = src_model_->index(row, comparsionCol);
        const auto rateIndex = src_model_->index(row, rateCol);
        const auto typeIndex = src_model_->index(row, typeCol);

        const auto comparsion = list.at(row).toMap()["评比系数"];
        const auto rate = list.at(row).toMap()["宽放率"];
        const auto type = list.at(row).toMap()["操作类型"];

        src_model_->setData(comparsionIndex, comparsion);
        src_model_->setData(rateIndex, rate);
        src_model_->setData(typeIndex, type);
    }

}

std::optional<action_ratio> form_widget::operation_ratio() const
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

QVariant form_widget::dump()
{
    QVariantMap data;
    QVariant task = taskData();
    QVariant obs = observationTime();
    QVariant result = resultData();

    data["作业内容"] = task;
    data["观测时间"] = obs;
    data["结果"] = result;

    return data;
}


void form_widget::set_row(int num)
{
    src_model_->setRowCount(num);
    setTable();
    const auto sum = src_model_->getStdSum();
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
