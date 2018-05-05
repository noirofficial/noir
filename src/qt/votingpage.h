#ifndef VOTINGPAGE_H
#define VOTINGPAGE_H

#include "primitives/transaction.h"
#include "platformstyle.h"
#include "sync.h"
#include "util.h"
#include "signverifymessagedialog.h"
#include "base58.h"

#include <QMenu>
#include <QTimer>
#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QByteArray>
#include <QNetworkReply>
#include <QGraphicsDropShadowEffect>



class ClientModel;
class WalletModel;


namespace Ui {
class VotingPage;
}

class VotingPage : public QWidget
{
    Q_OBJECT

public:
    explicit VotingPage(QWidget *parent = 0);
    ~VotingPage();
    QHBoxLayout *statusBar;
    QVBoxLayout *statusText;
    QLabel *priceBTC;
    QLabel *priceUSD;
    void setWalletModel(WalletModel *walletModel);


private:
    /*
    struct voteShare{
        std::string *address;
        std::string *amount;
    };

    std::vector<voteShare> *userAccounts;
    */
    void timerEvent(QTimerEvent *event);
    int timerId;
    Ui::VotingPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    QNetworkAccessManager* namGet;
    QNetworkAccessManager* namPost;





private Q_SLOTS:

    void refresh_Groupings();
    void on_VoteButton_Clicked();
    void check_Voting();
    void replyFinishedGet(QNetworkReply *reply);
    void replyFinishedPost(QNetworkReply *reply);


};

#endif // VOTINGPAGE_H
