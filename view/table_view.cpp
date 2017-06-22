#include "table_view.h"
#include <QDebug>
#include <QClipboard>
#include <string.h>
#include <QMouseEvent>
#include <boost/scope_exit.hpp>
#include <QAbstractItemView>
#include <QString>
#include <QMessageBox>
#include <memory>
#include <QMenu>
#include <QDir>
#include <QApplication>
#include <QPainter>

table_view::table_view(QWidget* parent)
    :QTableView (parent)
{
    auto scroll = this->scrollBarWidgets (Qt::AlignRight);
}

void table_view::mouseReleaseEvent (QMouseEvent *event)
{
    if (this->model () == nullptr)
    {
        return;
    }
    this->setSelectionMode (QAbstractItemView::SingleSelection);
    QTableView::mouseReleaseEvent (event);

    current_click_index_ = this->indexAt (event->pos ());

    if (event->button () == Qt::RightButton)
    {
        auto menu = std::make_unique<QMenu> ();

        auto action_cut = menu->addAction ("剪切");
        auto action_copy = menu->addAction ("复制");
        auto action_paste = menu->addAction ("粘贴");
        auto action_del = menu->addAction ("删除");

        connect (action_copy, &QAction::triggered, [this] {on_copy_del (OPERATION_COPY);});
        connect (action_cut, &QAction::triggered, [this] {on_copy_del (OPERATION_DEL | OPERATION_COPY);});
        connect (action_del, &QAction::triggered, [this] { on_copy_del (OPERATION_DEL);});
        connect (action_paste, &QAction::triggered, this, &table_view::on_paste);
        menu->exec (QCursor::pos ());
    }

    current_click_index_ = {};
}

void table_view::mousePressEvent(QMouseEvent *event)
{
    emit mouse_pressed ();
    if (event->button () == Qt::LeftButton)
    {
        this->clearSelection ();
    }
    this->setSelectionMode (QAbstractItemView::ExtendedSelection);
    return QTableView::mousePressEvent (event);
}

table_view::~table_view()
{

}



void table_view::on_copy_del(int flag)
{
    if (this->model() == nullptr)
    {
        return;
    }

    if (flag == OPERATION_DEL)
    {
        auto ret = QMessageBox::question (this, "删除", "是否真的要删除?");
        if (ret != QMessageBox::Yes)
        {
            return;
        }
    }

    auto board = QApplication::clipboard ();

    int min_row, min_col, max_row, max_col;
    bool is_ok;
    std::tie (min_row, min_col, max_row, max_col) = get_rect (&is_ok);
    if (!is_ok)
    {
        QMessageBox::information (this, "表格操作", "当前状态下无法操作");
        return;
    }

    QString clip_data;
    for (int i = min_row; i <= max_row; i ++)
    {
        for (int j = min_col; j <= max_col; j++)
        {
            auto model = this->model (); assert (model);
            auto index = model->index (i, j);

            if (flag & OPERATION_COPY)
            {
                clip_data += index.data ().toString ();

                if (j != max_col)
                {
                    clip_data += "\t";
                }
            }

            if (flag & OPERATION_DEL)
            {
                model->setData (index, {}, paste_role);
            }
        }
        if (i != max_row)
        {
            clip_data += "\n";
        }
    }
    board->setText (clip_data);
}

void table_view::on_paste()
{
    if (this->model () == nullptr)
    {
        return ;
    }

    BOOST_SCOPE_EXIT_ALL (&)
    {
        current_click_index_ = {};
    };

    if (!current_click_index_.isValid ())
    {
        int min_row, min_col;
        bool is_ok;
        std::tie (min_row, min_col, std::ignore, std::ignore) = get_rect (&is_ok);
        if (!is_ok)
        {
            QMessageBox::information (this, "粘贴", "无粘贴目的地");
            return;
        }

        auto data_model = this->model ();
        current_click_index_ = data_model->index (min_row, min_col);
    }

    auto data = get_clip_structure ();

    if (data.empty ())
    {
        return;
    }

    int row = current_click_index_.row (); assert (row >= 0);
    int col = current_click_index_.column (); assert (col >= 0);

    auto model = this->model (); assert (model);

    for (unsigned i = 0; i < data.size (); i ++)
    {
        for (unsigned j = 0; j < data[i].size (); j ++)
        {
            auto current_row = row + i;
            auto current_col = col + j;
            if (static_cast<int> (current_row) >= model->rowCount () or static_cast<int>(current_col) >= model->columnCount ())
            {
                continue;
            }

            auto index = model->index (current_row, current_col);
            model->setData (index, data[i][j].data (), paste_role);
        }
    }
}

void table_view::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers () & Qt::ControlModifier)
    {
        if (event->key () == Qt::Key_C)
        {
            table_view::on_copy_del(OPERATION_COPY);
            return;
        }
        else if (event->key () == Qt::Key_X)
        {
            table_view::on_copy_del(OPERATION_COPY | OPERATION_DEL);
            return;
        }
    }
    QTableView::keyPressEvent (event);
}

std::vector<std::vector<std::string>> table_view::get_clip_structure()
{
    auto board = QApplication::clipboard ();
    auto clip = board->text ().toStdString ();
    if (clip.empty ())
    {
        return {};
    }

    if (clip.back () == '\n')
    {
        clip.pop_back ();
    }

    auto lambda_split = [](const std::string& str, char delim)
    {
        std::vector<std::string> splitted;

        int split_begin = 0;
        for (unsigned i = 0; i < str.size (); i++)
        {
            if (str[i] == delim)
            {
                splitted.emplace_back (str.substr (split_begin, i - split_begin));
                split_begin = i + 1;
            }
        }
        splitted.emplace_back (str.substr (split_begin));

        return splitted;
    };

    auto lines = lambda_split (clip, '\n');

    std::vector<std::vector<std::string>> table_info;
    for_each (lines.begin (), lines.end (), [&] (auto&& iter) {table_info.emplace_back (lambda_split (iter, '\t'));});

    return table_info;
}

std::tuple<int, int, int, int> table_view::get_rect(bool* ok)
{
    if (ok) *ok = true;
    auto ret = std::make_tuple (-1, -1, -1, -1);
    auto list = this->selectionModel ()->selectedIndexes ();

    if (list.empty ())
    {
        if (ok) *ok = false;
        return ret;
    }

    int min_row = 9999, max_row = 0, min_col = 9999, max_col = 0;
    for (const auto& iter : list)
    {
        if (iter.row () < min_row)
        {
            min_row = iter.row ();
        }
        if (iter.row () > max_row)
        {
            max_row = iter.row ();
        }

        if (iter.column () < min_col)
        {
            min_col = iter.column ();
        }

        if (iter.column () > max_col)
        {
            max_col = iter.column ();
        }
    }
    return std::make_tuple (min_row, min_col, max_row, max_col);
}
