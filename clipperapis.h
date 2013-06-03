#ifndef CLIPPERAPIS_H
#define CLIPPERAPIS_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPixmap>
#include <QFile>
#include <QUrl>
#include <QTime>
#include <QDebug>

#include "json/json.h"

class ClipperAPIs : public QObject
{
    Q_OBJECT
public:
    explicit ClipperAPIs(QObject *parent = 0);
    
signals:
    void linkReady(QString link);
    
public slots:
    void onLinkShortened(QNetworkReply* reply);
    void onPasteLinkReady(QNetworkReply *reply);
    void googleLinkShorten(QString link);
    void tnyczPublish(QString text, QString title, QString password, bool is_code, bool is_private, bool is_protected);
    void imageshackUpload(QByteArray &picture);
    void onImageshackUploaded(QNetworkReply* response);
    
};

#endif // CLIPPERAPIS_H