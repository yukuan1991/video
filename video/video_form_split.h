#ifndef VIDEO_DES_H
#define VIDEO_DES_H

#include <QSortFilterProxyModel>
#include "video/video_form_model.h"
#include <base/qt/ui.hpp>

class video_form_split : public QSortFilterProxyModel
{
public:
    video_form_split(QObject* parent = nullptr);
    void set_range (int start, int end) {start_col_ = start; end_col_ = end;}
    bool filterAcceptsColumn(int source_col, const QModelIndex &source_parent) const override;

private:
    int start_col_ = 0;
    int end_col_ = 0;
};

#endif // VIDEO_DES_H
