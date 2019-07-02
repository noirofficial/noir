// Copyright (c) 2017 The Noir Core developers
// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletview.h"

#include "addressbookpage.h"
#include "zerocoinpage.h"
#include "sigmapage.h"
#include "askpassphrasedialog.h"
#include "bitcoingui.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "overviewpage.h"
#include "learnmorepage.h"
#include "communitypage.h"
#include "platformstyle.h"
#include "receivecoinsdialog.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "transactiontablemodel.h"
#include "transactionview.h"
#include "walletmodel.h"
#include "receivecoinsdialog.h"

#include "ui_interface.h"
#include "util.h"

#include <math.h>

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QObjectList>

WalletView::WalletView(const PlatformStyle *platformStyle, QWidget *parent):
    QStackedWidget(parent),
    clientModel(0),
    walletModel(0),
    platformStyle(platformStyle)
{

    transactionView = new TransactionView(platformStyle, this);
    QPushButton *exportButton = transactionView->exportButton;
    exportButton->setToolTip(tr("Export the data in the current tab to a file"));

    overviewPage = new OverviewPage(platformStyle);
    receiveCoinsPage = new ReceiveCoinsDialog(platformStyle,this);
    sendCoinsPage = new SendCoinsDialog(platformStyle);
    addressBookPage = new AddressBookPage(platformStyle);
    //usedSendingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::SendingTab, this);
    //usedReceivingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, this);
    zerocoinPage = new ZerocoinPage(platformStyle, ZerocoinPage::ForEditing, this);
    sigmaPage = new SigmaPage(platformStyle, this);
    communityPage = new CommunityPage();

    learnMorePage = new LearnMorePage();
    votingPage = new VotingPage();
    noirnodePage = new Noirnodes(platformStyle);
    addWidget(noirnodePage);

    addWidget(communityPage);
    addWidget(learnMorePage);
    addWidget(overviewPage);
    addWidget(transactionView);
    addWidget(receiveCoinsPage);
    addWidget(sendCoinsPage);
    addWidget(zerocoinPage);
    addWidget(sigmaPage);
    addWidget(addressBookPage);
    addWidget(votingPage);
    /*
    // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    // Clicking on "Export" allows to export the transaction list
    connect(exportButton, SIGNAL(clicked()), transactionView, SLOT(exportClicked()));

    // Pass through messages from sendCoinsPage
    connect(sendCoinsPage, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));
    // Pass through messages from transactionView
    connect(transactionView, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

    */
    connect(overviewPage->send, SIGNAL(pressed()), this, SLOT(gotoSendCoinsPage()));
    connect(overviewPage->receive, SIGNAL(pressed()), this, SLOT(gotoReceiveCoinsPage()));
    connect(overviewPage->transactions, SIGNAL(pressed()), this, SLOT(gotoHistoryPage()));
    // Clicking on a transaction on the overview page simply sends you to transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), this, SLOT(gotoHistoryPage()));
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), transactionView, SLOT(focusTransaction(QModelIndex)));

    // Double-clicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    // Clicking on "Send Coins" in the address book sends you to the send coins tab
    connect(addressBookPage, SIGNAL(sendCoins(QString, QString)), this, SLOT(gotoSendCoinsPage(QString, QString)));
    // Clicking on "Verify Message" in the address book opens the verify message tab in the Sign/Verify Message dialog
    //connect(addressBookPage, SIGNAL(verifyMessage(QString)), this, SLOT(gotoVerifyMessageTab(QString)));
    // Clicking on "Sign Message" in the receive coins page opens the sign message tab in the Sign/Verify Message dialog
    connect(receiveCoinsPage, SIGNAL(signMessage(QString)), this, SLOT(gotoSignMessageTab(QString)));
    // Clicking on "Export" allows to export the transaction list
    connect(exportButton, SIGNAL(clicked()), transactionView, SLOT(exportClicked()));
    //gotoOverviewPage();



    addressBookPage->table->horizontalHeader()->setHidden(true);
    addressBookPage->table->setStyleSheet("background-color: rgb(255, 255, 255); border:0;"
                                 "QTableView::item{font-size:16px;height:30px;}  "
                                 "QHeaderView::section {font-size:16px;color:white;height:40px; background-color:rgb(255,255,255,100); border: 0;}\n"
                                 "QHeaderView {background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #121548, stop: 1 #4a0e95);}\n"
                                  );
    
    // Subscribe message from sigma tab.
    connect(sigmaPage, SIGNAL(message(QString, QString, unsigned int)), this, SIGNAL(message(QString, QString, unsigned int)));

    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    fetchPrice();
    /* Create timer to fetch price every minute or as needed */
    timerId = startTimer(60000);
}

