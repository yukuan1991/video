#ifndef VIDEO_MODEL_H
#define VIDEO_MODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include "model/json_model.h"
#include <optional>
#include "video/utils.hpp"

class video_form_model final: public json_model
{
    Q_OBJECT
public:
    constexpr static int32_t max_rows = 10000;
    constexpr static int32_t max_round = 10;
    constexpr static int32_t data_col = max_round * 2;
    static const char * const cols[];
public:
    explicit video_form_model(QObject *parent = 0);
//    QVariant get_header_info (const QModelIndex&, int) const;
    QString get_std_sum ();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData (const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags (const QModelIndex &index) const noexcept override;
    void resize (unsigned len) override;
    void clear ();
    std::optional<action_ratio> operation_ratio () const;
    std::optional<overall_stats> operation_stats () const;
    std::vector<qreal> cycle_times () const;
private:
    QVariant get_des_header (const QModelIndex&, int) const;
    QVariant get_data_header (const QModelIndex&, int) const;
    QVariant get_result_header (const QModelIndex&, int) const;

    QVariant get_average (int row) const;
    QVariant get_result_table (const QModelIndex& index) const;
    std::optional<double> get_increment_data (const QModelIndex& index) const;

    QVariant get_data_table (const QModelIndex& index) const;
    bool set_data_table (const QModelIndex& index, const QVariant& value);
    bool set_result_table (const QModelIndex& index, const QVariant& value);

    double get_last_total_time (int row, int col) const;
    QVariant get_std_time (const QModelIndex& index) const;
    QVariant get_base_time (const QModelIndex& index) const;
    QVariant get_result_info (const std::string& key, int row) const;
    QModelIndex get_result_index (const std::string& key, int row) const;
    double get_allowance (const QModelIndex& index) const;
    void resize_helper (int len);
    bool paste_data (const QModelIndex& index, const QVariant& value) override;

    bool paste_result_table (const QModelIndex& index, const QVariant& value);
    bool paste_data_table (const QModelIndex& index, const QVariant& value);

private:
    int row_count_ = 0;
    double stdtime_sum_ = 0;
};

#endif // VIDEO_MODEL_H
