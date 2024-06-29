﻿#include "widget.h"
#include <QApplication>
#include <QSharedMemory>

int main(int argc, char *argv[])
{
    //检测程序是否已经运行
    QSharedMemory singleton("translate");
    if (!singleton.create(1)) {
        qDebug() << "另一个实例已经在运行";
        return 0;
    }

    QApplication a(argc, argv);
    Widget w;
    w.setWindowFlags(Qt::WindowTitleHint|Qt::WindowStaysOnTopHint);    //隐藏标题栏（无边框）
    w.hide();
    return a.exec();
}
