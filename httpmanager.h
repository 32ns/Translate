#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include <QNetworkAccessManager>
#include <QObject>

class HttpManager : public QObject
{
    Q_OBJECT
public:
    explicit HttpManager(QObject *parent = nullptr);
    ~HttpManager();

private:
    QNetworkAccessManager *manager;

signals:
    void sig_finished(QByteArray data);

private slots:
    void handleReply();

public slots:
    void sendGetRequest(const QString &url);
    void sendPostRequest(const QString &url, const QJsonObject &data, const QMap<QString, QString> &headers);
};

#endif // HTTPMANAGER_H
