#include "VideoFormModel.h"
#include <boost/range/adaptor/indexed.hpp>
#include <boost/algorithm/string.hpp>

#include <QDebug>

using namespace std;
using namespace boost::adaptors;

void VideoFormModel::init()
{
    static_assert (dataCol_ % 2 == 0, "data_col is not an even number");

    QStringList horizontalHeaderList;
    horizontalHeaderList << "编号" << "作业内容";
    editableColumns_ << "作业内容";

    for(int i = 0; i < dataCol_; i ++)
    {
        auto cycleHeader = to_string (i / 2 + 1);
        if (i % 2 == 0)
        {
            cycleHeader += "T";
            editableColumns_ << cycleHeader.data();
        }
        else
        {
            cycleHeader += "R";
        }
        horizontalHeaderList << cycleHeader.data();
    }

    horizontalHeaderList << "平均时间" << "评比系数" << "基本时间" << "宽放率" << "标准工时" << "增值/非增值" << "操作类型";
    editableColumns_ << "评比系数" << "宽放率" << "操作类型";
    setHorizontalHeaderLabels(horizontalHeaderList);
}

QString VideoFormModel::findHorizontalHeader(const QStandardItemModel *model, const QModelIndex &index)
{
    return model->headerData(index.column(), Qt::Horizontal).toString();
}

bool VideoFormModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    const auto header = findHorizontalHeader(this, index);
    if(editableColumns_.contains(header))
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
    if(editableColumns_.contains(header))
    {
        return QStandardItemModel::data(index, role);
    }

    if(header == "编号")
    {
        return index.row() + 1;
    }

    if(header.at(header.size() - 1) == "R")
    {
        const auto row = index.row();
        const auto col = index.column();
        if(row == 0)
        {
            const auto theCol = 3;
            if(col == theCol)
            {
                const auto theIndex = this->index(row, col -1);
                return theIndex.data();
            }
            if(col > theCol)
            {
                const auto theIndex = this->index(row, col -1);
                const auto anotherIndex = this->index(rowCount() - 1, col - 3);
                return QString::number((theIndex.data().toDouble() - anotherIndex.data().toDouble()), 'f', 2);
            }
        }
        else if(row > 0)
        {
            const auto theIndex = this->index(row - 1, col - 1);
            const auto anotherIndex = this->index(row, col -1);
            return QString::number((anotherIndex.data().toDouble() - theIndex.data().toDouble()), 'f', 2);
        }
    }

    if(header == "平均时间")
    {
        double sum = 0;
        int num = 0;
        const auto row = index.row();
        for(int i = 0; i < columnCount(); i++)
        {
            if(*(headerData(i, Qt::Horizontal).toString().rbegin()) == "R")
            {
                const auto theIndex = this->index(row, i);
                if(theIndex.data().isNull())
                {
                    continue;
                }
                sum += theIndex.data().toDouble();
                num++;
            }
        }
        return sum / num;
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
    return {};
}


