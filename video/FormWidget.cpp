#include "FormWidget.h"
#include "ui_FormWidget.h"
#include <QMessageBox>
#include <memory>
#include <QDebug>
#include <QScrollBar>
#include <QDir>
#include <boost/lexical_cast.hpp>
#include <QHeaderView>
#include <QPainter>

using namespace std;

FormWidget::FormWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormWidget)
{
    ui->setupUi(this);

    views_ = {ui->table_des, ui->table_data, ui->table_result};
    set_scrolls ();
    initConn();
}

FormWidget::~FormWidget()
{
    delete ui;
}

void FormWidget::mark(long long time_val)
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

optional<QModelIndex> FormWidget::get_next_index(const QModelIndex & index) const
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

    if(row == model->rowCount() - 1 and col == model_data_->columnCount() - 1)
    {
        return {};
    }

    if(col == model_data_->columnCount() - 1)
    {
        next_col = 0;
        next_row = row + 1;
    }
    else
    {
        next_col = col + 1;
        next_row = row;
    }

    return model->index (next_row, next_col);
}

void FormWidget::table_clicked(const QModelIndex &)
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

QVariant FormWidget::taskData()
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

QVariant FormWidget::observationTime()
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

QVariant FormWidget::resultData()
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

            map["操作类型"] = var.toString();
        }

        resultList.push_back(map);
    }

    return resultList;
}

void FormWidget::set_scrolls()
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

void FormWidget::initConn()
{
    connect(src_model_.get(), &QStandardItemModel::rowsInserted, this, &FormWidget::initTable);
    connect(src_model_.get(), &QStandardItemModel::rowsRemoved, this, &FormWidget::initTable);

    connect (src_model_.get (), &video_form_model::dataChanged,
             this, &FormWidget::data_changed);

    connect (src_model_.get (), &video_form_model::dataChanged, [this]{
        auto sum = src_model_->getStdSum ();
        emit total_time_changed (sum);
    });

    for (const auto & iter : views_)
    {
        connect (iter, &QTableView::pressed, this, &FormWidget::table_clicked);
    }
}

void FormWidget::initTable()
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

void FormWidget::setTable()
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

void FormWidget::loadTask(const QVariant &task)
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

void FormWidget::loadData(const QVariant &data)
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
            const auto index = src_model_->index(j, startCol + 2 * i);
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

void FormWidget::loadResult(const QVariant &result)
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

std::optional<action_ratio> FormWidget::operation_ratio() const
{
    return src_model_->operation_ratio ();
}

std::optional<overall_stats> FormWidget::operation_stats() const
{
    return src_model_->operation_stats ();
}

std::vector<qreal> FormWidget::cycle_times() const
{
    return src_model_->cycle_times ();
}

QVariant FormWidget::dump()
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


void FormWidget::set_row(int num)
{
    src_model_->setRowCount(num);
    setTable();
    const auto sum = src_model_->getStdSum();
    emit total_time_changed (sum);
}

void FormWidget::add_row(int num)
{
    auto current_row = row ();
    set_row (num+current_row);
}

void FormWidget::reduce_row(int num)
{
    auto current_row = row ();
    set_row (num-current_row);
}

bool FormWidget::task_content_check()
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

void FormWidget::scroll_to_index(const QModelIndex& index)
{
    assert (index.isValid ());
    ui->table_data->scrollTo (index);
}

void FormWidget::on_paste()
{
    if (current_view_ != nullptr)
    {
        current_view_->on_paste ();
    }
}

void FormWidget::on_copy()
{
    if (current_view_ != nullptr)
    {
        current_view_->on_copy_del(table_view::OPERATION_COPY);
    }
}

void FormWidget::on_cut()
{
    if (current_view_ != nullptr)
    {
        current_view_->on_copy_del(table_view::OPERATION_COPY | table_view::OPERATION_DEL);
    }
}

void FormWidget::on_del()
{
    if (current_view_ != nullptr)
    {
        current_view_->on_copy_del (table_view::OPERATION_DEL);
    }
}

int FormWidget::row()
{
    return src_model_->rowCount ();
}