WalletView::~WalletView()
{
    delete nam;
    killTimer(timerId);
}

void WalletView::timerEvent(QTimerEvent *event)
{
    fetchPrice();
}

void WalletView::setBitcoinGUI(BitcoinGUI *gui)
{
    if (gui)
    {

        this->gui = gui;

        gui->progressBar->setStyleSheet("QProgressBar { border: 0px solid grey; border-radius: 0px;background-color: #d8d8d8;} "
                                        "QProgressBar::chunk { background: #531C38; width: 5px; }");
        gui->progressBar->setTextVisible(false);
        gui->progressBar->setFixedSize(300,10);
        gui->progressBarLabel->setStyleSheet("color: rgb(158,158,158)");
        overviewPage->statusText->addWidget(gui->progressBarLabel);
        overviewPage->statusBar->addWidget(gui->progressBar);

        // Clicking on a transaction on the overview page simply sends you to transaction history page
        connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), gui, SLOT(gotoHistoryPage()));

        // Receive and report messages
        connect(this, SIGNAL(message(QString,QString,unsigned int)), gui, SLOT(message(QString,QString,unsigned int)));

        // Pass through encryption status changed signals
        connect(this, SIGNAL(encryptionStatusChanged(int)), gui, SLOT(setEncryptionStatus(int)));

        // Pass through transaction notifications
        connect(this, SIGNAL(incomingTransaction(QString,int,CAmount,QString,QString,QString)), gui, SLOT(incomingTransaction(QString,int,CAmount,QString,QString,QString)));
    }
}

void WalletView::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;

    overviewPage->setClientModel(clientModel);
    sendCoinsPage->setClientModel(clientModel);
    addressBookPage->setOptionsModel(clientModel->getOptionsModel());
    noirnodePage->setClientModel(clientModel);
    sigmaPage->setClientModel(clientModel);
}

void WalletView::setWalletModel(WalletModel *walletModel)
{

    this->walletModel = walletModel;

    // Put transaction list in tabs
    transactionView->setModel(walletModel);
    overviewPage->setWalletModel(walletModel);
    receiveCoinsPage->setModel(walletModel->getAddressTableModel());
    receiveCoinsPage->setWalletModel(walletModel);
    sendCoinsPage->setModel(walletModel);
    zerocoinPage->setModel(walletModel->getAddressTableModel());
    sigmaPage->setWalletModel(walletModel);
    addressBookPage->setModel(walletModel->getAddressTableModel());
    noirnodePage->setWalletModel(walletModel);
    votingPage->setWalletModel(walletModel);
    //usedReceivingAddressesPage->setModel(walletModel->getAddressTableModel());
    //usedSendingAddressesPage->setModel(walletModel->getAddressTableModel());

    if (walletModel)
    {
        // Receive and pass through messages from wallet model
        connect(walletModel, SIGNAL(message(QString,QString,unsigned int)), this, SIGNAL(message(QString,QString,unsigned int)));

        // Handle changes in encryption status
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SIGNAL(encryptionStatusChanged(int)));
        updateEncryptionStatus();

        // Balloon pop-up for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(processNewTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));

        // Show progress dialog
        connect(walletModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));
    }
}

