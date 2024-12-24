#include "widget.h"
#include "./ui_widget.h"
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QTimer>

// 初始化静态成员
Widget* Widget::s_instance = nullptr;

bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == this){
        switch (event->type()) {
        case QEvent::WindowDeactivate:
            this->hide();
            return true;
        default:
            break;
        }
    }
    return false;
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , clipboard(QApplication::clipboard())
    , m_lastCtrlCPressTime(0)
    , m_ctrlPress(false)
    , m_keyboardHook(nullptr)
{
    if (!clipboard) {
        qWarning() << "Failed to get clipboard instance";
    }
    
    ui->setupUi(this);    
    s_instance = this;    
    installEventFilter(this);
    connect(&http, &HttpManager::sig_finished, this, &Widget::finished);
    
    // 设置窗口属性
    setWindowFlags(Qt::Window | Qt::Tool);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);

    // 设置窗口图标
    QIcon icon(":/res/translate.svg");
    setWindowIcon(icon);
    QApplication::setWindowIcon(icon);
    
    // 创建托盘图标和菜单
    createActions();
    createTrayIcon();
    
    // 确保在构造函数最后安装钩子
    QTimer::singleShot(0, this, [this]() {
        installHook();
        qDebug() << "Hook installed in timer";
    });
    
    qDebug() << "Widget constructed";
}

Widget::~Widget()
{
    qDebug() << "Widget destructing";
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    s_instance = nullptr;
    uninstallHook();
    delete ui;
}

