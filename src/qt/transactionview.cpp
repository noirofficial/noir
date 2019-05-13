// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionview.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactiondescdialog.h"
#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"

#include "ui_transactionview.h"

#include "ui_interface.h"

#include <QGraphicsDropShadowEffect>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QSignalMapper>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>

TransactionView::TransactionView(const PlatformStyle *platformStyle, QWidget *parent) :
QWidget(parent), ui(new Ui::TransactionView), model(0), transactionProxyModel(0),
transactionView(0), abandonAction(0), columnResizingFixer(0)
{
    
    ui->setupUi(this);
    statusBar = ui->statusBar;
    statusText = ui->statusText;
    priceBTC = ui->priceBTC;
    priceUSD = ui->priceUSD;
    exportButton = ui->exportButton;
    
    
    //effect->setColor(QColor(247, 247, 247, 25));
    //setGraphicsEffect(effect);
    
    
    
    dateWidget = ui->dateWidget;
    ui->dateWidget->addItem(tr("All"), All);
    ui->dateWidget->addItem(tr("Today"), Today);
    ui->dateWidget->addItem(tr("This week"), ThisWeek);
    ui->dateWidget->addItem(tr("This month"), ThisMonth);
    ui->dateWidget->addItem(tr("Last month"), LastMonth);
    ui->dateWidget->addItem(tr("This year"), ThisYear);
    //ui->dateWidget->addItem(tr("Range..."), Range);
    //ui->dateWidget->setStyleSheet("font-size: 14px;border: 1px solid #D3D3D3;border-radius: 2px;padding: 8px;color: #333;");
    //dateWidget->setGraphicsEffect(effect);
    
    typeWidget = ui->typeWidget;
    ui->typeWidget->addItem(tr("All"), TransactionFilterProxy::ALL_TYPES);
    ui->typeWidget->addItem(tr("Received with"), TransactionFilterProxy::TYPE(TransactionRecord::RecvWithAddress) |
                            TransactionFilterProxy::TYPE(TransactionRecord::RecvFromOther));
    ui->typeWidget->addItem(tr("Sent to"), TransactionFilterProxy::TYPE(TransactionRecord::SendToAddress) |
                            TransactionFilterProxy::TYPE(TransactionRecord::SendToOther));
    ui->typeWidget->addItem(tr("To yourself"), TransactionFilterProxy::TYPE(TransactionRecord::SendToSelf));
    ui->typeWidget->addItem(tr("Mined"), TransactionFilterProxy::TYPE(TransactionRecord::Generated));
    ui->typeWidget->addItem(tr("Other"), TransactionFilterProxy::TYPE(TransactionRecord::Other));
    ui->typeWidget->addItem(tr("Spend to"), TransactionFilterProxy::TYPE(TransactionRecord::SpendToAddress));
    ui->typeWidget->addItem(tr("Spend to yourself"), TransactionFilterProxy::TYPE(TransactionRecord::SpendToSelf));
    ui->typeWidget->addItem(tr("Mint"), TransactionFilterProxy::TYPE(TransactionRecord::Mint));
    //ui->typeWidget->setStyleSheet("font-size: 14px;border: 1px solid #D3D3D3;border-radius: 2px;padding: 8px;color: #333;");
    //typeWidget->setGraphicsEffect(effect);
    
    addressWidget = ui->addressWidget;
    ui->addressWidget->setFixedHeight(40);
    //ui->addressWidget->setStyleSheet("font-size: 14px;");
    //addressWidget->setGraphicsEffect(effect);
    
    amountWidget = ui->amountWidget;
    //amountWidget->setGraphicsEffect(effect);
    amountWidget->setValidator(new QDoubleValidator(0, 1e20, 8, this));
    
    
    QTableView *view = ui->tableView;
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setShowGrid(false);
    view->setTabKeyNavigation(false);
    view->setAlternatingRowColors(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSortingEnabled(true);
    view->verticalHeader()->setVisible(false);
    view->horizontalHeader()->setFixedHeight(50);
    //view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    //view->setGraphicsEffect(effect);
    transactionView = ui->tableView;
    
    
    abandonAction = new QAction(tr("Abandon transaction"), this);
    resendAction = new QAction(tr("Re-broadcast transaction"), this);
    
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *copyTxHexAction = new QAction(tr("Copy raw transaction"), this);
    QAction *copyTxPlainText = new QAction(tr("Copy full transaction details"), this);
    QAction *editLabelAction = new QAction(tr("Edit label"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);
    
    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(copyTxHexAction);
    contextMenu->addAction(copyTxPlainText);
    contextMenu->addAction(showDetailsAction);
    contextMenu->addSeparator();
    contextMenu->addAction(abandonAction);
    contextMenu->addAction(editLabelAction);
    contextMenu->addAction(resendAction);
    
    mapperThirdPartyTxUrls = new QSignalMapper(this);
    
    // Connect actions
    connect(mapperThirdPartyTxUrls, SIGNAL(mapped(QString)), this, SLOT(openThirdPartyTxUrl(QString)));
    
    connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    connect(typeWidget, SIGNAL(activated(int)), this, SLOT(chooseType(int)));
    //connect(watchOnlyWidget, SIGNAL(activated(int)), this, SLOT(chooseWatchonly(int)));
    connect(addressWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPrefix(QString)));
    connect(amountWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAmount(QString)));
    
    connect(view, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)));
    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
    
    connect(abandonAction, SIGNAL(triggered()), this, SLOT(abandonTx()));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(copyTxHexAction, SIGNAL(triggered()), this, SLOT(copyTxHex()));
    connect(copyTxPlainText, SIGNAL(triggered()), this, SLOT(copyTxPlainText()));
    connect(editLabelAction, SIGNAL(triggered()), this, SLOT(editLabel()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
    connect(resendAction, SIGNAL(triggered()), this, SLOT(rebroadcastTx()));
    

    
    
    
}

void TransactionView::setModel(WalletModel *model)
{
    this->model = model;
    if(model)
    {
        transactionProxyModel = new TransactionFilterProxy(this);
        transactionProxyModel->setSourceModel(model->getTransactionTableModel());
        transactionProxyModel->setDynamicSortFilter(true);
        transactionProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        transactionProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        
        transactionProxyModel->setSortRole(Qt::EditRole);
        transactionView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        transactionView->setModel(transactionProxyModel);
        transactionView->setAlternatingRowColors(true);
        transactionView->setSelectionBehavior(QAbstractItemView::SelectRows);
        transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        transactionView->setSortingEnabled(true);
        transactionView->sortByColumn(TransactionTableModel::Date, Qt::DescendingOrder);
        transactionView->verticalHeader()->hide();
        transactionView->horizontalHeader()->setVisible(true);
        
        transactionView->setColumnWidth(TransactionTableModel::Status, STATUS_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Watchonly, WATCHONLY_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Date, DATE_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Type, TYPE_COLUMN_WIDTH);
        transactionView->setColumnWidth(TransactionTableModel::Amount, AMOUNT_MINIMUM_COLUMN_WIDTH);
        
        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(transactionView, AMOUNT_MINIMUM_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH, this);
        
        if (model->getOptionsModel())
        {
            // Add third party transaction URLs to context menu
            QStringList listUrls = model->getOptionsModel()->getThirdPartyTxUrls().split("|", QString::SkipEmptyParts);
            for (int i = 0; i < listUrls.size(); ++i)
            {
                QString host = QUrl(listUrls[i].trimmed(), QUrl::StrictMode).host();
                if (!host.isEmpty())
                {
                    QAction *thirdPartyTxUrlAction = new QAction(host, this); // use host as menu item label
                    if (i == 0)
                        contextMenu->addSeparator();
                    contextMenu->addAction(thirdPartyTxUrlAction);
                    connect(thirdPartyTxUrlAction, SIGNAL(triggered()), mapperThirdPartyTxUrls, SLOT(map()));
                    mapperThirdPartyTxUrls->setMapping(thirdPartyTxUrlAction, listUrls[i].trimmed());
                }
            }
        }
        
        // show/hide column Watch-only
        //updateWatchOnlyColumn(model->haveWatchOnly());
        
        // Watch-only signal
        //connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyColumn(bool)));
        QGraphicsDropShadowEffect* effect0 = new QGraphicsDropShadowEffect();
        effect0->setOffset(0);
        effect0->setBlurRadius(10.0);
        QGraphicsDropShadowEffect* effect1 = new QGraphicsDropShadowEffect();
        effect1->setOffset(0);
        effect1->setBlurRadius(10.0);
        QGraphicsDropShadowEffect* effect2 = new QGraphicsDropShadowEffect();
        effect2->setOffset(0);
        effect2->setBlurRadius(10.0);
        QGraphicsDropShadowEffect* effect3 = new QGraphicsDropShadowEffect();
        effect3->setOffset(0);
        effect3->setBlurRadius(10.0);
        QGraphicsDropShadowEffect* effect4 = new QGraphicsDropShadowEffect();
        effect4->setOffset(0);
        effect4->setBlurRadius(20.0);

        ui->frame_4->setGraphicsEffect(effect4);
        ui->frame_2->setGraphicsEffect(effect1);
        ui->frame_3->setGraphicsEffect(effect2);
        ui->frame_5->setGraphicsEffect(effect3);
        ui->frame_6->setGraphicsEffect(effect0);

        //transactionView->horizontalHeader()->setStyleSheet("QHeaderView::section {border: none; background-color: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #2b001e, stop: 1 #480027) ; color: white; font-size: 12pt;} QHeaderView::section:last {border: none; background-color: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #480027, stop: 1 #480027);  color: white; font-size: 12pt;} ");
        transactionView->horizontalHeader()->setStyleSheet("QHeaderView::section {height:50; border: none; background-color: #480027; color: white; font-size: 12pt \"NoirRegular\"; padding-left: 10; padding-right:10; } QHeaderView::down-arrow { image: url(:/icons/txdown); width: 8px; height: 8px; } QHeaderView::up-arrow { image: url(:/icons/txup); width: 8px; height: 8px; } ");


    }
}

void TransactionView::chooseDate(int idx)
{
    if(!transactionProxyModel)
        return;
    QDate current = QDate::currentDate();
    //dateRangeWidget->setVisible(false);
    switch(ui->dateWidget->itemData(idx).toInt())
    {
        case All:
            transactionProxyModel->setDateRange(
                                                TransactionFilterProxy::MIN_DATE,
                                                TransactionFilterProxy::MAX_DATE);
            break;
        case Today:
            transactionProxyModel->setDateRange(
                                                QDateTime(current),
                                                TransactionFilterProxy::MAX_DATE);
            break;
        case ThisWeek: {
            // Find last Monday
            QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
            transactionProxyModel->setDateRange(
                                                QDateTime(startOfWeek),
                                                TransactionFilterProxy::MAX_DATE);
            
        } break;
        case ThisMonth:
            transactionProxyModel->setDateRange(
                                                QDateTime(QDate(current.year(), current.month(), 1)),
                                                TransactionFilterProxy::MAX_DATE);
            break;
        case LastMonth:
            transactionProxyModel->setDateRange(
                                                QDateTime(QDate(current.year(), current.month(), 1).addMonths(-1)),
                                                QDateTime(QDate(current.year(), current.month(), 1)));
            break;
        case ThisYear:
            transactionProxyModel->setDateRange(
                                                QDateTime(QDate(current.year(), 1, 1)),
                                                TransactionFilterProxy::MAX_DATE);
            break;
        case Range:
            //dateRangeWidget->setVisible(true);
            dateRangeChanged();
            break;
    }

}

void TransactionView::chooseType(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setTypeFilter(
                                         ui->typeWidget->itemData(idx).toInt());
}

void TransactionView::chooseWatchonly(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setWatchOnlyFilter(
                                              (TransactionFilterProxy::WatchOnlyFilter)watchOnlyWidget->itemData(idx).toInt());
}

void TransactionView::changedPrefix(const QString &prefix)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setAddressPrefix(prefix);
}

