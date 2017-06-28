#include "video_form_model.h"
#include <QDebug>
#include <QFont>
#include <boost/algorithm/string.hpp>
#include <boost/scope_exit.hpp>
#include <string>
#include <math.h>

const char * const video_form_model::cols[] =
{
    "平均时间",
    "评比系数",
    "基本时间",
    "宽放率",
    "标准工时",
    "增值/非增值",
    "操作类型"
};
using namespace std;

video_form_model::video_form_model(QObject *parent)
    : json_model (parent)
{
    static_assert (data_col % 2 == 0, "data_col is not an even number");

    headers_ << "编号" << "作业内容";

    edit_col_ << "作业内容";

    for (int i = 0; i < data_col; i ++)
    {
        auto cycle_header = to_string (i / 2 + 1);
        if (i % 2 == 0)
        {
            cycle_header += "T";
            edit_col_ << cycle_header.data ();
            paste_col_ << cycle_header.data ();
        }
        else
        {
            cycle_header += "R";
        }
        headers_ << cycle_header.data ();
    }

    headers_ << "平均时间" << "评比系数" << "基本时间" << "宽放率" << "标准工时" << "增值/非增值" << "操作类型";
    edit_col_ << "评比系数" << "宽放率" << "操作类型";
    paste_col_ << "评比系数" << "作业内容" << "宽放率";

    resize_helper (0);
}

QString video_form_model::get_std_sum()
{
    double sum = 0;
    auto col = 2 + data_col + 5 -1;
    for (int row = 0; row < rowCount (); ++row)
    {
        auto pos = this->index (row, col);
        auto vat = data (pos);
        if (vat.isNull ())
        {
            continue;
        }
        else
        {
            auto stdtime = vat.toDouble ();
            sum += stdtime;
        }
    }
    return QString::number (sum, 'f', 2);
}


QVariant video_form_model::get_average(int row) const
{
    int valid_count = 0;
    double total_time = 0;
    for (unsigned i = 0; i < data_col; i += 2)
    {
        auto data_index = this->index (row, 2 + i + 1);
        auto op_inc = get_increment_data (data_index);

        if (op_inc)
        {
            valid_count ++;
            total_time += op_inc.value ();
        }
    }

    if (valid_count != 0)
    {
        return QString::number (static_cast<double> (total_time / valid_count), 'f', 2);
    }

    return {};
}


QVariant video_form_model::data(const QModelIndex &index, int role) const
{
    auto op_header = get_header (index);

    if (!index.isValid ()) return {};

    if (index.column () == 0 and role == Qt::TextAlignmentRole)
    {
        return Qt::AlignCenter;
    }

    if (index.column () >= 2 + data_col and role == Qt::DisplayRole)
    {
        return get_result_table (index);
    }

    if (index.column () >= 2 and index.column () < 2 + data_col)
    {
        if (Qt::DisplayRole == role)
        {
            return get_data_table (index);
        }
    }

    return json_model::data (index, role);
}

bool video_form_model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
        if (index.column () == 0)
        {
            return false;
        }

        if (index.column () >= 2 and index.column () < 2 + data_col)
        {
            return set_data_table (index, value);
        }

        if (index.column () >= 2 + data_col)
        {
            return set_result_table (index, value);
        }
    }


    return json_model::setData (index, value, role);
}

Qt::ItemFlags video_form_model::flags(const QModelIndex &index) const noexcept
{
    return json_model::flags (index) | Qt::ItemIsEditable;
}

void video_form_model::resize(unsigned len)
{
    assert (len <= max_rows);
    resize_helper (static_cast<int> (len));
}

void video_form_model::clear()
{
    this->resize (0);
    this->resize (rowCount ());
}

optional<action_ratio> video_form_model::operation_ratio() const
{
    std::array<qreal, 4> arr {{0, 0, 0, 0}};

    qreal & processing = arr.at (0);
    qreal & checking = arr.at (1);
    qreal & moving = arr.at (2);
    qreal & waiting = arr.at (3);

    for (int i = 0; i < this->rowCount (); i ++)
    {
        bool b;
        const auto time = get_value_by_key (i, "标准工时").toDouble (&b);

        if (!b)
        {
            continue;
        }

        const auto type = get_value_by_key (i, "操作类型").toString ();
        if (type == "加工")
        {
            processing += time;
        }
        else if (type == "搬运")
        {
            moving += time;
        }
        else if (type == "检查")
        {
            checking += time;
        }
        else if (type == "等待")
        {
            waiting += time;
        }
        else
        {
            assert (false);
        }
    }

    const auto total = accumulate (begin (arr), end (arr), qreal {0});
    if (total < 0.00001)
    {
        return {};
    }
    const auto ratio = 100.0 / total;

    for (auto & it : arr)
    {
        it *= ratio;
    }

    action_ratio ret;

    ret.processing = processing;
    ret.checking = checking;
    ret.moving = moving;
    ret.waiting = waiting;

    return ret;
}

