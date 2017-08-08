#pragma once
#include <QTabWidget>
#include <base/lang/not_null.h>
#include <memory>
#include <QToolButton>
#include <QPushButton>
#include <QTabBar>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyleOption>
#include <QLineEdit>
#include <QPainter>
#include <QMenu>


class ribbon : public QTabWidget
{
    Q_OBJECT
signals:
    void cut ();
    void copy ();
    void del ();
    void paste ();

    void change_task_count ();
    void change_example_cycle ();

    void create_new ();
    void open ();
    void save ();
    void save_as ();
    void quit ();

    void import_data ();
    void invalid_timespan ();

    void export_data ();
    void measure_date ();
    void measure_man ();
    void task_man ();
    void mdi_active (bool);
public:
    ribbon (QWidget * parent = nullptr);
private:
    static std::unique_ptr<QToolButton> make_button (const QPixmap & icon, const QString & text);
    void setup_ui ();
    void setup_menu ();
    std::unique_ptr<QLayout> setup_layout ();
    std::unique_ptr<QWidget> ui_edit ();
    std::unique_ptr<QWidget> ui_video ();
    std::unique_ptr<QWidget> ui_report ();
    std::unique_ptr<QWidget> ui_settings ();
};



class ribbon_button : public QPushButton
{
    Q_OBJECT
public:
    template<typename ... Args>
    ribbon_button (Args && ... p) : QPushButton (std::forward<Args> (p)...) {}
};

class ribbon_menu : public QMenu
{
    Q_OBJECT
public:
    template<typename ... Args>
    ribbon_menu (Args && ... p) : QMenu (std::forward<Args> (p)...) {}
};

class ribbon_tool : public QToolButton
{
    Q_OBJECT
public:
    template<typename ... Args>
    ribbon_tool (Args && ... p) : QToolButton (std::forward<Args> (p)...) {}
};

class ribbon_bar : public QTabBar
{
    Q_OBJECT
public:
    ribbon_bar (QWidget * parent = nullptr) : QTabBar (parent) {}
};

class ribbon_edit : public QLineEdit
{
    Q_OBJECT
public:
    template<typename ... Args>
    ribbon_edit (Args && ... p) : QLineEdit (std::forward<Args> (p)...) {}
};

class layout_horizontal : public QHBoxLayout
{
    Q_OBJECT
public:
    template<typename ... Args>
    layout_horizontal (Args && ... p) : QHBoxLayout (std::forward<Args> (p)...) {}
    void set_w (int w) { w_ = w; }
protected:
    QSize maximumSize () const override
    {
        if (w_ == -1)
        {
            return QHBoxLayout::maximumSize ();
        }
        else
        {
            return QSize (w_, QHBoxLayout::maximumSize ().height ());
        }
    }
    QSize minimumSize () const override
    {
        if (w_ == -1)
        {
            return QHBoxLayout::minimumSize ();
        }
        else
        {
            return QSize (w_, QHBoxLayout::minimumSize ().height ());
        }
    }
private:
    int w_ = -1;
};



class ribbon_menu_item : public QWidget
{
    Q_OBJECT
public:
    ribbon_menu_item(const QPixmap& pix, const QString& text, QWidget *parent = 0)
        :QWidget (parent)
    {
        QLabel* label_icon = new QLabel(this);
        label_icon->setFixedSize(24, 24);
        label_icon->setScaledContents(true);
        label_icon->setPixmap(pix);

        QLabel* label_text = new QLabel(text,this);
        label_text->setAlignment (Qt::AlignLeft | Qt::AlignVCenter);

        QHBoxLayout* layout = new QHBoxLayout;
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        layout->addWidget(label_icon);
        layout->addWidget(label_text);
        setLayout(layout);

        setFixedWidth(100);
        setFixedHeight (32);
    }
    ~ribbon_menu_item()
    {}

protected:
    void paintEvent(QPaintEvent*) override {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }

private:
};

