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
#include <QStyleFactory>
#include <base/io/file/file.hpp>


using namespace QtCharts;
using namespace std;

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
    connect (ui->form, &form_widget::data_changed, [this]
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
        update_box (cycles);
    });

    set_children_filter (this);
    ui->button_mark->setIcon (QIcon ("icon/mark.png"));

    this->setStyle (QStyleFactory::create ("fusion"));

    init_chart ();
}

video_analysis::~video_analysis()
{
    delete ui;
}



void video_analysis::set_video_file(const QString & video)
{
    ui->video_player->set_file (video);
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

void video_analysis::init_chart()
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

void video_analysis::load (const json &data)
{
    const auto iter_form = data.find ("form");
    assert (iter_form != end (data));
    assert (iter_form->is_object ());
    auto iter_task = iter_form->find ("作业内容");
    assert (iter_task != iter_form->end ());
    assert (iter_task->is_array ());
    auto iter_data = iter_form->find ("观测时间");
    assert (iter_data != iter_form->end () and iter_data->is_array ());
    auto iter_result = iter_form->find ("结果");
    assert (iter_result != iter_form->end () and iter_result->is_array ());



    ui->form->set_row (static_cast<int> (iter_task->size ()));
    ui->form->load_task (*iter_task);
    ui->form->load_data (*iter_data);
    ui->form->load_result (*iter_result);

    const auto file = data.find ("video-file");
    assert (iter_form != end (data));
    assert (file->is_string ());
    ui->video_player->set_file (QString::fromStdString (*file));
    const auto invalid = data.find ("invalid");

    if (invalid != end (data))
    {
        assert (invalid->is_array ());
        invalid_data_.clear ();
        invalid_data_.resize (invalid->size ());

        size_t i = 0;
        for (auto & it : invalid.value ())
        {
            invalid_data_.at (i) = it;
            i ++;
        }
        ui->video_player->set_invalid(invalid_data_);
    }

    const auto measure_date = data.find ("measure-date");
    if (measure_date != end (data) and measure_date->is_string ())
    {
        ui->measure_date->setText (QString::fromStdString (*measure_date));
    }
    const auto measure_man = data.find ("measure-man");
    if (measure_man != end (data) and measure_man->is_string ())
    {
        ui->measure_man->setText (QString::fromStdString (*measure_man));
    }
    const auto task_man = data.find ("task-man");
    if (task_man != end (data) and task_man->is_string ())
    {
        ui->task_man->setText (QString::fromStdString (*task_man));
    }

    const auto example_cycle = data.find ("example-cycle");
    if (example_cycle != end (data) and example_cycle->is_number () and *example_cycle != 0)
    {
        const auto i_example_cycle = int (*example_cycle);
        ui->example_cycle->setText (QString::number (i_example_cycle));
    }
    else
    {
        ui->example_cycle->setText ("无");
    }
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

void video_analysis::refresh_stats(overall_stats stats)
{
    ui->ct_max->setText (QString::number (stats.max_val, 'f', 2));
    ui->ct_min->setText (QString::number (stats.min_val, 'f', 2));
    ui->ct_average->setText (QString::number (stats.average, 'f', 2));
    ui->ct_deviation->setText (QString::number (stats.deviation, 'f', 2));
}

void video_analysis::update_box(gsl::span<qreal> data)
{
    if (data.size () <= 3)
    {
        return;
    }
    sort (begin (data), end (data), greater <>());

    whisker_data wh_data;
    wh_data.top = data.at (0);
    wh_data.bottom = data.at (data.size () - 1);
    if (data.size () % 2 == 0)
    {
        wh_data.mid = (data.at (data.size () / 2) + data.at (data.size () / 2 + 1)) / 2;
        const auto size = data.size () / 2 + 1;
        if (size % 2 == 0)
        {
            wh_data.bottom_quarter = (data.at (data.size () / 2 + size / 2 - 1) + data.at (data.size () / 2 + size / 2)) / 2;
            wh_data.top_quarter = (data.at (size / 2 - 1) + data.at (data.size () / 2)) / 2;
        }
        else
        {
            wh_data.bottom_quarter = data.at (data.size () / 2 + size / 2);
            wh_data.top_quarter = data.at (size / 2);
        }
    }
    else
    {
        wh_data.mid = data.at (data.size () / 2);
        const auto size = data.size () / 2 + 1;
        if (size % 2 == 0)
        {
            wh_data.bottom_quarter = (data.at (data.size () / 2 + size / 2 - 1) + data.at (data.size () / 2 + size / 2)) / 2;
            wh_data.top_quarter = (data.at (size / 2 - 1) + data.at (data.size () / 2)) / 2;
        }
        else
        {
            wh_data.bottom_quarter = data.at (data.size () / 2 + size / 2);
            wh_data.top_quarter = data.at (size / 2);
        }
    }

    ui->upper_quarter->setText (QString::number (wh_data.top_quarter, 'f', 2));
    ui->lower_quarter->setText (QString::number (wh_data.bottom_quarter, 'f', 2));
    ui->mid->setText (QString::number (wh_data.mid, 'f', 2));
    ui->wh_chart->set_data (wh_data);
}

void video_analysis::set_measure_date(const QDate &date)
{
    ui->measure_date->setText (date.toString ("yyyy-MM-dd"));
}

void video_analysis::set_measure_man(const QString &data)
{
    ui->measure_man->setText (data);
}

QString video_analysis::measure_man() const
{
    return ui->measure_man->text ();
}

void video_analysis::set_task_man(const QString &data)
{
    ui->task_man->setText (data);
}

QString video_analysis::task_man() const
{
    return ui->task_man->text ();
}

QString video_analysis::measure_date() const
{
    return ui->measure_date->text ();
}

json video_analysis::dump()
{
    json data;
    data ["form"] = ui->form->export_data ();
    data ["video-file"] = ui->video_player->file ().toStdString ();
    data ["invalid"] = invalid_data_;
    data ["measure-date"] = ui->measure_date->text ().toStdString ();
    data ["measure-man"] = ui->measure_man->text ().toStdString ();
    data ["task-man"] = ui->task_man->text ().toStdString ();
    data ["example-cycle"] = ui->example_cycle->text ().toInt ();

    return data;
}

void video_analysis::set_example_cycle(int cycle)
{
    ui->example_cycle->setText (QString::number (cycle));
}

int video_analysis::example_cycle() const noexcept
{
    return ui->example_cycle->text ().toInt ();
}

