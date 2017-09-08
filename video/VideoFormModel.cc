#include "VideoFormModel.h"
#include <boost/range/adaptor/indexed.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/counting_range.hpp>
#include <base/lang/range.hpp>
#include <base/lang/scope.hpp>


using namespace std;
using namespace boost::adaptors;
using namespace boost;

void VideoFormModel::init()
{
    static_assert (dataCol_ % 2 == 0, "data_col is not an even number");

    horizontalHeaderColumns_ << "编号" << "作业内容";
    originDataColumns_ << "作业内容";

    for(int i = 0; i < dataCol_; i ++)
    {
        auto cycleHeader = to_string (i / 2 + 1);
        if (i % 2 == 0)
        {
            cycleHeader += "T";
            originDataColumns_ << cycleHeader.data();
        }
        else
        {
            cycleHeader += "R";
            originDataColumns_ << cycleHeader.data();
        }
        horizontalHeaderColumns_ << cycleHeader.data();
    }

    horizontalHeaderColumns_ << "测量时间" << "评比系数" << "基本时间" << "宽放率" << "标准工时" << "增值/非增值" << "操作类型";
    originDataColumns_ << "评比系数" << "宽放率" << "操作类型";
    setHorizontalHeaderLabels(horizontalHeaderColumns_);
}

QString VideoFormModel::getStdSum()
{
    double sum = 0;
    const auto col = getHorizontalHeaderCol("标准工时");
    for(int row = 0; row < rowCount(); row++)
    {
        const auto index = this->index(row, col);
        const auto var = data(index);
        if(var.isNull())
        {
            continue;
        }
        else
        {
            const auto stdTime = var.toDouble();
            sum += stdTime;
        }
    }
    return QString::number(sum, 'f', 2);
}

int VideoFormModel::getHorizontalHeaderCol(const QString &name) const
{
    for(int i = 0; i < horizontalHeaderColumns_.size(); i++)
    {
        if(horizontalHeaderColumns_.at(i) == name)
        {
            return i;
        }
    }

    return -1;
}

QVariant VideoFormModel::getValueByKey(int row, const QString &key, int role) const
{
    assert (row >= 0 and row < rowCount ());
    auto col = getHorizontalHeaderCol(key);
    assert (col != -1);

    auto index = this->index (row, col);
    return index.data (role);
}

QString VideoFormModel::findHorizontalHeader(const QStandardItemModel *model, const QModelIndex &index)
{
    return model->headerData(index.column(), Qt::Horizontal).toString();
}

bool VideoFormModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    SCOPE_EXIT { emit dataChanged (this->index (0, 0), this->index (rowCount () - 1, columnCount () - 1)); };
    const auto header = findHorizontalHeader(this, index);
    if(originDataColumns_.contains(header))
    {
        if(header == "宽放率")
        {
            bool isOk = false;
            const auto value_str = value.toString();
            const auto allowance = value_str.toDouble(&isOk);
            if(isOk)
            {
                return QStandardItemModel::setData(index, allowance / 100, role);
            }
        }

        return QStandardItemModel::setData(index, value, role);
    }

    return false;
}