void TransactionView::changedAmount(const QString &amount)
{
    if(!transactionProxyModel)
        return;
    CAmount amount_parsed = 0;
    if(BitcoinUnits::parse(model->getOptionsModel()->getDisplayUnit(), amount, &amount_parsed))
    {
        transactionProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        transactionProxyModel->setMinAmount(0);
    }
}

void TransactionView::exportClicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
                                                tr("Export Transaction History"), QString(),
                                                tr("Comma separated file (*.csv)"), NULL);
    
    if (filename.isNull())
        return;
    
    CSVModelWriter writer(filename);
    
    // name, column, role
    writer.setModel(transactionProxyModel);
    writer.addColumn(tr("Confirmed"), 0, TransactionTableModel::ConfirmedRole);
    if (model && model->haveWatchOnly())
        writer.addColumn(tr("Watch-only"), TransactionTableModel::Watchonly);
    writer.addColumn(tr("Date"), 0, TransactionTableModel::DateRole);
    writer.addColumn(tr("Type"), TransactionTableModel::Type, Qt::EditRole);
    writer.addColumn(tr("Label"), 0, TransactionTableModel::LabelRole);
    writer.addColumn(tr("Address"), 0, TransactionTableModel::AddressRole);
    writer.addColumn(BitcoinUnits::getAmountColumnTitle(model->getOptionsModel()->getDisplayUnit()), 0, TransactionTableModel::FormattedAmountRole);
    writer.addColumn(tr("ID"), 0, TransactionTableModel::TxIDRole);
    
    if(!writer.write()) {
        Q_EMIT message(tr("Exporting Failed"), tr("There was an error trying to save the transaction history to %1.").arg(filename),
                       CClientUIInterface::MSG_ERROR);
    }
    else {
        Q_EMIT message(tr("Exporting Successful"), tr("The transaction history was successfully saved to %1.").arg(filename),
                       CClientUIInterface::MSG_INFORMATION);
    }
}

