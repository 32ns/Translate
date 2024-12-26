#include "widget.h"
#include <QApplication>
#include <QNetworkProxyFactory>
#include <QSharedMemory>

int main(int argc, char *argv[])
{
    //检测程序是否已经运行
    QSharedMemory singleton("translate");
    if (!singleton.create(1)) {
        qDebug() << "另一个实例已经在运行";
        return 0;
    }
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QApplication a(argc, argv);
    Widget w;
    w.setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);    // 保持正常窗口样式，并保持在最上层
    w.hide();
    return a.exec();
}
