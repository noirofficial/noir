// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H

#include <QDialog>
#include <QString>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace Ui {
    class SendCoinsDialog;
}
class WalletModel;
class SendCoinsEntry;
class SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

/** Dialog for sending bitcoins */
class SendCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendCoinsDialog(QWidget *parent = 0);
    ~SendCoinsDialog();

    void setModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setAddress(const QString &address, const QString &label);
    //void pasteEntry(const SendCoinsRecipient &rv);
    bool handleURI(const QString &uri);
    QHBoxLayout *statusBar;
    QVBoxLayout *statusText;
    QLabel *priceBTC;
    QLabel *priceUSD;

public slots:
    void clear();
    void reject();
    void accept();
    //SendCoinsEntry *addEntry();
    //void updateRemoveEnabled();
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);

private:
    Ui::SendCoinsDialog *ui;
    WalletModel *model;
    bool fNewRecipientAllowed;
    bool validate();
    SendCoinsRecipient getValue();

private slots:
    void on_sendButton_clicked();
    void removeEntry(SendCoinsEntry* entry);
    void updateDisplayUnit();
    void coinControlButtonClicked();
    void coinControlUpdateLabels();

    //void coinControlFeatureChanged(bool);
    /*
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString &);

    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardPriority();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();*/
    void txtChanged();

signals:


};

#endif // SENDCOINSDIALOG_H
