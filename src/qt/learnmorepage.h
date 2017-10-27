// Copyright (c) 2017 The Zoin Developers
// Created by Matthew Tawil
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LEARNMOREPAGE_H
#define LEARNMOREPAGE_H

#include <QWidget>


namespace Ui {
    class LearnMorePage;
}

class LearnMorePage : public QWidget
{
    Q_OBJECT


public:
    explicit LearnMorePage(QWidget *parent = nullptr);
    ~LearnMorePage();


private:
    Ui::LearnMorePage *ui;


signals:

public slots:


};


#endif
