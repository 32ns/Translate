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
    setWindowFlags(Qt::Window | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    
    // 设置窗口样式
    setStyleSheet(R"(
        QWidget {
            background-color: #FFFFFF;
            font-family: "Microsoft YaHei", "微软雅黑";
        }
        QTextEdit {
            border: 1px solid #E0E0E0;
            border-radius: 2px;
            padding: 8px;
            background-color: #FFFFFF;
            selection-background-color: #2196F3;
            selection-color: white;
        }
        QTextEdit:focus {
            border: 1px solid #2196F3;
        }
        QPushButton {
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 2px;
            padding: 8px 16px;
            font-size: 14px;
            min-height: 25px;
            max-height: 25px;
        }
        QPushButton:hover {
            background-color: #1E88E5;
        }
        QPushButton:pressed {
            background-color: #1976D2;
        }
    )");

    // 保存原始标题
    m_originalTitle = windowTitle();

    // 初始化标题动画定时器
    m_titleAnimTimer = new QTimer(this);
    m_titleAnimTimer->setInterval(500);
    connect(m_titleAnimTimer, &QTimer::timeout, this, &Widget::updateTitleAnimation);

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
    });
}

Widget::~Widget()
{
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
            
            // 检测 Ctrl 键
            if (pKeyInfo->vkCode == VK_LCONTROL || pKeyInfo->vkCode == VK_RCONTROL)
            {
                s_instance->m_ctrlPress = true;
            }
            // 检测 C 键
            else if (pKeyInfo->vkCode == 'C')
            {
                if (s_instance->m_ctrlPress)
                {
                    DWORD currentTime = GetTickCount();
                    DWORD timeDiff = currentTime - s_instance->m_lastCtrlCPressTime;
                    
                    if (timeDiff <= 500)
                    {
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
            }
            break;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void Widget::installHook()
{
    m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    if (m_keyboardHook == NULL)
    {
        DWORD error = GetLastError();
        qDebug() << "Failed to install keyboard hook. Error:" << error;
    }
}

void Widget::uninstallHook()
{
    if (m_keyboardHook != NULL)
    {
        if (!UnhookWindowsHookEx(m_keyboardHook))
        {
            qDebug() << "Failed to uninstall hook. Error:" << GetLastError();
        }
        m_keyboardHook = NULL;
    }
}

void Widget::Translation(QJsonArray textList)
{
    switch (apiVersion) {
    case API_VERSION::V1:{
        this->Translation_v1(textList);
    }break;
    case API_VERSION::V2:{
        this->Translation_v2(textList);
    }break;
    default:
        break;
    }
}

void Widget::keyDownHandle()
{
    QString data = getClipboardContent();
    if(data.isEmpty()){
        qDebug() << "Clipboard is empty";
        return;
    }
    
    data = data.trimmed();
    if (data.length() > 5000) {
        qWarning() << "Text too long, truncating to 5000 characters";
        data = data.left(5000);
    }
    
    ui->txt_source->setTextColor(QColor(97, 97, 97));
    ui->txt_source->clear();
    ui->txt_source->append(data);
    
    // 先显示窗口
    showAndActivateWindow();
    
    // 然后发送翻译请求
    Translation_v2(QJsonArray{data});
}

void Widget::showAndActivateWindow()
{
#ifdef Q_OS_WIN32
    if (HWND hwnd = (HWND)this->winId()) {
        // 获取当前前台窗口的线程ID
        HWND hForeWnd = GetForegroundWindow();
        DWORD dwForeID = GetWindowThreadProcessId(hForeWnd, NULL);
        DWORD dwCurID = GetCurrentThreadId();

        // 附加线程输入
        AttachThreadInput(dwCurID, dwForeID, TRUE);
        
        // 显示窗口
        ShowWindow(hwnd, SW_SHOW);
        
        // 激活窗口
        SetForegroundWindow(hwnd);
        
        // 分离线程输入
        AttachThreadInput(dwCurID, dwForeID, FALSE);
        
        // 确保窗口在最前面并获得焦点
        BringWindowToTop(hwnd);
        SetFocus(hwnd);
    }
#endif
    show();
    activateWindow();
    raise();
}

bool Widget::isChineseText(const QString& text) {
    for (const QChar& ch : text) {
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
            return true;
        }
    }
    return false;
}

void Widget::Translation_v1(QJsonArray textList)
{
    if (textList.isEmpty()) {
        qWarning() << "Empty text list for translation";
        return;
    }

    // 清空翻译结果
    ui->txt_target->clear();
    
    startTitleAnimation();

    // 获取源文本并按行分割
    QString sourceText = textList[0].toString();
    QStringList lines = sourceText.split('\n');

    // 创建新的文本列表，每行作为一个独立的翻译项
    QJsonArray lineArray;
    for (const QString& line : lines) {
        if (!line.trimmed().isEmpty()) {
            lineArray.append(line);
        }
    }

    if (lineArray.isEmpty()) {
        return;
    }

    // 自动检测源文本语言并设置目标语言
    QString targetLang = isChineseText(sourceText) ? "en" : "zh";

    QJsonObject json;
    json["source_language"] = "detect";
    json["target_language"] = targetLang;
    json["text_list"] = lineArray;  // 使用按行分割后的数组
    json["enable_user_glossary"] = false;
    json["category"] = "";

    QMap<QString, QString> headers;
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36";
    headers["content-type"] = "application/json";

    http.sendPostRequest("https://translate.volcengine.com/crx/translate/v2/", json, headers);
}


void Widget::Translation_v2(QJsonArray textList)
{
    if (textList.isEmpty()) {
        qWarning() << "Empty text list for translation";
        return;
    }

    // 清空翻译结果
    ui->txt_target->clear();
    
    startTitleAnimation();

    // 自动检测源文本语言并设置目标语言
    QString sourceText = textList[0].toString();
    QString targetLang = isChineseText(sourceText) ? "en" : "zh";
    QString sourceLang = isChineseText(sourceText) ? "zh" : "en";

    QJsonObject source;
    source["lang"] = sourceLang;
    source["text_list"] = textList;

    QJsonObject target;
    target["lang"] = targetLang;

    QJsonObject header;
    header["fn"] = "auto_translation";
    header["session"] = "";
    header["client_key"] = "browser-chrome-131.0.0";
    header["user"] = "";

    QJsonObject json;
    json["header"] = header;
    json["type"] = "plain";
    json["model_category"] = "normal";
    json["text_domain"] = "general";
    json["source"] = source;
    json["target"] = target;

    QMap<QString, QString> headers;
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";
    headers["content-type"] = "application/json";
    headers["Accept"] = "application/json, text/plain, */*";
    headers["Origin"] = "https://yi.qq.com";
    headers["Referer"] = "https://yi.qq.com/";

    http.sendPostRequest("https://yi.qq.com/api/imt", json, headers);
}

void Widget::finished(QByteArray data)
{
    stopTitleAnimation();

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
    QString result;

    // 检查是否是 API V2 格式
    if (json.contains("translations")) {
        // API V2 格式处理
        QJsonObject baseResp = json["base_resp"].toObject();
        if (baseResp["status_code"].toInt() != 0) {
            qWarning() << "Translation failed, error code:" << baseResp["status_code"].toInt();
            return;
        }

        QJsonArray translations = json["translations"].toArray();
        if (translations.isEmpty()) {
            qWarning() << "No translations in response";
            return;
        }

        for(const QJsonValue &value : translations) {
            if (!value.isString()) continue;
            result.append(value.toString());
        }
    }
    // 检查是否是 API V1 格式
    else if (json.contains("header")) {
        QJsonObject header = json["header"].toObject();
        if (header["ret_code"].toString() != "succ") {
            qWarning() << "Translation failed, error code:" << header["ret_code"].toString();
            return;
        }

        QJsonArray translationArray = json["auto_translation"].toArray();
        if (translationArray.isEmpty()) {
            qWarning() << "No translations in response";
            return;
        }

        for(const QJsonValue &value : translationArray) {
            if (!value.isString()) continue;
            result.append(value.toString());
        }
    }
    else {
        qWarning() << "Unknown API response format";
        return;
    }

    ui->txt_target->clear();
    ui->txt_target->append(result);
    ui->txt_source->setTextColor(QColor(46, 47, 48));
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
    Translation_v2(QJsonArray{data});
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
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (!isVisible()) {
                showAndActivateWindow();
            }
        }
    });
}

// 添加标题栏动画相关函数实现
void Widget::startTitleAnimation()
{
    m_animDots = 0;
    updateTitleAnimation();
    m_titleAnimTimer->start();
}

void Widget::stopTitleAnimation()
{
    m_titleAnimTimer->stop();
    setWindowTitle(m_originalTitle);
}

void Widget::updateTitleAnimation()
{
    QString dots;
    m_animDots = (m_animDots + 1) % 4;
    for(int i = 0; i < m_animDots; ++i) {
        dots += ".";
    }
    setWindowTitle(m_originalTitle + (dots.isEmpty() ? "" : " " + dots));
}

