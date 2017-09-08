#-------------------------------------------------
#
# Project created by QtCreator 2017-06-15T19:39:53
#
#-------------------------------------------------

QT       += core gui avwidgets charts xlsx

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
    video/progress_label.cpp \
    video/video_delegate.cc \
    video/video_form_model.cc \
    video/video_form_split.cc \
    video/video_widget.cc \
    model/json_model.cc \
    view/table_view.cpp \
    verification.cc \
    encryption.cc \
    verification_dlg.cc \
    net_utils.cc \
    des.cc \
    algorithm_utils.cc \
    video/whisker.cc \
    video/VideoFormModel.cc \
    video/FormWidget.cpp \
    video/VideoAnalysis.cc \
    video/VideoMainTrial.cc

HEADERS += \
    video/first_dlg.h \
    video/progress_label.h \
    video/ribbon.h \
    video/video_delegate.h \
    video/video_form_model.h \
    video/video_form_split.h \
    video/video_widget.h \
    video/video_player.hpp \
    model/json_model.h \
    view/table_view.h \
    video/utils.hpp \
    verification.h \
    verification_dlg.h \
    encryption.h \
    net_utils.h \
    des.h \
    krys_application.hpp \
    algorithm_utils.h \
    video/whisker.h \
    video/video_player.hpp \
    video/VideoFormModel.h \
    video/FormWidget.h \
    video/VideoAnalysis.h \
    video/VideoMainTrialh

FORMS += \
    video/first_dlg.ui \
    video/video_widget.ui \
    verification_dlg.ui \
    video/FormWidget.ui \
    video/VideoAnalysis.ui \
    video/VideoMainTrial.ui

QMAKE_CXXFLAGS += -Wextra
QMAKE_CXXFLAGS += -Wno-deprecated-declarations
QMAKE_CXXFLAGS += -Werror=write-strings
QMAKE_CXXFLAGS += -Werror=return-type
QMAKE_CXXFLAGS += -Werror=parentheses
QMAKE_CXXFLAGS += -Werror=maybe-uninitialized

LIBS += -lboost_filesystem
LIBS += -lboost_system
LIBS += -lboost_regex
LIBS += -lboost_thread
LIBS += -lboost_locale
LIBS += -liconv
LIBS += -lwininet
LIBS += -lws2_32
