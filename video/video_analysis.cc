#include "video/video_analysis.h"
#include "ui_video_analysis.h"
#include <QInputDialog>
#include <QDebug>
#include "video/first_dlg.h"
#include <QMessageBox>
#include <QMenu>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <base/qt/ui.hpp>
#include <QChart>
#include <QPieSeries>
#include <QPieSlice>

using namespace QtCharts;

video_analysis::video_analysis(QWidget *parent)
    :QWidget(parent)
    ,ui(new Ui::video_analysis)
    ,operation_type_ (new QPieSeries)
    ,efficiency_ (new QPieSeries)

{
    ui->setupUi(this);

    ui->button_sec_backward->setIcon(this->style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->button_sec_forward->setIcon(this->style()->standardIcon(QStyle::SP_MediaSkipForward));

    connect(ui->video_player, &video_widget::file_changed, [=] (const QString&) {ui->video_player->clear_invalid (); });
    connect (this, &video_analysis::marked, this, &video_analysis::on_marked);

    connect (ui->video_player, &video_widget::stepped_into_invalid, [=] (qint64, qint64 end)
    {
        ui->video_player->set_position(end);
    });

    set_children_filter (this);
    ui->button_mark->setIcon (QIcon ("icon/mark.png"));

    init_chart ();
}

video_analysis::~video_analysis()
{
    delete ui;
}



void video_analysis::set_video_file(const QString & video)
{
    current_video_ = video;
    ui->video_player->set_file (current_video_);
    file_opening_ = true;
    modify_invalid ();
}



void video_analysis::init_video_widget(const json &video_detail)
{
    auto iter_path = video_detail.find ("视频路径");
    assert (iter_path != video_detail.end ());
    assert (iter_path->is_string ());
    std::string path = *iter_path;
    ui->video_player->set_file (path.data ());
    current_video_ = QString (path.data ());

    auto iter_invalid = video_detail.find ("无效时间段");
    assert (iter_invalid != video_detail.end ());
    assert (iter_invalid->is_array ());
    auto& invalid = *iter_invalid;
    qint64 time;
    std::vector <qint64> vec;
    for (unsigned i=0; i<invalid.size (); ++i)
    {
        time = invalid.at (i);
        vec.emplace_back (time);
    }
    ui->video_player->set_invalid (vec);
}

void video_analysis::init_chart()
{
    const auto chart = new QChart;
    ui->pie->setChart (chart);

    chart->addSeries (operation_type_);
    chart->addSeries (efficiency_);
    chart->createDefaultAxes ();

    chart->setAnimationOptions (QChart::AllAnimations);
    ui->pie->setRenderHint (QPainter::Antialiasing);

    operation_type_->setHorizontalPosition (0.75);
    efficiency_->setHorizontalPosition (0.25);
    operation_type_->setVerticalPosition (0.6);
    efficiency_->setVerticalPosition (0.6);
    chart->setTitle ("增值/非增值 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;操作类型");


    for (int i = 0; i < 2; i ++)
    {
        auto slice = new QPieSlice;

        connect (slice, &QPieSlice::hovered, [slice] (bool b){ slice->setLabelVisible (b); });
        connect (slice, &QPieSlice::hovered, [slice] (bool b){ slice->setExploded (b); });
        slice->setExplodeDistanceFactor (0.05);

        efficiency_->append (slice);
    }

    for (int i = 0; i < 4; i ++)
    {
        auto slice = new QPieSlice;

        connect (slice, &QPieSlice::hovered, [slice] (bool b){ slice->setLabelVisible (b); });
        connect (slice, &QPieSlice::hovered, [slice] (bool b){ slice->setExploded (b); });
        slice->setExplodeDistanceFactor (0.05);

        operation_type_->append (slice);
    }

}

bool video_analysis::eventFilter(QObject *, QEvent *event)
{
    if (event->type () == QEvent::KeyPress)
    {
        QKeyEvent* key_event = dynamic_cast<QKeyEvent*> (event);

        if (key_event->key () == Qt::Key_Space)
        {
            ui->video_player->switch_play_pause ();
            return true;
        }
        else if (key_event->key () == Qt::Key_M)
        {
            if (key_event->modifiers () & Qt::ControlModifier)
            {
                if (ui->button_mark->isEnabled ())
                {
                    on_button_mark_clicked ();
                }
                return true;
            }
        }
        else
        {
            return false;
        }
    }

    return false;
}


void video_analysis::load_json(const json &data)
{
    auto iter_sheet = data.find ("表");
    assert (iter_sheet != data.end () and iter_sheet->is_object ());
    auto iter_video_analysis = iter_sheet->find ("视频分析法详细信息");
    assert (iter_video_analysis != iter_sheet->end () and iter_video_analysis->is_object ());
    auto iter_circulation = iter_video_analysis->find ("循环");
    assert (iter_circulation != iter_video_analysis->end () and iter_circulation->is_number_integer ());
    auto iter_task = iter_video_analysis->find ("作业内容");
    assert (iter_task != iter_video_analysis->end () and iter_task->is_array ());
    auto iter_data = iter_video_analysis->find ("观测时间");
    assert (iter_data != iter_video_analysis->end () and iter_data->is_array ());
    auto iter_result = iter_video_analysis->find ("结果");
    assert (iter_result != iter_video_analysis->end () and iter_result->is_array ());

    ui->form->set_row (iter_task->size ());
    ui->form->load_task (*iter_task);
    ui->form->load_data (*iter_data);
    ui->form->load_result (*iter_result);

    init_video_widget (*iter_video_analysis);
}

void video_analysis::on_combo_second_activated(int index)
{
    ui->video_player->set_position(ui->video_player->position() + index);
}

void video_analysis::on_video_player_state_changed(video_player::state_enum state)
{
    if (state == video_player::stopped_state)
    {
        ui->button_sec_backward->setEnabled(false);
        ui->button_sec_forward->setEnabled(false);

        ui->button_mark->setEnabled (false);
    }
    else if (state == video_player::playing_state)
    {
        ui->button_sec_backward->setEnabled(false);
        ui->button_sec_forward->setEnabled(false);

        ui->button_mark->setEnabled (true);
    }
    else
    {
        ui->button_sec_backward->setEnabled(true);
        ui->button_sec_forward->setEnabled(true);
    }
}

void video_analysis::on_button_sec_backward_clicked()
{
    float current_backward_second;
    try
    {
        current_backward_second = boost::lexical_cast<float> ( ui->combo_second->currentText().toStdString());
    }
    catch(...)
    {
        QMessageBox::information(this,"错误","second只能输入数字");
        return;
    }

    int micro_second = static_cast<int> (current_backward_second) * 1000;
    auto current_position = ui->video_player->position();

    ui->video_player->set_position(current_position - micro_second);
}

void video_analysis::on_button_sec_forward_clicked()
{
    float current_position;
    try
    {
        current_position = boost::lexical_cast<float> ( ui->combo_second->currentText().toStdString());
    }
    catch (...)
    {
        QMessageBox::information(this,"错误","second只能输入数字");
        return;
    }

    int micro_second = static_cast<int> (current_position) * 1000;
    auto current_pos = ui->video_player->position();

    ui->video_player->set_position(current_pos + micro_second);
}

void video_analysis::on_spinbox_rate_valueChanged(double arg1)
{
    ui->video_player->set_speed(arg1);
}

void video_analysis::modify_invalid ()
{
    ui->video_player->stop_video();
    if(current_video_.isEmpty())
    {
        QMessageBox::information(this,"错误","未载入文件");
        return;
    }

    dlg_->set_current_video(current_video_);

    invalid_data_ = ui->video_player->retrieve_invalid();
    dlg_->set_invalid(invalid_data_);

    if (QDialog::Accepted ==  dlg_->exec ())
    {
        if (file_opening_ == false)
        {
            auto ret = QMessageBox::question (this, "修改无效区域", "是否确认修改? (所有数据将会清空)", QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::No) return;
        }
        else
        {
            file_opening_ = false;
        }

        invalid_data_ = dlg_->retrieve_invalid();
        ui->video_player->set_invalid(invalid_data_);
    }
}


void video_analysis::on_video_player_stepped_into_invalid(qint64, qint64 pos_out)
{
    ui->video_player->set_position(pos_out);
}

void video_analysis::on_button_mark_clicked()
{
    assert(invalid_data_.size() % 2 == 0);
    auto cur_pos = ui->video_player->position();
    long long int invalid_time=0;
    for(unsigned int i = 0; i<invalid_data_.size (); i += 2)
    {
        if(cur_pos > 0 and static_cast<unsigned>(cur_pos) > invalid_data_[i] and static_cast<unsigned> (cur_pos) < invalid_data_[i+1])
        {
            assert(i < invalid_data_.size());
            QMessageBox::information(this,"错误","操作不能发生在无效区域");
            return;
        }
        if(cur_pos > 0 and invalid_data_[i+1] < static_cast<unsigned>(cur_pos))
        {
            invalid_time += invalid_data_[i+1] - invalid_data_[i];
        }
        else
        {
            break;
        }
    }
    auto valid_spent_time = cur_pos - invalid_time;

    emit marked(valid_spent_time);
}

void video_analysis::on_marked(long long msec)
{
    //if (pause_marked_action_->isChecked ())
    //{
    //    ui->video_player->pause_video ();
    //}

    ui->form->mark (msec);
}


void video_analysis::on_button_setting_rows_clicked()
{
    QInputDialog dlg;
    dlg.setWindowTitle ("更改作业内容步数");
    dlg.setLabelText ("请填写操作步数");
    dlg.setOkButtonText ("确 定");
    dlg.setCancelButtonText ("取 消");
    dlg.setInputMode (QInputDialog::IntInput);
    dlg.setIntRange (0, video_form_model::max_rows);

    if (dlg.exec () == QDialog::Accepted)
    {
        auto row = dlg.intValue ();
        ui->form->set_row (row);
    }
}




void video_analysis::on_paste()
{
    ui->form->on_paste ();
}


void video_analysis::set_task_count ()
{
    QInputDialog dlg;
    dlg.setWindowTitle ("新建");
    dlg.setLabelText ("请填写操作步数");
    dlg.setOkButtonText ("确 定");
    dlg.setCancelButtonText ("取 消");
    dlg.setInputMode (QInputDialog::IntInput);
    dlg.setIntRange (0, video_form_model::max_rows);

    if (dlg.exec () == QDialog::Accepted)
    {
        auto row = dlg.intValue ();
        ui->form->clear ();
        ui->form->set_row (row);
        this->current_file_data_.clear();
    }
}


void video_analysis::on_copy()
{
    ui->form->on_copy ();
}

void video_analysis::on_cut()
{
    ui->form->on_cut ();
}

void video_analysis::on_del()
{
    ui->form->on_del ();
}

void video_analysis::refresh_chart (action_ratio ratio)
{
    {
        const auto slices = operation_type_->slices ();

        {
            auto slice = slices.at (0);
            slice->setLabel ("加工 " + QString::number (ratio.processing) + "%");
            slice->setValue (ratio.processing);
        }

        {
            auto slice = slices.at (1);
            slice->setLabel ("检查 " + QString::number (ratio.checking) + "%");
            slice->setValue (ratio.checking);
        }

        {
            auto slice = slices.at (2);
            slice->setLabel ("搬运 " + QString::number (ratio.moving) + "%");
            slice->setValue (ratio.moving);
        }

        {
            auto slice = slices.at (3);
            slice->setLabel ("等待 " + QString::number (ratio.waiting) + "%");
            slice->setValue (ratio.waiting);
        }
    }

    {
        const auto slices = efficiency_->slices ();
        {
            auto slice = slices.at (0);
            slice->setLabel ("增值 " + QString::number (ratio.processing) + "%");
            slice->setValue (ratio.processing);
        }

        {
            auto slice = slices.at (1);
            slice->setLabel ("非增值 " + QString::number (100 - ratio.processing) + "%");
            slice->setValue (100 - ratio.processing);
        }
    }
}

