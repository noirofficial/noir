#include "zoinodes.h"
#include "ui_zoinodes.h"

#include "activezoinode.h"
#include "clientmodel.h"
#include "init.h"
#include "guiutil.h"
#include "zoinode-sync.h"
#include "zoinodeconfig.h"
#include "zoinodeman.h"
#include "sync.h"
#include "wallet/wallet.h"
#include "walletmodel.h"
#include "utiltime.h"

#include <QTimer>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QLinearGradient>


int GetOffsetFromUtc()
{
#if QT_VERSION < 0x050200
    const QDateTime dateTime1 = QDateTime::currentDateTime();
    const QDateTime dateTime2 = QDateTime(dateTime1.date(), dateTime1.time(), Qt::UTC);
    return dateTime1.secsTo(dateTime2);
#else
    return QDateTime::currentDateTime().offsetFromUtc();
#endif
}


Zoinodes::Zoinodes(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Zoinodes),
    clientModel(0),
    walletModel(0)
{

    ui->setupUi(this);

    statusBar = ui->statusBar;
    statusText = ui->statusText;
    priceUSD = ui->priceUSD;
    priceBTC = ui->priceBTC;
    //ui->startButton_4->setEnabled(false);

    int columnAliasWidth = 80;
    int columnAddressWidth = 80;
    int columnProtocolWidth = 80;
    int columnStatusWidth = 80;
    int columnActiveWidth = 80;
    int columnLastSeenWidth = 80;

    ui->tableWidgetMyZoinodes_4->setColumnWidth(0, columnAliasWidth);
    ui->tableWidgetMyZoinodes_4->setColumnWidth(1, columnAddressWidth);
    ui->tableWidgetMyZoinodes_4->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMyZoinodes_4->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMyZoinodes_4->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMyZoinodes_4->setColumnWidth(5, columnLastSeenWidth);
    ui->tableWidgetMyZoinodes_4->horizontalHeader()->setFixedHeight(50);
    ui->tableWidgetZoinodes_4->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetZoinodes_4->setColumnWidth(1, columnProtocolWidth);
    ui->tableWidgetZoinodes_4->setColumnWidth(2, columnStatusWidth);
    ui->tableWidgetZoinodes_4->setColumnWidth(3, columnActiveWidth);
    ui->tableWidgetZoinodes_4->setColumnWidth(4, columnLastSeenWidth);

    //ui->tableWidgetMyZoinodes_4->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableWidgetZoinodes_4->horizontalHeader()->setFixedHeight(50);

    //connect(ui->tableWidgetMyZoinodes_4, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(ui->startButton_4, SIGNAL(clicked()), this, SLOT(on_startButton_clicked()));
    connect(ui->startAllButton_4, SIGNAL(clicked()), this, SLOT(on_startAllButton_clicked()));
    connect(ui->startMissingButton_4, SIGNAL(clicked()), this, SLOT(on_startMissingButton_clicked()));
    connect(ui->UpdateButton_4, SIGNAL(clicked()), this, SLOT(on_UpdateButton_clicked()));
    connect(ui->swapButton, SIGNAL(triggered()), this, SLOT(on_swapButton_clicked()));
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    timer->start(1000);

    fFilterUpdated = false;
    nTimeFilterUpdated = GetTime();
    updateNodeList();


    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setOffset(0);
    effect->setBlurRadius(20.0);

    ui->frame_2->setGraphicsEffect(effect);

    ui->autoupdate_label_4->setVisible(true);
    ui->secondsLabel_4->setVisible(true);
    ui->label->setVisible(true);
    ui->tabLabel->setText("My Zoinodes ");
    ui->swapButton->setText("All Zoinodes ");
    ui->label_count_4->setText("My Node Count: ");
    ui->countLabel_4->setText(QString::number(ui->tableWidgetMyZoinodes_4->rowCount()));
    ui->tabWidget->setCurrentIndex(0);
}


Zoinodes::~Zoinodes()
{
    delete ui;
}



void Zoinodes::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model) {
        // try to update list when zoinode count changes
        connect(clientModel, SIGNAL(strZoinodesChanged(QString)), this, SLOT(updateNodeList()));
    }
}

