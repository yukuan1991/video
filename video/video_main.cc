#include "video/video_main.h"
#include "video/video_analysis.h"
#include "ui_video_main.h"
#include <QMdiSubWindow>
#include <QStyle>
#include <QDebug>
#include <QFileDialog>
#include <functional>


using namespace std;

video_main::video_main(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::video_main)
{
    ui->setupUi(this);
    ui->mdi->setViewMode (QMdiArea::TabbedView);

    init_conn ();
}

video_main::~video_main()
{
    delete ui;
}

QMdiArea *video_main::area()
{
    return ui->mdi;
}

not_null<video_analysis *> video_main::create_window()
{
    auto v_win = make_unique<video_analysis> ();
    v_win->setAttribute (Qt::WA_DeleteOnClose);
    auto w = ui->mdi->addSubWindow (v_win.get ());

    w->setWindowState (Qt::WindowMaximized);
    return v_win.release ();
}

void video_main::create_analysis()
{
    auto w = create_window ();
}

video_analysis *video_main::current_sub_window ()
{
    const auto active = ui->mdi->currentSubWindow ();
    if (active == nullptr)
    {
        return nullptr;
    }
    auto w = dynamic_cast<video_analysis *> (active->widget ());

    return w;
}

void video_main::apply_to_current(analysis_slot slot)
{
    auto w = current_sub_window ();
    if (w != nullptr)
    {
        (w->*slot) ();
    }
}

void video_main::invalid_timespan()
{
    auto w = current_sub_window ();
    if (w != nullptr)
    {
        w->modify_invalid ();
    }
}

void video_main::video_import()
{
    const QString type = tr ("Video Files (*.mp4 *.mpg *.mod *.mov *.mkv *.wmv *.avi)");
    const auto file = QFileDialog::getOpenFileName (this, "打开视频", ".", type);
    if (file.isEmpty ())
    {
        return;
    }

    auto w = current_sub_window ();

    if (w == nullptr)
    {
        return;
    }

    w->set_video_file (file);
}

void video_main::init_conn()
{
    connect (ui->video_ribbon, &ribbon::create_new, this, &video_main::create_analysis);
    connect (ui->video_ribbon, &ribbon::import_data, this, &video_main::video_import);
    //connect (ui->video_ribbon, &ribbon::change_task_count, this, &video_main::change_task_count);
    connect (ui->video_ribbon, &ribbon::change_task_count, [this] { apply_to_current (&video_analysis::set_task_count); });
    connect (ui->video_ribbon, &ribbon::invalid_timespan, [this] { apply_to_current (&video_analysis::modify_invalid); });
    connect (ui->video_ribbon, &ribbon::paste, [this] { apply_to_current (&video_analysis::on_paste); });
}

void video_main::change_task_count()
{
    auto w = current_sub_window ();
    if (w != nullptr)
    {
        w->set_task_count ();
    }
}
