// Copyright (c) 2017 The Zoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ZEROCOINPAGE_H
#define ZEROCOINPAGE_H

#include <QDialog>
#include <QBoxLayout>
#include <QLabel>

namespace Ui {
    class ZeroCoinPage;
}
class AddressTableModel;
class OptionsModel;

QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QSortFilterProxyModel;
class QMenu;
class QModelIndex;
QT_END_NAMESPACE

/** Widget that shows a list of sending or receiving addresses.
  */
class ZeroCoinPage : public QDialog
{
    Q_OBJECT

public:


    explicit ZeroCoinPage(QWidget *parent = 0);
    ~ZeroCoinPage();

    void setModel(AddressTableModel *model);
    void setOptionsModel(OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }
    QHBoxLayout *statusBar;
    QVBoxLayout *statusText;
    QLabel *priceBTC;
    QLabel *priceUSD;

public slots:
    void done(int retval);

private:
    Ui::ZeroCoinPage *ui;
    AddressTableModel *model;
    OptionsModel *optionsModel;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction; // to be able to explicitly disable it
    QString newAddressToSelect;


private slots:
    /** Delete currently selected address entry */
    void on_deleteAddress_clicked();
    /** Create a new address for receiving coins and / or add a new address book entry */
    void on_newAddress_clicked();
    /** Copy address of currently selected address entry to clipboard */
    void on_copyAddress_clicked();
    /** Open the sign message tab in the Sign/Verify Message dialog with currently selected address */
    void on_signMessage_clicked();
    /** Open the verify message tab in the Sign/Verify Message dialog with currently selected address */
    void on_verifyMessage_clicked();
    /** Open send coins dialog for currently selected address (no button) */
    void onSendCoinsAction();
    /** Generate a QR Code from the currently selected address */
    void on_showQRCode_clicked();
    /** Copy label of currently selected address entry to clipboard (no button) */
    void onCopyLabelAction();
    /** Edit currently selected address entry (no button) */
    void onEditAction();
    /** Export button clicked */
    void on_exportButton_clicked();
    /** Zerocoin Mint clicked */
    void on_zerocoinMintButton_clicked();
    /** Zerocoin Spend clicked */
    void on_zerocoinSpendButton_clicked();


    /** Set button states based on selected tab and selection */
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for address book entry */
    void contextualMenu(const QPoint &point);
    /** New entry/entries were added to address table */
    void selectNewAddress(const QModelIndex &parent, int begin, int /*end*/);

signals:
    void signMessage(QString addr);
    void verifyMessage(QString addr);
    void sendCoins(QString addr);
};

#endif // ADDRESSBOOKPAGE_H
