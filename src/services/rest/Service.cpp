#include "Service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTcpSocket>

Q_LOGGING_CATEGORY(BackendQtRestService, "BackendQtRestService")

namespace Services::Rest
{

namespace
{
QString safeFileName(const QString& raw)
{
    return QFileInfo(raw).fileName();
}

bool parseMultipartFile(const QByteArray& body, const QByteArray& boundary, QString* filenameOut, QByteArray* dataOut)
{
    const QByteArray delimiter = QByteArray("--") + boundary;

    int cursor = 0;
    while (true) {
        const int start = body.indexOf(delimiter, cursor);
        if (start < 0) {
            break;
        }

        const int dataStart = start + delimiter.size();
        if (body.mid(dataStart, 2) == "--") {
            break;
        }

        int partStart = dataStart;
        if (body.mid(partStart, 2) == "\r\n") {
            partStart += 2;
        }

        const int nextBoundary = body.indexOf(delimiter, partStart);
        if (nextBoundary < 0) {
            break;
        }

        QByteArray part = body.mid(partStart, nextBoundary - partStart);
        if (part.endsWith("\r\n")) {
            part.chop(2);
        }

        const int headerEnd = part.indexOf("\r\n\r\n");
        if (headerEnd > 0) {
            const QByteArray headers = part.left(headerEnd);
            const QByteArray payload = part.mid(headerEnd + 4);
            if (headers.contains("name=\"file\"")) {
                QRegularExpression re(QStringLiteral("filename=\"([^\"]+)\""));
                const QString headerText = QString::fromUtf8(headers);
                const QRegularExpressionMatch match = re.match(headerText);
                if (match.hasMatch()) {
                    const QString fileName = safeFileName(match.captured(1));
                    if (!fileName.isEmpty()) {
                        *filenameOut = fileName;
                        *dataOut = payload;
                        return true;
                    }
                }
            }
        }

        cursor = nextBoundary;
    }

    return false;
}

} // namespace

Service::Service(const QString& dataDir, QObject* parent)
    : QObject(parent),
      m_dataDir(dataDir)
{
}

void Service::handleHttpRequest(QTcpSocket* socket,
                                const QByteArray& method,
                                const QString& path,
                                const QHash<QByteArray, QByteArray>& headers,
                                const QByteArray& body)
{
    if (method == "OPTIONS") {
        sendResponse(socket, 204, "No Content", "text/plain", QByteArray());
        return;
    }

    if (method == "POST" && path == QStringLiteral("/api/media")) {
        handleUploadMedia(socket, headers, body);
        return;
    }

    if (method == "GET" && path.startsWith(QStringLiteral("/media/"))) {
        handleGetMedia(socket, path);
        return;
    }

    sendResponse(socket, 404, "Not Found", "text/plain", "Not Found");
}

void Service::handleUploadMedia(QTcpSocket* socket, const QHash<QByteArray, QByteArray>& headers, const QByteArray& body)
{
    const QByteArray contentType = headers.value("content-type");
    const QByteArray marker = "boundary=";
    const int boundaryIndex = contentType.indexOf(marker);
    if (boundaryIndex < 0) {
        sendResponse(socket, 400, "Bad Request", "text/plain", "Missing multipart boundary");
        return;
    }

    QByteArray boundary = contentType.mid(boundaryIndex + marker.size()).trimmed();
    if (boundary.startsWith('"') && boundary.endsWith('"') && boundary.size() > 1) {
        boundary = boundary.mid(1, boundary.size() - 2);
    }

    QString filename;
    QByteArray fileData;
    if (!parseMultipartFile(body, boundary, &filename, &fileData)) {
        sendResponse(socket, 400, "Bad Request", "text/plain", "Missing file field");
        return;
    }

    QDir mediaDir(QDir(m_dataDir).filePath(QStringLiteral("media")));
    if (!mediaDir.exists() && !mediaDir.mkpath(QStringLiteral("."))) {
        sendResponse(socket, 500, "Internal Server Error", "text/plain", "Failed to create media directory");
        return;
    }

    QFile out(mediaDir.filePath(filename));
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        sendResponse(socket, 500, "Internal Server Error", "text/plain", "Failed to store uploaded file");
        return;
    }
    out.write(fileData);
    out.close();

    qCInfo(BackendQtRestService) << "Uploaded media file:" << filename;
    sendResponse(socket, 200, "OK", "text/plain", "Uploaded");
}

void Service::handleGetMedia(QTcpSocket* socket, const QString& path)
{
    const QString fileName = safeFileName(path.mid(QStringLiteral("/media/").size()));
    if (fileName.isEmpty()) {
        sendResponse(socket, 404, "Not Found", "text/plain", "Not Found");
        return;
    }

    const QString filePath = QDir(m_dataDir).filePath(QStringLiteral("media/%1").arg(fileName));
    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        sendResponse(socket, 404, "Not Found", "text/plain", "Not Found");
        return;
    }

    sendResponse(socket, 200, "OK", contentTypeForFile(filePath), file.readAll());
}

void Service::sendResponse(QTcpSocket* socket,
                           int statusCode,
                           const QByteArray& reason,
                           const QByteArray& contentType,
                           const QByteArray& body)
{
    if (!socket) {
        return;
    }

    QByteArray response;
    response += "HTTP/1.1 " + QByteArray::number(statusCode) + " " + reason + "\r\n";
    response += "Connection: close\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response += "Access-Control-Allow-Headers: Content-Type\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "\r\n";
    response += body;

    socket->write(response);
    socket->disconnectFromHost();
}

QByteArray Service::contentTypeForFile(const QString& filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("png")) {
        return "image/png";
    }
    if (suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg")) {
        return "image/jpeg";
    }
    if (suffix == QStringLiteral("gif")) {
        return "image/gif";
    }
    if (suffix == QStringLiteral("svg")) {
        return "image/svg+xml";
    }
    if (suffix == QStringLiteral("mp3")) {
        return "audio/mpeg";
    }
    if (suffix == QStringLiteral("wav")) {
        return "audio/wav";
    }
    return "application/octet-stream";
}

} // namespace Services::Rest