LRESULT CALLBACK Widget::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && s_instance)
    {
        KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;
        
        switch (wParam)
        {
        case WM_KEYDOWN:
            qDebug() << "Key Down:" << pKeyInfo->vkCode << "Ctrl state:" << s_instance->m_ctrlPress;
            
            // 检测 Ctrl 键
            if (pKeyInfo->vkCode == VK_LCONTROL || pKeyInfo->vkCode == VK_RCONTROL)
            {
                s_instance->m_ctrlPress = true;
                qDebug() << "Ctrl pressed, state:" << s_instance->m_ctrlPress;
            }
            // 检测 C 键
            else if (pKeyInfo->vkCode == 'C')
            {
                if (s_instance->m_ctrlPress)
                {
                    DWORD currentTime = GetTickCount();
                    DWORD timeDiff = currentTime - s_instance->m_lastCtrlCPressTime;
                    qDebug() << "Ctrl+C pressed, time diff:" << timeDiff;
                    
                    if (timeDiff <= 500)
                    {
                        qDebug() << "Double Ctrl+C detected, posting message";
                        QMetaObject::invokeMethod(s_instance, "keyDownHandle", Qt::QueuedConnection);
                    }
                    s_instance->m_lastCtrlCPressTime = currentTime;
                }
            }
            break;
            
        case WM_KEYUP:
            // 检测 Ctrl 键释放
            if (pKeyInfo->vkCode == VK_LCONTROL || pKeyInfo->vkCode == VK_RCONTROL)
            {
                s_instance->m_ctrlPress = false;
                qDebug() << "Ctrl released, state:" << s_instance->m_ctrlPress;
            }
            break;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void Widget::installHook()
{
    qDebug() << "Installing hook...";
    m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    if (m_keyboardHook == NULL)
    {
        DWORD error = GetLastError();
        qDebug() << "Failed to install keyboard hook. Error:" << error;
    }
    else
    {
        qDebug() << "Hook installed successfully";
    }
}

void Widget::uninstallHook()
{
    qDebug() << "Uninstalling hook...";
    if (m_keyboardHook != NULL)
    {
        if (UnhookWindowsHookEx(m_keyboardHook))
        {
            qDebug() << "Hook uninstalled successfully";
        }
        else
        {
            qDebug() << "Failed to uninstall hook. Error:" << GetLastError();
        }
        m_keyboardHook = NULL;
    }
}

void Widget::keyDownHandle()
{
    qDebug() << "keyDownHandle called";
    QString data = getClipboardContent();
    if(data.isEmpty()){
        qDebug() << "Clipboard is empty";
        return;
    }
    
    qDebug() << "Clipboard content:" << data;
    data = data.trimmed();
    if (data.length() > 5000) {
        qWarning() << "Text too long, truncating to 5000 characters";
        data = data.left(5000);
    }
    
    Translation(QJsonArray{data}, "zh");
    ui->txt_source->setTextColor(QColor(97, 97, 97));
    ui->txt_source->clear();
    ui->txt_source->append(data);
    
    showAndActivateWindow();
}

void Widget::showAndActivateWindow()
{
    // 显示窗口
    show();
    raise();
    activateWindow();
    
    // 使用定时器延迟激活窗口
    QTimer::singleShot(50, this, [this]() {
#ifdef Q_OS_WIN32
        if (HWND hwnd = (HWND)this->winId()) {
            if (HWND hForgroundWnd = GetForegroundWindow()) {
                DWORD dwForeID = ::GetWindowThreadProcessId(hForgroundWnd, NULL);
                DWORD dwCurID = ::GetCurrentThreadId();
                
                if (::AttachThreadInput(dwCurID, dwForeID, TRUE)) {
                    ::SetForegroundWindow(hwnd);
                    ::SetActiveWindow(hwnd);
                    ::AttachThreadInput(dwCurID, dwForeID, FALSE);
                }
            }
        }
#endif
        qDebug() << "Window activated";
    });
}

bool Widget::isChineseText(const QString& text) {
    for (const QChar& ch : text) {
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
            return true;
        }
    }
    return false;
}

void Widget::Translation(QJsonArray textList, QString to_language)
{
    if (textList.isEmpty()) {
        qWarning() << "Empty text list for translation";
        return;
    }

    // 自动检测源文本语言并设置目标语言
    QString sourceText = textList[0].toString();
    QString targetLang = isChineseText(sourceText) ? "en" : "zh";

    QJsonObject json;
    json["source_language"] = "detect";
    json["target_language"] = targetLang;
    json["text_list"] = textList;
    json["enable_user_glossary"] = false;
    json["category"] = "";

    QMap<QString, QString> headers;
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36";
    headers["content-type"] = "application/json";
    
    http.sendPostRequest("https://translate.volcengine.com/crx/translate/v2/", json, headers);
}

void Widget::finished(QByteArray data)
{
    if (data.isEmpty()) {
        qWarning() << "Received empty response from server";
        return;
    }

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    QJsonObject json = jsonDoc.object();
    QJsonArray jsonArray = json.value("translations").toArray();
    if (jsonArray.isEmpty()) {
        qWarning() << "No translations in response";
        return;
    }

    QString result;
    for(const QJsonValue &value : jsonArray) {
        if (!value.isString()) continue;
        result.append(value.toString());
    }

    ui->txt_target->clear();
    ui->txt_target->append(result);
    ui->txt_source->setTextColor(QColor(46, 47, 48));

    showAndActivateWindow();
}

QString Widget::getClipboardContent()
{
    if (!clipboard) {
        qWarning() << "Clipboard is not available";
        return QString();
    }
    return clipboard->text().trimmed();
}

void Widget::on_btn_translate_clicked()
{
    QString data = ui->txt_source->toPlainText().trimmed();
    if(data.isEmpty()){
        return;
    }
    Translation(QJsonArray{data},"zh");
}

void Widget::createActions()
{
    m_quitAction = new QAction(tr("退出"), this);
    connect(m_quitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void Widget::createTrayIcon()
{
    m_trayIconMenu = new QMenu(this);
    m_trayIconMenu->addAction(m_quitAction);

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayIconMenu);
    
    // 使用相同的图标
    QIcon icon(":/res/translate.svg");
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip(tr("翻译工具"));
    
    // 显示托盘图标
    m_trayIcon->show();
    
    // 连接托盘图标的信号
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (!isVisible()) {
                showAndActivateWindow();
            }
        }
    });
}

