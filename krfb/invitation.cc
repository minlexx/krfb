/***************************************************************************
                               invitation.cpp
                             -------------------
    begin                : Sat Mar 30 2002
    copyright            : (C) 2002 by Tim Jansen
    email                : tim@tjansen.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "invitation.h"

Invitation::Invitation() :
	m_viewItem(0) {
	m_password = KApplication::randomString(4)+
		"-"+
		KApplication::randomString(4);
	m_creationTime = QDateTime::currentDateTime();
	m_expirationTime = QDateTime::currentDateTime().addSecs(INVITATION_DURATION);
}

Invitation::Invitation(const Invitation &x) :
	m_password(x.m_password),
	m_creationTime(x.m_creationTime),
	m_expirationTime(x.m_expirationTime),
	m_viewItem(0) {
}

Invitation::Invitation(KConfig* config, int num) {
	m_password = config->readEntry(QString("password%1").arg(num), "");
	m_creationTime = config->readDateTimeEntry(QString("creation%1").arg(num));
	m_expirationTime = config->readDateTimeEntry(QString("expiration%1").arg(num));
	m_viewItem = 0;
}

Invitation::~Invitation() {
	if (m_viewItem)
		delete m_viewItem;
}

Invitation &Invitation::operator= (const Invitation&x) {
	m_password = x.m_password;
	m_creationTime = x.m_creationTime;
	m_expirationTime = x.m_expirationTime;
	if (m_viewItem)
		delete m_viewItem;
	m_viewItem = 0;
	return *this;
}

void Invitation::save(KConfig *config, int num) const {
	config->writeEntry(QString("password%1").arg(num), m_password);
	config->writeEntry(QString("creation%1").arg(num), m_creationTime);
	config->writeEntry(QString("expiration%1").arg(num), m_expirationTime);
}

QString Invitation::password() const {
	return m_password;
}

QDateTime Invitation::expirationTime() const {
	return m_expirationTime;
}

QDateTime Invitation::creationTime() const {
	return m_creationTime;
}

bool Invitation::isValid() const {
	return m_expirationTime > QDateTime::currentDateTime();
}

void Invitation::setViewItem(KListViewItem *i) {
	if (m_viewItem)
		delete m_viewItem;
	m_viewItem = i;
}

KListViewItem *Invitation::getViewItem() const{
	return m_viewItem;
}