void TransactionView::contextualMenu(const QPoint &point)
{
    QModelIndex index = transactionView->indexAt(point);
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    if (selection.empty())
        return;
    
    // check if transaction can be abandoned, disable context menu action in case it doesn't
    uint256 hash;
    hash.SetHex(selection.at(0).data(TransactionTableModel::TxHashRole).toString().toStdString());
    abandonAction->setEnabled(model->transactionCanBeAbandoned(hash));
    resendAction->setEnabled(model->transactionCanBeRebroadcast(hash));
    
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void TransactionView::abandonTx()
{
    if(!transactionView || !transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    
    // get the hash from the TxHashRole (QVariant / QString)
    uint256 hash;
    QString hashQStr = selection.at(0).data(TransactionTableModel::TxHashRole).toString();
    hash.SetHex(hashQStr.toStdString());
    
    // Abandon the wallet transaction over the walletModel
    model->abandonTransaction(hash);
    
    // Update the table
    model->getTransactionTableModel()->updateTransaction(hashQStr, CT_UPDATED, false);
}

void TransactionView::rebroadcastTx()
{
    if(!transactionView || !transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    
    // get the hash from the TxHashRole (QVariant / QString)
    uint256 hash;
    QString hashQStr = selection.at(0).data(TransactionTableModel::TxHashRole).toString();
    hash.SetHex(hashQStr.toStdString());
    
    if (model->rebroadcastTransaction(hash))
        Q_EMIT message(tr("Re-broadcast"), tr("Broadcast succeeded"), CClientUIInterface::MSG_INFORMATION);
    else
        Q_EMIT message(tr("Re-broadcast"), tr("There was an error trying to broadcast the message"),
                       CClientUIInterface::MSG_ERROR);
    
    // Update the table
    model->getTransactionTableModel()->updateTransaction(hashQStr, CT_UPDATED, true);
}

void TransactionView::copyAddress()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::AddressRole);
}

void TransactionView::copyLabel()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::LabelRole);
}

