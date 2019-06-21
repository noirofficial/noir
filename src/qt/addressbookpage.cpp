// Copyright (c) 2017 The Noir Core developers
// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "addressbookpage.h"
#include "ui_addressbookpage.h"

#include "addresstablemodel.h"
#include "bitcoingui.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "platformstyle.h"

#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>

AddressBookPage::AddressBookPage(const PlatformStyle *platformStyle, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::AddressBookPage),
        model(0) {

    ui->setupUi(this);
    table = ui->tableView;
    statusBar = ui->statusBar;
    statusText = ui->statusText;
    priceBTC = ui->priceBTC;
    priceUSD = ui->priceUSD;

    ui->tableView->horizontalHeader()->setStyleSheet("QHeaderView::section:first {border: none; background-color: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #2b001e, stop: 1 #480027) ; color: white; font-size: 12pt;} QHeaderView::section:last {border: none; background-color: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #480027, stop: 1 #480027);  color: white; font-size: 12pt;} ");

    ui->tableView->verticalHeader()->hide();
    ui->tableView->setShowGrid(false);

    ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    ui->tableView->horizontalHeader()->setFixedHeight(50);

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

AddressBookPage::~AddressBookPage() {
    delete ui;
}

void AddressBookPage::setModel(AddressTableModel *model) {

    this->model = model;
    if (!model)
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
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Address, QHeaderView::Stretch);
#endif


    ui->tableView->horizontalHeader()->setVisible(true);

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(selectionChanged()));

    // Select row for newly created address
    connect(model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(selectNewAddress(QModelIndex, int, int)));

    selectionChanged();
}

void AddressBookPage::on_copyAddress_clicked() {
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Address);
}

void AddressBookPage::onCopyLabelAction() {
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Label);
}

void AddressBookPage::onSendCoinsAction()
{
    QTableView *table = ui->tableView;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);
    QModelIndexList names = table->selectionModel()->selectedRows(AddressTableModel::Label);

    Q_FOREACH (QModelIndex index, indexes)
    {
        QString address = index.data().toString();
        QModelIndex name = names.at(0);
        QString n = name.data().toString();
        Q_EMIT sendCoins(address, n);
    }
}


void AddressBookPage::onEditAction() {
    if (!model)
        return;

    if (!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if (indexes.isEmpty())
        return;

    EditAddressDialog dlg(EditAddressDialog::EditSendingAddress , this);
    dlg.setModel(model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg.loadRow(origIndex.row());
    dlg.exec();
}

void AddressBookPage::on_newAddress_clicked() {
    if (!model)
        return;

    EditAddressDialog dlg(EditAddressDialog::NewSendingAddress, this);
    dlg.setModel(model);
    if (dlg.exec()) {
        newAddressToSelect = dlg.getAddress();
    }
}

void AddressBookPage::on_deleteAddress_clicked() {
    QTableView *table = ui->tableView;
    if (!table->selectionModel())
        return;

    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if (!indexes.isEmpty()) {
        table->model()->removeRow(indexes.at(0).row());
    }
}

void AddressBookPage::selectionChanged() {
    // Set button states based on selected tab and selection
/*
      QTableView *table = ui->tableView;
    if (!table->selectionModel())
        return;

    if (table->selectionModel()->hasSelection()) {

        // In sending tab, allow deletion of selection
        ui->deleteAddress->setEnabled(true);
        ui->sendAddress->setEnabled(true);
        ui->copyAddress->setEnabled(true);
    } else {
        ui->sendAddress->setEnabled(false);
        ui->deleteAddress->setEnabled(false);
        ui->copyAddress->setEnabled(false);
    }

    QTableView *table = ui->tableView;
*/
    if(!table->selectionModel())
        return;
    if(table->selectionModel()->currentIndex().row() >= 0)
        table->selectionModel()->selectedRows(table->selectionModel()->currentIndex().row());
    else
        table->selectionModel()->clearCurrentIndex();



    if(table->selectionModel()->isSelected(table->currentIndex())){
        ui->deleteAddress->setEnabled(true);
        ui->sendAddress->setEnabled(true);
        ui->copyAddress->setEnabled(true);
        ui->deleteAddress->setStyleSheet("background-color: #2b001e;color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->copyAddress->setStyleSheet("background-color: #2b001e;color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->sendAddress->setStyleSheet("background-color: #2b001e;color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
    }
    else{
        ui->sendAddress->setEnabled(false);
        ui->deleteAddress->setEnabled(false);
        ui->copyAddress->setEnabled(false);
        ui->deleteAddress->setStyleSheet("background-color: rgb(216, 216, 219);color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->copyAddress->setStyleSheet("background-color: rgb(216, 216, 219);color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
        ui->sendAddress->setStyleSheet("background-color: rgb(216, 216, 219);color: white;border-radius:15px;height:35px;width:120px;border-color:gray;border-width:0px;border-style:solid;");
    }
}

void AddressBookPage::setOptionsModel(OptionsModel *optionsModel)
{
    //this->optionsModel = optionsModel;
}


void AddressBookPage::done(int retval) {
    QTableView *table = ui->tableView;
    if (!table->selectionModel() || !table->model())
        return;

    // Figure out which address was selected, and return it
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);

    Q_FOREACH(
    const QModelIndex &index, indexes) {
        QVariant address = table->model()->data(index);
        returnValue = address.toString();
    }

    if (returnValue.isEmpty()) {
        // If no address entry selected, return rejected
        retval = Rejected;
    }

    QDialog::done(retval);
}

void AddressBookPage::on_exportButton_clicked() {
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this,
                                                tr("Export Address List"), QString(),
                                                tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Label", AddressTableModel::Label, Qt::EditRole);
    writer.addColumn("Address", AddressTableModel::Address, Qt::EditRole);

    if (!writer.write()) {
        QMessageBox::critical(this, tr("Exporting Failed"),
                              tr("There was an error trying to save the address list to %1. Please try again.").arg(
                                      filename));
    }
}

void AddressBookPage::contextualMenu(const QPoint &point) {
    QModelIndex index = ui->tableView->indexAt(point);
    if (index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void AddressBookPage::selectNewAddress(const QModelIndex &parent, int begin, int /*end*/) {
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, AddressTableModel::Address, parent));
    if (idx.isValid() && (idx.data(Qt::EditRole).toString() == newAddressToSelect)) {
        // Select row of newly created address, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newAddressToSelect.clear();
    }
}
