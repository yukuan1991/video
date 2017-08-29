#include "VideoFormModel.h"
#include <boost/range/adaptor/indexed.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/counting_range.hpp>
#include <base/lang/range.hpp>

#include <QDebug>

using namespace std;
using namespace boost::adaptors;
using namespace boost;

void VideoFormModel::init()
{
    static_assert (dataCol_ % 2 == 0, "data_col is not an even number");

    QStringList horizontalHeaderList;
    horizontalHeaderList << "编号" << "作业内容";
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
        }
        horizontalHeaderList << cycleHeader.data();
    }

    horizontalHeaderList << "平均时间" << "评比系数" << "基本时间" << "宽放率" << "标准工时" << "增值/非增值" << "操作类型";
    originDataColumns_ << "评比系数" << "宽放率" << "操作类型";
    setHorizontalHeaderLabels(horizontalHeaderList);
}

QString VideoFormModel::findHorizontalHeader(const QStandardItemModel *model, const QModelIndex &index)
{
    return model->headerData(index.column(), Qt::Horizontal).toString();
}

bool VideoFormModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    const auto header = findHorizontalHeader(this, index);
    if(originDataColumns_.contains(header))
    {
        return QStandardItemModel::setData(index, value, role);
    }

    return false;
}

QVariant VideoFormModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return QStandardItemModel::data (index, role);
    }

    const auto header = findHorizontalHeader(this, index);
    if(originDataColumns_.contains(header))
    {
        return QStandardItemModel::data(index, role);
    }

    if (header == "编号")
    {
        return index.row () + 1;
    }

    if(header.at(header.size() - 1) == "R")
    {
        const auto row = index.row();
        const auto col = index.column();

        const QVariant currentTime = data (this->index (row, col - 1), Qt::DisplayRole);
        const QVariant lastTime = previousData (index);

        if (currentTime.type () != QVariant::Double or lastTime.type () != QVariant::Double)
        {
            return {};
        }
        else
        {
            return currentTime.toDouble () - lastTime.toDouble ();
        }
    }

    if(header == "平均时间")
    {
        std::vector<double> times;
        counting_range (2, 2 + dataCol_)
                | transformed ([&] (auto && c) { return this->index (index.row (), c); })
                | filtered ([&] (auto && c) { return *(findHorizontalHeader (this, c).rbegin ()) == "R"; })
                | filtered ([&] (auto && c) { return c.data (Qt::DisplayRole).type () == QVariant::Double; })
                | transformed ([] (auto && c) { return c.data (Qt::DisplayRole).toDouble (); })
                | append_to (times);

        if (times.empty ())
        {
            return {};
        }
        else
        {
            return (times | accumulated (qreal {0}))  / times.size ();
        }
    }

    if(header == "基本时间")
    {
        ///基本时间为平均时间*评比系数
        const auto row = index.row();
        const auto col = index.column();
        const auto theIndex = this->index(row, col - 1);
        const auto anotherIndex = this->index(row, col - 2);
        const auto theData = theIndex.data().toDouble();
        const auto anotherData = anotherIndex.data().toDouble();
        return (theData * anotherData);
    }

    if(header == "标准工时")
    {
        ///标准工时为基本时间*（1+宽放率）
        const auto row = index.row();
        const auto col = index.column();
        const auto theIndex = this->index(row, col - 1);
        const auto anotherIndex = this->index(row, col - 2);
        const auto theData = theIndex.data().toDouble();
        const auto anotherData = anotherIndex.data().toDouble();
        return ((theData + 1) * anotherData);
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

    qDebug () << __PRETTY_FUNCTION__ << " " << __LINE__;

    return {};
}

QVariant VideoFormModel::previousData(const QModelIndex &index) const
{
    if (index.row () == 0)
    {
        if (index.column () == 3)
        {
            return qreal {0};
        }
        else
        {
            return data (this->index (rowCount () - 1, index.column () - 3), Qt::DisplayRole);
        }
    }
    else
    {
        return data (this->index (index.row () - 1, index.column () - 1));
    }
}