void TransactionView::copyAmount()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::FormattedAmountRole);
}

void TransactionView::copyTxID()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxIDRole);
}

void TransactionView::copyTxHex()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxHexRole);
}

void TransactionView::copyTxPlainText()
{
    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxPlainTextRole);
}

void TransactionView::editLabel()
{
    if(!transactionView->selectionModel() ||!model)
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        AddressTableModel *addressBook = model->getAddressTableModel();
        if(!addressBook)
            return;
        QString address = selection.at(0).data(TransactionTableModel::AddressRole).toString();
        if(address.isEmpty())
        {
            // If this transaction has no associated address, exit
            return;
        }
        // Is address in address book? Address book can miss address when a transaction is
        // sent from outside the UI.
        int idx = addressBook->lookupAddress(address);
        if(idx != -1)
        {
            // Edit sending / receiving address
            QModelIndex modelIdx = addressBook->index(idx, 0, QModelIndex());
            // Determine type of address, launch appropriate editor dialog type
            QString type = modelIdx.data(AddressTableModel::TypeRole).toString();
            
            EditAddressDialog dlg(
                                  type == AddressTableModel::Receive
                                  ? EditAddressDialog::EditReceivingAddress
                                  : EditAddressDialog::EditSendingAddress, this);
            dlg.setModel(addressBook);
            dlg.loadRow(idx);
            dlg.exec();
        }
        else
        {
            // Add sending address
            EditAddressDialog dlg(EditAddressDialog::NewSendingAddress,
                                  this);
            dlg.setModel(addressBook);
            dlg.setAddress(address);
            dlg.exec();
        }
    }
}

