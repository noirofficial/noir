// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionview.h"

#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "transactiontablemodel.h"
#include "bitcoinunits.h"
#include "csvmodelwriter.h"
#include "transactiondescdialog.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "guiutil.h"

#include <QScrollBar>
#include <QComboBox>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QTableView>
#include <QHeaderView>
#include <QMessageBox>
#include <QPoint>
#include <QMenu>
#include <QLabel>
#include <QDateTimeEdit>
#include <QGraphicsDropShadowEffect>

TransactionView::TransactionView(QWidget *parent) :
    QWidget(parent), model(0), transactionProxyModel(0),
    transactionView(0)
{
    parent->setContentsMargins(0,0,0,0);

    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_3;
    QFrame *frame_3;
    QHBoxLayout *verticalLayout_5;
    QLabel *label_2;
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
    verticalLayout_2 = new QVBoxLayout();
    verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
    horizontalLayout_3 = new QHBoxLayout();
    horizontalLayout_3->setSpacing(0);
    horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
    horizontalLayout_3->setContentsMargins(-1, 0, -1, 0);
    frame_3 = new QFrame(this);
    frame_3->setObjectName(QStringLiteral("frame_3"));
    sizePolicy.setHeightForWidth(frame_3->sizePolicy().hasHeightForWidth());
    frame_3->setSizePolicy(sizePolicy);
    frame_3->setMinimumSize(QSize(500, 100));
    frame_3->setFixedHeight(100);
    frame_3->setLayoutDirection(Qt::LeftToRight);
    frame_3->setAutoFillBackground(false);
    frame_3->setStyleSheet(QLatin1String("background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #121548, stop: 1 #4a0e95);\n"
"border:0;\n"
""));
    frame_3->setFrameShape(QFrame::StyledPanel);
    frame_3->setFrameShadow(QFrame::Raised);

    verticalLayout_5 = new QHBoxLayout(frame_3);
    verticalLayout_5->setSpacing(0);
    verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
    verticalLayout_5->setContentsMargins(60,0,60,0);
    label_2 = new QLabel(frame_3);
    label_2->setObjectName(QStringLiteral("label_2"));
    sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
    label_2->setSizePolicy(sizePolicy);
    label_2->setText("Transactions");
    QFont font1;
    font1.setFamily(QStringLiteral("Yu Gothic UI Light"));
    font1.setPointSize(40);
    font1.setBold(false);
    //font1.setWeight(50);
    label_2->setFont(font1);
    label_2->setAutoFillBackground(false);
    label_2->setStyleSheet(QLatin1String("color:white;\n"
"background-color: rgba(255, 255, 255,0);"));


    verticalLayout_5->addWidget(label_2);



    QVBoxLayout *verticalLayout_8 = new QVBoxLayout();
    verticalLayout_8->setObjectName(QStringLiteral("verticalLayout_5"));
    priceUSD = new QLabel(frame_3);
    priceUSD->setObjectName(QStringLiteral("priceUSD"));
    QFont font2;
    font2.setFamily(QStringLiteral("Yu Gothic UI"));
    font2.setPointSize(14);
    font2.setBold(true);
    font2.setWeight(75);
    priceUSD->setFont(font2);
    priceUSD->setStyleSheet(QLatin1String("background-color: rgb(0,0,0,0);\n"
"color: white;"));
    priceUSD->setAlignment(Qt::AlignBottom|Qt::AlignRight|Qt::AlignTrailing);

    verticalLayout_8->addWidget(priceUSD);

    priceBTC = new QLabel(frame_3);
    priceBTC->setObjectName(QStringLiteral("priceBTC"));
    QFont font3;
    font3.setFamily(QStringLiteral("Yu Gothic UI Light"));
    font3.setPointSize(14);
    priceBTC->setFont(font3);
    priceBTC->setStyleSheet(QLatin1String("background-color: rgb(0,0,0,0);\n"
"color: white;"));
    priceBTC->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

    verticalLayout_8->addWidget(priceBTC);


    verticalLayout_5->addLayout(verticalLayout_8);

    QLabel *label_5 = new QLabel(frame_3);
    label_5->setObjectName(QStringLiteral("label_5"));
    QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(label_5->sizePolicy().hasHeightForWidth());
    label_5->setSizePolicy(sizePolicy1);
    label_5->setStyleSheet(QStringLiteral("background: rgb(0,0,0,0);"));
    label_5->setPixmap(QPixmap(QString::fromUtf8(":/icons/Exports_45")));

    verticalLayout_5->addWidget(label_5);







    //priceLayout->addWidget(priceUSD);

    //tickerLayout->addWidget(priceUSD);

    horizontalLayout_3->addWidget(frame_3);


    verticalLayout_2->addLayout(horizontalLayout_3);

    horizontalLayout->addLayout(verticalLayout_2);

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setOffset(0);
    effect->setBlurRadius(20.0);
    //effect->setColor(QColor(247, 247, 247, 25));
    setGraphicsEffect(effect);

    setStyleSheet("border:0;");
    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(60,40,43,40); // THISSSSSSSSS

    hlayout->setSpacing(10);
    hlayout->addSpacing(0);


    dateWidget = new QComboBox(this);
    dateWidget->setFixedHeight(40);

    dateWidget->setFixedWidth(141);

    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range..."), Range);
    dateWidget->setStyleSheet("font-size: 14px;border: 1px solid #D3D3D3;border-radius: 2px;padding: 8px;color: #333;");
    hlayout->addWidget(dateWidget);

    typeWidget = new QComboBox(this);
    typeWidget->setFixedHeight(40);
#ifdef Q_OS_MAC
    typeWidget->setFixedWidth(141);
#else
    typeWidget->setFixedWidth(140);
#endif

    typeWidget->addItem(tr("All"), TransactionFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Received with"), TransactionFilterProxy::TYPE(TransactionRecord::RecvWithAddress) |
                                        TransactionFilterProxy::TYPE(TransactionRecord::RecvFromOther));
    typeWidget->addItem(tr("Sent to"), TransactionFilterProxy::TYPE(TransactionRecord::SendToAddress) |
                                  TransactionFilterProxy::TYPE(TransactionRecord::SendToOther));
    typeWidget->addItem(tr("To yourself"), TransactionFilterProxy::TYPE(TransactionRecord::SendToSelf));
    typeWidget->addItem(tr("Mined"), TransactionFilterProxy::TYPE(TransactionRecord::Generated));
    typeWidget->addItem(tr("Other"), TransactionFilterProxy::TYPE(TransactionRecord::Other));
    typeWidget->setStyleSheet("font-size: 14px;border: 1px solid #D3D3D3;border-radius: 2px;padding: 8px;color: #333;");
    hlayout->addWidget(typeWidget);

    addressWidget = new QLineEdit(this);
    addressWidget->setFixedHeight(40);
    addressWidget->setStyleSheet("font-size: 14px;");
#if QT_VERSION >= 0x040700
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    addressWidget->setPlaceholderText(tr("Enter address or label to search"));
#endif
    hlayout->addWidget(addressWidget);

    amountWidget = new QLineEdit(this);
    amountWidget->setFixedHeight(40);
#if QT_VERSION >= 0x040700
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    amountWidget->setPlaceholderText(tr("Min amount"));
#endif
#ifdef Q_OS_MAC
    amountWidget->setFixedWidth(127);
#else
    amountWidget->setFixedWidth(130);
#endif
    amountWidget->setValidator(new QDoubleValidator(0, 1e20, 8, this));
    amountWidget->setStyleSheet("font-size: 14px;");
    hlayout->addWidget(amountWidget);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout22 = new QHBoxLayout(this);
    hlayout22->setContentsMargins(60,0,60,0);    // THISSSSSSSS
    //vlayout->setMargin(0);
    //vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setShowGrid(false);
    view->setStyleSheet(QLatin1String("QTableView::item{font-size:16px;height:30px;}"
                                      "QHeaderView::section {font-size:16px;color:white;height:40px; background-color:rgb(255,255,255,0); border: 0;}\n"
                                      "QHeaderView {background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #121548, stop: 1 #4a0e95);}\n"
                                      ));

    view->setTabKeyNavigation(false);
    view->setAlternatingRowColors(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSortingEnabled(true);
    view->verticalHeader()->setVisible(false);
    view->horizontalHeader()->setFixedHeight(40);


    vlayout->addLayout(horizontalLayout);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(createDateRangeWidget());
    hlayout22->addWidget(view);
    vlayout->addLayout(hlayout22);
    vlayout->setSpacing(0);
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
#ifdef Q_OS_MAC
    hlayout->addSpacing(width+2);
#else
    hlayout->addSpacing(width);
#endif
    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    transactionView = view;

    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *editLabelAction = new QAction(tr("Edit label"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);

    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(editLabelAction);
    contextMenu->addAction(showDetailsAction);

    // Connect actions
    connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    connect(typeWidget, SIGNAL(activated(int)), this, SLOT(chooseType(int)));
    connect(addressWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPrefix(QString)));
    connect(amountWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAmount(QString)));

    connect(view, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)));
    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(editLabelAction, SIGNAL(triggered()), this, SLOT(editLabel()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
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

        transactionView->setModel(transactionProxyModel);
        transactionView->setAlternatingRowColors(true);
        transactionView->setSelectionBehavior(QAbstractItemView::SelectRows);
        transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        transactionView->setSortingEnabled(true);
        transactionView->sortByColumn(TransactionTableModel::Status, Qt::DescendingOrder);
        transactionView->verticalHeader()->hide();

        transactionView->horizontalHeader()->resizeSection(TransactionTableModel::Status, 23);
        transactionView->horizontalHeader()->resizeSection(TransactionTableModel::Date, 120);
        transactionView->horizontalHeader()->resizeSection(TransactionTableModel::Type, 120);
#if QT_VERSION < 0x050000
        transactionView->horizontalHeader()->setResizeMode(TransactionTableModel::ToAddress, QHeaderView::Stretch);
#else
        transactionView->horizontalHeader()->setSectionResizeMode(TransactionTableModel::ToAddress, QHeaderView::Stretch);
#endif
        transactionView->horizontalHeader()->resizeSection(TransactionTableModel::Amount, 120);

        transactionView->horizontalHeader()->setFixedHeight(53);
    }
}

