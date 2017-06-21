#include "model/json_model.h"
#include "QDebug"
#include <QColor>
#include <QBrush>
#include <QFont>
#include <base/lang/scope.hpp>

json_model::json_model(QObject *parent)
    : QAbstractTableModel(parent)
{
}

using std::optional;

QVariant json_model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (Qt::Horizontal == orientation)
    {
        if (role == Qt::DisplayRole)
        {
            return headers_[section];
        }
        else if (role == Qt::FontRole)
        {
            QFont font;
            font.setBold (true);
            return font;
        }
    }

    return QAbstractTableModel::headerData (section, orientation, role);
}

int json_model::rowCount(const QModelIndex &) const
{
    return data_.size ();
}

int json_model::columnCount(const QModelIndex &) const
{
    return headers_.size ();
}

void json_model::resize(unsigned len)
{
    data_.resize (len);
}

optional<QString> json_model::get_header(const QModelIndex &index) const
{
    if (index.isValid ())
    {
        return headers_[index.column ()];
    }
    else
    {
        return {};
    }
}

QVariant json_model::data(const QModelIndex &index, int role) const
{
    if (role == Qt::BackgroundRole)
    {
        return decoration_data (index);
    }

    if (role != Qt::DisplayRole or !index.isValid ())
    {
        return {};
    }

    auto row = index.row ();
    if (!(row < static_cast<signed>(data_.size ()) and row >= 0))
    {
        return {};
    }

    const auto& row_data = data_[row];

    auto header = get_header (index);
    assert (header);

    auto iter = row_data.find (*header);

    if (iter == row_data.end ())
    {
        return {};
    }
    else
    {
        return iter->second;
    }
}

QVariant json_model::decoration_data(const QModelIndex &index) const
{
    auto op_header = get_header (index);
    assert (op_header);

    if (!edit_col_.contains (*op_header))
    {
        return QBrush (QColor (245, 245, 245));
    }
    else
    {
        return {};
    }
}

bool json_model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid ())
    {
        return false;
    }

    SCOPE_EXIT
    {
        emit dataChanged (this->index (0, 0), this->index (rowCount () - 1, columnCount () - 1));
    };

    if (role == paste_role)
    {
        return paste_data (index, value);
    }
    else if (role == Qt::EditRole)
    {
        auto row = index.row ();
        assert (row >= 0 and row < static_cast<signed>(data_.size ()));

        auto& row_data = data_[row];

        auto op_header = get_header (index);
        assert (op_header);

        row_data[*op_header] = value;

        return true;
    }
    return false;
}

Qt::ItemFlags json_model::flags(const QModelIndex &index) const
{
    auto flag = QAbstractTableModel::flags (index);
    auto op_header = get_header (index);
    assert (op_header);
    if (edit_col_.contains (*op_header))
    {
        flag |= Qt::ItemIsEditable;
    }

    return flag;
}

void json_model::add_editable(const QStringList &list)
{
    for (auto & iter : list)
    {
        edit_col_ << iter;
    }
}

void json_model::remove_editable(const QStringList &list)
{
    for (auto & iter : list)
    {
        edit_col_.removeOne (iter);
    }
}



json json_model::get_json() const
{
    auto j_array = json::array ();
    for (unsigned i = 0; i < data_.size (); i++)
    {
        json row_obj;
        const auto& row_data = data_[i];
        for (const auto & iter_header : headers_)
        {
            auto iter = row_data.find (iter_header);
            if (iter == row_data.end ())
            {
                row_obj[iter_header.toStdString ()] = nullptr;
                continue;
            }

            bool is_ok = true;

            if (!iter->second.isValid ())
            {
                row_obj[iter->first.toStdString ()] = nullptr;
            }
            if (iter->second.type () == QVariant::Bool)
            {
                row_obj[iter->first.toStdString ()] = iter->second.toBool ();
            }
            else if (iter->second.type () == QVariant::String)
            {
                row_obj[iter->first.toStdString ()] = iter->second.toString ().toStdString ();
            }
            else if (iter->second.type () == QVariant::Int)
            {
                row_obj[iter->first.toStdString ()] = iter->second.toInt (&is_ok);
            }
            else if (iter->second.type () == QVariant::UInt)
            {
                row_obj[iter->first.toStdString ()] = iter->second.toUInt (&is_ok);
            }
            else if (iter->second.type () == QVariant::Double)
            {
                row_obj[iter->first.toStdString ()] = iter->second.toDouble (&is_ok);
            }
            else if (iter->second.type () == QVariant::StringList)
            {
                json cell_obj = json::array ();
                auto list = iter->second.toStringList ();
                for (const auto& iter_list : list)
                {
                    cell_obj.push_back (iter_list.toStdString ());
                }
                row_obj[iter->first.toStdString ()] = cell_obj;
            }
            assert (is_ok);
        }
        j_array.push_back (row_obj);
    }

    return j_array;
}

void json_model::load_json(const std::string &json_str)
{
    try
    {
        json json_obj = json::parse (json_str);
        load_json (json_obj);
    }
    catch (std::exception& e)
    {
        qDebug () << e.what ();
        return;
    }
}

void json_model::load_json(const json &json_obj)
{
    if (!json_obj.is_array ())
    {
        return;
    }

    resize (json_obj.size ());

    for (unsigned i = 0; i < json_obj.size (); i++)
    {
        for (int j = 0; j < edit_col_.size (); j++)
        {
            auto col = get_col_from_name (headers_[j]);
            auto load_index = this->index (i, col);

            QVariant cell_data;

            auto j_cell = json_obj[i].find (headers_[j].toStdString ());

            if (j_cell == json_obj[i].end ())
            {
                continue;
            }

            if (j_cell->is_string ())
            {
                std::string str_data = *j_cell;
                cell_data = str_data.data ();
            }
            else if (j_cell->is_number_integer ())
            {
                int n = *j_cell;
                cell_data = n;
            }
            else if (j_cell->is_number_float ())
            {
                double d = *j_cell;
                cell_data = d;
            }
            else if (j_cell->is_array ())
            {
                QStringList list;
                try
                {
                    for (const auto& iter : *j_cell)
                    {
                        std::string str = iter;
                        list << str.data ();
                    }
                }
                catch (std::exception& e)
                {
                    qDebug () << e.what ();
                }
                cell_data = list;
            }

            this->setData (load_index, cell_data, Qt::EditRole);
        }
    }
}

int json_model::get_col_from_name(const QString &name) const
{
    int i = -1;
    auto iter = std::find_if (headers_.begin (), headers_.end (), [&] (const auto& it) { i++; return it == name;});

    if (iter == headers_.end ())
    {
        return -1;
    }
    else
    {
        return i;
    }
}

QVariant json_model::get_value_by_key(int row, const QString &key, int role) const
{
    assert (row >= 0 and row < rowCount ());
    auto col = get_col_from_name (key);
    assert (col != -1);

    auto index = this->index (row, col);
    return index.data (role);
}

void json_model::clear()
{
    data_.clear ();
}

bool json_model::paste_data(const QModelIndex &index, const QVariant &value)
{
    auto op_header = get_header (index);
    assert (op_header);

    if (paste_col_.contains (*op_header))
    {
        return setData (index, value, Qt::EditRole);
    }
    else
    {
        return false;
    }
}

























