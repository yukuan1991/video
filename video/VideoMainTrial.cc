#include "video/VideoMainTrial.h"
#include "video/VideoAnalysis.h"
#include "ui_VideoMainTrial.h"
#include <QMdiSubWindow>
#include <QStyle>
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
#include <QJsonDocument>
#include <QJsonParseError>

using namespace std;

VideoMainTrial::VideoMainTrial(QWidget *parent)
    :QWidget(parent)
    ,ui(new Ui::VideoMainTrial)
{
    ui->setupUi(this);
    ui->mdi->setViewMode (QMdiArea::TabbedView);
    init_conn ();
    setStyle (QStyleFactory::create ("fusion"));
}

VideoMainTrial::~VideoMainTrial()
{
    delete ui;
}

QMdiArea *VideoMainTrial::area()
{
    return ui->mdi;
}

not_null<VideoAnalysis *> VideoMainTrial::create_window(const QString &title)
{
    auto v_win = make_unique<VideoAnalysis> ();
    v_win->setAttribute (Qt::WA_DeleteOnClose);
    auto w = ui->mdi->addSubWindow (v_win.get ());
    w->setWindowTitle (title);

    w->setWindowState (Qt::WindowMaximized);
    return v_win.release ();
}

void VideoMainTrial::create_analysis()
{
    auto w = create_window ("未命名");
    w->set_task_count ();
}

VideoAnalysis *VideoMainTrial::current_sub_window ()
{
    const auto active = ui->mdi->currentSubWindow ();
    if (active == null)
    {
        return null;
    }
    auto w = dynamic_cast<VideoAnalysis *> (active->widget ());

    return w;
}

void VideoMainTrial::mdi_changed(QMdiSubWindow * window)
{
    ui->video_ribbon->mdi_active (window != null);
}

void VideoMainTrial::apply_to_current(analysis_slot slot)
{
    auto w = current_sub_window ();
    if (w != null)
    {
        (w->*slot) ();
    }
}

void VideoMainTrial::invalid_timespan()
{
    auto w = current_sub_window ();
    if (w != null)
    {
        w->modify_invalid ();
    }
}

void VideoMainTrial::on_measure_date()
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

void VideoMainTrial::on_measure_man()
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

void VideoMainTrial::on_task_man()
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

void VideoMainTrial::video_import()
{
//    const QString type = tr ("Video Files (*.mp4 *.mpg *.mod *.mov *.mkv *.wmv *.avi *.vid)");
//    const auto file = QFileDialog::getOpenFileName (this, "打开视频", ".", type);
//    if (file.isEmpty ())
//    {
//        return;
//    }

//    QFileInfo info (file);
//    const auto src_name = info.fileName ();
//    QDir video_dir (".");

//    if (not video_dir.mkpath ("video_data"))
//    {
//        QMessageBox::information (this, "导入", "无法导入视频,数据路径创建失败");
//        return;
//    }

//    const auto dest_path = "video_data/" + src_name;

//    if (QFile::exists (dest_path))
//    {
//        QFile::remove (dest_path);
//    }

//    QMessageBox::information (this, "xxx", "去看看删了没有");

//    if (not QFile::copy (file, dest_path))
//    {
//        QMessageBox::information (this, "导入", "无法导入视频,拷贝文件失败");
//        return;
//    }

    auto w = current_sub_window ();

    if (w == null)
    {
        return;
    }

    w->set_video_file ("video_data/1.mp4");
}

