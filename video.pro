#-------------------------------------------------
#
# Project created by QtCreator 2017-06-15T19:39:53
#
#-------------------------------------------------

QT       += core gui avwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG -= c++11
QMAKE_CXXFLAGS += -std=c++1z

TARGET = video
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
	main.cc \
    video/ribbon.cc \
    video/first_dlg.cpp \
    video/form_widget.cpp \
    video/progress_label.cpp \
    video/video_main.cc \
    video/video_analysis.cc \
    video/video_delegate.cc \
    video/video_form_model.cc \
    video/video_form_split.cc \
    video/video_widget.cc \
    model/json_model.cc \
    view/table_view.cpp

HEADERS += \
    video/first_dlg.h \
    video/form_widget.h \
    video/progress_label.h \
    video/ribbon.h \
    video/video_analysis.h \
    video/video_delegate.h \
    video/video_form_model.h \
    video/video_form_split.h \
    video/video_main.h \
    video/video_widget.h \
    video/video_player.hpp \
    model/json_model.h \
    view/table_view.h

FORMS += \
    video/first_dlg.ui \
    video/form_widget.ui \
    video/video_analysis.ui \
    video/video_main.ui \
    video/video_widget.ui

LIBS += -lboost_filesystem
LIBS += -lboost_system
