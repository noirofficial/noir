// Copyright (c) 2018 The Noir Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "votingpage.h"
#include "random.h"
#include "main.h" // For strMessageMagic
#include "walletmodel.h"
#include "ui_votingpage.h"
#include "wallet/rpcwallet.cpp"

VotingPage::VotingPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VotingPage)
{
    ui->setupUi(this);
    //Setup sync bar and price tickers
    statusBar = ui->statusBar;
    statusText = ui->statusText;
    priceUSD = ui->priceUSD;
    priceBTC = ui->priceBTC;

    connect(ui->voteButton, SIGNAL(clicked()), this, SLOT(on_VoteButton_Clicked()));

    //ui->addressList
    //ui->votingChoice
    //set up network manager to POST
    ui->voteEnabled->setStyleSheet("color: green;");
    namGet = new QNetworkAccessManager(this);
    connect(namGet, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinishedGet(QNetworkReply*)));

    namPost = new QNetworkAccessManager(this);
    connect(namPost, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinishedPost(QNetworkReply*)));

    refresh_Groupings();
    ui->votingChoice->addItem("Yes");
    ui->votingChoice->addItem("Of course!");

    /* Create timer to fetch vote poll every 5 minutes */
    timerId = startTimer(300000);

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setOffset(0);
    effect->setBlurRadius(20.0);
    ui->voteFrame->setGraphicsEffect(effect);
}

VotingPage::~VotingPage()
{
    delete ui;
}

void VotingPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void VotingPage::timerEvent(QTimerEvent *event)
{
    check_Voting();
}
void VotingPage::refresh_Groupings(){

    /*try{
        std::string strPrint;
        UniValue *temp;
        UniValue jsonGroupings(UniValue::VARR);
        jsonGroupings = listaddressgroupings(temp, false);
        QString groupings;
        strPrint = jsonGroupings.write(2);
        int first = strPrint.find("\"Z");
        int last;
        while(first != -1){
            strPrint.erase(0,first+1);
            last = strPrint.find("\",");
            std::string address = strPrint.substr (0,last);
            strPrint.erase(0,last+2);
            last = (strPrint.find("]") < strPrint.find(",")) ? strPrint.find("]"):strPrint.find(",");
            std::string amount = strPrint.substr (0,last);
            first = strPrint.find("\"Z");

            if(((int)QString::fromStdString(amount).toDouble()) > 0){
                groupings = QString::fromStdString(address) + ": " + QString::number((int)QString::fromStdString(amount).toDouble());
                ui->addressList->addItem(groupings);
            }
        }
    }
    catch(...){
        LogPrintf("VotingPage::refresh_Groupings(): ERROR in parsing addresses and values \n");
    }*/
}

void VotingPage::on_VoteButton_Clicked(){

    std::string text = ui->addressList->currentText().toStdString();
    int index = text.find(": ");
    std::string add = text.substr(0,index);
    std::string amount = text.substr(index + 2,text.length());


    std::string message = std::to_string(GetRandInt(INT_MAX));

    CBitcoinAddress addr(add);
    if (!addr.IsValid())
    {
        //ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        //ui->statusLabel_SM->setText(tr("The entered address is invalid.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }
    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {
        //ui->addressIn_SM->setValid(false);
        //ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        //ui->statusLabel_SM->setText(tr("The entered address does not refer to a key.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
    {
        //ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        //ui->statusLabel_SM->setText(tr("Wallet unlock was cancelled."));
        return;
    }

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
    {
        //ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        //ui->statusLabel_SM->setText(tr("Private key for the entered address is not available."));
        return;
    }

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    //make user sign their message
    ss << message;

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
    {
        //ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        //ui->statusLabel_SM->setText(QString("<nobr>") + tr("Message signing failed.") + QString("</nobr>"));
        return;
    }

    //ui->statusLabel_SM->setStyleSheet("QLabel { color: green; }");
    //ui->statusLabel_SM->setText(QString("<nobr>") + tr("Message signed.") + QString("</nobr>"));


    QString sig = QString::fromStdString(EncodeBase64(&vchSig[0], vchSig.size()));



    QUrlQuery postData;
    postData.addQueryItem("poll_id", "0");
    postData.addQueryItem("poll_option_id", "2");
    postData.addQueryItem("address", QString::fromStdString(add));
    postData.addQueryItem("message", QString::fromStdString(message));
    postData.addQueryItem("sig", sig);
    postData.addQueryItem("votes", QString::fromStdString(amount));
    LogPrintf("DATA %s \n", postData.toString().toStdString());
    QNetworkRequest request;
    request.setUrl(QUrl("http://voting.dev.netabuse.net/vote?"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
        "application/x-www-form-urlencoded");
    namPost->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

}

void VotingPage::check_Voting()
{

    QNetworkRequest request;
    QNetworkReply *reply = NULL;

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2);
    request.setSslConfiguration(config);
    request.setUrl(QUrl("https://api.coinmarketcap.com/v1/ticker/noir/?convert=USD"));
    request.setHeader(QNetworkRequest::ServerHeader, "application/json");
    namGet->get(request);
}


void VotingPage::replyFinishedGet(QNetworkReply *reply)
{
    if(1){
        ui->voteEnabled->setStyleSheet("color: green;");
        ui->voteEnabled->setText("Yes");
    }
    else{
        ui->voteEnabled->setStyleSheet("color: red;");
        ui->voteEnabled->setText("No");
    }
}

void VotingPage::replyFinishedPost(QNetworkReply *reply)
{
    try{
        QByteArray bytes = reply->readAll();
        QString str = QString::fromUtf8(bytes.data(), bytes.size());
        LogPrintf("LOGGED %s", str.toStdString());
    }
    catch(...){
        return;
    }
    if(1){
        ui->voteEnabled->setStyleSheet("color: green;");
        ui->voteEnabled->setText("Yes");
    }
    else{
        ui->voteEnabled->setStyleSheet("color: red;");
        ui->voteEnabled->setText("No");
    }
}
