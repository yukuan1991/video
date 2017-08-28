#include "video/video_main.h"
#include "video/video_analysis.h"
#include "ui_video_main.h"
#include <QMdiSubWindow>
#include <QStyle>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <functional>
#include <QMessageBox>
#include <base/io/file/file.hpp>
#include <QPixmap>
#include <base/utils/charset.hpp>
#include <QtXlsx>
#include <QDateEdit>
#include <QStyleFactory>
#include <QInputDialog>
#include <base/lang/not_null.h>
#include <QInputDialog>

using namespace std;

video_main::video_main(QWidget *parent)
    :QWidget(parent)
    ,ui(new Ui::video_main)
{
    ui->setupUi(this);
    ui->mdi->setViewMode (QMdiArea::TabbedView);
    init_conn ();
    setStyle (QStyleFactory::create ("fusion"));
}

video_main::~video_main()
{
    delete ui;
}

QMdiArea *video_main::area()
{
    return ui->mdi;
}

not_null<video_analysis *> video_main::create_window(const QString &title)
{
    auto v_win = make_unique<video_analysis> ();
    v_win->setAttribute (Qt::WA_DeleteOnClose);
    auto w = ui->mdi->addSubWindow (v_win.get ());
    w->setWindowTitle (title);

    w->setWindowState (Qt::WindowMaximized);
    return v_win.release ();
}

void video_main::create_analysis()
{
    auto w = create_window ("未命名");
    w->set_task_count ();
}

video_analysis *video_main::current_sub_window ()
{
    const auto active = ui->mdi->currentSubWindow ();
    if (active == null)
    {
        return null;
    }
    auto w = dynamic_cast<video_analysis *> (active->widget ());

    return w;
}

void video_main::mdi_changed(QMdiSubWindow * window)
{
    ui->video_ribbon->mdi_active (window != null);
}

void video_main::apply_to_current(analysis_slot slot)
{
    auto w = current_sub_window ();
    if (w != null)
    {
        (w->*slot) ();
    }
}

void video_main::invalid_timespan()
{
    auto w = current_sub_window ();
    if (w != null)
    {
        w->modify_invalid ();
    }
}

void video_main::on_measure_date()
{
    auto w = current_sub_window ();
    if (w == null)
    {
        return;
    }

    QDialog dlg (this);

    auto edit = new QDateEdit (&dlg);
    edit->setCalendarPopup (true);
    edit->setDate (QDate::currentDate ());

    auto ok_button = new QPushButton (&dlg);
    ok_button->setText ("确定");

    auto layout = new QHBoxLayout;

    layout->addWidget (edit);
    layout->addWidget (ok_button);
    dlg.setLayout (layout);

    connect (ok_button, &QPushButton::clicked, &dlg, &QDialog::accept);
    const auto res = dlg.exec ();

    if (res != QDialog::Accepted)
    {
        return;
    }

    w->set_measure_date (edit->date ());
}

void video_main::on_measure_man()
{
    auto w = current_sub_window ();
    if (w == null)
    {
        return;
    }

    bool is_ok;
    const auto old_data = w->measure_man ();
    const auto data = QInputDialog::getText (this, "测量人", "测量人", QLineEdit::Normal, old_data, &is_ok);
    if (is_ok)
    {
        w->set_measure_man (data);
    }
}

void video_main::on_task_man()
{
    auto w = current_sub_window ();
    if (w == null)
    {
        return;
    }

    bool is_ok;
    const auto old_data = w->task_man ();
    const auto data = QInputDialog::getText (this, "作业员", "作业员", QLineEdit::Normal, old_data, &is_ok);

    if (is_ok)
    {
        w->set_task_man (data);
    }
}

void video_main::video_import()
{
    const QString type = tr ("Video Files (*.mp4 *.mpg *.mod *.mov *.mkv *.wmv *.avi *.vid)");
    const auto file = QFileDialog::getOpenFileName (this, "打开视频", ".", type);
    if (file.isEmpty ())
    {
        return;
    }

    QFileInfo info (file);
    const auto src_name = info.fileName ();
    QDir video_dir (".");

    if (not video_dir.mkpath ("video_data"))
    {
        QMessageBox::information (this, "导入", "无法导入视频,数据路径创建失败");
        return;
    }

    const auto dest_path = "video_data/" + src_name;

    if (QFile::exists (dest_path))
    {
        QFile::remove (dest_path);
    }

    if (not QFile::copy (file, dest_path))
    {
        QMessageBox::information (this, "导入", "无法导入视频,拷贝文件失败");
        return;
    }

    auto w = current_sub_window ();

    if (w == null)
    {
        return;
    }

    w->set_video_file (dest_path);
}