void Zoinodes::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void Zoinodes::showContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidgetMyZoinodes_4->itemAt(point);
    if(item) contextMenu->exec(QCursor::pos());
}

void Zoinodes::StartAlias(std::string strAlias)
{
    std::string strStatusHtml;
    strStatusHtml += "<center>Alias: " + strAlias;

    BOOST_FOREACH(CZoinodeConfig::CZoinodeEntry mne, zoinodeConfig.getEntries()) {
        if(mne.getAlias() == strAlias) {
            std::string strError;
            CZoinodeBroadcast mnb;

            bool fSuccess = CZoinodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

            if(fSuccess) {
                strStatusHtml += "<br>Successfully started zoinode.";
                mnodeman.UpdateZoinodeList(mnb);
                mnb.RelayZoiNode();
                mnodeman.NotifyZoinodeUpdates();
            } else {
                strStatusHtml += "<br>Failed to start zoinode.<br>Error: " + strError;
            }
            break;
        }
    }
    strStatusHtml += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(strStatusHtml));
    msg.exec();

    updateMyNodeList(true);
}

void Zoinodes::StartAll(std::string strCommand)
{
    int nCountSuccessful = 0;
    int nCountFailed = 0;
    std::string strFailedHtml;

    BOOST_FOREACH(CZoinodeConfig::CZoinodeEntry mne, zoinodeConfig.getEntries()) {
        std::string strError;
        CZoinodeBroadcast mnb;

        int32_t nOutputIndex = 0;
        if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
            continue;
        }

        COutPoint outpoint = COutPoint(uint256S(mne.getTxHash()), nOutputIndex);

        if(strCommand == "start-missing" && mnodeman.Has(CTxIn(outpoint))) continue;

        bool fSuccess = CZoinodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

        if(fSuccess) {
            nCountSuccessful++;
            mnodeman.UpdateZoinodeList(mnb);
            mnb.RelayZoiNode();
            mnodeman.NotifyZoinodeUpdates();
        } else {
            nCountFailed++;
            strFailedHtml += "\nFailed to start " + mne.getAlias() + ". Error: " + strError;
        }
    }
    pwalletMain->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d zoinodes, failed to start %d, total %d", nCountSuccessful, nCountFailed, nCountFailed + nCountSuccessful);
    if (nCountFailed > 0) {
        returnObj += strFailedHtml;
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();

    updateMyNodeList(true);
}

void Zoinodes::updateMyZoinodeInfo(QString strAlias, QString strAddr, const COutPoint& outpoint)
{
    bool fOldRowFound = false;
    int nNewRow = 0;

    for(int i = 0; i < ui->tableWidgetMyZoinodes_4->rowCount(); i++) {
        if(ui->tableWidgetMyZoinodes_4->item(i, 0)->text() == strAlias) {
            fOldRowFound = true;
            nNewRow = i;
            break;
        }
    }

    if(nNewRow == 0 && !fOldRowFound) {
        nNewRow = ui->tableWidgetMyZoinodes_4->rowCount();
        ui->tableWidgetMyZoinodes_4->insertRow(nNewRow);
    }

    zoinode_info_t infoMn = mnodeman.GetZoinodeInfo(CTxIn(outpoint));
    bool fFound = infoMn.fInfoValid;

    QTableWidgetItem *aliasItem = new QTableWidgetItem(strAlias);
    QTableWidgetItem *addrItem = new QTableWidgetItem(fFound ? QString::fromStdString(infoMn.addr.ToString()) : strAddr);
    QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(fFound ? infoMn.nProtocolVersion : -1));
    QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(fFound ? CZoinode::StateToString(infoMn.nActiveState) : "MISSING"));
    QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(fFound ? (infoMn.nTimeLastPing - infoMn.sigTime) : 0)));
    QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M",
                                                                                                   fFound ? infoMn.nTimeLastPing + GetOffsetFromUtc() : 0)));
    QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(fFound ? CBitcoinAddress(infoMn.pubKeyCollateralAddress.GetID()).ToString() : ""));
    QLinearGradient grad1(50,50,50,50);
    grad1.setColorAt(0,QColor(0,0,0,0));
    grad1.setColorAt(0,QColor(255,255,255));
    QBrush brush(grad1);
    aliasItem->setBackground(brush);
    ui->tableWidgetMyZoinodes_4->setItem(nNewRow, 0, aliasItem);
    ui->tableWidgetMyZoinodes_4->setItem(nNewRow, 1, addrItem);
    ui->tableWidgetMyZoinodes_4->setItem(nNewRow, 2, protocolItem);
    ui->tableWidgetMyZoinodes_4->setItem(nNewRow, 3, statusItem);
    ui->tableWidgetMyZoinodes_4->setItem(nNewRow, 4, activeSecondsItem);
    ui->tableWidgetMyZoinodes_4->setItem(nNewRow, 5, lastSeenItem);
    ui->tableWidgetMyZoinodes_4->setItem(nNewRow, 6, pubkeyItem);
    //ui->tableWidgetMyZoinodes_4->setColumnWidth(0, 100);

}

