/*
 *  Watches Inet interfaces
 *  Copyright (C) 2002 Tim Jansen <tim@tjansen.de>
 *
 *  $Id$
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 */
#ifndef KINETINTERFACEWATCHER_H
#define KINETINTERFACEWATCHER_H

#include <kinetinterface.h>
#include <qobject.h>
#include <qvaluevector.h>
#include <qcstring.h>
#include <qstring.h>


class KInetInterfaceWatcherPrivate;


/**
 * KInetInterfaceWatcher can watch the state of one or all 
 * of the system's network interfaces. 
 * The watcher will emit the signal @ref changed() when an 
 * interface changed or a interface has been added or removed.
 *
 * @author Tim Jansen <tim@tjansen.de>
 * @short Watches the state of the network interfaces
 * @see KInetInterface
 * @since 3.2
 */
class KInetInterfaceWatcher : public QObject {
  Q_OBJECT
public:
  /**
   * Creates a new KInetInterfaceWatcher. It watches either one
   * or all network interfaces.
   * @param interface the name of the interface to watch (e.g.'eth0')
   *                  or QString::null to watch all interfaces
   * @param minInterval the minimum interval between two checks in 
   *                    seconds. Be careful not to check too often, to
   *                    avoid unneccessary wasting of CPU time
   */
  KInetInterfaceWatcher(const QString &interface = QString::null,
			int minInterval = 60);

  /**
   * Returns the name of the interface that is being watched.
   * @return the name of the interface, or QString::null if all
   *         interfaces are watched
   */
  QString interface() const;

  /**
   * Destructor
   */
  virtual ~KInetInterfaceWatcher();

signals:
  /**
   * Emitted when one or more watched interfaces have changed. The 
   * @p interfaceName is the name of the interface being watched, not
   * the interface that has changed (because more than one interface
   * may have changed). 
   * @param interfaceName the name of the interface that is watched,
   *                      by the emitter, or QString::null if all 
   *                      interfaces are being watched
   */
  void changed(QString interfaceName);

private:
  KInetInterfaceWatcherPrivate* d;
};

#endif