void WalletView::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!walletModel || !clientModel || clientModel->inInitialBlockDownload())
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    if (!ttm || ttm->processingQueuedTransactions())
        return;

    QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
    QModelIndex index = ttm->index(start, 0, parent);
    QString address = ttm->data(index, TransactionTableModel::AddressRole).toString();
    QString label = ttm->data(index, TransactionTableModel::LabelRole).toString();

    Q_EMIT incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address, label);
}

void WalletView::gotoOverviewPage()
{
    //gui->getOverviewAction()->setChecked(true);
    setCurrentWidget(overviewPage);
    overviewPage->statusBar->addWidget(gui->frameBlocks, 0, Qt::AlignRight);
    overviewPage->statusText->addWidget(gui->progressBarLabel);
    overviewPage->statusBar->addWidget(gui->progressBar);
}


void WalletView::gotoCommunityPage()
{
    //gui->getOverviewAction()->setChecked(true);
    setCurrentWidget(communityPage);
}

void WalletView::gotoHistoryPage()
{ 
    transactionView->statusBar->addWidget(gui->frameBlocks, 0, Qt::AlignRight);
    transactionView->statusText->addWidget(gui->progressBarLabel);
    transactionView->statusBar->addWidget(gui->progressBar);
   // gui->getHistoryAction()->setChecked(true);
    gui->menu->SimulateTransactionsClick();
    setCurrentWidget(transactionView);
}

void WalletView::gotoAddressBookPage()
{
    addressBookPage->statusBar->addWidget(gui->frameBlocks, 0, Qt::AlignRight);
    addressBookPage->statusText->addWidget(gui->progressBarLabel);
    addressBookPage->statusBar->addWidget(gui->progressBar);
    //gui->getAddressBookAction()->setChecked(true);
    gui->menu->SimulateAddressClick();
    setCurrentWidget(addressBookPage);
}

void WalletView::gotoReceiveCoinsPage()
{
    receiveCoinsPage->statusBar->addWidget(gui->frameBlocks, 0, Qt::AlignRight);
    receiveCoinsPage->statusText->addWidget(gui->progressBarLabel);
    receiveCoinsPage->statusBar->addWidget(gui->progressBar);
    //gui->getReceiveCoinsAction()->setChecked(true);
    gui->menu->SimulateReceiveClick();
    setCurrentWidget(receiveCoinsPage);
}

void WalletView::gotoNoirnodePage()
{
    noirnodePage->statusBar->addWidget(gui->frameBlocks, 0, Qt::AlignRight);
    noirnodePage->statusText->addWidget(gui->progressBarLabel);
    noirnodePage->statusBar->addWidget(gui->progressBar);
    gui->menu->SimulateNoirnodeClick();
    setCurrentWidget(noirnodePage);
}
void WalletView::gotoZerocoinPage()
{
    zerocoinPage->statusBar->addWidget(gui->frameBlocks, 0, Qt::AlignRight);
    zerocoinPage->statusText->addWidget(gui->progressBarLabel);
    zerocoinPage->statusBar->addWidget(gui->progressBar);
    //gui->getZerocoinAction()->setChecked(true);
    //gui->menu->SimulateZerocoinClick();
    setCurrentWidget(zerocoinPage);
}

void WalletView::gotoSigmaPage()
{
    sigmaPage->statusBar->addWidget(gui->frameBlocks, 0, Qt::AlignRight);
    sigmaPage->statusText->addWidget(gui->progressBarLabel);
    sigmaPage->statusBar->addWidget(gui->progressBar);
    gui->menu->SimulateSigmaClick();
    setCurrentWidget(sigmaPage);
}

void WalletView::gotoLearnMorePage()
{
    //gui->getOverviewAction()->setChecked(true);
    setCurrentWidget(learnMorePage);

}
void WalletView::gotoVotingPage()
{
    votingPage->statusBar->addWidget(gui->frameBlocks, 0, Qt::AlignRight);
    votingPage->statusText->addWidget(gui->progressBarLabel);
    votingPage->statusBar->addWidget(gui->progressBar);
    gui->menu->SimulateVotingClick();
    setCurrentWidget(votingPage);

}