void Zoinodes::updateMyNodeList(bool fForce)
{
    TRY_LOCK(cs_mymnlist, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }
    static int64_t nTimeMyListUpdated = 0;

    // automatically update my zoinode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t nSecondsTillUpdate = nTimeMyListUpdated + MY_MASTERNODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel_4->setText(QString::number(nSecondsTillUpdate) + "s");

    if(nSecondsTillUpdate > 0 && !fForce) return;
    nTimeMyListUpdated = GetTime();

    ui->tableWidgetZoinodes_4->setSortingEnabled(false);
    BOOST_FOREACH(CZoinodeConfig::CZoinodeEntry mne, zoinodeConfig.getEntries()) {
        int32_t nOutputIndex = 0;
        if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
            continue;
        }

        updateMyZoinodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), COutPoint(uint256S(mne.getTxHash()), nOutputIndex));
    }
    ui->tableWidgetZoinodes_4->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel_4->setText("0");
}

void Zoinodes::updateNodeList()
{
    TRY_LOCK(cs_mnlist, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }

    static int64_t nTimeListUpdated = GetTime();

    // to prevent high cpu usage update only once in MASTERNODELIST_UPDATE_SECONDS seconds
    // or MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
    int64_t nSecondsToWait = fFilterUpdated
                            ? nTimeFilterUpdated - GetTime() + MASTERNODELIST_FILTER_COOLDOWN_SECONDS
                            : nTimeListUpdated - GetTime() + MASTERNODELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countLabel_4->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(nSecondsToWait > 0) return;

    nTimeListUpdated = GetTime();
    fFilterUpdated = false;

    QString strToFilter;
    ui->countLabel_4->setText("Updating...");
    ui->tableWidgetZoinodes_4->setSortingEnabled(false);
    ui->tableWidgetZoinodes_4->clearContents();
    ui->tableWidgetZoinodes_4->setRowCount(0);
//    std::map<COutPoint, CZoinode> mapZoinodes = mnodeman.GetFullZoinodeMap();
    std::vector<CZoinode> vZoinodes = mnodeman.GetFullZoinodeVector();
    int offsetFromUtc = GetOffsetFromUtc();

    BOOST_FOREACH(CZoinode & mn, vZoinodes)
    {
//        CZoinode mn = mnpair.second;
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
        QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(mn.nProtocolVersion));
        QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(mn.GetStatus()));
        QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(mn.lastPing.sigTime - mn.sigTime)));
        QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat("%Y-%m-%d %H:%M", mn.lastPing.sigTime + offsetFromUtc)));
        QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString()));

        if (strCurrentFilter != "")
        {
            strToFilter =   addressItem->text() + " " +
                            protocolItem->text() + " " +
                            statusItem->text() + " " +
                            activeSecondsItem->text() + " " +
                            lastSeenItem->text() + " " +
                            pubkeyItem->text();
            if (!strToFilter.contains(strCurrentFilter)) continue;
        }

        ui->tableWidgetZoinodes_4->insertRow(0);
        ui->tableWidgetZoinodes_4->setItem(0, 0, addressItem);
        ui->tableWidgetZoinodes_4->setItem(0, 1, protocolItem);
        ui->tableWidgetZoinodes_4->setItem(0, 2, statusItem);
        ui->tableWidgetZoinodes_4->setItem(0, 3, activeSecondsItem);
        ui->tableWidgetZoinodes_4->setItem(0, 4, lastSeenItem);
        ui->tableWidgetZoinodes_4->setItem(0, 5, pubkeyItem);
    }

    if(ui->tabWidget->currentIndex()==0){
        ui->label_count_4->setText("My Node Count: ");
        ui->countLabel_4->setText(QString::number(ui->tableWidgetMyZoinodes_4->rowCount()));
    }
    else if(ui->tabWidget->currentIndex()==1){
        ui->label_count_4->setText("Total Node Count: ");
        ui->countLabel_4->setText(QString::number(ui->tableWidgetZoinodes_4->rowCount()));
    }
    ui->tableWidgetZoinodes_4->setSortingEnabled(true);
}

