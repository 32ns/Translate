#include "httpmanager.h"

#include <QJsonDocument>
#include <QNetworkProxyFactory>
#include <QNetworkReply>

HttpManager::HttpManager(QObject *parent)
    : QObject{parent}, manager(new QNetworkAccessManager(this))
{
    QNetworkProxyFactory::setUseSystemConfiguration(false);
    QNetworkProxy noProxy(QNetworkProxy::NoProxy);
    QNetworkProxy::setApplicationProxy(noProxy);
}

HttpManager::~HttpManager()
{
    delete manager;
}

void HttpManager::handleReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply->error()) {
        qDebug() << "Error: " << reply->errorString();
        emit sig_finished("");
    } else {
        QByteArray responseData = reply->readAll();
        // qDebug() << "Response: " << responseData;
        emit sig_finished(responseData);
    }
    reply->deleteLater();
}

void HttpManager::sendGetRequest(const QString &url)
{
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::SslProtocol::AnyProtocol);
    config.setPeerVerifyMode(QSslSocket::VerifyNone);

    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);
    request.setSslConfiguration(config);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "*/*");

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, &HttpManager::handleReply);
}

void HttpManager::sendPostRequest(const QString &url, const QJsonObject &data, const QMap<QString, QString> &headers)
{
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::SslProtocol::AnyProtocol);
    config.setPeerVerifyMode(QSslSocket::VerifyNone);

    QUrl requestUrl(url);
    QNetworkRequest request(requestUrl);
    request.setSslConfiguration(config);

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QByteArray postData = QJsonDocument(data).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = manager->post(request, postData);
    connect(reply, &QNetworkReply::finished, this, &HttpManager::handleReply);
}