void WalletView::gotoSendCoinsPage(QString addr, QString name)
{
    sendCoinsPage->statusBar->addWidget(gui->frameBlocks, 0, Qt::AlignRight);
    sendCoinsPage->statusText->addWidget(gui->progressBarLabel);
    sendCoinsPage->statusBar->addWidget(gui->progressBar);
    //gui->getSendCoinsAction()->setChecked(true);
    gui->menu->SimulateSendClick();
    setCurrentWidget(sendCoinsPage);
    if (!addr.isEmpty())
        sendCoinsPage->setAddress(addr, name);
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_SM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void WalletView::gotoVerifyMessageTab(QString addr)
{
    // calls show() in showTab_VM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_VM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

bool WalletView::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    return sendCoinsPage->handlePaymentRequest(recipient);
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
    overviewPage->showOutOfSyncWarning(fShow);
}

void WalletView::updateEncryptionStatus()
{
    Q_EMIT encryptionStatusChanged(walletModel->getEncryptionStatus());
}

void WalletView::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt : AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    updateEncryptionStatus();
}

void WalletView::backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Wallet"), QString(),
        tr("Wallet Data (*.dat)"), NULL);

    if (filename.isEmpty())
        return;

    if (!walletModel->backupWallet(filename)) {
        Q_EMIT message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        Q_EMIT message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void WalletView::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void WalletView::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void WalletView::usedSendingAddresses()
{
    if(!walletModel)
        return;

  //  usedSendingAddressesPage->show();
  //  usedSendingAddressesPage->raise();
  //  usedSendingAddressesPage->activateWindow();
}

void WalletView::usedReceivingAddresses()
{
    if(!walletModel)
        return;

    //usedReceivingAddressesPage->show();
   // usedReceivingAddressesPage->raise();
    //usedReceivingAddressesPage->activateWindow();
}

void WalletView::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}


void WalletView::fetchPrice()
{

    QSettings configs;
    QNetworkRequest request;
    QNetworkReply *reply = NULL;

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2);
    request.setSslConfiguration(config);
    if(configs.value("Currency").toInt() == 0)
        request.setUrl(QUrl("https://api.coingecko.com/api/v3/coins/bring?localization=false&tickers=false&market_data=true&community_data=false&developer_data=false&sparkline=false"));
    else if(configs.value("Currency").toInt() == 1)
        request.setUrl(QUrl("https://api.coingecko.com/api/v3/coins/bring?localization=false&tickers=false&market_data=true&community_data=false&developer_data=false&sparkline=false"));

    request.setHeader(QNetworkRequest::ServerHeader, "application/json");
    //url.setPort(8850);
    nam->get(request);
}

