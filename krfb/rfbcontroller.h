/***************************************************************************
                               rfbcontroller.h
                             -------------------
    begin                : Sun Dec 9 2001
    copyright            : (C) 2001 by Tim Jansen
    email                : tim@tjansen.de
 ***************************************************************************/

/***************************************************************************
 * Contains portions & concept from rfb's x0rfbserver.cc
 * Copyright (C) 2000 heXoNet Support GmbH, D-66424 Homburg.
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef RFBCONTROLLER_H
#define RFBCONTROLLER_H

#include "configuration.h"
#include <ksock.h>
#include <qobject.h>
// rfbconnection must be last because of X11 headers
#include "rfbconnection.h"

using namespace rfb;

/**
 * Manages sockets, drives the RGBConnection and triggers the connection
 * dialog.
 * The controller has three states: 'waiting for connection',
 * 'waiting for confirmation' and 'connected'. In the first state socket and 
 * connection are null, in the second socket is set and in the last both are 
 * set.
 * @author Tim Jansen
 */
class RFBController : public QObject  {
	Q_OBJECT
public: 
	RFBController(Configuration *c);
	~RFBController();

public slots:	
	void rebind();
	void closeSession();

signals:
        void sessionEstablished();
	void sessionFinished(); 
 
private:	
	void startServer();
	void checkWriteBuffer();
	void prepareIdleUpdate();

	Configuration *configuration;
	KServerSocket *serversocket;
	KSocket *socket;
	RFBConnection *connection;
	bool idleUpdateScheduled;

private slots:
	void idleSlot();
	void accepted(KSocket*);
        void socketReadable(); 
        void socketWritable(); 
};

#endif