void video_main::init_conn()
{
    connect (ui->mdi, &QMdiArea::subWindowActivated, this, &video_main::mdi_changed);
    connect (ui->video_ribbon, &ribbon::create_new, this, &video_main::create_analysis);
    connect (ui->video_ribbon, &ribbon::import_data, this, &video_main::video_import);
    connect (ui->video_ribbon, &ribbon::change_task_count, [this] { apply_to_current (&video_analysis::set_task_count); });
    connect (ui->video_ribbon, &ribbon::invalid_timespan, [this] { apply_to_current (&video_analysis::modify_invalid); });
    connect (ui->video_ribbon, &ribbon::paste, [this] { apply_to_current (&video_analysis::on_paste); });
    connect (ui->video_ribbon, &ribbon::save, this, &video_main::on_save);
    connect (ui->video_ribbon, &ribbon::save_as, this, &video_main::on_save_as);
    connect (ui->video_ribbon, &ribbon::open, this, &video_main::on_open);
    connect (ui->video_ribbon, &ribbon::quit, this, &video_main::close);
    connect (ui->video_ribbon, &ribbon::export_data, this, &video_main::export_xlsx);
    connect (ui->video_ribbon, &ribbon::measure_date, this, &video_main::on_measure_date);
    connect (ui->video_ribbon, &ribbon::measure_man, this, &video_main::on_measure_man);
    connect (ui->video_ribbon, &ribbon::task_man, this, &video_main::on_task_man);
    connect (ui->video_ribbon, &ribbon::change_example_cycle, this, &video_main::on_example_cycle);
}

void video_main::change_task_count()
{
    auto w = current_sub_window ();
    if (w != null)
    {
        w->set_task_count ();
    }
}

void video_main::export_xlsx()
{
    auto w = current_sub_window ();
    if (w == null)
    {
        return;
    }

    const auto path = QFileDialog::getSaveFileName(this, "导出", ".", tr ("Video Analysis File (*.xlsx)"));
    if (path.isEmpty ())
    {
        return;
    }

    const auto data = w->dump ();
    const auto form = data.find ("form");
    assert (form != end (data));
    QXlsx::Document xlsx (path);
    const auto task_list = form->find ("作业内容");
    assert (task_list != end (*form));
    assert (task_list->is_array ());

    const auto measure_data = form->find ("观测时间");
    assert (measure_data != end (*form));
    assert (measure_data->is_array ());

    const auto result_data = form->find ("结果");
    assert (result_data != end (*form));
    assert (result_data->is_array ());

    xlsx.write (1, 1, "编号");
    xlsx.write (1, 2, "作业内容");

    for (size_t i = 0; i < task_list->size (); i ++)
    {
        auto s_i = static_cast<int> (i);
        const string task_name = task_list->at (i);
        xlsx.write (s_i + 2, 1, QString::number (s_i + 1));
        xlsx.write (s_i + 2, 2, QString::fromStdString (task_name));
    }

    constexpr size_t data_start = 5;

    for (size_t i = 0; i < measure_data->size (); i ++)
    {
        xlsx.write (1, static_cast<int> (i * 2 + data_start + 0), QString::number (i + 1) + "T");
        xlsx.write (1, static_cast<int> (i * 2 + data_start + 1), QString::number (i + 1) + "R");
        for (size_t j = 0; j < measure_data->at (i).size (); j ++)
        {
            const auto & cell = measure_data->at (i).at (j);
            assert (cell.is_object ());
            auto t = cell.find ("T"); assert (t != end (cell) and t->is_number_float ());
            auto r = cell.find ("R"); assert (r != end (cell) and r->is_number_float ());
            qreal value_t = *t;
            qreal value_r = *r;

            if (fabs (value_t) > 0.00000001)
            {
                xlsx.write (static_cast<int> (j + 2), static_cast<int> (i * 2 + data_start + 0), value_t);
            }
            if (fabs (value_r) > 0.00000001)
            {
                xlsx.write (static_cast<int> (j + 2), static_cast<int> (i * 2 + data_start + 1), value_r);
            }
        }
    }
    const auto start_result = static_cast<int> (data_start + measure_data->size ()  * 2 + 2);

    xlsx.write (1, start_result + 0, "平均时间");
    xlsx.write (1, start_result + 1, "评比系数");
    xlsx.write (1, start_result + 2, "基本时间");
    xlsx.write (1, start_result + 3, "宽放率");
    xlsx.write (1, start_result + 4, "标准时间");
    xlsx.write (1, start_result + 5, "增值/非增值");
    xlsx.write (1, start_result + 6, "操作分类");
    for (int i = 0; i < static_cast<int> (result_data->size ()); i ++)
    {
        const auto unsigned_i = static_cast<size_t> (i);
        const auto index = result_data->at (unsigned_i);
        const auto average = index.find ("平均时间"); assert (average != end (index) and average->is_number_float ());
        const qreal val_average = *average;

        const auto coefficient = index.find ("评比系数"); assert (coefficient != end (index) and coefficient->is_number_float ());
        const qreal val_co = *coefficient;

        const auto base = index.find ("基本时间"); assert (base != end (index) and base->is_number_float ());
        const qreal val_base = *base;

        const auto tolerance = index.find ("宽放率"); assert (tolerance != end (index) and tolerance->is_string ());
        const string val_tolerance = *tolerance;

        const auto std = index.find ("标准时间"); assert (std != end (index) and std->is_number_float ());
        const qreal val_std = *std;

        const auto useful = index.find ("增值/非增值"); assert (useful != end (index) and useful->is_string ());
        const string val_useful = *useful;

        const auto type = index.find ("操作分类"); assert (type != end (index) and type->is_string ());
        const string val_type = *type;

        xlsx.write (i + 2, start_result + 0, val_average);
        xlsx.write (i + 2, start_result + 1, val_co);
        xlsx.write (i + 2, start_result + 2, val_base);
        xlsx.write (i + 2, start_result + 3, val_tolerance.data ());
        xlsx.write (i + 2, start_result + 4, val_std);
        xlsx.write (i + 2, start_result + 5, val_useful.data ());
        xlsx.write (i + 2, start_result + 6, val_type.data ());
    }


    const auto date = data.find ("measure-date");
    assert (date != end (data) and date->is_string ());
    xlsx.write (static_cast<int> (task_list->size () + 3), 1, "测量日期");
    xlsx.write (static_cast<int> (task_list->size () + 3), 2, QString::fromStdString (*date));

    const auto measure_man = data.find ("measure-man");
    assert (measure_man != end (data) and measure_man->is_string ());
    xlsx.write (static_cast<int> (task_list->size () + 3 + 1), 1, "测量人");
    xlsx.write (static_cast<int> (task_list->size () + 3 + 1), 2, QString::fromStdString (*measure_man));

    const auto task_man = data.find ("task-man");
    assert (task_man != end (data) and task_man->is_string ());
    xlsx.write (static_cast<int> (task_list->size () + 3 + 2), 1, "作业员");
    xlsx.write (static_cast<int> (task_list->size () + 3 + 2), 2, QString::fromStdString (*task_man));

    if (QFile::exists (path))
    {
        QFile::remove (path);
    }

    if (xlsx.save ())
    {
        QMessageBox::information (this, "导出Excel", "已导出");
    }
    else
    {
        QMessageBox::information (this, "导出Excel", "导出失败 请确认文件没有被其他程序锁定");
    }
}