void WalletView::replyFinished(QNetworkReply *reply)
{

    QSettings configs;
    try{
    QByteArray bytes = reply->readAll();
    QString str = QString::fromUtf8(bytes.data(), bytes.size());
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    size_t s;
    string newPriceUSD;
    if(configs.value("Currency").toInt() == 0){
        s = str.toStdString().find("\"usd\":");
        newPriceUSD = "$";
    }
    else if(configs.value("Currency").toInt() == 1){
        s = str.toStdString().find("\"eur\":");
        newPriceUSD = "€";
    }
    size_t e = str.toStdString().find(",", s);
    string priceUSD = str.toStdString().substr(s + 6, e - s - 6);
    QString priceUSDq = QString::fromStdString(priceUSD);
    qDebug()<< priceUSDq;
    s = str.toStdString().find("\"btc\":");
    e = str.toStdString().find(",", s);
    string priceBTC = str.toStdString().substr(s + 6, e - s - 6);
    QString priceBTCq = QString::fromStdString(priceBTC);
    qDebug()<< priceBTCq;
    newPriceUSD.append(priceUSD);

    QString temp = overviewPage->labelBalance->text() + overviewPage->labelBalanceDecimal->text();
    s = temp.toStdString().find(" N");
    QString walletAmountConfirmed = QString::fromStdString(temp.toStdString().substr(0, s));

    temp = overviewPage->labelUnconfirmed->text() + overviewPage->labelUnconfirmedDecimal->text();
    s = temp.toStdString().find(" N");
    QString walletAmountUnconfirmed = QString::fromStdString(temp.toStdString().substr(0, s));


    try{
        if(configs.value("Currency").toInt() == 0){
            newPriceUSD = "$"+ QString::number(priceUSDq.toDouble(), 'f', 2).toStdString();
            overviewPage->labelBalanceUSD->setText(QString::number(priceBTCq.toDouble() * walletAmountConfirmed.toDouble(), 'f', 4) + " BTC " + "($" + QString::number(priceUSDq.toDouble() * walletAmountConfirmed.toDouble(), 'f', 2) + ")");
            overviewPage->labelUnconfirmedUSD->setText(QString::number(priceBTCq.toDouble() * walletAmountUnconfirmed.toDouble(), 'f', 4) + " BTC " + "($" + QString::number(priceUSDq.toDouble() * walletAmountUnconfirmed.toDouble(), 'f', 2) + ")");
        }
        if(configs.value("Currency").toInt() == 1){
            newPriceUSD = "€"+ QString::number(priceUSDq.toDouble(), 'f', 2).toStdString();
            overviewPage->labelBalanceUSD->setText(QString::number(priceBTCq.toDouble() * walletAmountConfirmed.toDouble(), 'f', 4) + " BTC " + "(€" + QString::number(priceUSDq.toDouble() * walletAmountConfirmed.toDouble(), 'f', 2) + ")");
            overviewPage->labelUnconfirmedUSD->setText(QString::number(priceBTCq.toDouble() * walletAmountUnconfirmed.toDouble(), 'f', 4) + " BTC " + "(€" + QString::number(priceUSDq.toDouble() * walletAmountUnconfirmed.toDouble(), 'f', 2) + ")");
        }
        overviewPage->priceUSD->setText(QString::fromStdString(newPriceUSD));
        overviewPage->priceBTC->setText(QString::number(priceBTCq.toDouble(), 'f', 8) + " BTC");
        sendCoinsPage->priceUSD->setText(QString::fromStdString(newPriceUSD));
        sendCoinsPage->priceBTC->setText(QString::number(priceBTCq.toDouble(), 'f', 8) + " BTC");
        receiveCoinsPage->priceUSD->setText(QString::fromStdString(newPriceUSD));
        receiveCoinsPage->priceBTC->setText(QString::number(priceBTCq.toDouble(), 'f', 8) + " BTC");
        zerocoinPage->priceUSD->setText(QString::fromStdString(newPriceUSD));
        zerocoinPage->priceBTC->setText(QString::number(priceBTCq.toDouble(), 'f', 8) + " BTC");
        addressBookPage->priceUSD->setText(QString::fromStdString(newPriceUSD));
        addressBookPage->priceBTC->setText(QString::number(priceBTCq.toDouble(), 'f', 8) + " BTC");
        transactionView->priceUSD->setText(QString::fromStdString(newPriceUSD));
        transactionView->priceBTC->setText(QString::number(priceBTCq.toDouble(), 'f', 8) + " BTC");
        noirnodePage->priceUSD->setText(QString::fromStdString(newPriceUSD));
        noirnodePage->priceBTC->setText(QString::number(priceBTCq.toDouble(), 'f', 8) + " BTC");
        votingPage->priceUSD->setText(QString::fromStdString(newPriceUSD));
        votingPage->priceBTC->setText(QString::number(priceBTCq.toDouble(), 'f', 8) + " BTC");
        sigmaPage->priceUSD->setText(QString::fromStdString(newPriceUSD));
        sigmaPage->priceBTC->setText(QString::number(priceBTCq.toDouble(), 'f', 8) + " BTC");
    }
    catch(...){
        LogPrintf("Error receiving price data\n");
    }
    }
    catch(...){
        LogPrintf("Error connecting ticker\n");
    }

    delete reply;
}
