#pragma once
#include <QStandardItemModel>
#include "utils.hpp"


class VideoFormModel : public QStandardItemModel
{
    Q_OBJECT
public:
    constexpr static int32_t maxRound_ = 5;
    constexpr static int32_t dataCol_ = maxRound_ * 2;
public:
    template<typename ...ARGS>
    VideoFormModel(ARGS && ...args) : QStandardItemModel(std::forward<ARGS> (args)...) { init(); }

    void init();

    QString getStdSum ();
    int getHorizontalHeaderCol(const QString& name) const;
    QVariant getValueByKey (int row, const QString& key, int role = Qt::DisplayRole) const;
    static QString findHorizontalHeader(const QStandardItemModel* model, const QModelIndex& index);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant previousData (const QModelIndex & index) const;

    std::optional<action_ratio> operation_ratio () const;
    std::optional<overall_stats> operation_stats () const;
    std::vector<qreal> cycle_times () const;
private:
    QStringList horizontalHeaderColumns_;
    QStringList originDataColumns_;
};

