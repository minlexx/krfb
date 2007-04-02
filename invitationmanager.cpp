/* This file is part of the KDE project
   Copyright (C) 2007 Alessandro Praduroux <pradu@pradu.it>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version 2
   of the License.
*/
#include "invitationmanager.h"
#include "invitationmanager.moc"

#include <KStaticDeleter>
#include <KConfigGroup>
#include <KConfig>
#include <KGlobal>

#include <QTimer>

static KStaticDeleter<InvitationManager> sd;
InvitationManager * InvitationManager::_self = 0;
InvitationManager * InvitationManager::self() {
    if (!_self) sd.setObject(_self, new InvitationManager);
    return _self;
}

InvitationManager::InvitationManager()
{
    loadInvitations();

    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), SLOT(loadInvitations()));
    refreshTimer->start(1000*60);
}


InvitationManager::~InvitationManager()
{

}

void InvitationManager::invalidateOldInvitations() {
    int invNum = invitationList.size();

    while(invNum--) {
        if (!invitationList[invNum].isValid()) {
            invitationList.removeAt(invNum);
        }
    }
    saveInvitations();
}


void InvitationManager::loadInvitations()
{
    int invNum = invitationList.size();

    KSharedConfigPtr conf = KGlobal::config();
    KConfigGroup invitationConfig(conf, "Invitations");
    int numInv = invitationConfig.readEntry("invitation_num",0);

    for (int i = 0; i < numInv; i++) {
        KConfigGroup ic(conf, QString("Invitation_%1").arg(i));
        invitationList.append(Invitation(ic));
    }

    invalidateOldInvitations();
    if (numInv != invNum) {
        emit invitationNumChanged(invitationList.size());
    }

}

Invitation InvitationManager::addInvitation()
{
    Invitation i;
    invitationList.append(i);
    emit invitationNumChanged(invitationList.size());
    saveInvitations();
    return i;
}

const QList< Invitation > & InvitationManager::invitations()
{
    return invitationList;
}

void InvitationManager::saveInvitations()
{
    KSharedConfigPtr conf = KGlobal::config();
    KConfigGroup invitationConfig(conf, "Invitations");
    int invNum = invitationList.size();
    invitationConfig.writeEntry("invitation_num",invNum);
    for (int i = 0; i < invNum; i++) {
        KConfigGroup ic(conf, QString("Invitation_%1").arg(i));
        invitationList[i].save(ic);
    }
    conf->sync();
}




