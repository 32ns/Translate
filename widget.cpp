#include "widget.h"
#include "./ui_widget.h"
#include <QApplication>
#include <QJsonObject>
#include <qjsondocument.h>


DWORD lastCtrlCPressTime = 0;
bool ctrlPress = false;
HHOOK keyboardHook = NULL;
Widget* thisWindow;


bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if( watched == this ){
        switch (event->type()) {
        case QEvent::WindowActivate:{

        }break;
        case QEvent::WindowDeactivate:{
            this->hide();
            return true;
        }break;
        default:
            break;
        }
    }

    return false;
}

Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);    
    thisWindow = this;    
    QWidget::installEventFilter(this);//为这个窗口安装过滤器
    connect(&http,&HttpManager::sig_finished,this,&Widget::finished);
    installHook();
}

Widget::~Widget()
{
    delete ui;
    uninstallHook();
}

/***************************************************************************************************************
    键盘钩子过程
***************************************************************************************************************/
LRESULT KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        // wParam 是键盘事件类型
        if (wParam == WM_KEYDOWN)
        {
            KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;

            // 检测Ctrl按下
            if (pKeyInfo->vkCode == 162)
            {
                ctrlPress = true;
            }
            else if (pKeyInfo->vkCode == 'C')
            {
                DWORD currentTime = GetTickCount();
                // 检查两次按下Ctrl+C之间的时间间隔
                if (ctrlPress && currentTime - lastCtrlCPressTime <= 500)
                {
                    thisWindow->keyDownHandle();
                }
                // 更新上次按下Ctrl+C的时间和计数
                lastCtrlCPressTime = currentTime;
            }
        }
        else if (wParam == WM_KEYUP)
        {
            // 当键释放时
            KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;
            if (pKeyInfo->vkCode == 162)
            {
                ctrlPress = false;
            }
        }
    }
    // 调用下一个钩子过程
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}


/***************************************************************************************************************
    安装钩子
***************************************************************************************************************/
void Widget::installHook()
{
    // 安装键盘钩子
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (keyboardHook == NULL)
    {
        qDebug() << "Failed to install keyboard hook";
    }
}


/***************************************************************************************************************
    卸载钩子
***************************************************************************************************************/
void Widget::uninstallHook()
{
    // 卸载键盘钩子
    if (keyboardHook != NULL)
    {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = NULL;
    }
}


/***************************************************************************************************************
    按键事件处理
***************************************************************************************************************/
void Widget::keyDownHandle()
{
    QString data = getClipboardContent();
    if(data.isEmpty()){
        return;
    }
    Translation(QJsonArray{data},"zh");
    ui->txt_source->clear();
    ui->txt_source->append(data);
}


/***************************************************************************************************************
    翻译函数
***************************************************************************************************************/
void Widget::Translation(QJsonArray textList, QString to_language)
{
    QJsonObject json;
    json["source_language"] = "detect";
    json["target_language"] = to_language;
    json["text_list"] = textList;

    QJsonArray glossaryList;
    json["glossary_list"] = glossaryList;
    json["enable_user_glossary"] = false;
    json["category"] = "";

    QMap<QString, QString> headers;
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36";
    headers["content-type"] = "application/json";
    http.sendPostRequest("https://translate.volcengine.com/crx/translate/v2/", json, headers);
}


/***************************************************************************************************************
    翻译完成
***************************************************************************************************************/
void Widget::finished(QByteArray data)
{
    QJsonObject json = QJsonDocument::fromJson(data).object();
    QJsonArray jsonArray = json.value("translations").toArray();
    QString result;
    for(int i=0;i<jsonArray.size();i++){
        result.append(jsonArray[i].toString());
    }
    ui->txt_target->clear();
    ui->txt_target->append(result);

    //激活窗口
    this->raise();
    this->show();
    this->activateWindow();

#ifdef Q_OS_WIN32
    HWND hForgroundWnd = GetForegroundWindow();
    DWORD dwForeID = ::GetWindowThreadProcessId(hForgroundWnd, NULL);
    DWORD dwCurID = ::GetCurrentThreadId();

    ::AttachThreadInput(dwCurID, dwForeID, TRUE);
    ::SetForegroundWindow((HWND)winId());
#endif

}

/***************************************************************************************************************
    获取剪切板内容
***************************************************************************************************************/
QString Widget::getClipboardContent()
{
    if (clipboard == nullptr) {
        clipboard = QApplication::clipboard();
    }
    return clipboard->text();
}

/***************************************************************************************************************
    翻译
***************************************************************************************************************/
void Widget::on_btn_translate_clicked()
{
    QString data = ui->txt_source->toPlainText().trimmed();
    if(data.isEmpty()){
        return;
    }
    Translation(QJsonArray{data},"zh");
}

