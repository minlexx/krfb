/* This file is part of the KDE project
   Copyright (C) 2007 Alessandro Praduroux <pradu@pradu.it>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version 2
   of the License.
*/

#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QVector>
#include <QRect>

#include <rfb/rfb.h>

class KrfbServer;
class FrameBuffer;

/**
	@author Alessandro Praduroux <pradu@pradu.it>
*/
class ConnectionController : public QObject
{
Q_OBJECT
public:
    explicit ConnectionController(int connFD, KrfbServer *parent);

    ~ConnectionController();

    bool handleCheckPassword(rfbClientPtr cl, const char *response, int len);
    enum rfbNewClientAction handleNewClient(struct _rfbClientRec *cl);
    void sendKNotifyEvent(const QString &name, const QString &desc);
    void handleNegotiationFinished(struct _rfbClientRec *cl);

    void handleKeyEvent(bool down , rfbKeySym keySym );
    void handlePointerEvent( int bm, int x, int y);
    void handleClientGone();
    void clipboardToServer(const QString &);
    void sendSessionEstablished();
    void run();

Q_SIGNALS:
    void sessionEstablished(QString);
    void notification(QString, QString);

public Q_SLOTS:

    void processEvents();

private:
    int fd;
    KrfbServer *server;
    FrameBuffer *fb;
    rfbScreenInfoPtr screen;
    QString remoteIp;
    rfbClientPtr client;
};

#endif