void VideoMainTrial::init_conn()
{
    connect (ui->mdi, &QMdiArea::subWindowActivated, this, &VideoMainTrial::mdi_changed);
    connect (ui->video_ribbon, &ribbon::create_new, this, &VideoMainTrial::create_analysis);
    connect (ui->video_ribbon, &ribbon::import_data, this, &VideoMainTrial::video_import);
    connect (ui->video_ribbon, &ribbon::change_task_count, [this] { apply_to_current (&VideoAnalysis::set_task_count); });
    connect (ui->video_ribbon, &ribbon::invalid_timespan, [this] { apply_to_current (&VideoAnalysis::modify_invalid); });
    connect (ui->video_ribbon, &ribbon::paste, [this] { apply_to_current (&VideoAnalysis::on_paste); });
    connect (ui->video_ribbon, &ribbon::save, this, &VideoMainTrial::on_save);
    connect (ui->video_ribbon, &ribbon::save_as, this, &VideoMainTrial::on_save_as);
    connect (ui->video_ribbon, &ribbon::open, this, &VideoMainTrial::on_open);
    connect (ui->video_ribbon, &ribbon::quit, this, &VideoMainTrial::close);
    connect (ui->video_ribbon, &ribbon::export_data, this, &VideoMainTrial::exportXlsx);
    connect (ui->video_ribbon, &ribbon::measure_date, this, &VideoMainTrial::on_measure_date);
    connect (ui->video_ribbon, &ribbon::measure_man, this, &VideoMainTrial::on_measure_man);
    connect (ui->video_ribbon, &ribbon::task_man, this, &VideoMainTrial::on_task_man);
//    connect (ui->video_ribbon, &ribbon::change_example_cycle, this, &VideoMainTrial::on_example_cycle);
}

void VideoMainTrial::change_task_count()
{
    auto w = current_sub_window ();
    if (w != null)
    {
        w->set_task_count ();
    }
}

