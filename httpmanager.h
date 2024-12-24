#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>

class HttpManager : public QObject
{
    Q_OBJECT
public:
    explicit HttpManager(QObject *parent = nullptr);
    ~HttpManager();

    void sendGetRequest(const QString &url);
    void sendPostRequest(const QString &url, const QJsonObject &data, const QMap<QString, QString> &headers);

signals:
    void sig_finished(QByteArray data);

private slots:
    void handleReply();
    void handleTimeout();

private:
    void setupReply(QNetworkReply *reply);
    
    QNetworkAccessManager *manager;
    const int timeout;  // 超时时间（毫秒）
};

#endif // HTTPMANAGER_H
