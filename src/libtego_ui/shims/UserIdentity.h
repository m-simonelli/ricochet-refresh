#pragma once

#include "core/IncomingRequestManager.h"

#include "ContactsManager.h"
namespace shims
{
    class UserIdentity : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY(UserIdentity)

        // needed by createDialog("ContactRequestDialog.qml",...) in main.qml
        Q_PROPERTY(QList<QObject*> requests READ requestObjects NOTIFY requestsChanged)
        // used in TorPreferences.qml
        Q_PROPERTY(bool isOnline READ isServiceOnline NOTIFY statusChanged)
        // this originally had a contactIDChanged signal
        Q_PROPERTY(QString contactID READ contactID CONSTANT)
        // needed in MainWindow.qml
        Q_PROPERTY(shims::ContactsManager *contacts READ getContacts CONSTANT)
    public:
        UserIdentity(tego_context_t* context);

        QList<QObject*> requestObjects() const;
        bool isServiceOnline() const;
        QString contactID() const;
        shims::ContactsManager* getContacts();

        void setOnline(bool);

        static UserIdentity* userIdentity;
        shims::ContactsManager contacts;

        tego_context_t* getContext() { return context; }

    signals:
        void statusChanged();
        // used in main.qml
        void requestAdded(IncomingContactRequest *request);
        void requestsChanged();
        // used in MainWindow.qml
        void unreadCountChanged(ContactUser *user, int unreadCount);
        void contactStatusChanged(ContactUser* user, int status);

    private:
        tego_context_t *context;
        bool online;
    };
}