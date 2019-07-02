// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef NOIRNODE_SYNC_H
#define NOIRNODE_SYNC_H

#include "chain.h"
#include "net.h"
#include  "utiltime.h"
#include <univalue.h>

class CNoirnodeSync;

static const int NOIRNODE_SYNC_FAILED          = -1;
static const int NOIRNODE_SYNC_INITIAL         = 0;
static const int NOIRNODE_SYNC_SPORKS          = 1;
static const int NOIRNODE_SYNC_LIST            = 2;
static const int NOIRNODE_SYNC_MNW             = 3;
static const int NOIRNODE_SYNC_FINISHED        = 999;

static const int NOIRNODE_SYNC_TICK_SECONDS    = 6;
static const int NOIRNODE_SYNC_TIMEOUT_SECONDS = 30; // our blocks are 2.5 minutes so 30 seconds should be fine

static const int NOIRNODE_SYNC_ENOUGH_PEERS    = 6;  //Mainnet PARAMS
static const int NOIRNODE_SYNC_ENOUGH_PEERS_TESTNET    = 1;  //Testnet PARAMS

extern CNoirnodeSync noirnodeSync;

//
// CNoirnodeSync : Sync noirnode assets in stages
//

class CNoirnodeSync
{
private:
    // Keep track of current asset
    int nRequestedNoirnodeAssets;
    // Count peers we've requested the asset from
    int nRequestedNoirnodeAttempt;

    // Time when current noirnode asset sync started
    int64_t nTimeAssetSyncStarted;

    // Last time when we received some noirnode asset ...
    int64_t nTimeLastNoirnodeList;
    int64_t nTimeLastPaymentVote;
    int64_t nTimeLastGovernanceItem;
    // ... or failed
    int64_t nTimeLastFailure;

    // How many times we failed
    int nCountFailures;

    // Keep track of current block index
    const CBlockIndex *pCurrentBlockIndex;

    bool CheckNodeHeight(CNode* pnode, bool fDisconnectStuckNodes = false);
    void Fail();
    void ClearFulfilledRequests();

public:
    CNoirnodeSync() { Reset(); }

    void AddedNoirnodeList() { nTimeLastNoirnodeList = GetTime(); }
    void AddedPaymentVote() { nTimeLastPaymentVote = GetTime(); }
    void AddedGovernanceItem() { nTimeLastGovernanceItem = GetTime(); }

    void SendGovernanceSyncRequest(CNode* pnode);

    bool IsFailed() { return nRequestedNoirnodeAssets == NOIRNODE_SYNC_FAILED; }
    bool IsBlockchainSynced(bool fBlockAccepted = false);
    bool IsNoirnodeListSynced() { return nRequestedNoirnodeAssets > NOIRNODE_SYNC_LIST; }
    bool IsWinnersListSynced() { return nRequestedNoirnodeAssets > NOIRNODE_SYNC_MNW; }
    bool IsSynced() { return nRequestedNoirnodeAssets == NOIRNODE_SYNC_FINISHED; }

    int GetAssetID() { return nRequestedNoirnodeAssets; }
    int GetAttempt() { return nRequestedNoirnodeAttempt; }
    std::string GetAssetName();
    std::string GetSyncStatus();

    void Reset();
    void SwitchToNextAsset();

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void ProcessTick();

    void UpdatedBlockTip(const CBlockIndex *pindex);
};

#endif
