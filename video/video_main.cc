#include "video/video_main.h"
#include "video/video_analysis.h"
#include "ui_video_main.h"
#include <QMdiSubWindow>
#include <QStyle>
#include <QDebug>
#include <QFileDialog>

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

void video_main::video_import()
{
    const QString type = tr ("Video Files (*.mp4 *.mpg *.mod *.mov *.mkv *.wmv *.avi)");
    const auto file = QFileDialog::getOpenFileName (this, "打开视频", ".", type);
    if (file.isEmpty ())
    {
        return;
    }

    const auto active = ui->mdi->activeSubWindow ();
    if (active == nullptr)
    {
        return;
    }
    auto w = dynamic_cast<video_analysis *> (active->widget ());

    if (w == nullptr)
    {
        return;
    }



}

void video_main::init_conn()
{
    connect (ui->video_ribbon, &ribbon::create_new, this, &video_main::create_analysis);
    connect (ui->video_ribbon, &ribbon::import_data, this, &video_main::video_import);
}
