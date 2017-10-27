// Copyright (c) 2017 The Zoin Developers
// Created by Matthew Tawil
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMUNITYPAGE_H
#define COMMUNITYPAGE_H

#include <QWidget>

namespace Ui {
    class CommunityPage;
}

class CommunityPage : public QWidget
{
    Q_OBJECT


public:
    explicit CommunityPage(QWidget *parent = nullptr);
    ~CommunityPage();


private:
    Ui::CommunityPage *ui;

private Q_SLOTS:
    void OpenWebsite();
    void OpenReddit();
    void OpenBTCTalk();
    void OpenTwitter();
    void OpenSlack();
    void OpenFacebook();
    void OpenSlackInv();
    void OpenGit();


signals:

public slots:


};


#endif



