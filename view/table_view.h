#pragma once

#include <QTableView>
#include <vector>
#include <string>
#include "model/json_model.h"
#include <tuple>
#include <QPoint>

class table_view : public QTableView
{
    Q_OBJECT
public:
    table_view(QWidget* parent);

    void mouseReleaseEvent (QMouseEvent* event) override;
    void mousePressEvent (QMouseEvent* event) override;

    ~table_view () override;

    void on_copy_del (int flag = 0);
    void on_paste ();
signals:
    void mouse_pressed ();
public:
    static constexpr uint32_t OPERATION_DEL = 0b1;
    static constexpr uint32_t OPERATION_COPY = 0b10;
protected:
    void keyPressEvent (QKeyEvent* event) override;
    void paintEvent(QPaintEvent *) override;
private:
    std::vector<std::vector<std::string>> get_clip_structure ();
    std::tuple<int, int, int, int> get_rect (bool* ok = nullptr);

private:
    QModelIndex current_click_index_;
};

