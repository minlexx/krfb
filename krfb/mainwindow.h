/* This file is part of the KDE project
   Copyright (C) 2007 Alessandro Praduroux <pradu@pradu.it>
   Copyright (C) 2013 Amandeep Singh <aman.dedman@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#ifndef MANAGEINVITATIONSDIALOG_H
#define MANAGEINVITATIONSDIALOG_H

#include "ui_mainwidget.h"

#include <KXmlGuiWindow>

#ifdef KRFB_WITH_KDE_TELEPATHY
#include <KTp/contact.h>
#include <TelepathyQt/PendingReady>
namespace KTp {
    class ContactViewWidget;
}
namespace Tp {
    class PendingOperation;
}
#endif

class KLineEdit;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = 0);
        ~MainWindow();

    public Q_SLOTS:
        void showConfiguration();

    protected:
        virtual void readProperties(const KConfigGroup & group);
        virtual void saveProperties(KConfigGroup & group);

    private Q_SLOTS:
        void editPassword();
        void editUnattendedPassword();
        void toggleDesktopSharing(bool enable);
        void passwordChanged(const QString&);
        void aboutConnectionAddress();
        void aboutUnattendedMode();
#ifdef KRFB_WITH_KDE_TELEPATHY
        void onContactDoubleClicked(const Tp::AccountPtr &, const KTp::ContactPtr &);
        void pendingDesktopShareFinished(Tp::PendingOperation*);
#endif

    private:
        Ui::MainWidget m_ui;
        bool m_passwordEditable;
        KLineEdit *m_passwordLineEdit;
#ifdef KRFB_WITH_KDE_TELEPATHY
        KTp::ContactViewWidget *m_contactViewWidget;
#endif

};

#endif
