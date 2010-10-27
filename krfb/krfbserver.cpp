/* This file is part of the KDE project
   Copyright (C) 2007 Alessandro Praduroux <pradu@pradu.it>
             (C) 2001-2003 by Tim Jansen <tim@tjansen.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#include "krfbserver.h"

#include "abstractconnectioncontroller.h"
#include "framebuffer.h"
#include "framebuffermanager.h"
#include "krfbconfig.h"
#include "krfbconnectioncontroller.h"
#include "invitationmanager.h"
#include "servermanager.h"
#include "sockethelpers.h"

#include <dnssd/publicservice.h>

#include <KGlobal>
#include <KUser>
#include <KLocale>
#include <KDebug>
#include <KMessageBox>

#include <QtCore/QPointer>
#include <QtCore/QTimer>

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>

#include <QtNetwork/QHostInfo>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

KrfbServer::KrfbServer()
    : AbstractRfbServer()

{
    kDebug() << "starting ";
    QTimer::singleShot(0, this, SLOT(doStartListening()));
    connect(InvitationManager::self(), SIGNAL(invitationNumChanged(int)), SLOT(updatePassword()));
}

KrfbServer::~KrfbServer()
{
}


void KrfbServer::doStartListening()
{
    // Set up the parameters for the server before we get it to start listening.
    setFrameBuffer(FrameBufferManager::instance()->frameBuffer(QApplication::desktop()->winId()));
    setListeningPort(KrfbConfig::port());
    setListeningAddress("0.0.0.0");  // Listen on all available network addresses
    setDesktopName(i18n("%1@%2 (shared desktop)",
                        KUser().loginName(),
                        QHostInfo::localHostName()).toLatin1());

    if (KrfbConfig::publishService()) {
        DNSSD::PublicService *service = new DNSSD::PublicService(i18n("%1@%2 (shared desktop)",
                KUser().loginName(),
                QHostInfo::localHostName()),
                "_rfb._tcp",
                listeningPort());
        service->publishAsync();
    }

    startListening();

    // configure passwords and desktop control here
    updateSettings();
}

void KrfbServer::updateSettings()
{
    enableDesktopControl(KrfbConfig::allowDesktopControl());
    updatePassword();
}

enum rfbNewClientAction KrfbServer::handleNewClient(struct _rfbClientRec *cl)
{
    KrfbConnectionController *cc = new KrfbConnectionController(cl, this);

    cc->setControlEnabled(KrfbConfig::allowDesktopControl());
    cc->setControlCanBeEnabled(KrfbConfig::allowDesktopControl());

    connect(cc, SIGNAL(sessionEstablished(QString)), SIGNAL(sessionEstablished(QString)));
    connect(cc, SIGNAL(clientDisconnected(AbstractConnectionController *)), SLOT(clientDisconnected(AbstractConnectionController *)));

    return cc->handleNewClient();
}


#include "krfbserver.moc"
