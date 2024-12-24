#include "httpmanager.h"

#include <QJsonDocument>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QTimer>

HttpManager::HttpManager(QObject *parent)
    : QObject{parent}
    , manager(new QNetworkAccessManager(this))
    , timeout(15000)  // 15秒超时
{
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy noProxy(QNetworkProxy::NoProxy);
    QNetworkProxy::setApplicationProxy(noProxy);
}

HttpManager::~HttpManager()
{
    // QObject 会自动删除子对象，不需要手动删除 manager
}

void HttpManager::handleReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        qWarning() << "Invalid reply object";
        return;
    }

    // 停止超时计时器
    QTimer *timer = reply->findChild<QTimer*>();
    if (timer) {
        timer->stop();
        timer->deleteLater();
    }

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network error:" << reply->errorString() 
                  << "for URL:" << reply->url().toString();
        emit sig_finished(QByteArray());
    } else {
        QByteArray responseData = reply->readAll();
        emit sig_finished(responseData);
    }
    
    reply->deleteLater();
}

void HttpManager::handleTimeout()
{
    QTimer *timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(timer->parent());
    if (reply) {
        qWarning() << "Request timeout for URL:" << reply->url().toString();
        reply->abort();
        reply->deleteLater();
    }

    timer->deleteLater();
    emit sig_finished(QByteArray());
}

void HttpManager::setupReply(QNetworkReply *reply)
{
    if (!reply) return;

    // 创建超时计时器
    QTimer *timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &HttpManager::handleTimeout);
    timer->start(timeout);

    connect(reply, &QNetworkReply::finished, this, &HttpManager::handleReply);
}

void HttpManager::sendGetRequest(const QString &url)
{
    if (url.isEmpty()) {
        qWarning() << "Empty URL for GET request";
        return;
    }

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::SslProtocol::TlsV1_2OrLater);
    config.setPeerVerifyMode(QSslSocket::VerifyNone);

    QUrl requestUrl(url);
    if (!requestUrl.isValid()) {
        qWarning() << "Invalid URL:" << url;
        return;
    }

    QNetworkRequest request(requestUrl);
    request.setSslConfiguration(config);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "*/*");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Connection", "keep-alive");

    setupReply(manager->get(request));
}

void HttpManager::sendPostRequest(const QString &url, const QJsonObject &data, const QMap<QString, QString> &headers)
{
    if (url.isEmpty()) {
        qWarning() << "Empty URL for POST request";
        return;
    }

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::SslProtocol::TlsV1_2OrLater);
    config.setPeerVerifyMode(QSslSocket::VerifyNone);

    QUrl requestUrl(url);
    if (!requestUrl.isValid()) {
        qWarning() << "Invalid URL:" << url;
        return;
    }

    QNetworkRequest request(requestUrl);
    request.setSslConfiguration(config);

    // 设置默认请求头
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Connection", "keep-alive");

    // 设置自定义请求头
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QByteArray postData = QJsonDocument(data).toJson(QJsonDocument::Compact);
    setupReply(manager->post(request, postData));
}
