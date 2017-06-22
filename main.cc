﻿#include <QApplication>
#include "video/ribbon.h"
#include <variant>
#include <memory>
#include <QFile>
#include <base/io/file/file.hpp>
#include <QComboBox>
#include <boost/filesystem.hpp>
#include <boost/range/adaptors.hpp>
#include "video/video_main.h"
#include <QPixmap>
#include <QStyle>
#include <QIcon>
#include <QMdiArea>
#include <QWidget>
#include <QMdiArea>
#include "video/video_analysis.h"
#include <QMdiSubWindow>


void set_style ()
{
    using namespace boost::filesystem;

    auto rng = boost::make_iterator_range (directory_iterator ("."), directory_iterator ());

    std::string qss;
    for (auto & it : rng)
    {
        if (it.path ().extension ().string () == ".css")
        {
            auto res = file::read_all (it.path ().string ().data ());
            if (res)
            {
                qss += res.value ();
            }
        }
    }

    qApp->setStyleSheet (QString::fromStdString (qss));
}




int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    set_style ();

    video_main v;
    v.resize (1366, 768);
    v.setWindowIcon (QPixmap ("icon.ico"));
    v.show ();
    auto area = v.area ();

    for (auto & active : area->subWindowList ())
    {
        active->setWindowFilePath ("未命名");
        active->setWindowIcon (v.style ()->standardIcon (QStyle::SP_FileIcon));
    }

    return a.exec();
}
