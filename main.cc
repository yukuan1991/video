#include <QApplication>
#include "video/ribbon.h"
#include <variant>
#include <memory>
#include <QFile>
#include <base/io/file/file.hpp>
#include <QComboBox>
#include <boost/filesystem.hpp>
#include <boost/range/adaptors.hpp>
#include "video/VideoMainTrial.h"
#include <QPixmap>
#include <QStyle>
#include <QIcon>
#include <QMdiArea>
#include <QWidget>
#include <QMdiArea>
#include "video/VideoAnalysis.h"
#include <QMdiSubWindow>
#include "verification.h"
#include "krys_application.hpp"
#include <chrono>
#include <QTimer>


#include <QTableView>
#include "video/VideoFormModel.h"

using namespace std::chrono_literals;
using namespace std::string_view_literals;


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

    qApp->setFont (QFont ("宋体", 11));
    qApp->setStyleSheet (QString::fromStdString (qss));
}




int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    VideoMainTrial v;
    v.resize (1366, 768);
    v.setWindowIcon (QPixmap ("icon.ico"));
    v.show ();

    set_style ();

    return a.exec();
}
