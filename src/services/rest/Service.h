#ifndef SERVICES_REST_SERVICE_H
#define SERVICES_REST_SERVICE_H

#include <QHash>
#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QTcpSocket)

namespace Services::Rest
{

class Service : public QObject
{
    Q_OBJECT

  public:
    explicit Service(QObject* parent = nullptr);

    void handleHttpRequest(QTcpSocket* socket,
                           const QByteArray& method,
                           const QString& path,
                           const QHash<QByteArray, QByteArray>& headers,
                           const QByteArray& body);

  signals:
    void mediaUploaded(const QString& filename);

  private:
    void handleUploadMedia(QTcpSocket* socket, const QHash<QByteArray, QByteArray>& headers, const QByteArray& body);
    void handleGetMedia(QTcpSocket* socket, const QString& path);

    void sendResponse(QTcpSocket* socket,
                      int statusCode,
                      const QByteArray& reason,
                      const QByteArray& contentType,
                      const QByteArray& body = QByteArray());

    static QByteArray contentTypeForFile(const QString& filePath);
};

} // namespace Services::Rest

#endif // SERVICES_REST_SERVICE_H
