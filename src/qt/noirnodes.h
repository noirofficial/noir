#ifndef NOIRNODES_H
#define NOIRNODES_H

#include "primitives/transaction.h"
#include "platformstyle.h"
#include "sync.h"
#include "util.h"

#include <QMenu>
#include <QTimer>
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define MY_MASTERNODELIST_UPDATE_SECONDS                 60
#define MASTERNODELIST_UPDATE_SECONDS                    15
#define MASTERNODELIST_FILTER_COOLDOWN_SECONDS            3

namespace Ui {
    class Noirnodes;
}

class ClientModel;
class WalletModel;

class Noirnodes : public QWidget
{
    Q_OBJECT

public:
    explicit Noirnodes(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~Noirnodes();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void StartAlias(std::string strAlias);
    void StartAll(std::string strCommand = "start-all");
    QHBoxLayout *statusBar;
    QVBoxLayout *statusText;
    QLabel *priceBTC;
    QLabel *priceUSD;

private:
    QMenu *contextMenu;
    int64_t nTimeFilterUpdated;
    bool fFilterUpdated;

public Q_SLOTS:
    void updateMyNoirnodeInfo(QString strAlias, QString strAddr, const COutPoint& outpoint);
    void updateMyNodeList(bool fForce = false);
    void updateNodeList();



Q_SIGNALS:

private:
    QTimer *timer;
    Ui::Noirnodes *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;

    // Protects tableWidgetNoirnodes
    CCriticalSection cs_mnlist;

    // Protects tableWidgetMyNoirnodes
    CCriticalSection cs_mymnlist;

    QString strCurrentFilter;

private Q_SLOTS:
    void showContextMenu(const QPoint &);
    void on_filterLineEdit_textChanged(const QString &strFilterIn);
    void on_startButton_clicked();
    void on_startAllButton_clicked();
    void on_startMissingButton_clicked();
    void on_tableWidgetMyNoirnodes_itemSelectionChanged();
    void on_UpdateButton_clicked();
    void on_swapButton_clicked();

};

#endif // NOIRNODES_H