void TransactionView::showDetails()
{
    if(!transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog *dlg = new TransactionDescDialog(selection.at(0));
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
}

void TransactionView::openThirdPartyTxUrl(QString url)
{
    if(!transactionView || !transactionView->selectionModel())
        return;
    QModelIndexList selection = transactionView->selectionModel()->selectedRows(0);
    if(!selection.isEmpty())
        QDesktopServices::openUrl(QUrl::fromUserInput(url.replace("%s", selection.at(0).data(TransactionTableModel::TxHashRole).toString())));
}

QWidget *TransactionView::createDateRangeWidget()
{
    dateRangeWidget = new QFrame();
    dateRangeWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
    dateRangeWidget->setContentsMargins(1,1,1,1);
    QHBoxLayout *layout = new QHBoxLayout(dateRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(23);
    layout->addWidget(new QLabel(tr("Range:")));
    
    dateFrom = new QDateTimeEdit(this);
    dateFrom->setDisplayFormat("dd/MM/yy");
    dateFrom->setCalendarPopup(true);
    dateFrom->setMinimumWidth(100);
    dateFrom->setDate(QDate::currentDate().addDays(-7));
    layout->addWidget(dateFrom);
    layout->addWidget(new QLabel(tr("to")));
    
    dateTo = new QDateTimeEdit(this);
    dateTo->setDisplayFormat("dd/MM/yy");
    dateTo->setCalendarPopup(true);
    dateTo->setMinimumWidth(100);
    dateTo->setDate(QDate::currentDate());
    layout->addWidget(dateTo);
    layout->addStretch();
    
    // Hide by default
    dateRangeWidget->setVisible(false);
    
    // Notify on change
    connect(dateFrom, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));
    connect(dateTo, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));
    
    return dateRangeWidget;
}

void TransactionView::dateRangeChanged()
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setDateRange(
                                        QDateTime(dateFrom->date()),
                                        QDateTime(dateTo->date()).addDays(1));
}

void TransactionView::focusTransaction(const QModelIndex &idx)
{
    if(!transactionProxyModel)
        return;
    QModelIndex targetIdx = transactionProxyModel->mapFromSource(idx);
    transactionView->scrollTo(targetIdx);
    transactionView->setCurrentIndex(targetIdx);
    transactionView->setFocus();
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void TransactionView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(TransactionTableModel::ToAddress);
}

// Need to override default Ctrl+C action for amount as default behaviour is just to copy DisplayRole text
bool TransactionView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_C && ke->modifiers().testFlag(Qt::ControlModifier))
        {
            GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::TxPlainTextRole);
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// show/hide column Watch-only
void TransactionView::updateWatchOnlyColumn(bool fHaveWatchOnly)
{
    watchOnlyWidget->setVisible(fHaveWatchOnly);
    transactionView->setColumnHidden(TransactionTableModel::Watchonly, !fHaveWatchOnly);
}
