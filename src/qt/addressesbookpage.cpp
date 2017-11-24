// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressesbookpage.h"
#include "ui_addressesbookpage.h"

#include "addresstablemodel.h"
#include "optionsmodel.h"
#include "bitcoingui.h"
#include "editaddressdialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include <QGraphicsDropShadowEffect>
#include <QDebug>

#ifdef USE_QRCODE
#include "qrcodedialog.h"
#endif

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>

AddressesBookPage::AddressesBookPage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddressesBookPage),
    model(0),
    optionsModel(0)
{
    ui->setupUi(this);
    table = ui->tableView;
    statusBar = ui->statusBar;
    statusText = ui->statusText;
    priceBTC = ui->priceBTC;
    priceUSD = ui->priceUSD;

    //ui->tableView->horizontalHeader()->hide();
    //ui->tableView->horizontalHeader()->setStyleSheet("QHeaderView::section {border: none; background-color: #121548; color: white; font-size: 15pt;}");
    ui->tableView->verticalHeader()->hide();


    ui->tableView->setShowGrid(false);
    ui->tableView->horizontalHeader()->setStyleSheet("background-color: red;");
    /*
    ui->tableView->setStyleSheet("background-color: rgb(255, 255, 255); border:0;"
                                 "QTableView::item{font-size:16px;height:30px;}  "
                                 "QHeaderView::section {font-size:16px;color:white;height:40px; background-color:rgb(255,255,255,100); border: 0;}\n"
                                 "QHeaderView {background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #121548, stop: 1 #4a0e95);}\n"
                                  );
*/
    //view->setSelectionMode(QAbstractItemView::SingleSelection);
    //view->setSelectionBehavior(QAbstractItemView::SelectRows);
    //view->setSortingEnabled(true);
    //ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->horizontalHeader()->setFixedHeight(40);



    //ui->tableView->setStyleSheet("QHeaderView::section {font-size:24px;color:white;height:30px;background-color:QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #121548, stop: 1 #4a0e95)}");

/*
    switch(mode)
    {
    case ForSending:
        connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableView->setFocus();
        ui->exportButton->hide();
        break;
    case ForEditing:
        /ui->buttonBox->setVisible(false);

        break;
    }
*/
    /*
    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);

    if(tab != ZerocoinTab){
        contextMenu->addAction(editAction);
    }

    if(tab == SendingTab)
        contextMenu->addAction(deleteAction);
    contextMenu->addSeparator();
    if(tab == SendingTab)
        contextMenu->addAction(sendCoinsAction);
#ifdef USE_QRCODE
    contextMenu->addAction(showQRCodeAction);
#endif
    if(tab == ReceivingTab)
        contextMenu->addAction(signMessageAction);
    else if(tab == SendingTab)
        contextMenu->addAction(verifyMessageAction);
*/
    // Connect signals for context menu actions

    connect(ui->newAddress, SIGNAL(pressed()), this, SLOT(on_newAddress_clicked()));
    connect(ui->copyAddress, SIGNAL(pressed()), this, SLOT(on_copyAddress_clicked()));
    connect(ui->deleteAddress, SIGNAL(pressed()), this, SLOT(on_deleteAddress_clicked()));
    connect(ui->sendAddress, SIGNAL(pressed()), this, SLOT(onSendCoinsAction()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setOffset(0);
    effect->setBlurRadius(20.0);
    //effect->setColor(QColor(247, 247, 247, 25));
    ui->frame_4->setGraphicsEffect(effect);

}

AddressesBookPage::~AddressesBookPage()
{
    delete ui;
}

void AddressesBookPage::setModel(AddressTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterRole(AddressTableModel::TypeRole);
    proxyModel->setFilterFixedString(AddressTableModel::Send);


    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Set column widths
#if QT_VERSION < 0x050000
    ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setResizeMode(AddressTableModel::Address, QHeaderView::ResizeToContents);
#else
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Address, QHeaderView::ResizeToContents);
#endif

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    // Select row for newly created address
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(selectNewAddress(QModelIndex,int,int)));
    //selectionChanged();
}

void AddressesBookPage::setOptionsModel(OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
}

