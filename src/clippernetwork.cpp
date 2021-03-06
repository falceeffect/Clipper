#include "clippernetwork.h"

ClipperNetwork::ClipperNetwork(QObject *parent) :
    QObject(parent)
{
    network = new QNetworkAccessManager();
}

void ClipperNetwork::uploadImage(std::shared_ptr<QByteArray> image)
{
    QNetworkRequest *request = new QNetworkRequest(QUrl(IMAGE_UPLOAD_URL));
    QByteArray requestData;

    request->setRawHeader("Host", "deviantsart.com");
    request->setRawHeader("Cache-Control", "no-cache");
    request->setRawHeader("Accept","*/*");
    request->setRawHeader("Content-Type", "multipart/form-data; boundary=--ptkDjQJ3nhyCxEDg");

    requestData.append("----ptkDjQJ3nhyCxEDg\r\n");
    requestData.append("Content-Disposition: form-data; name=\"file\"; filename=\"image.png\"");
    requestData.append("Content-Type: image/png\r\n");
    requestData.append("\r\n");
    requestData.append(*image);
    requestData.append("\r\n----ptkDjQJ3nhyCxEDg--");

    request->setHeader(QNetworkRequest::ContentLengthHeader, requestData.length());

    QNetworkReply *networkRequest = network->post(*request, requestData);

    connect(networkRequest, &QNetworkReply::finished, [networkRequest, this]() {
        if (networkRequest->error() == QNetworkReply::NoError)
        {
            QVariantMap responseMap = JSON::parse(networkRequest->readAll()).toMap();
            QString link = responseMap["url"].toString();
            if (!link.isEmpty())
                emit linkReady(link);
            else
            {
                qDebug() << responseMap;
                emit failure(UNKNOWN_ERROR);
            }
        }
        else
            emit failure(networkRequest->errorString());

        networkRequest->deleteLater();
    });
}

void ClipperNetwork::shareText(QString &text)
{
    std::shared_ptr<QNetworkRequest> request(new QNetworkRequest(QUrl(HASTEBIN_PUBLISH_URL)));

    QByteArray data;

    request->setRawHeader("Host", "hastebin.com");
    request->setRawHeader("Content-Type", "text/plain");
    data.append(text);

    QNetworkReply *networkRequest = network->post(*request, data);

    connect(networkRequest, &QNetworkReply::finished, [networkRequest, this]() {
        if (networkRequest->error() == QNetworkReply::NoError)
        {
            QVariantMap replyMap = JSON::parse(networkRequest->readAll()).toMap();
            QString link = replyMap["key"].toString();
            if (!link.isEmpty())
                emit linkReady(HASTEBIN_BASE_URL+link);
            else
            {
                qDebug() << replyMap;
                emit failure(UNKNOWN_ERROR);
            }
        }
        else
            emit failure(networkRequest->errorString());

        networkRequest->deleteLater();
    });
}

void ClipperNetwork::shortenLink(QString link)
{
    QNetworkRequest request;
    request.setUrl(QUrl(LINK_SHORTEN_URL));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QVariantMap args;
    args["longUrl"] = link;

    QNetworkReply* networkRequest = network->post(request, JSON::serialize(QVariant(args), 4));
    connect(networkRequest, &QNetworkReply::finished, [networkRequest, this]() {
        if (networkRequest->error() == QNetworkReply::NoError)
        {
            QVariantMap replyMap = JSON::parse(networkRequest->readAll()).toMap();
            QString link = replyMap["id"].toString();
            if (!link.isEmpty())
                emit linkReady(link);
            else
            {
                qDebug() << replyMap;
                emit failure(UNKNOWN_ERROR);
            }
        }
        else
            emit failure(networkRequest->errorString());

        networkRequest->deleteLater();
    });
}

void ClipperNetwork::convertToQRCode(QString text)
{
    QUrlQuery requestQuery;
    requestQuery.addQueryItem("d", QUrl::toPercentEncoding(text));
    QUrl requestUrl(QR_CODE_URL);
    requestUrl.setQuery(requestQuery);

    QNetworkReply *networkRequest = network->get(QNetworkRequest(requestUrl));

    connect(networkRequest, &QNetworkReply::finished, [networkRequest, this]() {
        if (networkRequest->error() == QNetworkReply::NoError)
        {
            std::shared_ptr<QPixmap> qrCode(new QPixmap());
            qrCode->loadFromData(networkRequest->readAll());
            emit qrCodeReady(qrCode);
        }
        else
            emit failure(networkRequest->errorString());

        networkRequest->deleteLater();
    });
}