std::optional<overall_stats> video_form_model::operation_stats() const
{
    bool b;
    std::vector<qreal> cts;
    cts.reserve (max_round);

    for (int i = 0; i < max_round; i ++)
    {
        optional<qreal> total = 0.0;
        for (int j = 0; j < this->rowCount (); j ++)
        {
            const auto time = get_value_by_key (j, QString::number (i + 1) + "R").toDouble (&b);
            if (!b)
            {
                total = {};
                break;
            }
            total.value () += time;
        }

        if (total)
        {
            cts.emplace_back (total.value ());
        }
    }

    if (cts.empty ())
    {
        return {};
    }

    const auto max_val = *(max_element (begin (cts), end (cts)));
    const auto min_val = *(min_element (begin (cts), end (cts)));
    const auto average = accumulate (begin (cts), end (cts), qreal {0}) / cts.size ();

    const auto square_total = accumulate (begin (cts), end (cts) , qreal {0}, [&] (qreal tmp, qreal it)
    {  return tmp + (average - it) * (average - it); });

    const auto deviation = ::sqrt (square_total / cts.size ());

    overall_stats ret;
    ret.max_val = max_val;
    ret.min_val = min_val;
    ret.average = average;
    ret.deviation = deviation;

    return ::move (ret);
}

std::vector<qreal> video_form_model::cycle_times() const
{
    vector<qreal> ret;
    for (int i = 2; i < 2 + max_round * 2; i += 2)
    {
        optional<qreal> sum = 0.0;
        for (int j = 0; j < rowCount (); j ++)
        {
            const auto var = data (index (j, i + 1));
            if (var.isNull ())
            {
                sum = {};
                break;
            }
            sum.value () += var.toDouble ();
        }
        if (not sum)
        {
            break;
        }
        ret.emplace_back (sum.value ());
    }

    return ret;
}

QVariant video_form_model::get_result_table(const QModelIndex &index) const
{
    auto op_header = get_header (index);
    assert (op_header);

    if (*op_header == "平均时间")
    {
        return get_average (index.row ());
    }

    if (*op_header == "评比系数")
    {
        auto var = json_model::data (index, Qt::DisplayRole);
        return var.type () == QVariant::Double ? var : double {1};
    }

    if (*op_header == "基本时间")
    {
        return get_base_time (index);
    }

    if (*op_header == "宽放率")
    {
        return QString::number (get_allowance (index) * 100, 'f', 2) + " %";
    }

    if (*op_header == "标准工时")
    {
        return get_std_time (index);
    }

    if (*op_header == "增值/非增值")
    {
        auto content = get_result_info ("操作类型", index.row ());
        auto content_text = content.toString ();
        if (content_text == "加工")
        {
            return "增值";
        }
        else if (content_text == "检查" or
                 content_text == "搬运" or
                 content_text == "等待")
        {
            return "非增值";
        }
        assert (false);
    }

    return json_model::data (index, Qt::DisplayRole);
}

optional<double> video_form_model::get_increment_data(const QModelIndex &index) const
{
    bool is_ok;
    auto last_total_time = get_last_total_time (index.row (), index.column ());

    auto t_index = this->index (index.row (), index.column () - 1);
    assert (t_index.isValid ());
    auto variant = t_index.data (Qt::DisplayRole);
    auto current_total_time = variant.toDouble (&is_ok);

    if (is_ok)
    {
        auto result = current_total_time - last_total_time;
        return result;
    }

    return {};
}

QVariant video_form_model::get_data_table(const QModelIndex &index) const
{
    if (index.column () % 2 == 1)
    {
        auto op_inc = get_increment_data (index);

        if (op_inc)
        {
            return *op_inc;
        }
        else
        {
            return {};
        }
    }
    else
    {
        bool is_ok;
        auto variant = json_model::data (index, Qt::DisplayRole);
        double t_data = variant.toDouble (&is_ok);
        if (is_ok)
        {
            return QString::number (t_data, 'f', 2);
        }
        return {};

    }

}

bool video_form_model::set_data_table(const QModelIndex &index, const QVariant& value)
{

    assert (value.type () == QVariant::Double or !value.isValid ());

    if (index.column () % 2 == 1)
    {
        auto real_index = this->index (index.row (), index.column () - 1);
        return json_model::setData (real_index, value, Qt::EditRole);
    }
    else
    {
        return json_model::setData (index, value, Qt::EditRole);
    }

    return false;
}

