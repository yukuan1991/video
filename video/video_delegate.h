#ifndef TABLE_DES_DELEGATE_H
#define TABLE_DES_DELEGATE_H
#include <QStyledItemDelegate>
#include "video/video_form_model.h"


class video_delegate : public QStyledItemDelegate
{
public:
    video_delegate(QObject* parent = nullptr);

    QWidget *createEditor (QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData (QWidget* editor, const QModelIndex& index) const override;
    void setModelData (QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry (QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    compl video_delegate () override = default;

private:
    QWidget* create_result_editor (QWidget* parent, const QModelIndex& index, video_form_model* src_model) const;
    void set_result_editor (QWidget* editor, const QModelIndex& index, video_form_model* src_model) const;
    void set_result_model (QWidget* editor, video_form_model* src_model, const QModelIndex& index) const;
};

#endif // TABLE_DES_DELEGATE_H
