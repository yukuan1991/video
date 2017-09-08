#include "video/VideoAnalysis.h"
#include "ui_VideoAnalysis.h"
#include <QInputDialog>
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
#include <QStyleFactory>
#include <base/io/file/file.hpp>
#include <base/lang/range.hpp>


using namespace QtCharts;
using namespace std;

VideoAnalysis::VideoAnalysis(QWidget *parent)
    :QWidget(parent)
    ,ui(new Ui::VideoAnalysis)
    ,operation_type_ (new QPieSeries)
    ,efficiency_ (new QPieSeries)

{
    ui->setupUi(this);

    ui->button_sec_backward->setIcon(this->style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->button_sec_forward->setIcon(this->style()->standardIcon(QStyle::SP_MediaSkipForward));

    connect(ui->video_player, &video_widget::file_changed, [=] (const QString&) {ui->video_player->clear_invalid (); });
    connect (this, &VideoAnalysis::marked, this, &VideoAnalysis::on_marked);

    connect (ui->video_player, &video_widget::stepped_into_invalid, [=] (qint64, qint64 end)
    {
        ui->video_player->set_position(end);
    });
    connect (ui->form, &FormWidget::data_changed, [this]
    {
        const auto ratio = ui->form->operation_ratio ();
        const auto stats = ui->form->operation_stats ();
        auto cycles = ui->form->cycle_times ();
        if (ratio)
        {
            this->refresh_chart (ratio.value ());
        }
        if (stats)
        {
            this->refresh_stats (stats.value ());
        }
    });

    set_children_filter (this);
    ui->button_mark->setIcon (QIcon ("icon/mark.png"));

    this->setStyle (QStyleFactory::create ("fusion"));

    init_chart ();

    ui->video_player->set_mark_enabled(false);
}

VideoAnalysis::~VideoAnalysis()
{
    delete ui;
}



void VideoAnalysis::set_video_file(const QString & video)
{
    ui->video_player->set_file (video);
    file_opening_ = true;
    modify_invalid ();
}



void VideoAnalysis::init_video_widget(const json &video_detail)
{
    auto iter_path = video_detail.find ("视频路径");
    assert (iter_path != video_detail.end ());
    assert (iter_path->is_string ());
    std::string path = *iter_path;
    ui->video_player->set_file (path.data ());

    auto iter_invalid = video_detail.find ("无效时间段");
    assert (iter_invalid != video_detail.end ());
    assert (iter_invalid->is_array ());
    auto& invalid = *iter_invalid;
    qint64 time;
    std::vector <qint64> vec;
    vec.reserve (invalid.size ());
    for (unsigned i=0; i < invalid.size (); ++i)
    {
        time = invalid.at (i);
        vec.emplace_back (time);
    }
    ui->video_player->set_invalid (vec);
}

void VideoAnalysis::init_chart()
{
    const auto effic_chart = new QChart;
    const auto op_type_chart = new QChart;

    ui->efficiency_view->setChart (effic_chart);
    ui->operation_type_view->setChart (op_type_chart);

    ui->efficiency_view->setRenderHint (QPainter::Antialiasing);
    ui->operation_type_view->setRenderHint (QPainter::Antialiasing);

    op_type_chart->addSeries (operation_type_);
    effic_chart->addSeries (efficiency_);

    effic_chart->setAnimationOptions (QChart::AllAnimations);
    op_type_chart->setAnimationOptions (QChart::AllAnimations);

    operation_type_->setVerticalPosition (0.55);
    efficiency_->setVerticalPosition (0.55);

    efficiency_->setPieSize (0.5);
    operation_type_->setPieSize (0.5);

    effic_chart->setTitle ("增值/非增值");
    op_type_chart->setTitle ("操作类型");


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

bool VideoAnalysis::eventFilter(QObject *, QEvent *event)
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

void VideoAnalysis::load(const QVariant &data)
{
    const auto totalMap = data.toMap();
    if(totalMap.size() == 0)
    {
        return;
    }

    const auto formMap = totalMap["form"].toMap();
    const auto task = formMap["作业内容"];
    const auto observation = formMap["观测时间"];
    const auto result = formMap["结果"];

    const auto row = task.toList().size();
    if(row <= 0)
    {
        return;
    }

    ui->form->set_row(row);
    ui->form->loadTask(task);
    ui->form->loadData(observation);
    ui->form->loadResult(result);

    const auto file = totalMap["video-file"].toString();
    ui->video_player->set_file(file);

    const auto invalid = totalMap["invalid"].toList();
    invalid_data_.clear();
    invalid_data_.resize(static_cast<size_t>(invalid.size()) );
    const auto vec = invalid.toVector().toStdVector();
    size_t i = 0;
    for(auto & it : vec)
    {
        invalid_data_.at(i) = it.toLongLong();
        i++;
    }
    ui->video_player->set_invalid(invalid_data_);

    const auto measureDate = totalMap["measure-date"].toString();
    ui->measure_date->setText(measureDate);

    const auto measureMan = totalMap["measure-man"].toString();
    ui->measure_man->setText(measureMan);

    const auto taskMan = totalMap["task-man"].toString();
    ui->task_man->setText(taskMan);
}

void VideoAnalysis::on_combo_second_activated(int index)
{
    ui->video_player->set_position(ui->video_player->position() + index);
}

void VideoAnalysis::on_video_player_state_changed(video_player::state_enum state)
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

void VideoAnalysis::on_button_sec_backward_clicked()
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

void VideoAnalysis::on_button_sec_forward_clicked()
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

void VideoAnalysis::on_spinbox_rate_valueChanged(double arg1)
{
    ui->video_player->set_speed(arg1);
}

void VideoAnalysis::modify_invalid ()
{
    ui->video_player->stop_video();
    if (ui->video_player->file ().isEmpty ())
    {
        QMessageBox::information(this,"错误","未载入文件");
        return;
    }

    dlg_->set_current_video(ui->video_player->file ());

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


void VideoAnalysis::on_video_player_stepped_into_invalid(qint64, qint64 pos_out)
{
    ui->video_player->set_position(pos_out);
}

void VideoAnalysis::on_button_mark_clicked()
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

void VideoAnalysis::on_marked(long long msec)
{

    ui->form->mark (msec);
}


void VideoAnalysis::on_button_setting_rows_clicked()
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




void VideoAnalysis::on_paste()
{
    ui->form->on_paste ();
}


void VideoAnalysis::set_task_count ()
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
//        ui->form->clear ();
        ui->form->set_row (row);
    }
}


void VideoAnalysis::on_copy()
{
    ui->form->on_copy ();
}

void VideoAnalysis::on_cut()
{
    ui->form->on_cut ();
}

void VideoAnalysis::on_del()
{
    ui->form->on_del ();
}

void VideoAnalysis::refresh_chart (action_ratio ratio)
{
    {
        const auto slices = operation_type_->slices ();
        {
            auto slice = slices.at (0);
            slice->setLabel ("加工 " + QString::number (ratio.processing, 'f', 1) + "%");
            slice->setValue (ratio.processing);
        }

        {
            auto slice = slices.at (1);
            slice->setLabel ("检查 " + QString::number (ratio.checking, 'f', 1) + "%");
            slice->setValue (ratio.checking);
        }

        {
            auto slice = slices.at (2);
            slice->setLabel ("搬运 " + QString::number (ratio.moving, 'f', 1) + "%");
            slice->setValue (ratio.moving);
        }

        {
            auto slice = slices.at (3);
            slice->setLabel ("等待 " + QString::number (ratio.waiting, 'f', 1) + "%");
            slice->setValue (ratio.waiting);
        }
    }

    {
        const auto slices = efficiency_->slices ();
        {
            auto slice = slices.at (0);
            slice->setLabel ("增值 " + QString::number (ratio.processing, 'f', 1) + "%");
            slice->setValue (ratio.processing);
        }

        {
            auto slice = slices.at (1);
            slice->setLabel ("非增值 " + QString::number (100 - ratio.processing, 'f', 1) + "%");
            slice->setValue (100 - ratio.processing);
        }
    }
}

void VideoAnalysis::refresh_stats(overall_stats stats)
{
    if(stats.ct_val > 0)
    {
        ui->ct_value->setText(QString::number(stats.ct_val, 'f', 2));
    }

}

void VideoAnalysis::set_measure_date(const QDate &date)
{
    ui->measure_date->setText (date.toString ("yyyy-MM-dd"));
}

void VideoAnalysis::set_measure_man(const QString &data)
{
    ui->measure_man->setText (data);
}

QString VideoAnalysis::measure_man() const
{
    return ui->measure_man->text ();
}

void VideoAnalysis::set_task_man(const QString &data)
{
    ui->task_man->setText (data);
}

QString VideoAnalysis::task_man() const
{
    return ui->task_man->text ();
}

QString VideoAnalysis::measure_date() const
{
    return ui->measure_date->text ();
}


QVariant VideoAnalysis::dump()
{
    QVariantMap data;
    std::vector<QVariant> var_vec;
    invalid_data_ | append_to (var_vec);
    const auto invalid_list =
            QVariantList::fromVector(QVector<QVariant>::fromStdVector (var_vec));

    data ["form"] = ui->form->dump();
    data ["video-file"] = ui->video_player->file ();
    data ["invalid"] = invalid_list;
    data ["measure-date"] = ui->measure_date->text ();
    data ["measure-man"] = ui->measure_man->text ();
    data ["task-man"] = ui->task_man->text ();
    return data;
}


