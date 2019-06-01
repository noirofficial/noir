// Copyright (c) 2017 The Noir Core developers
// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "zerocoinpage.h"
#include "ui_zerocoinpage.h"

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
#include <QGraphicsDropShadowEffect>

ZerocoinPage::ZerocoinPage(const PlatformStyle *platformStyle, Mode mode, QWidget *parent) :
        QWidget(parent),
        ui(new Ui::ZerocoinPage),
        model(0),
        mode(mode){
    ui->setupUi(this);

    statusBar = ui->statusBar;
    statusText = ui->statusText;
    priceBTC = ui->priceBTC;
    priceUSD = ui->priceUSD;



    switch (mode) {
        case ForSelection:
            setWindowTitle(tr("Zerocoin"));
            connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(accept()));
            ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
            ui->tableView->setFocus();
            //ui->exportButton->hide();
            break;
        case ForEditing:
            setWindowTitle(tr("Zerocoin"));
    }
    ui->zerocoinAmount->setVisible(true);
    ui->zerocoinMintButton->setVisible(true);
    ui->zerocoinSpendButton->setVisible(true);
    ui->zerocoinAmount->addItem("1");
    ui->zerocoinAmount->addItem("10");
    ui->zerocoinAmount->addItem("25");
    ui->zerocoinAmount->addItem("50");
    ui->zerocoinAmount->addItem("100");

    // Build context menu
    contextMenu = new QMenu(this);

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
    ui->tableView->horizontalHeader()->setStyleSheet("QHeaderView::section:first {border: none; background-color: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #2b001e, stop: 1 #480027) ; color: white; font-size: 12pt;} QHeaderView::section:last {border: none; background-color: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #480027, stop: 1 #480027);  color: white; font-size: 12pt;} ");
    connect(ui->zerocoinSpendToMeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(zerocoinSpendToMeCheckBoxChecked(int)));

    ui->tableView->verticalHeader()->hide();
    ui->tableView->setShowGrid(false);

    ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    ui->tableView->horizontalHeader()->setFixedHeight(50);



    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setOffset(0);
    effect->setBlurRadius(20.0);
    ui->frame_4->setGraphicsEffect(effect);
}

ZerocoinPage::~ZerocoinPage() {
    delete ui;
}

void ZerocoinPage::setModel(AddressTableModel *model) {
    this->model = model;
    if (!model)
        return;

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterRole(AddressTableModel::TypeRole);
    proxyModel->setFilterFixedString(AddressTableModel::Zerocoin);

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

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
           this, SLOT(selectionChanged()));

    // Select row for newly created address
    connect(model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(selectNewAddress(QModelIndex, int, int)));

    selectionChanged();
}

void ZerocoinPage::on_zerocoinMintButton_clicked() {
    QString amount = ui->zerocoinAmount->currentText();
    std::string denomAmount = amount.toStdString();
    std::string stringError;
    if(!model->zerocoinMint(stringError, denomAmount)){
        QString t = tr(stringError.c_str());

        QMessageBox::critical(this, tr("Error"),
                              tr("You cannot mint zerocoin because %1").arg(t),
                              QMessageBox::Ok, QMessageBox::Ok);
    } else{
        QMessageBox::information(this, tr("Success"),
                                      tr("You have successfully minted some zerocoins."),
                                      QMessageBox::Ok, QMessageBox::Ok);
    }
}

void ZerocoinPage::on_zerocoinSpendButton_clicked() {
    QString amount = ui->zerocoinAmount->currentText();
    QString address = ui->spendToThirdPartyAddress->text();
    std::string denomAmount = amount.toStdString();
    std::string thirdPartyAddress = address.toStdString();
    std::string stringError;
    if(ui->zerocoinSpendToMeCheckBox->isChecked() == false && thirdPartyAddress == ""){
        QMessageBox::critical(this, tr("Error"),
                                      tr("Your \"Spend To\" field is empty, please check again"),
                                      QMessageBox::Ok, QMessageBox::Ok);
    }else{
        if(!model->zerocoinSpend(stringError, thirdPartyAddress, denomAmount)){
            QString t = tr(stringError.c_str());
            QMessageBox::critical(this, tr("Error"),
                                  tr("You cannot spend zerocoin because %1").arg(t),
                                  QMessageBox::Ok, QMessageBox::Ok);
        }else{
            QMessageBox::information(this, tr("Success"),
                                          tr("You have successfully spent some zerocoins."),
                                          QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}
 void ZerocoinPage::zerocoinSpendToMeCheckBoxChecked(int state) {
    if (state == Qt::Checked)
    {
        ui->spendToThirdPartyAddress->clear();
        ui->spendToThirdPartyAddress->setEnabled(false);
    }else{
        ui->spendToThirdPartyAddress->setEnabled(true);
    }
}

void ZerocoinPage::selectionChanged()
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
        ui->zerocoinSpendButton->setEnabled(true);
    }
    else{
        ui->zerocoinSpendButton->setEnabled(true);
    }
}



void ZerocoinPage::on_exportButton_clicked() {
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(this, tr("Export Address List"), QString(), tr("Comma separated file (*.csv)"), NULL);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Label", AddressTableModel::Label, Qt::EditRole);
    writer.addColumn("Address", AddressTableModel::Address, Qt::EditRole);

    if (!writer.write()) {
        QMessageBox::critical(this, tr("Exporting Failed"), tr("There was an error trying to save the address list to %1. Please try again.").arg(
                filename));
    }
}

void ZerocoinPage::contextualMenu(const QPoint &point) {
    QModelIndex index = ui->tableView->indexAt(point);
    if (index.isValid()) {
        contextMenu->exec(QCursor::pos());
    }
}

void ZerocoinPage::selectNewAddress(const QModelIndex &parent, int begin, int /*end*/) {
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, AddressTableModel::Address, parent));
    if (idx.isValid() && (idx.data(Qt::EditRole).toString() == newAddressToSelect)) {
        // Select row of newly created address, once
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newAddressToSelect.clear();
    }
}