void VideoMainTrial::exportXlsx()
{
    auto w = current_sub_window();
    if (w == null)
    {
        return;
    }

    const auto path = QFileDialog::getSaveFileName(this, "导出", ".", tr ("Video Analysis File (*.xlsx)"));
    if (path.isEmpty ())
    {
        return;
    }

    const auto totalMap = w->dump().toMap();
    if(totalMap.empty())
    {
        return;
    }

    QXlsx::Document xlsx (path);
    const auto formMap = totalMap["form"].toMap();
    if(formMap.empty())
    {
        return;
    }

    const auto taskList = formMap["作业内容"].toList();
    if(taskList.size() == 0)
    {
        return;
    }

    const auto observationList = formMap["观测时间"].toList();
    if(observationList.size() == 0)
    {
        return;
    }

    const auto resultList = formMap["结果"].toList();
    if(resultList.size() == 0)
    {
        return;
    }

    xlsx.write (1, 1, "编号");
    xlsx.write (1, 2, "作业内容");

    for (int i = 0; i < taskList.size (); i ++)
    {
        auto s_i = static_cast<int> (i);
        const auto taskName = taskList.at (i).toString();
        xlsx.write (s_i + 2, 1, QString::number (s_i + 1));
        xlsx.write (s_i + 2, 2, taskName);
    }

    constexpr int dataStart = 5;
    for (int i = 0; i < observationList.size (); i ++)
    {
        xlsx.write (1, static_cast<int> (i * 2 + dataStart + 0), QString::number (i + 1) + "T");
        xlsx.write (1, static_cast<int> (i * 2 + dataStart + 1), QString::number (i + 1) + "R");

        const auto list = observationList.at(i).toList();

        for (int j = 0; j < list.size (); j ++)
        {
            const auto map = list.at(j).toMap();
            const auto cellT = map["T"].toDouble();
            const auto cellR = map["R"].toDouble();

            if (fabs (cellT) > 0.00000001)
            {
                xlsx.write (static_cast<int> (j + 2), static_cast<int> (i * 2 + dataStart + 0), cellT);
            }
            if (fabs (cellR) > 0.00000001)
            {
                xlsx.write (static_cast<int> (j + 2), static_cast<int> (i * 2 + dataStart + 1), cellR);
            }
        }

        const auto start_result = static_cast<int> (dataStart + observationList.size ()  * 2 + 2);

        xlsx.write (1, start_result + 0, "平均时间");
        xlsx.write (1, start_result + 1, "评比系数");
        xlsx.write (1, start_result + 2, "基本时间");
        xlsx.write (1, start_result + 3, "宽放率");
        xlsx.write (1, start_result + 4, "标准时间");
        xlsx.write (1, start_result + 5, "增值/非增值");
        xlsx.write (1, start_result + 6, "操作类型");

        for (int i = 0; i < static_cast<int> (resultList.size ()); i ++)
        {
            const auto averageTime = resultList.at(i).toMap()["平均时间"].toDouble();
            const auto comparsion = resultList.at(i).toMap()["评比系数"].toDouble();
            const auto basicTime = resultList.at(i).toMap()["基本时间"].toDouble();
            const auto rate = resultList.at(i).toMap()["宽放率"].toString();
            const auto stdTime = resultList.at(i).toMap()["标准时间"].toDouble();
            const auto appreciation = resultList.at(i).toMap()["增值/非增值"].toString();
            const auto type = resultList.at(i).toMap()["操作类型"].toString();

            xlsx.write (i + 2, start_result + 0, averageTime);
            xlsx.write (i + 2, start_result + 1, comparsion);
            xlsx.write (i + 2, start_result + 2, basicTime);
            xlsx.write (i + 2, start_result + 3, rate);
            xlsx.write (i + 2, start_result + 4, stdTime);
            xlsx.write (i + 2, start_result + 5, appreciation);
            xlsx.write (i + 2, start_result + 6, type);
        }

    }

    const auto date = totalMap["measure-date"].toString();
    xlsx.write (static_cast<int> (taskList.size () + 3), 1, "测量日期");
    xlsx.write (static_cast<int> (taskList.size () + 3), 2, date);

    const auto measure_man = totalMap["measure-man"].toString();
    xlsx.write (static_cast<int> (taskList.size () + 3 + 1), 1, "测量人");
    xlsx.write (static_cast<int> (taskList.size () + 3 + 1), 2, measure_man);

    const auto task_man = totalMap["task-man"].toString();
    xlsx.write (static_cast<int> (taskList.size () + 3 + 2), 1, "作业员");
    xlsx.write (static_cast<int> (taskList.size () + 3 + 2), 2, task_man);

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

void VideoMainTrial::on_save()
{
    const auto active = ui->mdi->currentSubWindow ();
    if (active == null)
    {
        return;
    }
    auto w = dynamic_cast<VideoAnalysis *> (active->widget ());

    if (w == null)
    {
        return;
    }

    if (const auto title_path = active->windowTitle ();
            title_path == "未命名")
    {
        const auto path = QFileDialog::getSaveFileName(this, "文件保存", ".", tr ("Video Analysis File (*.vaf)"));
        const auto dumpData = w->dump();
        QJsonDocument document = QJsonDocument::fromVariant(dumpData);
        const auto text = document.toJson();
        const auto data = QString(text).toStdString();

        file::write_buffer (::utf_to_sys (path.toStdString ()).data (), data);
    }
    else
    {
        const auto dumpData = w->dump();
        QJsonDocument document = QJsonDocument::fromVariant(dumpData);
        const auto text = document.toJson();
        const auto data = QString(text).toStdString();
        file::write_buffer (::utf_to_sys (title_path.toStdString ()).data (), data);
    }

}

void VideoMainTrial::on_open()
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
        QJsonParseError jsonError;
        const auto text = QString(res.value().data());
        QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &jsonError);
        if(jsonError.error == QJsonParseError::NoError)
        {
            const auto data = document.toVariant();
            auto w = create_window (path);
//            w->load (data);
            w->load(data);
        }
    }
    catch (std::exception &)
    {
        QMessageBox::information (this, "打开", "文件格式错误 无法打开");
        return;
    }
}

void VideoMainTrial::on_save_as()
{
    auto w = current_sub_window ();
    if (w != null)
    {
        const auto path = QFileDialog::getSaveFileName(this, "文件保存", ".", tr ("Video Analysis File (*.vaf)"));
        const auto dumpData = w->dump();
        QJsonDocument document = QJsonDocument::fromVariant(dumpData);
        const auto text = document.toJson();
        const auto data = QString(text).toStdString();

        file::write_buffer (::utf_to_sys (path.toStdString ()).data (), data);
    }
}


