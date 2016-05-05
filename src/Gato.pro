#-------------------------------------------------
#
# Project created by QtCreator 2013-11-28T15:09:51
#
#-------------------------------------------------

QT	+= core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Gato
TEMPLATE = app


SOURCES	+=  main.cpp \
	    mainwindow.cpp \
	    client.cpp \
	    connection.cpp \
	    peermanager.cpp \
	    server.cpp

HEADERS  += mainwindow.h \
	    client.h \
	    connection.h \
	    peermanager.h \
	    server.h

FORMS    += mainwindow.ui \