void video_main::on_save()
{
    const auto active = ui->mdi->currentSubWindow ();
    if (active == null)
    {
        return;
    }
    auto w = dynamic_cast<video_analysis *> (active->widget ());

    if (w == null)
    {
        return;
    }

    if (const auto title_path = active->windowTitle ();
            title_path == "未命名")
    {
        const auto path = QFileDialog::getSaveFileName(this, "文件保存", ".", tr ("Video Analysis File (*.vaf)"));
        const auto data = w->dump ();

        file::write_buffer (::utf_to_sys (path.toStdString ()).data (), data.dump (4));
    }
    else
    {
        const auto data = w->dump ();
        file::write_buffer (::utf_to_sys (title_path.toStdString ()).data (), data.dump (4));
    }

}

void video_main::on_open()
{
    const auto path = QFileDialog::getOpenFileName (this, "文件打开", ".", tr ("Video Analysis File (*.vaf)"));
    if (path.isEmpty ())
    {
        return;
    }

    auto res = file::read_all (::utf_to_sys (path.toStdString ()).data ());
    if (not res)
    {
        QMessageBox::information (this, "打开", "文件无法打开,读取失败");
        return;
    }
    try
    {
        const auto data = json::parse (res.value ());
        auto w = create_window (path);
        w->load (data);
    }
    catch (std::exception &)
    {
        QMessageBox::information (this, "打开", "文件格式错误 无法打开");
        return;
    }
}

void video_main::on_save_as()
{
    auto w = current_sub_window ();
    if (w != null)
    {
        const auto path = QFileDialog::getSaveFileName(this, "文件保存", ".", tr ("Video Analysis File (*.vaf)"));
        const auto data = w->dump ();

        file::write_buffer (::utf_to_sys (path.toStdString ()).data (), data.dump (4));
    }
}

void video_main::on_example_cycle()
{
    auto w = current_sub_window ();
    if (w == null)
    {
        return;
    }

    const auto old_cycle = w->example_cycle ();
    const auto cycle = QInputDialog::getInt (this, "实例循环", "设置示例循环", old_cycle, 1, 10);

    if (cycle == w->example_cycle ())
    {
        return;
    }

    w->set_example_cycle (cycle);
}