void TransactionView::chooseDate(int idx)
{
    if(!transactionProxyModel)
        return;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
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
                QDateTime(QDate(current.year(), current.month()-1, 1)),
                QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), 1, 1)),
                TransactionFilterProxy::MAX_DATE);
        break;
    case Range:
        dateRangeWidget->setVisible(true);
        dateRangeChanged();
        break;
    }
}

void TransactionView::chooseType(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setTypeFilter(
        typeWidget->itemData(idx).toInt());
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
    qint64 amount_parsed = 0;
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
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Transaction Data"), QString(),
            tr("Comma separated file (*.csv)"));

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(transactionProxyModel);
    writer.addColumn(tr("Confirmed"), 0, TransactionTableModel::ConfirmedRole);
    writer.addColumn(tr("Date"), 0, TransactionTableModel::DateRole);
    writer.addColumn(tr("Type"), TransactionTableModel::Type, Qt::EditRole);
    writer.addColumn(tr("Label"), 0, TransactionTableModel::LabelRole);
    writer.addColumn(tr("Address"), 0, TransactionTableModel::AddressRole);
    writer.addColumn(tr("Amount"), 0, TransactionTableModel::FormattedAmountRole);
    writer.addColumn(tr("ID"), 0, TransactionTableModel::TxIDRole);

    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}

void TransactionView::contextualMenu(const QPoint &point)
{
    QModelIndex index = transactionView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
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

            EditAddressDialog dlg(type==AddressTableModel::Receive
                                         ? EditAddressDialog::EditReceivingAddress
                                         : EditAddressDialog::EditSendingAddress,
                                  this);
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
        TransactionDescDialog dlg(selection.at(0));
        dlg.exec();
    }
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
