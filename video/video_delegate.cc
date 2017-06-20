#include "video_delegate.h"
#include <QComboBox>
#include <QSortFilterProxyModel>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include "video/video_form_model.h"
#include <memory>

using namespace std;

video_delegate::video_delegate(QObject* parent)
    :QStyledItemDelegate (parent)
{

}

QWidget *video_delegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
    auto proxy_model = dynamic_cast<const QSortFilterProxyModel*>(index.model ()); assert (proxy_model);
    auto src_index = proxy_model->mapToSource (index);
    auto src_abstract_model = proxy_model->sourceModel ();
    auto src_model = dynamic_cast<video_form_model*>(src_abstract_model);

    if (!src_index.isValid ())
    {
        return nullptr;
    }

    if (src_index.column () == 1)
    {
        return make_unique<QLineEdit> (parent).release ();
    }

    if (src_index.column () > 2 + static_cast<int> (video_form_model::data_col))
    {
        return create_result_editor (parent, src_index, src_model);
    }

    return nullptr;
}

void video_delegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    assert (editor);
    auto proxy_model = dynamic_cast<const QSortFilterProxyModel*>(index.model ()); assert (proxy_model);
    auto src_index = proxy_model->mapToSource (index);
    auto src_abstract_model = proxy_model->sourceModel ();
    auto src_model = dynamic_cast<video_form_model*>(src_abstract_model);

    if (!src_index.isValid ())
    {
        return;
    }

    if (src_index.column () == 1)
    {
        auto edit = dynamic_cast<QLineEdit*>(editor); assert (edit);
        auto variant = index.data (Qt::DisplayRole);
        edit->setText (variant.toString ());
    }

    if (src_index.column () > 2 + static_cast<int> (video_form_model::data_col))
    {
        return set_result_editor (editor, src_index, src_model);
    }
}

void video_delegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto proxy_model = dynamic_cast<QSortFilterProxyModel*>(model); assert (proxy_model);
    auto src_index = proxy_model->mapToSource (index);
    auto src_abstract_model = proxy_model->sourceModel ();
    auto src_model = dynamic_cast<video_form_model*>(src_abstract_model);

    if (src_index.column ()== 1)
    {
        auto edit = dynamic_cast<QLineEdit*>(editor); assert (edit);
        model->setData (index, edit->text ());
    }

    if (src_index.column () > 2 + static_cast<int>(video_form_model::data_col))
    {
        return set_result_model (editor, src_model, src_index);
    }
}

void video_delegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
{
    editor->setGeometry (option.rect);
}

QWidget* video_delegate::create_result_editor(QWidget *parent, const QModelIndex &index, video_form_model* src_model) const
{
    assert (index.isValid ());
    assert (src_model);
    auto op_header = src_model->get_header (index);

    if (*op_header == "评比系数")
    {
        auto spinbox = make_unique<QDoubleSpinBox> (parent);
        spinbox->setMinimum (0.1);
        spinbox->setMaximum (10);
        spinbox->setSingleStep (0.1);
        return spinbox.release ();
    }
    else if (*op_header == "宽放率")
    {
        return new QLineEdit {parent};
    }
    else if (*op_header == "操作类型")
    {
        auto combo = make_unique<QComboBox> (parent);
        auto items = QStringList {};
        items << "加工" << "检查" << "搬运" << "等待";
        combo->addItems (items);

        return combo.release ();
    }

    return nullptr;
}

void video_delegate::set_result_editor(QWidget *editor, const QModelIndex &index, video_form_model* src_model) const
{
    assert (index.isValid ());
    assert (src_model);
    auto var = index.data ();
    auto op_header = src_model->get_header (index);

    if (*op_header == "评比系数")
    {
        auto spinbox = dynamic_cast<QDoubleSpinBox*>(editor); assert (spinbox);
        assert (var.type () == QVariant::Double);
        spinbox->setValue (var.toDouble ());
    }
    else if (*op_header == "宽放率")
    {
        auto edit = dynamic_cast<QLineEdit*>(editor); assert (edit);
        auto list = var.toString ().split (" "); assert (list.size () == 2);
        edit->setText (list[0]);
    }
    else if (*op_header == "操作类型")
    {
        auto combo = dynamic_cast<QComboBox*>(editor); assert (combo);
        auto text_str = var.toString ();

        for (int i = 0; i < combo->count (); i++)
        {
            if (text_str == combo->itemText (i))
            {
                combo->setCurrentIndex (i);
                return;
            }
        }
        assert (false);
    }
}

void video_delegate::set_result_model(QWidget *editor, video_form_model *src_model, const QModelIndex &index) const
{
    assert (src_model);
    assert (index.isValid ());

    auto op_header = src_model->get_header (index);
    if (*op_header == "评比系数")
    {
        auto spinbox = dynamic_cast<QDoubleSpinBox*>(editor); assert (spinbox);
        double val = spinbox->value ();
        src_model->setData (index, val);
    }
    else if (*op_header == "宽放率")
    {
        auto edit = dynamic_cast<QLineEdit*>(editor); assert (edit);
        auto text = edit->text ();

        src_model->setData (index, text);
    }
    else if (*op_header == "操作类型")
    {
        auto combo = dynamic_cast<QComboBox*>(editor); assert (combo);
        auto text = combo->currentText ();

        src_model->setData (index, text);
    }
}




















