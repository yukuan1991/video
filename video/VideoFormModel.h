#pragma once
#include <QStandardItemModel>


class VideoFormModel : public QStandardItemModel
{
    Q_OBJECT
public:
    constexpr static int32_t maxRound_ = 10;
    constexpr static int32_t dataCol_ = maxRound_ * 2;
public:
    template<typename ...ARGS>
    VideoFormModel(ARGS && ...args) : QStandardItemModel(std::forward<ARGS> (args)...) { init(); }
    void init();

    int getHorizontalHeaderCol(const QString& name) const;
    static QString findHorizontalHeader(const QStandardItemModel* model, const QModelIndex& index);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant previousData (const QModelIndex & index) const;
private:
    QStringList horizontalHeaderColumns_;
    QStringList originDataColumns_;
};