bool video_form_model::set_result_table(const QModelIndex &index, const QVariant &value)
{
    //auto header = this->index (0, index.column ()).data (Qt::DisplayRole).toString ();
    auto op_header = get_header (index);
    auto value_str = value.toString ();
    bool is_ok;

    if (*op_header == "操作类型")
    {
        if (!value.isValid ())
        {
            return json_model::setData (index, "加工", Qt::EditRole);
        }

        auto arr = {"加工", "检查", "搬运", "等待"};

        for (auto && name : arr)
        {
            auto value_std_str = value_str.toStdString ();
            boost::trim (value_std_str);
            if (value_std_str == name)
            {
                return json_model::setData (index, value_std_str.data (), Qt::EditRole);
            }
        }
        return false;
    }
    else if (*op_header == "宽放率")
    {
        auto allowance = value_str.toDouble (&is_ok);

        if (is_ok)
        {
            return json_model::setData (index, allowance / 100, Qt::EditRole);
        }
        else
        {
            return false;
        }
    }

    return json_model::setData (index, value, Qt::EditRole);
}

double video_form_model::get_last_total_time(int row, int col) const
{
    assert (col % 2 == 1);

    int last_row;
    int last_col;
    if (col == 3 and row == 0)
    {
        return 0;
    }

    if (0 == row)
    {
        last_col = col - 3;
        last_row = rowCount () - 1;
    }
    else
    {
        last_col = col - 1;
        last_row = row - 1;
    }

    auto last_index = this->index (last_row, last_col); assert (last_index.isValid ());

    return last_index.data (Qt::DisplayRole).toDouble ();
}

QVariant video_form_model::get_std_time(const QModelIndex &index) const
{
    auto base_time = get_result_info ("基本时间", index.row ());
    auto allowance_index = get_result_index ("宽放率", index.row ());
    auto allowance_val = json_model::data (allowance_index);

    if (base_time.type () != QVariant::Double)
    {
        return {};
    }
    else
    {
        return (allowance_val.toDouble () + 1) * base_time.toDouble ();
    }
}


QVariant video_form_model::get_base_time(const QModelIndex &index) const
{
    int row = index.row ();
    auto average = get_result_info ("平均时间", row);
    auto rate = get_result_info ("评比系数", row);

    double db_rate = rate.type () == QVariant::Double ? rate.toDouble () : 1;

    bool is_ok;
    double db_average = average.toDouble (&is_ok);
    if (is_ok)
    {
        return db_average * db_rate;
    }
    else
    {
        return {};
    }
}

QVariant video_form_model::get_result_info(const string& key, int row) const
{
    auto index = get_result_index (key, row);
    return index.data ();
}

QModelIndex video_form_model::get_result_index(const std::string &key, int row) const
{
    auto col = get_col_from_name (key.data ());
    return this->index (row, col);
}

double video_form_model::get_allowance(const QModelIndex &index) const
{
    auto var = json_model::data (index, Qt::DisplayRole);

    if (var.type () == QVariant::Double)
    {
        return var.toDouble ();
    }
    else
    {
        return 0;
    }
}


void video_form_model::resize_helper(int len)
{
    json_model::resize (len);

    for (int i = 0; i < len; i++)
    {
        auto index_content = get_result_index ("操作类型", i);
        auto current_content = json_model::data (index_content, Qt::DisplayRole);

        if (current_content.type () != QVariant::String)
        {
            json_model::setData (index_content, "加工", Qt::EditRole);
        }

        auto index_var = index (i, 0);
        json_model::setData (index_var, i + 1, Qt::EditRole);
    }
}

bool video_form_model::paste_data(const QModelIndex &index, const QVariant &value)
{
    if (index.column () > static_cast<int> (2 + data_col))
    {
        return paste_result_table (index, value);
    }

    if (index.column () >= 2 and index.column () < static_cast<int> (2 + data_col))
    {
        return paste_data_table (index, value);
    }

    if (index.column () == 1)
    {
        setData (index, value, Qt::EditRole);
    }

    return false;
}

bool video_form_model::paste_result_table(const QModelIndex &index, const QVariant &value)
{
    auto op_header = get_header (index);
    assert (op_header);
    auto value_str = value.toString ();
    bool is_ok;

    auto data_refined = value;

    if (*op_header == "评比系数")
    {
        if (!value.isValid ())
        {
            data_refined = double {1};
        }
        else
        {
            data_refined = value_str.toDouble (&is_ok);
            if (!is_ok)
            {
                return false;
            }
        }

    }

    if (*op_header == "宽放率")
    {
        if (!value.isValid ())
        {
            data_refined = double {0};
        }
        else
        {
            auto list = value_str.split (" %");

            if (list.empty ())
            {
                return false;
            }
            else
            {
                data_refined = list[0];
            }

        }
    }
    return json_model::paste_data (index, data_refined);

}

bool video_form_model::paste_data_table(const QModelIndex &index, const QVariant &value)
{
    if (index.column () % 2 == 0)
    {
        if (!value.isValid ())
        {
            return json_model::paste_data(index, value);
        }
        else
        {
            bool is_ok;
            auto double_value = value.toDouble (&is_ok);
            if (is_ok and double_value > 0)
            {
                return json_model::paste_data (index, double_value);
            }

        }
    }

    return false;
}





