QVariant VideoFormModel::data(const QModelIndex &index, int role) const
{
    const auto header = findHorizontalHeader(this, index);

    if (role == Qt::BackgroundRole)
    {
        if (!originDataColumns_.contains (header))
        {
            return QBrush (QColor (245, 245, 245));
        }
        else
        {
            return {};
        }
    }

    if (role != Qt::DisplayRole)
    {
        return QStandardItemModel::data (index, role);
    }

    if (header == "编号")
    {
        return index.row () + 1;
    }

    if(header.at(header.size() - 1) == "T")
    {
        bool isOk = false;
        auto var = QStandardItemModel::data(index, role);
        double data = var.toDouble(&isOk);
        if(isOk)
        {
            return QString::number(data, 'f', 2);
        }

        return {};
    }


    if(header.at(header.size() - 1) == "R")
    {
        bool isOk = false;
        auto var = QStandardItemModel::data(index, role);
        double data = var.toDouble(&isOk);
        if(isOk)
        {
            return QString::number(data, 'f', 2);
        }

        return {};
    }

    if(header == "测量时间")
    {
        double averageTime = 0;
        for(int col = 2; col < 2 + dataCol_; col += 2)
        {
            auto prevTime = this->index(index.row(), col).data(Qt::DisplayRole).toDouble();
            auto nextTime = this->index(index.row(), col + 1).data(Qt::DisplayRole).toDouble();

            if(nextTime > 0 and prevTime > 0)
            {
                auto time = nextTime - prevTime;
                averageTime += time;
            }
        }

        if(averageTime > 0)
        {
            return QString::number(averageTime);
        }
        else
        {
            return {};
        }

    }

    if(header == "基本时间")
    {
        ///基本时间为平均时间*评比系数
        const auto row = index.row();
        const auto col = index.column();
        const auto averageTimeIndex = this->index(row, col - 1);
        const auto comparisonIndex = this->index(row, col - 2);

        bool isAverageOk = false;
        bool isComparisonOk = false;
        const auto averageTime = averageTimeIndex.data().toDouble(&isAverageOk);
        const auto comparison = comparisonIndex.data().toDouble(&isComparisonOk);
        if(isAverageOk and isComparisonOk)
        {
            return QString::number( (averageTime * comparison), 'f', 2);
        }
        else
        {
            return {};
        }
    }

    if (header == "宽放率")
    {
        const auto var = QStandardItemModel::data(index, Qt::DisplayRole);
        bool isOk = false;
        const auto rate = var.toDouble(&isOk);
        if(isOk)
        {
            return QString::number (rate * 100, 'f', 2) + " %";
        }
        else
        {
            return QString::number (0, 'f', 2) + " %";
        }
    }

    if(header == "标准工时")
    {
        ///标准工时为基本时间*（1+宽放率）
        const auto row = index.row();
        const auto col = index.column();
        const auto rateIndex = this->index(row, col - 1);
        const auto basicTimeIndex = this->index(row, col - 2);

        bool isRateOk = false;
        bool isBasicTimeOk = false;
        const auto data = rateIndex.data().toString();
        auto list = data.split("%");
        if(list.size() <= 0)
        {
            return {};
        }

        auto rate = list.at(0).toDouble(&isRateOk);
        const auto basicTime = basicTimeIndex.data().toDouble(&isBasicTimeOk);

        if(isBasicTimeOk)
        {
            return ((rate / 100 + 1) * basicTime);
        }
        else
        {
            return {};
        }
    }

    if(header == "增值/非增值")
    {
        const auto row = index.row();
        const auto col = index.column();
        const auto theIndex = this->index(row, col + 1);
        const auto theData = theIndex.data().toString();

        if(theData == "加工")
        {
            return "增值";
        }
        else
        {
            return "非增值";
        }
    }

    return QStandardItemModel::data(index, role);
}

QVariant VideoFormModel::previousData(const QModelIndex &index) const
{    
    if(index.column() == 3)
    {
        return qreal {0};
    }

    if(index.column() > 3 or index.column()  <= dataCol_ + 2)
    {
        return data(this->index(index.row(), index.column() - 1));
    }

    return {};
}

std::optional<action_ratio> VideoFormModel::operation_ratio() const
{
    std::array<qreal, 4> arr {{0, 0, 0, 0}};

    qreal & processing = arr.at (0);
    qreal & checking = arr.at (1);
    qreal & moving = arr.at (2);
    qreal & waiting = arr.at (3);

    for (int i = 0; i < this->rowCount (); i ++)
    {
        bool b;
        const auto time = getValueByKey(i, "标准工时").toDouble (&b);

        if (!b)
        {
            continue;
        }

        const auto type = getValueByKey (i, "操作类型").toString ();
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
    }

    const auto total = accumulate (std::begin (arr), std::end (arr), qreal {0});
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

std::optional<overall_stats> VideoFormModel::operation_stats() const
{
    const auto col = getHorizontalHeaderCol("测量时间");
    const auto rows = rowCount();

    double ct_value = 0;
    for(int row = 0; row < rows; row++)
    {
        const auto time = this->index(row, col).data(Qt::DisplayRole).toDouble();

        ct_value += time;
    }

    overall_stats ret;
    if(ct_value > 0)
    {
        ret.ct_val = ct_value;
    }

    return ret;
}

std::vector<qreal> VideoFormModel::cycle_times() const
{
    vector<qreal> ret;
    for (int i = 2; i < 2 + maxRound_ * 2; i += 2)
    {
        std::optional<qreal> sum = 0.0;
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


