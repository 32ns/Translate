#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QClipboard>
#include <QJsonArray>
#include <QEvent>
#include <windows.h>
#include <QSystemTrayIcon>
#include <QMenu>
#include "httpmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_btn_translate_clicked();
    void finished(QByteArray data);
    void keyDownHandle();

private:
    void installHook();
    void uninstallHook();
    void Translation(QJsonArray textList, QString to_language);
    QString getClipboardContent();
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    void showAndActivateWindow();
    bool isChineseText(const QString& text);
    void createTrayIcon();
    void createActions();

private:
    Ui::Widget *ui;
    HttpManager http;
    QClipboard *clipboard{nullptr};
    
    // 将原来的全局变量转为成员变量
    DWORD m_lastCtrlCPressTime{0};
    bool m_ctrlPress{false};
    HHOOK m_keyboardHook{NULL};
    
    // 用于存储当前窗口实例的静态指针
    static Widget* s_instance;

    // 新增：托盘图标相关成员
    QSystemTrayIcon *m_trayIcon{nullptr};
    QMenu *m_trayIconMenu{nullptr};
    QAction *m_quitAction{nullptr};
};

#endif // WIDGET_H
