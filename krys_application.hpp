#pragma once

#include <QApplication>
#include <functional>
#include <boost/thread.hpp>
#include <memory>
#include <thread>

class krys_application;

template<bool b>
struct _app_helper_
{
    static krys_application* the_app;
};

template<bool b>
krys_application* _app_helper_<b>::the_app;

using app = _app_helper_<true>;

constexpr QEvent::Type event_msg = static_cast<QEvent::Type> (10000);
class msg_event : public QEvent
{
public:
    msg_event (std::function<void ()>& func):
        QEvent (event_msg)
    {
        p_func = std::make_unique<std::function<void()>> (std::move (func));
    }

    msg_event (std::function<void ()> &&func) :
        QEvent (event_msg)
    {
        p_func = std::make_unique<std::function<void()>> (func);
    }
    std::unique_ptr<std::function<void ()>> p_func;
};

class krys_application : public QApplication
{
    Q_OBJECT
public:
    krys_application(int argc, char** argv):
        QApplication (argc, argv)
    {

    }

    virtual ~ krys_application ()
    {

    }
protected:
    void customEvent (QEvent* event) override
    {
        if (event_msg == event->type ())
        {
            auto msg = dynamic_cast<msg_event*> (event); assert (msg);

            (*msg->p_func) ();
        }

        QApplication::customEvent (event);
    }
private:
};


#define call_after __CALL_AFTER__{},

struct __CALL_AFTER__
{
    template<typename T>
    void operator, (T&& t)
    {
        krys_application::postEvent (app::the_app, std::make_unique<msg_event> (std::forward<T> (t)).release ());
    }
};


#define go __GO__{},

struct __GO__
{
    template<typename T>
    void operator, (T&& t)
    {
        boost::thread {std::forward<T>(t)}.detach();
    }
};