void Zoinodes::on_filterLineEdit_textChanged(const QString &strFilterIn)
{
    strCurrentFilter = strFilterIn;
    nTimeFilterUpdated = GetTime();
    fFilterUpdated = true;
    ui->countLabel_4->setText(QString::fromStdString(strprintf("Please wait... %d", MASTERNODELIST_FILTER_COOLDOWN_SECONDS)));
}

void Zoinodes::on_startButton_clicked()
{
    std::cout << "START" << std::endl;
    std::string strAlias;
    {
        LOCK(cs_mymnlist);
        // Find selected node alias
        QItemSelectionModel* selectionModel = ui->tableWidgetMyZoinodes_4->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();

        if(selected.count() == 0) return;

        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        strAlias = ui->tableWidgetMyZoinodes_4->item(nSelectedRow, 0)->text().toStdString();
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm zoinode start"),
        tr("Are you sure you want to start zoinode %1?").arg(QString::fromStdString(strAlias)),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAlias(strAlias);
        return;
    }

    StartAlias(strAlias);
}

void Zoinodes::on_startAllButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all zoinodes start"),
        tr("Are you sure you want to start ALL zoinodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll();
        return;
    }

    StartAll();
}

void Zoinodes::on_startMissingButton_clicked()
{

    if(!zoinodeSync.IsZoinodeListSynced()) {
        QMessageBox::critical(this, tr("Command is not available right now"),
            tr("You can't use this command until zoinode list is synced"));
        return;
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this,
        tr("Confirm missing zoinodes start"),
        tr("Are you sure you want to start MISSING zoinodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll("start-missing");
        return;
    }

    StartAll("start-missing");
}

void Zoinodes::on_tableWidgetMyZoinodes_itemSelectionChanged()
{
    if(ui->tableWidgetMyZoinodes_4->selectedItems().count() > 0) {
        ui->startButton_4->setEnabled(true);
    }
}

void Zoinodes::on_UpdateButton_clicked()
{
    updateMyNodeList(true);
}

void Zoinodes::on_swapButton_clicked()
{
    if(ui->tabWidget->currentIndex()==0){
        ui->autoupdate_label_4->setVisible(false);
        ui->secondsLabel_4->setVisible(false);
        ui->label->setVisible(false);
        ui->tabLabel->setText("All Zoinodes ");
        ui->swapButton->setText("My Zoinodes ");
        ui->label_count_4->setText("Total Node Count: ");
        ui->countLabel_4->setText(QString::number(ui->tableWidgetZoinodes_4->rowCount()));
        ui->tabWidget->setCurrentIndex(1);
        return;
    }
    else if(ui->tabWidget->currentIndex()==1){
        ui->autoupdate_label_4->setVisible(true);
        ui->secondsLabel_4->setVisible(true);
        ui->label->setVisible(true);
        ui->tabLabel->setText("My Zoinodes ");
        ui->swapButton->setText("All Zoinodes ");
        ui->label_count_4->setText("My Node Count: ");
        ui->countLabel_4->setText(QString::number(ui->tableWidgetMyZoinodes_4->rowCount()));
        ui->tabWidget->setCurrentIndex(0);
        return;
    }
}

