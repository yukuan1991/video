#include "first_dlg.h"
#include "ui_first_dlg.h"
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QStyleFactory>

first_dlg::first_dlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::first_dlg)
{
    ui->setupUi(this);
    this->setStyle (QStyleFactory::create ("fusion"));
    ui->widget->set_clearable (true);
    this->resize (QApplication::desktop()->screenGeometry ().width () / 2,
                  QApplication::desktop()->screenGeometry ().height () / 2);

}

first_dlg::~first_dlg()
{
    delete ui;
}

void first_dlg::set_current_video(const QString &file)
{
    ui->widget->set_file(file);
}

const std::vector<qint64> &first_dlg::retrieve_invalid()
{
    return ui->widget->retrieve_invalid();
}

void first_dlg::set_invalid(std::vector<qint64> &v)
{
   ui->widget->set_invalid(v);
}

void first_dlg::on_widget_state_changed(const video_player::state_enum &state)
{
    if(state != video_player::stopped_state)
    {
        ui->button_start->setEnabled(true);
    }
    else
    {
        ui->button_start->setEnabled(false);
        start_mark_time_=0;

        ui->button_end->setEnabled(false);
        end_mark_time_=0;
    }
}

void first_dlg::on_button_start_clicked()
{
    if(!start_mark_)
    {
        ui->button_start->setText("取消无效动作开始点");
        start_mark_time_ = ui->widget->position();
        ++start_mark_;

        ui->button_end->setEnabled(true);
    }
    else
    {
        ui->button_start->setText("标记无效动作开始点");
        start_mark_time_=0;
        start_mark_=0;

        ui->button_end->setEnabled(false);
        end_mark_time_=0;
    }
}

void first_dlg::on_button_end_clicked()
{
    end_mark_time_ = ui->widget->position();
    if(start_mark_time_ >= end_mark_time_)
    {
        QMessageBox::information(this,"错误","开始标记时间必须小于结束标记时间");
        end_mark_time_=0;
        return;
    }
    bool add_result = ui->widget->set_progress_label_position(std::pair<unsigned long long, unsigned long long>(start_mark_time_,end_mark_time_));
    if(!add_result)
    {
        QMessageBox::information(this,"错误","标记定位失败");
        ui->button_start->setText("标记无效动作开始点");
        start_mark_time_ = 0;
        end_mark_time_ = 0;
    }
    ui->button_end->setEnabled(false);
    ui->button_start->setText("标记无效动作开始点");
    start_mark_=0;
}

void first_dlg::on_button_confirm_clicked()
{
    accept ();
}

void first_dlg::on_first_dlg_finished(int)
{
    ui->widget->stop_video ();
}
