// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "ui_receivecoinspage.h"

#include "addresstablemodel.h"
#include "optionsmodel.h"
#include "bitcoingui.h"
#include "editaddressdialog.h"
#include "csvmodelwriter.h"
#include "guiutil.h"
#include "receivecoinspage.h"
#include <QGraphicsDropShadowEffect>
#include <QDebug>

#ifdef USE_QRCODE
#include "qrcodedialog.h"
#endif

#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include<QDialog>

ReceiveCoinsPage::ReceiveCoinsPage(QWidget *parent) : QWidget(parent),
    ui(new Ui::ReceiveCoinsPage),
    model(0),
    optionsModel(0)
{
    ui->setupUi(this);
    statusBar = ui->statusBar;
    statusText = ui->statusText;
    priceBTC = ui->priceBTC;
    priceUSD = ui->priceUSD;

    ui->tableView->horizontalHeader()->hide();
    //ui->tableView->horizontalHeader()->setStyleSheet("QHeaderView::section {border: none; background-color: #121548; color: white; font-size: 15pt;}");
    ui->tableView->verticalHeader()->hide();
    ui->tableView->setShowGrid(false);

    //ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    //ui->tableView->horizontalHeader()->setFixedHeight(55);


    // Connect signals for context menu actions
    connect(ui->copyAddress, SIGNAL(pressed()), this, SLOT(on_copyAddress_clicked()));

    //connect(editAction, SIGNAL(triggered()), this, SLOT(onEditAction()));

    //connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(onSendCoinsAction()));

    //connect(ui->deleteAddress, SIGNAL(triggered()), this, SLOT(on_deleteAddress_clicked()));
    connect(ui->showQRCode, SIGNAL(pressed()), this, SLOT(on_showQRCode_clicked()));
    connect(ui->signMessage, SIGNAL(pressed()), this, SLOT(on_signMessage_clicked()));
    //connect(ui->verifyMessage, SIGNAL(triggered()), this, SLOT(on_verifyMessage_clicked()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    // Pass through accept action from button box
    //connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setOffset(0);
    effect->setBlurRadius(20.0);
    //effect->setColor(QColor(247, 247, 247, 25));
    ui->frame_4->setGraphicsEffect(effect);
}

ReceiveCoinsPage::~ReceiveCoinsPage()
{
    delete ui;
}

void ReceiveCoinsPage::setModel(AddressTableModel *model)
{
    this->model = model;
    if(!model)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        // Receive filter
    proxyModel->setFilterRole(AddressTableModel::TypeRole);
    proxyModel->setFilterFixedString(AddressTableModel::Receive);


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

    selectionChanged();
}

void ReceiveCoinsPage::setOptionsModel(OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
}

void ReceiveCoinsPage::on_copyAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Address);
}

void ReceiveCoinsPage::onCopyLabelAction()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Label);
}

void ReceiveCoinsPage::onEditAction()
{
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;

    EditAddressDialog dlg(EditAddressDialog::EditReceivingAddress);
    dlg.setModel(model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

/*
void ReceiveCoinsPage::on_zerocoinMintButton_clicked()
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

void ReceiveCoinsPage::on_zerocoinSpendButton_clicked(){

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
*/

void ReceiveCoinsPage::on_signMessage_clicked()
{
    QTableView *table = ui->tableView;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QString address = index.data().toString();
        emit signMessage(address);
    }
}

/*
void ReceiveCoinsPage::on_verifyMessage_clicked()
{
    QTableView *table = ui->tableView;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QString address = index.data().toString();
        emit verifyMessage(address);
    }
}
*/

void ReceiveCoinsPage::onSendCoinsAction()
{
    QTableView *table = ui->tableView;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QString address = index.data().toString();
        emit sendCoins(address);
    }
}

void ReceiveCoinsPage::on_newAddress_clicked()
{
    if(!model)
        return;

    EditAddressDialog dlg(EditAddressDialog::NewReceivingAddress, this);
    dlg.setModel(model);
    if(dlg.exec())
    {
        newAddressToSelect = dlg.getAddress();
    }
}

/*
void ReceiveCoinsPage::on_deleteAddress_clicked()
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
*/

void ReceiveCoinsPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;


    if(!table->selectionModel())
        return;
    if(table->selectionModel()->currentIndex().row() >= 0)
        table->selectionModel()->selectedRows(table->selectionModel()->currentIndex().row());
    else
        table->selectionModel()->clearCurrentIndex();

    if(table->selectionModel()->isSelected(table->currentIndex())){
        ui->copyAddress->setStyleSheet("background-color: #121349;color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->signMessage->setStyleSheet("background-color: #121349;color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->showQRCode->setStyleSheet("background-color: #121349;color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
    }
    else{
        ui->copyAddress->setStyleSheet("background-color: rgb(216, 216, 219);color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->signMessage->setStyleSheet("background-color: rgb(216, 216, 219);color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->showQRCode->setStyleSheet("background-color: rgb(216, 216, 219);color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
    }

        // Deleting receiving addresses, however, is not allowed
        //ui->deleteAddress->setEnabled(false);
        //ui->deleteAddress->setVisible(false);
        //deleteAction->setEnabled(false);
}

void ReceiveCoinsPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;
    // When this is a tab/widget and not a model dialog, ignore "done"
    if(mode == ForEditing)
        return;

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
        retval = QDialog::Rejected;
    }

    //QDialog::done(retval);
}

void ReceiveCoinsPage::on_exportButton_clicked()
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

void ReceiveCoinsPage::on_showQRCode_clicked()
{
#ifdef USE_QRCODE
    QTableView *table = ui->tableView;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    foreach (QModelIndex index, indexes)
    {
        QString address = index.data().toString();
        QString label = index.sibling(index.row(), 0).data(Qt::EditRole).toString();

        QRCodeDialog *dialog = new QRCodeDialog(address, label, tab == ReceivingTab, this);
        dialog->setModel(optionsModel);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    }
#endif
}

void ReceiveCoinsPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void ReceiveCoinsPage::selectNewAddress(const QModelIndex &parent, int begin, int /*end*/)
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
