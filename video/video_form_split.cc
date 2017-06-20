#include "video_form_split.h"

video_form_split::video_form_split(QObject* parent)
    :QSortFilterProxyModel (parent)
{

}

bool video_form_split::filterAcceptsColumn(int source_col, const QModelIndex &) const
{
    if (source_col >= start_col_ and source_col < end_col_)
    {
        return true;
    }
    return false;
}