void AddressesBookPage::on_copyAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Address);
}

void AddressesBookPage::onCopyLabelAction()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Label);
}

void AddressesBookPage::onEditAction()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditAddressDialog dlg( EditAddressDialog::EditSendingAddress );
    dlg.setModel(model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void AddressesBookPage::on_zerocoinMintButton_clicked()
{

    std::string stringError;
    if(!model->zerocoinMint(this, stringError))
    {
        if (stringError != "")
        {
            QString t = tr(stringError.c_str());

            QMessageBox::critical(this, tr("Error"),
                tr("You cannot mint zerocoin because %1").arg(t),
                QMessageBox::Ok, QMessageBox::Ok);
        }
    }

}

void AddressesBookPage::on_zerocoinSpendButton_clicked(){

    std::string stringError;
    if(!model->zerocoinSpend(this, stringError))
    {
        if (stringError != "")
        {
            QString t = tr(stringError.c_str());

            QMessageBox::critical(this, tr("Error"),
                tr("You cannot spend zerocoin because %1").arg(t),
                QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}


void AddressesBookPage::on_signMessage_clicked()
{
    QTableView *table = ui->tableView;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QString address = index.data().toString();
        emit signMessage(address);
    }
}

void AddressesBookPage::on_verifyMessage_clicked()
{
    QTableView *table = ui->tableView;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QString address = index.data().toString();
        emit verifyMessage(address);
    }
}

void AddressesBookPage::onSendCoinsAction()
{
    QTableView *table = ui->tableView;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);
    QModelIndexList names = table->selectionModel()->selectedRows(AddressTableModel::Label);

    foreach (QModelIndex index, indexes)
    {
        QString address = index.data().toString();
        QModelIndex name = names.at(0);
        QString n = name.data().toString();
        emit sendCoins(address, n);
    }
}

void AddressesBookPage::on_newAddress_clicked()
{
    if(!model)
        return;

    EditAddressDialog dlg(EditAddressDialog::NewSendingAddress, this);
    dlg.setModel(model);
    if(dlg.exec())
    {
        newAddressToSelect = dlg.getAddress();
    }
}

void AddressesBookPage::on_deleteAddress_clicked()
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        table->model()->removeRow(indexes.at(0).row());
    }
}


void AddressesBookPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;

    if(table->selectionModel()->isSelected(table->currentIndex())){
        ui->sendAddress->setStyleSheet("background-color: #121349;color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->copyAddress->setStyleSheet("background-color: #121349;color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->deleteAddress->setStyleSheet("background-color: #121349;color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
    }
    else{
        ui->sendAddress->setStyleSheet("background-color: rgb(216, 216, 219);color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->copyAddress->setStyleSheet("background-color: rgb(216, 216, 219);color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->deleteAddress->setStyleSheet("background-color: rgb(216, 216, 219);color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
    }

}

void AddressesBookPage::done(int retval)
{
    return;
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;
    // When this is a tab/widget and not a model dialog, ignore "done"


    // Figure out which address was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QVariant address = table->model()->data(index);
        returnValue = address.toString();
    }

    if(returnValue.isEmpty())
    {
        // If no address entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void AddressesBookPage::on_exportButton_clicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Address Book Data"), QString(),
            tr("Comma separated file (*.csv)"));

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Label", AddressTableModel::Label, Qt::EditRole);
    writer.addColumn("Address", AddressTableModel::Address, Qt::EditRole);

    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}

void AddressesBookPage::on_showQRCode_clicked()
{
#ifdef USE_QRCODE
    QTableView *table = ui->tableView;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QString address = index.data().toString();
        QString label = index.sibling(index.row(), 0).data(Qt::EditRole).toString();

        QRCodeDialog *dialog = new QRCodeDialog(address, label, 1, this);
        dialog->setModel(optionsModel);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    }
#endif
}

void AddressesBookPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void AddressesBookPage::selectNewAddress(const QModelIndex &parent, int begin, int /*end*/)
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, AddressTableModel::Address, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newAddressToSelect))
    {
        // Select row of newly created address, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newAddressToSelect.clear();
    }
}
