#ifndef WIDGET_H
#define WIDGET_H

#include <QClipboard>
#include <QEvent>
#include <QJsonArray>
#include <QWidget>
#include <windows.h>
#include "httpmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

signals:
    void sig_ShortcutDetected();

protected:
    bool eventFilter(QObject *o, QEvent *e);//事件过滤器

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void installHook();
    void uninstallHook();
    void keyDownHandle();
    void Translation(QJsonArray textList, QString to_language);
    void finished(QByteArray data);
    QString getClipboardContent();

private slots:
    void on_btn_translate_clicked();

private:
    Ui::Widget *ui;
    HttpManager http;
    QClipboard *clipboard = nullptr;

};

#endif // WIDGET_H
