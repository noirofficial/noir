// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activenoirnode.h"
#include "addrman.h"
#include "darksend.h"
//#include "governance.h"
#include "noirnode-payments.h"
#include "noirnode-sync.h"
#include "noirnodeman.h"
#include "netfulfilledman.h"
#include "util.h"
//#include "random.h"

/** Noirnode manager */
CNoirnodeMan mnodeman;

const std::string CNoirnodeMan::SERIALIZATION_VERSION_STRING = "CNoirnodeMan-Version-4";

struct CompareLastPaidBlock
{
    bool operator()(const std::pair<int, CNoirnode*>& t1,
                    const std::pair<int, CNoirnode*>& t2) const
    {
        return (t1.first != t2.first) ? (t1.first < t2.first) : (t1.second->vin < t2.second->vin);
    }
};

struct CompareScoreMN
{
    bool operator()(const std::pair<int64_t, CNoirnode*>& t1,
                    const std::pair<int64_t, CNoirnode*>& t2) const
    {
        return (t1.first != t2.first) ? (t1.first < t2.first) : (t1.second->vin < t2.second->vin);
    }
};

CNoirnodeIndex::CNoirnodeIndex()
    : nSize(0),
      mapIndex(),
      mapReverseIndex()
{}

bool CNoirnodeIndex::Get(int nIndex, CTxIn& vinNoirnode) const
{
    rindex_m_cit it = mapReverseIndex.find(nIndex);
    if(it == mapReverseIndex.end()) {
        return false;
    }
    vinNoirnode = it->second;
    return true;
}

int CNoirnodeIndex::GetNoirnodeIndex(const CTxIn& vinNoirnode) const
{
    index_m_cit it = mapIndex.find(vinNoirnode);
    if(it == mapIndex.end()) {
        return -1;
    }
    return it->second;
}

void CNoirnodeIndex::AddNoirnodeVIN(const CTxIn& vinNoirnode)
{
    index_m_it it = mapIndex.find(vinNoirnode);
    if(it != mapIndex.end()) {
        return;
    }
    int nNextIndex = nSize;
    mapIndex[vinNoirnode] = nNextIndex;
    mapReverseIndex[nNextIndex] = vinNoirnode;
    ++nSize;
}

void CNoirnodeIndex::Clear()
{
    mapIndex.clear();
    mapReverseIndex.clear();
    nSize = 0;
}
struct CompareByAddr

{
    bool operator()(const CNoirnode* t1,
                    const CNoirnode* t2) const
    {
        return t1->addr < t2->addr;
    }
};

void CNoirnodeIndex::RebuildIndex()
{
    nSize = mapIndex.size();
    for(index_m_it it = mapIndex.begin(); it != mapIndex.end(); ++it) {
        mapReverseIndex[it->second] = it->first;
    }
}

CNoirnodeMan::CNoirnodeMan() : cs(),
  vNoirnodes(),
  mAskedUsForNoirnodeList(),
  mWeAskedForNoirnodeList(),
  mWeAskedForNoirnodeListEntry(),
  mWeAskedForVerification(),
  mMnbRecoveryRequests(),
  mMnbRecoveryGoodReplies(),
  listScheduledMnbRequestConnections(),
  nLastIndexRebuildTime(0),
  indexNoirnodes(),
  indexNoirnodesOld(),
  fIndexRebuilt(false),
  fNoirnodesAdded(false),
  fNoirnodesRemoved(false),
//  vecDirtyGovernanceObjectHashes(),
  nLastWatchdogVoteTime(0),
  mapSeenNoirnodeBroadcast(),
  mapSeenNoirnodePing(),
  nDsqCount(0)
{}

bool CNoirnodeMan::Add(CNoirnode &mn)
{
    LOCK(cs);

    CNoirnode *pmn = Find(mn.vin);
    if (pmn == NULL) {
        LogPrint("noirnode", "CNoirnodeMan::Add -- Adding new Noirnode: addr=%s, %i now\n", mn.addr.ToString(), size() + 1);
        vNoirnodes.push_back(mn);
        indexNoirnodes.AddNoirnodeVIN(mn.vin);
        fNoirnodesAdded = true;
        return true;
    }

    return false;
}

void CNoirnodeMan::AskForMN(CNode* pnode, const CTxIn &vin)
{
    if(!pnode) return;

    LOCK(cs);

    std::map<COutPoint, std::map<CNetAddr, int64_t> >::iterator it1 = mWeAskedForNoirnodeListEntry.find(vin.prevout);
    if (it1 != mWeAskedForNoirnodeListEntry.end()) {
        std::map<CNetAddr, int64_t>::iterator it2 = it1->second.find(pnode->addr);
        if (it2 != it1->second.end()) {
            if (GetTime() < it2->second) {
                // we've asked recently, should not repeat too often or we could get banned
                return;
            }
            // we asked this node for this outpoint but it's ok to ask again already
            LogPrintf("CNoirnodeMan::AskForMN -- Asking same peer %s for missing noirnode entry again: %s\n", pnode->addr.ToString(), vin.prevout.ToStringShort());
        } else {
            // we already asked for this outpoint but not this node
            LogPrintf("CNoirnodeMan::AskForMN -- Asking new peer %s for missing noirnode entry: %s\n", pnode->addr.ToString(), vin.prevout.ToStringShort());
        }
    } else {
        // we never asked any node for this outpoint
        LogPrintf("CNoirnodeMan::AskForMN -- Asking peer %s for missing noirnode entry for the first time: %s\n", pnode->addr.ToString(), vin.prevout.ToStringShort());
    }
    mWeAskedForNoirnodeListEntry[vin.prevout][pnode->addr] = GetTime() + DSEG_UPDATE_SECONDS;

    pnode->PushMessage(NetMsgType::DSEG, vin);
}

void CNoirnodeMan::Check()
{
    LOCK(cs);

//    LogPrint("noirnode", "CNoirnodeMan::Check -- nLastWatchdogVoteTime=%d, IsWatchdogActive()=%d\n", nLastWatchdogVoteTime, IsWatchdogActive());

    BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {
        mn.Check();
    }
}

void CNoirnodeMan::CheckAndRemove()
{
    if(!noirnodeSync.IsNoirnodeListSynced()) return;

    LogPrintf("CNoirnodeMan::CheckAndRemove\n");

    {
        // Need LOCK2 here to ensure consistent locking order because code below locks cs_main
        // in CheckMnbAndUpdateNoirnodeList()
        LOCK2(cs_main, cs);

        Check();

        // Remove spent noirnodes, prepare structures and make requests to reasure the state of inactive ones
        std::vector<CNoirnode>::iterator it = vNoirnodes.begin();
        std::vector<std::pair<int, CNoirnode> > vecNoirnodeRanks;
        // ask for up to MNB_RECOVERY_MAX_ASK_ENTRIES noirnode entries at a time
        int nAskForMnbRecovery = MNB_RECOVERY_MAX_ASK_ENTRIES;
        while(it != vNoirnodes.end()) {
            CNoirnodeBroadcast mnb = CNoirnodeBroadcast(*it);
            uint256 hash = mnb.GetHash();
            // If collateral was spent ...
            if ((*it).IsOutpointSpent()) {
                LogPrint("noirnode", "CNoirnodeMan::CheckAndRemove -- Removing Noirnode: %s  addr=%s  %i now\n", (*it).GetStateString(), (*it).addr.ToString(), size() - 1);

                // erase all of the broadcasts we've seen from this txin, ...
                mapSeenNoirnodeBroadcast.erase(hash);
                mWeAskedForNoirnodeListEntry.erase((*it).vin.prevout);

                // and finally remove it from the list
//                it->FlagGovernanceItemsAsDirty();
                it = vNoirnodes.erase(it);
                fNoirnodesRemoved = true;
            } else {
                bool fAsk = pCurrentBlockIndex &&
                            (nAskForMnbRecovery > 0) &&
                            noirnodeSync.IsSynced() &&
                            it->IsNewStartRequired() &&
                            !IsMnbRecoveryRequested(hash);
                if(fAsk) {
                    // this mn is in a non-recoverable state and we haven't asked other nodes yet
                    std::set<CNetAddr> setRequested;
                    // calulate only once and only when it's needed
                    if(vecNoirnodeRanks.empty()) {
                        int nRandomBlockHeight = GetRandInt(pCurrentBlockIndex->nHeight);
                        vecNoirnodeRanks = GetNoirnodeRanks(nRandomBlockHeight);
                    }
                    bool fAskedForMnbRecovery = false;
                    // ask first MNB_RECOVERY_QUORUM_TOTAL noirnodes we can connect to and we haven't asked recently
                    for(int i = 0; setRequested.size() < MNB_RECOVERY_QUORUM_TOTAL && i < (int)vecNoirnodeRanks.size(); i++) {
                        // avoid banning
                        if(mWeAskedForNoirnodeListEntry.count(it->vin.prevout) && mWeAskedForNoirnodeListEntry[it->vin.prevout].count(vecNoirnodeRanks[i].second.addr)) continue;
                        // didn't ask recently, ok to ask now
                        CService addr = vecNoirnodeRanks[i].second.addr;
                        setRequested.insert(addr);
                        listScheduledMnbRequestConnections.push_back(std::make_pair(addr, hash));
                        fAskedForMnbRecovery = true;
                    }
                    if(fAskedForMnbRecovery) {
                        LogPrint("noirnode", "CNoirnodeMan::CheckAndRemove -- Recovery initiated, noirnode=%s\n", it->vin.prevout.ToStringShort());
                        nAskForMnbRecovery--;
                    }
                    // wait for mnb recovery replies for MNB_RECOVERY_WAIT_SECONDS seconds
                    mMnbRecoveryRequests[hash] = std::make_pair(GetTime() + MNB_RECOVERY_WAIT_SECONDS, setRequested);
                }
                ++it;
            }
        }

        // proces replies for NOIRNODE_NEW_START_REQUIRED noirnodes
        LogPrint("noirnode", "CNoirnodeMan::CheckAndRemove -- mMnbRecoveryGoodReplies size=%d\n", (int)mMnbRecoveryGoodReplies.size());
        std::map<uint256, std::vector<CNoirnodeBroadcast> >::iterator itMnbReplies = mMnbRecoveryGoodReplies.begin();
        while(itMnbReplies != mMnbRecoveryGoodReplies.end()){
            if(mMnbRecoveryRequests[itMnbReplies->first].first < GetTime()) {
                // all nodes we asked should have replied now
                if(itMnbReplies->second.size() >= MNB_RECOVERY_QUORUM_REQUIRED) {
                    // majority of nodes we asked agrees that this mn doesn't require new mnb, reprocess one of new mnbs
                    LogPrint("noirnode", "CNoirnodeMan::CheckAndRemove -- reprocessing mnb, noirnode=%s\n", itMnbReplies->second[0].vin.prevout.ToStringShort());
                    // mapSeenNoirnodeBroadcast.erase(itMnbReplies->first);
                    int nDos;
                    itMnbReplies->second[0].fRecovery = true;
                    CheckMnbAndUpdateNoirnodeList(NULL, itMnbReplies->second[0], nDos);
                }
                LogPrint("noirnode", "CNoirnodeMan::CheckAndRemove -- removing mnb recovery reply, noirnode=%s, size=%d\n", itMnbReplies->second[0].vin.prevout.ToStringShort(), (int)itMnbReplies->second.size());
                mMnbRecoveryGoodReplies.erase(itMnbReplies++);
            } else {
                ++itMnbReplies;
            }
        }
    }
    {
        // no need for cm_main below
        LOCK(cs);

        std::map<uint256, std::pair< int64_t, std::set<CNetAddr> > >::iterator itMnbRequest = mMnbRecoveryRequests.begin();
        while(itMnbRequest != mMnbRecoveryRequests.end()){
            // Allow this mnb to be re-verified again after MNB_RECOVERY_RETRY_SECONDS seconds
            // if mn is still in NOIRNODE_NEW_START_REQUIRED state.
            if(GetTime() - itMnbRequest->second.first > MNB_RECOVERY_RETRY_SECONDS) {
                mMnbRecoveryRequests.erase(itMnbRequest++);
            } else {
                ++itMnbRequest;
            }
        }

        // check who's asked for the Noirnode list
        std::map<CNetAddr, int64_t>::iterator it1 = mAskedUsForNoirnodeList.begin();
        while(it1 != mAskedUsForNoirnodeList.end()){
            if((*it1).second < GetTime()) {
                mAskedUsForNoirnodeList.erase(it1++);
            } else {
                ++it1;
            }
        }

        // check who we asked for the Noirnode list
        it1 = mWeAskedForNoirnodeList.begin();
        while(it1 != mWeAskedForNoirnodeList.end()){
            if((*it1).second < GetTime()){
                mWeAskedForNoirnodeList.erase(it1++);
            } else {
                ++it1;
            }
        }

        // check which Noirnodes we've asked for
        std::map<COutPoint, std::map<CNetAddr, int64_t> >::iterator it2 = mWeAskedForNoirnodeListEntry.begin();
        while(it2 != mWeAskedForNoirnodeListEntry.end()){
            std::map<CNetAddr, int64_t>::iterator it3 = it2->second.begin();
            while(it3 != it2->second.end()){
                if(it3->second < GetTime()){
                    it2->second.erase(it3++);
                } else {
                    ++it3;
                }
            }
            if(it2->second.empty()) {
                mWeAskedForNoirnodeListEntry.erase(it2++);
            } else {
                ++it2;
            }
        }

        std::map<CNetAddr, CNoirnodeVerification>::iterator it3 = mWeAskedForVerification.begin();
        while(it3 != mWeAskedForVerification.end()){
            if(it3->second.nBlockHeight < pCurrentBlockIndex->nHeight - MAX_POSE_BLOCKS) {
                mWeAskedForVerification.erase(it3++);
            } else {
                ++it3;
            }
        }

        // NOTE: do not expire mapSeenNoirnodeBroadcast entries here, clean them on mnb updates!

        // remove expired mapSeenNoirnodePing
        std::map<uint256, CNoirnodePing>::iterator it4 = mapSeenNoirnodePing.begin();
        while(it4 != mapSeenNoirnodePing.end()){
            if((*it4).second.IsExpired()) {
                LogPrint("noirnode", "CNoirnodeMan::CheckAndRemove -- Removing expired Noirnode ping: hash=%s\n", (*it4).second.GetHash().ToString());
                mapSeenNoirnodePing.erase(it4++);
            } else {
                ++it4;
            }
        }

        // remove expired mapSeenNoirnodeVerification
        std::map<uint256, CNoirnodeVerification>::iterator itv2 = mapSeenNoirnodeVerification.begin();
        while(itv2 != mapSeenNoirnodeVerification.end()){
            if((*itv2).second.nBlockHeight < pCurrentBlockIndex->nHeight - MAX_POSE_BLOCKS){
                LogPrint("noirnode", "CNoirnodeMan::CheckAndRemove -- Removing expired Noirnode verification: hash=%s\n", (*itv2).first.ToString());
                mapSeenNoirnodeVerification.erase(itv2++);
            } else {
                ++itv2;
            }
        }

        LogPrintf("CNoirnodeMan::CheckAndRemove -- %s\n", ToString());

        if(fNoirnodesRemoved) {
            CheckAndRebuildNoirnodeIndex();
        }
    }

    if(fNoirnodesRemoved) {
        NotifyNoirnodeUpdates();
    }
}

void CNoirnodeMan::Clear()
{
    LOCK(cs);
    vNoirnodes.clear();
    mAskedUsForNoirnodeList.clear();
    mWeAskedForNoirnodeList.clear();
    mWeAskedForNoirnodeListEntry.clear();
    mapSeenNoirnodeBroadcast.clear();
    mapSeenNoirnodePing.clear();
    nDsqCount = 0;
    nLastWatchdogVoteTime = 0;
    indexNoirnodes.Clear();
    indexNoirnodesOld.Clear();
}

int CNoirnodeMan::CountNoirnodes(int nProtocolVersion)
{
    LOCK(cs);
    int nCount = 0;
    nProtocolVersion = nProtocolVersion == -1 ? mnpayments.GetMinNoirnodePaymentsProto() : nProtocolVersion;

    BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {
        if(mn.nProtocolVersion < nProtocolVersion) continue;
        nCount++;
    }

    return nCount;
}

int CNoirnodeMan::CountEnabled(int nProtocolVersion)
{
    LOCK(cs);
    int nCount = 0;
    nProtocolVersion = nProtocolVersion == -1 ? mnpayments.GetMinNoirnodePaymentsProto() : nProtocolVersion;

    BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {
        if(mn.nProtocolVersion < nProtocolVersion || !mn.IsEnabled()) continue;
        nCount++;
    }

    return nCount;
}

/* Only IPv4 noirnodes are allowed in 12.1, saving this for later
int CNoirnodeMan::CountByIP(int nNetworkType)
{
    LOCK(cs);
    int nNodeCount = 0;

    BOOST_FOREACH(CNoirnode& mn, vNoirnodes)
        if ((nNetworkType == NET_IPV4 && mn.addr.IsIPv4()) ||
            (nNetworkType == NET_TOR  && mn.addr.IsTor())  ||
            (nNetworkType == NET_IPV6 && mn.addr.IsIPv6())) {
                nNodeCount++;
        }

    return nNodeCount;
}
*/

void CNoirnodeMan::DsegUpdate(CNode* pnode)
{
    LOCK(cs);

    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(!(pnode->addr.IsRFC1918() || pnode->addr.IsLocal())) {
            std::map<CNetAddr, int64_t>::iterator it = mWeAskedForNoirnodeList.find(pnode->addr);
            if(it != mWeAskedForNoirnodeList.end() && GetTime() < (*it).second) {
                LogPrintf("CNoirnodeMan::DsegUpdate -- we already asked %s for the list; skipping...\n", pnode->addr.ToString());
                return;
            }
        }
    }
    
    pnode->PushMessage(NetMsgType::DSEG, CTxIn());
    int64_t askAgain = GetTime() + DSEG_UPDATE_SECONDS;
    mWeAskedForNoirnodeList[pnode->addr] = askAgain;

    LogPrint("noirnode", "CNoirnodeMan::DsegUpdate -- asked %s for the list\n", pnode->addr.ToString());
}

CNoirnode* CNoirnodeMan::Find(const CScript &payee)
{
    LOCK(cs);

    BOOST_FOREACH(CNoirnode& mn, vNoirnodes)
    {
        if(GetScriptForDestination(mn.pubKeyCollateralAddress.GetID()) == payee)
            return &mn;
    }
    return NULL;
}

CNoirnode* CNoirnodeMan::Find(const CTxIn &vin)
{
    LOCK(cs);

    BOOST_FOREACH(CNoirnode& mn, vNoirnodes)
    {
        if(mn.vin.prevout == vin.prevout)
            return &mn;
    }
    return NULL;
}

CNoirnode* CNoirnodeMan::Find(const CPubKey &pubKeyNoirnode)
{
    LOCK(cs);

    BOOST_FOREACH(CNoirnode& mn, vNoirnodes)
    {
        if(mn.pubKeyNoirnode == pubKeyNoirnode)
            return &mn;
    }
    return NULL;
}

bool CNoirnodeMan::Get(const CPubKey& pubKeyNoirnode, CNoirnode& noirnode)
{
    // Theses mutexes are recursive so double locking by the same thread is safe.
    LOCK(cs);
    CNoirnode* pMN = Find(pubKeyNoirnode);
    if(!pMN)  {
        return false;
    }
    noirnode = *pMN;
    return true;
}

bool CNoirnodeMan::Get(const CTxIn& vin, CNoirnode& noirnode)
{
    // Theses mutexes are recursive so double locking by the same thread is safe.
    LOCK(cs);
    CNoirnode* pMN = Find(vin);
    if(!pMN)  {
        return false;
    }
    noirnode = *pMN;
    return true;
}

noirnode_info_t CNoirnodeMan::GetNoirnodeInfo(const CTxIn& vin)
{
    noirnode_info_t info;
    LOCK(cs);
    CNoirnode* pMN = Find(vin);
    if(!pMN)  {
        return info;
    }
    info = pMN->GetInfo();
    return info;
}

noirnode_info_t CNoirnodeMan::GetNoirnodeInfo(const CPubKey& pubKeyNoirnode)
{
    noirnode_info_t info;
    LOCK(cs);
    CNoirnode* pMN = Find(pubKeyNoirnode);
    if(!pMN)  {
        return info;
    }
    info = pMN->GetInfo();
    return info;
}

bool CNoirnodeMan::Has(const CTxIn& vin)
{
    LOCK(cs);
    CNoirnode* pMN = Find(vin);
    return (pMN != NULL);
}

char* CNoirnodeMan::GetNotQualifyReason(CNoirnode& mn, int nBlockHeight, bool fFilterSigTime, int nMnCount)
{
    if (!mn.IsValidForPayment()) {
        char* reasonStr = new char[256];
        sprintf(reasonStr, "false: 'not valid for payment'");
        return reasonStr;
    }
    // //check protocol version
    if (mn.nProtocolVersion < mnpayments.GetMinNoirnodePaymentsProto()) {
        // LogPrintf("Invalid nProtocolVersion!\n");
        // LogPrintf("mn.nProtocolVersion=%s!\n", mn.nProtocolVersion);
        // LogPrintf("mnpayments.GetMinNoirnodePaymentsProto=%s!\n", mnpayments.GetMinNoirnodePaymentsProto());
        char* reasonStr = new char[256];
        sprintf(reasonStr, "false: 'Invalid nProtocolVersion', nProtocolVersion=%d", mn.nProtocolVersion);
        return reasonStr;
    }
    //it's in the list (up to 8 entries ahead of current block to allow propagation) -- so let's skip it
    if (mnpayments.IsScheduled(mn, nBlockHeight)) {
        // LogPrintf("mnpayments.IsScheduled!\n");
        char* reasonStr = new char[256];
        sprintf(reasonStr, "false: 'is scheduled'");
        return reasonStr;
    }
    //it's too new, wait for a cycle
    if (fFilterSigTime && mn.sigTime + (nMnCount * 2.6 * 60) > GetAdjustedTime()) {
        // LogPrintf("it's too new, wait for a cycle!\n");
        char* reasonStr = new char[256];
        sprintf(reasonStr, "false: 'too new', sigTime=%s, will be qualifed after=%s",
                DateTimeStrFormat("%Y-%m-%d %H:%M UTC", mn.sigTime).c_str(), DateTimeStrFormat("%Y-%m-%d %H:%M UTC", mn.sigTime + (nMnCount * 2.6 * 60)).c_str());
        return reasonStr;
    }
    //make sure it has at least as many confirmations as there are noirnodes
    if (mn.GetCollateralAge() < nMnCount) {
        // LogPrintf("mn.GetCollateralAge()=%s!\n", mn.GetCollateralAge());
        // LogPrintf("nMnCount=%s!\n", nMnCount);
        char* reasonStr = new char[256];
        sprintf(reasonStr, "false: 'collateralAge < znCount', collateralAge=%d, znCount=%d", mn.GetCollateralAge(), nMnCount);
        return reasonStr;
    }
    return NULL;
}

//
// Deterministically select the oldest/best noirnode to pay on the network
//
CNoirnode* CNoirnodeMan::GetNextNoirnodeInQueueForPayment(bool fFilterSigTime, int& nCount)
{
    if(!pCurrentBlockIndex) {
        nCount = 0;
        return NULL;
    }
    return GetNextNoirnodeInQueueForPayment(pCurrentBlockIndex->nHeight, fFilterSigTime, nCount);
}

CNoirnode* CNoirnodeMan::GetNextNoirnodeInQueueForPayment(int nBlockHeight, bool fFilterSigTime, int& nCount)
{
    // Need LOCK2 here to ensure consistent locking order because the GetBlockHash call below locks cs_main
    LOCK2(cs_main,cs);

    CNoirnode *pBestNoirnode = NULL;
    std::vector<std::pair<int, CNoirnode*> > vecNoirnodeLastPaid;

    /*
        Make a vector with all of the last paid times
    */
    int nMnCount = CountEnabled();
    int index = 0;
    BOOST_FOREACH(CNoirnode &mn, vNoirnodes)
    {
        index += 1;
        // LogPrintf("index=%s, mn=%s\n", index, mn.ToString());
        /*if (!mn.IsValidForPayment()) {
            LogPrint("noirnodeman", "Noirnode, %s, addr(%s), not-qualified: 'not valid for payment'\n",
                     mn.vin.prevout.ToStringShort(), CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString());
            continue;
        }
        // //check protocol version
        if (mn.nProtocolVersion < mnpayments.GetMinNoirnodePaymentsProto()) {
            // LogPrintf("Invalid nProtocolVersion!\n");
            // LogPrintf("mn.nProtocolVersion=%s!\n", mn.nProtocolVersion);
            // LogPrintf("mnpayments.GetMinNoirnodePaymentsProto=%s!\n", mnpayments.GetMinNoirnodePaymentsProto());
            LogPrint("noirnodeman", "Noirnode, %s, addr(%s), not-qualified: 'invalid nProtocolVersion'\n",
                     mn.vin.prevout.ToStringShort(), CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString());
            continue;
        }
        //it's in the list (up to 8 entries ahead of current block to allow propagation) -- so let's skip it
        if (mnpayments.IsScheduled(mn, nBlockHeight)) {
            // LogPrintf("mnpayments.IsScheduled!\n");
            LogPrint("noirnodeman", "Noirnode, %s, addr(%s), not-qualified: 'IsScheduled'\n",
                     mn.vin.prevout.ToStringShort(), CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString());
            continue;
        }
        //it's too new, wait for a cycle
        if (fFilterSigTime && mn.sigTime + (nMnCount * 2.6 * 60) > GetAdjustedTime()) {
            // LogPrintf("it's too new, wait for a cycle!\n");
            LogPrint("noirnodeman", "Noirnode, %s, addr(%s), not-qualified: 'it's too new, wait for a cycle!', sigTime=%s, will be qualifed after=%s\n",
                     mn.vin.prevout.ToStringShort(), CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString(), DateTimeStrFormat("%Y-%m-%d %H:%M UTC", mn.sigTime).c_str(), DateTimeStrFormat("%Y-%m-%d %H:%M UTC", mn.sigTime + (nMnCount * 2.6 * 60)).c_str());
            continue;
        }
        //make sure it has at least as many confirmations as there are noirnodes
        if (mn.GetCollateralAge() < nMnCount) {
            // LogPrintf("mn.GetCollateralAge()=%s!\n", mn.GetCollateralAge());
            // LogPrintf("nMnCount=%s!\n", nMnCount);
            LogPrint("noirnodeman", "Noirnode, %s, addr(%s), not-qualified: 'mn.GetCollateralAge() < nMnCount', CollateralAge=%d, nMnCount=%d\n",
                     mn.vin.prevout.ToStringShort(), CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString(), mn.GetCollateralAge(), nMnCount);
            continue;
        }*/
        char* reasonStr = GetNotQualifyReason(mn, nBlockHeight, fFilterSigTime, nMnCount);
        if (reasonStr != NULL) {
            LogPrint("noirnodeman", "Noirnode, %s, addr(%s), qualify %s\n",
                     mn.vin.prevout.ToStringShort(), CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString(), reasonStr);
            delete [] reasonStr;
            continue;
        }
        vecNoirnodeLastPaid.push_back(std::make_pair(mn.GetLastPaidBlock(), &mn));
    }
    nCount = (int)vecNoirnodeLastPaid.size();

    //when the network is in the process of upgrading, don't penalize nodes that recently restarted
    if(fFilterSigTime && nCount < nMnCount / 3) {
        // LogPrintf("Need Return, nCount=%s, nMnCount/3=%s\n", nCount, nMnCount/3);
        return GetNextNoirnodeInQueueForPayment(nBlockHeight, false, nCount);
    }

    // Sort them low to high
    sort(vecNoirnodeLastPaid.begin(), vecNoirnodeLastPaid.end(), CompareLastPaidBlock());

    uint256 blockHash;
    if(!GetBlockHash(blockHash, nBlockHeight - 101)) {
        LogPrintf("CNoirnode::GetNextNoirnodeInQueueForPayment -- ERROR: GetBlockHash() failed at nBlockHeight %d\n", nBlockHeight - 101);
        return NULL;
    }
    // Look at 1/10 of the oldest nodes (by last payment), calculate their scores and pay the best one
    //  -- This doesn't look at who is being paid in the +8-10 blocks, allowing for double payments very rarely
    //  -- 1/100 payments should be a double payment on mainnet - (1/(3000/10))*2
    //  -- (chance per block * chances before IsScheduled will fire)
    int nTenthNetwork = nMnCount/10;
    int nCountTenth = 0;
    arith_uint256 nHighest = 0;
    BOOST_FOREACH (PAIRTYPE(int, CNoirnode*)& s, vecNoirnodeLastPaid){
        arith_uint256 nScore = s.second->CalculateScore(blockHash);
        if(nScore > nHighest){
            nHighest = nScore;
            pBestNoirnode = s.second;
        }
        nCountTenth++;
        if(nCountTenth >= nTenthNetwork) break;
    }
    return pBestNoirnode;
}

CNoirnode* CNoirnodeMan::FindRandomNotInVec(const std::vector<CTxIn> &vecToExclude, int nProtocolVersion)
{
    LOCK(cs);

    nProtocolVersion = nProtocolVersion == -1 ? mnpayments.GetMinNoirnodePaymentsProto() : nProtocolVersion;

    int nCountEnabled = CountEnabled(nProtocolVersion);
    int nCountNotExcluded = nCountEnabled - vecToExclude.size();

    LogPrintf("CNoirnodeMan::FindRandomNotInVec -- %d enabled noirnodes, %d noirnodes to choose from\n", nCountEnabled, nCountNotExcluded);
    if(nCountNotExcluded < 1) return NULL;

    // fill a vector of pointers
    std::vector<CNoirnode*> vpNoirnodesShuffled;
    BOOST_FOREACH(CNoirnode &mn, vNoirnodes) {
        vpNoirnodesShuffled.push_back(&mn);
    }

    InsecureRand insecureRand;
    // shuffle pointers
    std::random_shuffle(vpNoirnodesShuffled.begin(), vpNoirnodesShuffled.end(), insecureRand);
    bool fExclude;

    // loop through
    BOOST_FOREACH(CNoirnode* pmn, vpNoirnodesShuffled) {
        if(pmn->nProtocolVersion < nProtocolVersion || !pmn->IsEnabled()) continue;
        fExclude = false;
        BOOST_FOREACH(const CTxIn &txinToExclude, vecToExclude) {
            if(pmn->vin.prevout == txinToExclude.prevout) {
                fExclude = true;
                break;
            }
        }
        if(fExclude) continue;
        // found the one not in vecToExclude
        LogPrint("noirnode", "CNoirnodeMan::FindRandomNotInVec -- found, noirnode=%s\n", pmn->vin.prevout.ToStringShort());
        return pmn;
    }

    LogPrint("noirnode", "CNoirnodeMan::FindRandomNotInVec -- failed\n");
    return NULL;
}

int CNoirnodeMan::GetNoirnodeRank(const CTxIn& vin, int nBlockHeight, int nMinProtocol, bool fOnlyActive)
{
    std::vector<std::pair<int64_t, CNoirnode*> > vecNoirnodeScores;

    //make sure we know about this block
    uint256 blockHash = uint256();
    if(!GetBlockHash(blockHash, nBlockHeight)) return -1;

    LOCK(cs);

    // scan for winner
    BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {
        if(mn.nProtocolVersion < nMinProtocol) continue;
        if(fOnlyActive) {
            if(!mn.IsEnabled()) continue;
        }
        else {
            if(!mn.IsValidForPayment()) continue;
        }
        int64_t nScore = mn.CalculateScore(blockHash).GetCompact(false);

        vecNoirnodeScores.push_back(std::make_pair(nScore, &mn));
    }

    sort(vecNoirnodeScores.rbegin(), vecNoirnodeScores.rend(), CompareScoreMN());

    int nRank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CNoirnode*)& scorePair, vecNoirnodeScores) {
        nRank++;
        if(scorePair.second->vin.prevout == vin.prevout) return nRank;
    }

    return -1;
}

std::vector<std::pair<int, CNoirnode> > CNoirnodeMan::GetNoirnodeRanks(int nBlockHeight, int nMinProtocol)
{
    std::vector<std::pair<int64_t, CNoirnode*> > vecNoirnodeScores;
    std::vector<std::pair<int, CNoirnode> > vecNoirnodeRanks;

    //make sure we know about this block
    uint256 blockHash = uint256();
    if(!GetBlockHash(blockHash, nBlockHeight)) return vecNoirnodeRanks;

    LOCK(cs);

    // scan for winner
    BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {

        if(mn.nProtocolVersion < nMinProtocol || !mn.IsEnabled()) continue;

        int64_t nScore = mn.CalculateScore(blockHash).GetCompact(false);

        vecNoirnodeScores.push_back(std::make_pair(nScore, &mn));
    }

    sort(vecNoirnodeScores.rbegin(), vecNoirnodeScores.rend(), CompareScoreMN());

    int nRank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CNoirnode*)& s, vecNoirnodeScores) {
        nRank++;
        vecNoirnodeRanks.push_back(std::make_pair(nRank, *s.second));
    }

    return vecNoirnodeRanks;
}

CNoirnode* CNoirnodeMan::GetNoirnodeByRank(int nRank, int nBlockHeight, int nMinProtocol, bool fOnlyActive)
{
    std::vector<std::pair<int64_t, CNoirnode*> > vecNoirnodeScores;

    LOCK(cs);

    uint256 blockHash;
    if(!GetBlockHash(blockHash, nBlockHeight)) {
        LogPrintf("CNoirnode::GetNoirnodeByRank -- ERROR: GetBlockHash() failed at nBlockHeight %d\n", nBlockHeight);
        return NULL;
    }

    // Fill scores
    BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {

        if(mn.nProtocolVersion < nMinProtocol) continue;
        if(fOnlyActive && !mn.IsEnabled()) continue;

        int64_t nScore = mn.CalculateScore(blockHash).GetCompact(false);

        vecNoirnodeScores.push_back(std::make_pair(nScore, &mn));
    }

    sort(vecNoirnodeScores.rbegin(), vecNoirnodeScores.rend(), CompareScoreMN());

    int rank = 0;
    BOOST_FOREACH (PAIRTYPE(int64_t, CNoirnode*)& s, vecNoirnodeScores){
        rank++;
        if(rank == nRank) {
            return s.second;
        }
    }

    return NULL;
}

void CNoirnodeMan::ProcessNoirnodeConnections()
{
    //we don't care about this for regtest
    if(Params().NetworkIDString() == CBaseChainParams::REGTEST) return;

    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes) {
        if(pnode->fNoirnode) {
            if(darkSendPool.pSubmittedToNoirnode != NULL && pnode->addr == darkSendPool.pSubmittedToNoirnode->addr) continue;
            // LogPrintf("Closing Noirnode connection: peer=%d, addr=%s\n", pnode->id, pnode->addr.ToString());
            pnode->fDisconnect = true;
        }
    }
}

std::pair<CService, std::set<uint256> > CNoirnodeMan::PopScheduledMnbRequestConnection()
{
    LOCK(cs);
    if(listScheduledMnbRequestConnections.empty()) {
        return std::make_pair(CService(), std::set<uint256>());
    }

    std::set<uint256> setResult;

    listScheduledMnbRequestConnections.sort();
    std::pair<CService, uint256> pairFront = listScheduledMnbRequestConnections.front();

    // squash hashes from requests with the same CService as the first one into setResult
    std::list< std::pair<CService, uint256> >::iterator it = listScheduledMnbRequestConnections.begin();
    while(it != listScheduledMnbRequestConnections.end()) {
        if(pairFront.first == it->first) {
            setResult.insert(it->second);
            it = listScheduledMnbRequestConnections.erase(it);
        } else {
            // since list is sorted now, we can be sure that there is no more hashes left
            // to ask for from this addr
            break;
        }
    }
    return std::make_pair(pairFront.first, setResult);
}


void CNoirnodeMan::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{

//    LogPrint("noirnode", "CNoirnodeMan::ProcessMessage, strCommand=%s\n", strCommand);
    if(fLiteMode) return; // disable all Dash specific functionality
    if(!noirnodeSync.IsBlockchainSynced()) return;

    if (strCommand == NetMsgType::MNANNOUNCE) { //Noirnode Broadcast
        CNoirnodeBroadcast mnb;
        vRecv >> mnb;

        pfrom->setAskFor.erase(mnb.GetHash());

        LogPrintf("MNANNOUNCE -- Noirnode announce, noirnode=%s\n", mnb.vin.prevout.ToStringShort());

        int nDos = 0;

        if (CheckMnbAndUpdateNoirnodeList(pfrom, mnb, nDos)) {
            // use announced Noirnode as a peer
            addrman.Add(CAddress(mnb.addr, NODE_NETWORK), pfrom->addr, 2*60*60);
        } else if(nDos > 0) {
            Misbehaving(pfrom->GetId(), nDos);
        }

        if(fNoirnodesAdded) {
            NotifyNoirnodeUpdates();
        }
    } else if (strCommand == NetMsgType::MNPING) { //Noirnode Ping

        CNoirnodePing mnp;
        vRecv >> mnp;

        uint256 nHash = mnp.GetHash();

        pfrom->setAskFor.erase(nHash);

        LogPrint("noirnode", "MNPING -- Noirnode ping, noirnode=%s\n", mnp.vin.prevout.ToStringShort());

        // Need LOCK2 here to ensure consistent locking order because the CheckAndUpdate call below locks cs_main
        LOCK2(cs_main, cs);

        if(mapSeenNoirnodePing.count(nHash)) return; //seen
        mapSeenNoirnodePing.insert(std::make_pair(nHash, mnp));

        LogPrint("noirnode", "MNPING -- Noirnode ping, noirnode=%s new\n", mnp.vin.prevout.ToStringShort());

        // see if we have this Noirnode
        CNoirnode* pmn = mnodeman.Find(mnp.vin);

        // too late, new MNANNOUNCE is required
        if(pmn && pmn->IsNewStartRequired()) return;

        int nDos = 0;
        if(mnp.CheckAndUpdate(pmn, false, nDos)) return;

        if(nDos > 0) {
            // if anything significant failed, mark that node
            Misbehaving(pfrom->GetId(), nDos);
        } else if(pmn != NULL) {
            // nothing significant failed, mn is a known one too
            return;
        }

        // something significant is broken or mn is unknown,
        // we might have to ask for a noirnode entry once
        AskForMN(pfrom, mnp.vin);

    } else if (strCommand == NetMsgType::DSEG) { //Get Noirnode list or specific entry
        // Ignore such requests until we are fully synced.
        // We could start processing this after noirnode list is synced
        // but this is a heavy one so it's better to finish sync first.
        if (!noirnodeSync.IsSynced()) return;

        CTxIn vin;
        vRecv >> vin;

        LogPrint("noirnode", "DSEG -- Noirnode list, noirnode=%s\n", vin.prevout.ToStringShort());

        LOCK(cs);

        if(vin == CTxIn()) { //only should ask for this once
            //local network
            bool isLocal = (pfrom->addr.IsRFC1918() || pfrom->addr.IsLocal());

            if(!isLocal && Params().NetworkIDString() == CBaseChainParams::MAIN) {
                std::map<CNetAddr, int64_t>::iterator i = mAskedUsForNoirnodeList.find(pfrom->addr);
                if (i != mAskedUsForNoirnodeList.end()){
                    int64_t t = (*i).second;
                    if (GetTime() < t) {
                        Misbehaving(pfrom->GetId(), 34);
                        LogPrintf("DSEG -- peer already asked me for the list, peer=%d\n", pfrom->id);
                        return;
                    }
                }
                int64_t askAgain = GetTime() + DSEG_UPDATE_SECONDS;
                mAskedUsForNoirnodeList[pfrom->addr] = askAgain;
            }
        } //else, asking for a specific node which is ok

        int nInvCount = 0;

        BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {
            if (vin != CTxIn() && vin != mn.vin) continue; // asked for specific vin but we are not there yet
            if (mn.addr.IsRFC1918() || mn.addr.IsLocal()) continue; // do not send local network noirnode
            if (mn.IsUpdateRequired()) continue; // do not send outdated noirnodes

            LogPrint("noirnode", "DSEG -- Sending Noirnode entry: noirnode=%s  addr=%s\n", mn.vin.prevout.ToStringShort(), mn.addr.ToString());
            CNoirnodeBroadcast mnb = CNoirnodeBroadcast(mn);
            uint256 hash = mnb.GetHash();
            pfrom->PushInventory(CInv(MSG_NOIRNODE_ANNOUNCE, hash));
            pfrom->PushInventory(CInv(MSG_NOIRNODE_PING, mn.lastPing.GetHash()));
            nInvCount++;

            if (!mapSeenNoirnodeBroadcast.count(hash)) {
                mapSeenNoirnodeBroadcast.insert(std::make_pair(hash, std::make_pair(GetTime(), mnb)));
            }

            if (vin == mn.vin) {
                LogPrintf("DSEG -- Sent 1 Noirnode inv to peer %d\n", pfrom->id);
                return;
            }
        }

        if(vin == CTxIn()) {
            pfrom->PushMessage(NetMsgType::SYNCSTATUSCOUNT, NOIRNODE_SYNC_LIST, nInvCount);
            LogPrintf("DSEG -- Sent %d Noirnode invs to peer %d\n", nInvCount, pfrom->id);
            return;
        }
        // smth weird happen - someone asked us for vin we have no idea about?
        LogPrint("noirnode", "DSEG -- No invs sent to peer %d\n", pfrom->id);

    } else if (strCommand == NetMsgType::MNVERIFY) { // Noirnode Verify

        // Need LOCK2 here to ensure consistent locking order because the all functions below call GetBlockHash which locks cs_main
        LOCK2(cs_main, cs);

        CNoirnodeVerification mnv;
        vRecv >> mnv;

        if(mnv.vchSig1.empty()) {
            // CASE 1: someone asked me to verify myself /IP we are using/
            SendVerifyReply(pfrom, mnv);
        } else if (mnv.vchSig2.empty()) {
            // CASE 2: we _probably_ got verification we requested from some noirnode
            ProcessVerifyReply(pfrom, mnv);
        } else {
            // CASE 3: we _probably_ got verification broadcast signed by some noirnode which verified another one
            ProcessVerifyBroadcast(pfrom, mnv);
        }
    }
}

// Verification of noirnodes via unique direct requests.

void CNoirnodeMan::DoFullVerificationStep()
{
    if(activeNoirnode.vin == CTxIn()) return;
    if(!noirnodeSync.IsSynced()) return;

    std::vector<std::pair<int, CNoirnode> > vecNoirnodeRanks = GetNoirnodeRanks(pCurrentBlockIndex->nHeight - 1, MIN_POSE_PROTO_VERSION);

    // Need LOCK2 here to ensure consistent locking order because the SendVerifyRequest call below locks cs_main
    // through GetHeight() signal in ConnectNode
    LOCK2(cs_main, cs);

    int nCount = 0;

    int nMyRank = -1;
    int nRanksTotal = (int)vecNoirnodeRanks.size();

    // send verify requests only if we are in top MAX_POSE_RANK
    std::vector<std::pair<int, CNoirnode> >::iterator it = vecNoirnodeRanks.begin();
    while(it != vecNoirnodeRanks.end()) {
        if(it->first > MAX_POSE_RANK) {
            LogPrint("noirnode", "CNoirnodeMan::DoFullVerificationStep -- Must be in top %d to send verify request\n",
                        (int)MAX_POSE_RANK);
            return;
        }
        if(it->second.vin == activeNoirnode.vin) {
            nMyRank = it->first;
            LogPrint("noirnode", "CNoirnodeMan::DoFullVerificationStep -- Found self at rank %d/%d, verifying up to %d noirnodes\n",
                        nMyRank, nRanksTotal, (int)MAX_POSE_CONNECTIONS);
            break;
        }
        ++it;
    }

    // edge case: list is too short and this noirnode is not enabled
    if(nMyRank == -1) return;

    // send verify requests to up to MAX_POSE_CONNECTIONS noirnodes
    // starting from MAX_POSE_RANK + nMyRank and using MAX_POSE_CONNECTIONS as a step
    int nOffset = MAX_POSE_RANK + nMyRank - 1;
    if(nOffset >= (int)vecNoirnodeRanks.size()) return;

    std::vector<CNoirnode*> vSortedByAddr;
    BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {
        vSortedByAddr.push_back(&mn);
    }

    sort(vSortedByAddr.begin(), vSortedByAddr.end(), CompareByAddr());

    it = vecNoirnodeRanks.begin() + nOffset;
    while(it != vecNoirnodeRanks.end()) {
        if(it->second.IsPoSeVerified() || it->second.IsPoSeBanned()) {
            LogPrint("noirnode", "CNoirnodeMan::DoFullVerificationStep -- Already %s%s%s noirnode %s address %s, skipping...\n",
                        it->second.IsPoSeVerified() ? "verified" : "",
                        it->second.IsPoSeVerified() && it->second.IsPoSeBanned() ? " and " : "",
                        it->second.IsPoSeBanned() ? "banned" : "",
                        it->second.vin.prevout.ToStringShort(), it->second.addr.ToString());
            nOffset += MAX_POSE_CONNECTIONS;
            if(nOffset >= (int)vecNoirnodeRanks.size()) break;
            it += MAX_POSE_CONNECTIONS;
            continue;
        }
        LogPrint("noirnode", "CNoirnodeMan::DoFullVerificationStep -- Verifying noirnode %s rank %d/%d address %s\n",
                    it->second.vin.prevout.ToStringShort(), it->first, nRanksTotal, it->second.addr.ToString());
        if(SendVerifyRequest(CAddress(it->second.addr, NODE_NETWORK), vSortedByAddr)) {
            nCount++;
            if(nCount >= MAX_POSE_CONNECTIONS) break;
        }
        nOffset += MAX_POSE_CONNECTIONS;
        if(nOffset >= (int)vecNoirnodeRanks.size()) break;
        it += MAX_POSE_CONNECTIONS;
    }

    LogPrint("noirnode", "CNoirnodeMan::DoFullVerificationStep -- Sent verification requests to %d noirnodes\n", nCount);
}

// This function tries to find noirnodes with the same addr,
// find a verified one and ban all the other. If there are many nodes
// with the same addr but none of them is verified yet, then none of them are banned.
// It could take many times to run this before most of the duplicate nodes are banned.

void CNoirnodeMan::CheckSameAddr()
{
    if(!noirnodeSync.IsSynced() || vNoirnodes.empty()) return;

    std::vector<CNoirnode*> vBan;
    std::vector<CNoirnode*> vSortedByAddr;

    {
        LOCK(cs);

        CNoirnode* pprevNoirnode = NULL;
        CNoirnode* pverifiedNoirnode = NULL;

        BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {
            vSortedByAddr.push_back(&mn);
        }

        sort(vSortedByAddr.begin(), vSortedByAddr.end(), CompareByAddr());

        BOOST_FOREACH(CNoirnode* pmn, vSortedByAddr) {
            // check only (pre)enabled noirnodes
            if(!pmn->IsEnabled() && !pmn->IsPreEnabled()) continue;
            // initial step
            if(!pprevNoirnode) {
                pprevNoirnode = pmn;
                pverifiedNoirnode = pmn->IsPoSeVerified() ? pmn : NULL;
                continue;
            }
            // second+ step
            if(pmn->addr == pprevNoirnode->addr) {
                if(pverifiedNoirnode) {
                    // another noirnode with the same ip is verified, ban this one
                    vBan.push_back(pmn);
                } else if(pmn->IsPoSeVerified()) {
                    // this noirnode with the same ip is verified, ban previous one
                    vBan.push_back(pprevNoirnode);
                    // and keep a reference to be able to ban following noirnodes with the same ip
                    pverifiedNoirnode = pmn;
                }
            } else {
                pverifiedNoirnode = pmn->IsPoSeVerified() ? pmn : NULL;
            }
            pprevNoirnode = pmn;
        }
    }

    // ban duplicates
    BOOST_FOREACH(CNoirnode* pmn, vBan) {
        LogPrintf("CNoirnodeMan::CheckSameAddr -- increasing PoSe ban score for noirnode %s\n", pmn->vin.prevout.ToStringShort());
        pmn->IncreasePoSeBanScore();
    }
}

bool CNoirnodeMan::SendVerifyRequest(const CAddress& addr, const std::vector<CNoirnode*>& vSortedByAddr)
{
    if(netfulfilledman.HasFulfilledRequest(addr, strprintf("%s", NetMsgType::MNVERIFY)+"-request")) {
        // we already asked for verification, not a good idea to do this too often, skip it
        LogPrint("noirnode", "CNoirnodeMan::SendVerifyRequest -- too many requests, skipping... addr=%s\n", addr.ToString());
        return false;
    }

    CNode* pnode = ConnectNode(addr, NULL, false, true);
    if(pnode == NULL) {
        LogPrintf("CNoirnodeMan::SendVerifyRequest -- can't connect to node to verify it, addr=%s\n", addr.ToString());
        return false;
    }

    netfulfilledman.AddFulfilledRequest(addr, strprintf("%s", NetMsgType::MNVERIFY)+"-request");
    // use random nonce, store it and require node to reply with correct one later
    CNoirnodeVerification mnv(addr, GetRandInt(999999), pCurrentBlockIndex->nHeight - 1);
    mWeAskedForVerification[addr] = mnv;
    LogPrintf("CNoirnodeMan::SendVerifyRequest -- verifying node using nonce %d addr=%s\n", mnv.nonce, addr.ToString());
    pnode->PushMessage(NetMsgType::MNVERIFY, mnv);

    return true;
}

void CNoirnodeMan::SendVerifyReply(CNode* pnode, CNoirnodeVerification& mnv)
{
    // only noirnodes can sign this, why would someone ask regular node?
    if(!fNoirNode) {
        // do not ban, malicious node might be using my IP
        // and trying to confuse the node which tries to verify it
        return;
    }

    if(netfulfilledman.HasFulfilledRequest(pnode->addr, strprintf("%s", NetMsgType::MNVERIFY)+"-reply")) {
//        // peer should not ask us that often
        LogPrintf("NoirnodeMan::SendVerifyReply -- ERROR: peer already asked me recently, peer=%d\n", pnode->id);
        Misbehaving(pnode->id, 20);
        return;
    }

    uint256 blockHash;
    if(!GetBlockHash(blockHash, mnv.nBlockHeight)) {
        LogPrintf("NoirnodeMan::SendVerifyReply -- can't get block hash for unknown block height %d, peer=%d\n", mnv.nBlockHeight, pnode->id);
        return;
    }

    std::string strMessage = strprintf("%s%d%s", activeNoirnode.service.ToString(), mnv.nonce, blockHash.ToString());

    if(!darkSendSigner.SignMessage(strMessage, mnv.vchSig1, activeNoirnode.keyNoirnode)) {
        LogPrintf("NoirnodeMan::SendVerifyReply -- SignMessage() failed\n");
        return;
    }

    std::string strError;

    if(!darkSendSigner.VerifyMessage(activeNoirnode.pubKeyNoirnode, mnv.vchSig1, strMessage, strError)) {
        LogPrintf("NoirnodeMan::SendVerifyReply -- VerifyMessage() failed, error: %s\n", strError);
        return;
    }

    pnode->PushMessage(NetMsgType::MNVERIFY, mnv);
    netfulfilledman.AddFulfilledRequest(pnode->addr, strprintf("%s", NetMsgType::MNVERIFY)+"-reply");
}

void CNoirnodeMan::ProcessVerifyReply(CNode* pnode, CNoirnodeVerification& mnv)
{
    std::string strError;

    // did we even ask for it? if that's the case we should have matching fulfilled request
    if(!netfulfilledman.HasFulfilledRequest(pnode->addr, strprintf("%s", NetMsgType::MNVERIFY)+"-request")) {
        LogPrintf("CNoirnodeMan::ProcessVerifyReply -- ERROR: we didn't ask for verification of %s, peer=%d\n", pnode->addr.ToString(), pnode->id);
        Misbehaving(pnode->id, 20);
        return;
    }

    // Received nonce for a known address must match the one we sent
    if(mWeAskedForVerification[pnode->addr].nonce != mnv.nonce) {
        LogPrintf("CNoirnodeMan::ProcessVerifyReply -- ERROR: wrong nounce: requested=%d, received=%d, peer=%d\n",
                    mWeAskedForVerification[pnode->addr].nonce, mnv.nonce, pnode->id);
        Misbehaving(pnode->id, 20);
        return;
    }

    // Received nBlockHeight for a known address must match the one we sent
    if(mWeAskedForVerification[pnode->addr].nBlockHeight != mnv.nBlockHeight) {
        LogPrintf("CNoirnodeMan::ProcessVerifyReply -- ERROR: wrong nBlockHeight: requested=%d, received=%d, peer=%d\n",
                    mWeAskedForVerification[pnode->addr].nBlockHeight, mnv.nBlockHeight, pnode->id);
        Misbehaving(pnode->id, 20);
        return;
    }

    uint256 blockHash;
    if(!GetBlockHash(blockHash, mnv.nBlockHeight)) {
        // this shouldn't happen...
        LogPrintf("NoirnodeMan::ProcessVerifyReply -- can't get block hash for unknown block height %d, peer=%d\n", mnv.nBlockHeight, pnode->id);
        return;
    }

//    // we already verified this address, why node is spamming?
    if(netfulfilledman.HasFulfilledRequest(pnode->addr, strprintf("%s", NetMsgType::MNVERIFY)+"-done")) {
        LogPrintf("CNoirnodeMan::ProcessVerifyReply -- ERROR: already verified %s recently\n", pnode->addr.ToString());
        Misbehaving(pnode->id, 20);
        return;
    }

    {
        LOCK(cs);

        CNoirnode* prealNoirnode = NULL;
        std::vector<CNoirnode*> vpNoirnodesToBan;
        std::vector<CNoirnode>::iterator it = vNoirnodes.begin();
        std::string strMessage1 = strprintf("%s%d%s", pnode->addr.ToString(), mnv.nonce, blockHash.ToString());
        while(it != vNoirnodes.end()) {
            if(CAddress(it->addr, NODE_NETWORK) == pnode->addr) {
                if(darkSendSigner.VerifyMessage(it->pubKeyNoirnode, mnv.vchSig1, strMessage1, strError)) {
                    // found it!
                    prealNoirnode = &(*it);
                    if(!it->IsPoSeVerified()) {
                        it->DecreasePoSeBanScore();
                    }
                    netfulfilledman.AddFulfilledRequest(pnode->addr, strprintf("%s", NetMsgType::MNVERIFY)+"-done");

                    // we can only broadcast it if we are an activated noirnode
                    if(activeNoirnode.vin == CTxIn()) continue;
                    // update ...
                    mnv.addr = it->addr;
                    mnv.vin1 = it->vin;
                    mnv.vin2 = activeNoirnode.vin;
                    std::string strMessage2 = strprintf("%s%d%s%s%s", mnv.addr.ToString(), mnv.nonce, blockHash.ToString(),
                                            mnv.vin1.prevout.ToStringShort(), mnv.vin2.prevout.ToStringShort());
                    // ... and sign it
                    if(!darkSendSigner.SignMessage(strMessage2, mnv.vchSig2, activeNoirnode.keyNoirnode)) {
                        LogPrintf("NoirnodeMan::ProcessVerifyReply -- SignMessage() failed\n");
                        return;
                    }

                    std::string strError;

                    if(!darkSendSigner.VerifyMessage(activeNoirnode.pubKeyNoirnode, mnv.vchSig2, strMessage2, strError)) {
                        LogPrintf("NoirnodeMan::ProcessVerifyReply -- VerifyMessage() failed, error: %s\n", strError);
                        return;
                    }

                    mWeAskedForVerification[pnode->addr] = mnv;
                    mnv.Relay();

                } else {
                    vpNoirnodesToBan.push_back(&(*it));
                }
            }
            ++it;
        }
        // no real noirnode found?...
        if(!prealNoirnode) {
            // this should never be the case normally,
            // only if someone is trying to game the system in some way or smth like that
            LogPrintf("CNoirnodeMan::ProcessVerifyReply -- ERROR: no real noirnode found for addr %s\n", pnode->addr.ToString());
            Misbehaving(pnode->id, 20);
            return;
        }
        LogPrintf("CNoirnodeMan::ProcessVerifyReply -- verified real noirnode %s for addr %s\n",
                    prealNoirnode->vin.prevout.ToStringShort(), pnode->addr.ToString());
        // increase ban score for everyone else
        BOOST_FOREACH(CNoirnode* pmn, vpNoirnodesToBan) {
            pmn->IncreasePoSeBanScore();
            LogPrint("noirnode", "CNoirnodeMan::ProcessVerifyBroadcast -- increased PoSe ban score for %s addr %s, new score %d\n",
                        prealNoirnode->vin.prevout.ToStringShort(), pnode->addr.ToString(), pmn->nPoSeBanScore);
        }
        LogPrintf("CNoirnodeMan::ProcessVerifyBroadcast -- PoSe score increased for %d fake noirnodes, addr %s\n",
                    (int)vpNoirnodesToBan.size(), pnode->addr.ToString());
    }
}

void CNoirnodeMan::ProcessVerifyBroadcast(CNode* pnode, const CNoirnodeVerification& mnv)
{
    std::string strError;

    if(mapSeenNoirnodeVerification.find(mnv.GetHash()) != mapSeenNoirnodeVerification.end()) {
        // we already have one
        return;
    }
    mapSeenNoirnodeVerification[mnv.GetHash()] = mnv;

    // we don't care about history
    if(mnv.nBlockHeight < pCurrentBlockIndex->nHeight - MAX_POSE_BLOCKS) {
        LogPrint("noirnode", "NoirnodeMan::ProcessVerifyBroadcast -- Outdated: current block %d, verification block %d, peer=%d\n",
                    pCurrentBlockIndex->nHeight, mnv.nBlockHeight, pnode->id);
        return;
    }

    if(mnv.vin1.prevout == mnv.vin2.prevout) {
        LogPrint("noirnode", "NoirnodeMan::ProcessVerifyBroadcast -- ERROR: same vins %s, peer=%d\n",
                    mnv.vin1.prevout.ToStringShort(), pnode->id);
        // that was NOT a good idea to cheat and verify itself,
        // ban the node we received such message from
        Misbehaving(pnode->id, 100);
        return;
    }

    uint256 blockHash;
    if(!GetBlockHash(blockHash, mnv.nBlockHeight)) {
        // this shouldn't happen...
        LogPrintf("NoirnodeMan::ProcessVerifyBroadcast -- Can't get block hash for unknown block height %d, peer=%d\n", mnv.nBlockHeight, pnode->id);
        return;
    }

    int nRank = GetNoirnodeRank(mnv.vin2, mnv.nBlockHeight, MIN_POSE_PROTO_VERSION);

    if (nRank == -1) {
        LogPrint("noirnode", "CNoirnodeMan::ProcessVerifyBroadcast -- Can't calculate rank for noirnode %s\n",
                    mnv.vin2.prevout.ToStringShort());
        return;
    }

    if(nRank > MAX_POSE_RANK) {
        LogPrint("noirnode", "CNoirnodeMan::ProcessVerifyBroadcast -- Mastrernode %s is not in top %d, current rank %d, peer=%d\n",
                    mnv.vin2.prevout.ToStringShort(), (int)MAX_POSE_RANK, nRank, pnode->id);
        return;
    }

    {
        LOCK(cs);

        std::string strMessage1 = strprintf("%s%d%s", mnv.addr.ToString(), mnv.nonce, blockHash.ToString());
        std::string strMessage2 = strprintf("%s%d%s%s%s", mnv.addr.ToString(), mnv.nonce, blockHash.ToString(),
                                mnv.vin1.prevout.ToStringShort(), mnv.vin2.prevout.ToStringShort());

        CNoirnode* pmn1 = Find(mnv.vin1);
        if(!pmn1) {
            LogPrintf("CNoirnodeMan::ProcessVerifyBroadcast -- can't find noirnode1 %s\n", mnv.vin1.prevout.ToStringShort());
            return;
        }

        CNoirnode* pmn2 = Find(mnv.vin2);
        if(!pmn2) {
            LogPrintf("CNoirnodeMan::ProcessVerifyBroadcast -- can't find noirnode2 %s\n", mnv.vin2.prevout.ToStringShort());
            return;
        }

        if(pmn1->addr != mnv.addr) {
            LogPrintf("CNoirnodeMan::ProcessVerifyBroadcast -- addr %s do not match %s\n", mnv.addr.ToString(), pnode->addr.ToString());
            return;
        }

        if(darkSendSigner.VerifyMessage(pmn1->pubKeyNoirnode, mnv.vchSig1, strMessage1, strError)) {
            LogPrintf("NoirnodeMan::ProcessVerifyBroadcast -- VerifyMessage() for noirnode1 failed, error: %s\n", strError);
            return;
        }

        if(darkSendSigner.VerifyMessage(pmn2->pubKeyNoirnode, mnv.vchSig2, strMessage2, strError)) {
            LogPrintf("NoirnodeMan::ProcessVerifyBroadcast -- VerifyMessage() for noirnode2 failed, error: %s\n", strError);
            return;
        }

        if(!pmn1->IsPoSeVerified()) {
            pmn1->DecreasePoSeBanScore();
        }
        mnv.Relay();

        LogPrintf("CNoirnodeMan::ProcessVerifyBroadcast -- verified noirnode %s for addr %s\n",
                    pmn1->vin.prevout.ToStringShort(), pnode->addr.ToString());

        // increase ban score for everyone else with the same addr
        int nCount = 0;
        BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {
            if(mn.addr != mnv.addr || mn.vin.prevout == mnv.vin1.prevout) continue;
            mn.IncreasePoSeBanScore();
            nCount++;
            LogPrint("noirnode", "CNoirnodeMan::ProcessVerifyBroadcast -- increased PoSe ban score for %s addr %s, new score %d\n",
                        mn.vin.prevout.ToStringShort(), mn.addr.ToString(), mn.nPoSeBanScore);
        }
        LogPrintf("CNoirnodeMan::ProcessVerifyBroadcast -- PoSe score incresed for %d fake noirnodes, addr %s\n",
                    nCount, pnode->addr.ToString());
    }
}

std::string CNoirnodeMan::ToString() const
{
    std::ostringstream info;

    info << "Noirnodes: " << (int)vNoirnodes.size() <<
            ", peers who asked us for Noirnode list: " << (int)mAskedUsForNoirnodeList.size() <<
            ", peers we asked for Noirnode list: " << (int)mWeAskedForNoirnodeList.size() <<
            ", entries in Noirnode list we asked for: " << (int)mWeAskedForNoirnodeListEntry.size() <<
            ", noirnode index size: " << indexNoirnodes.GetSize() <<
            ", nDsqCount: " << (int)nDsqCount;

    return info.str();
}

void CNoirnodeMan::UpdateNoirnodeList(CNoirnodeBroadcast mnb)
{
    try {
        LogPrintf("CNoirnodeMan::UpdateNoirnodeList\n");
        LOCK2(cs_main, cs);
        mapSeenNoirnodePing.insert(std::make_pair(mnb.lastPing.GetHash(), mnb.lastPing));
        mapSeenNoirnodeBroadcast.insert(std::make_pair(mnb.GetHash(), std::make_pair(GetTime(), mnb)));

        LogPrintf("CNoirnodeMan::UpdateNoirnodeList -- noirnode=%s  addr=%s\n", mnb.vin.prevout.ToStringShort(), mnb.addr.ToString());

        CNoirnode *pmn = Find(mnb.vin);
        if (pmn == NULL) {
            CNoirnode mn(mnb);
            if (Add(mn)) {
                noirnodeSync.AddedNoirnodeList();
            }
        } else {
            CNoirnodeBroadcast mnbOld = mapSeenNoirnodeBroadcast[CNoirnodeBroadcast(*pmn).GetHash()].second;
            if (pmn->UpdateFromNewBroadcast(mnb)) {
                noirnodeSync.AddedNoirnodeList();
                mapSeenNoirnodeBroadcast.erase(mnbOld.GetHash());
            }
        }
    } catch (const std::exception &e) {
        PrintExceptionContinue(&e, "UpdateNoirnodeList");
    }
}

bool CNoirnodeMan::CheckMnbAndUpdateNoirnodeList(CNode* pfrom, CNoirnodeBroadcast mnb, int& nDos)
{
    // Need LOCK2 here to ensure consistent locking order because the SimpleCheck call below locks cs_main
    LOCK(cs_main);

    {
        LOCK(cs);
        nDos = 0;
        LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- noirnode=%s\n", mnb.vin.prevout.ToStringShort());

        uint256 hash = mnb.GetHash();
        if (mapSeenNoirnodeBroadcast.count(hash) && !mnb.fRecovery) { //seen
            LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- noirnode=%s seen\n", mnb.vin.prevout.ToStringShort());
            // less then 2 pings left before this MN goes into non-recoverable state, bump sync timeout
            if (GetTime() - mapSeenNoirnodeBroadcast[hash].first > NOIRNODE_NEW_START_REQUIRED_SECONDS - NOIRNODE_MIN_MNP_SECONDS * 2) {
                LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- noirnode=%s seen update\n", mnb.vin.prevout.ToStringShort());
                mapSeenNoirnodeBroadcast[hash].first = GetTime();
                noirnodeSync.AddedNoirnodeList();
            }
            // did we ask this node for it?
            if (pfrom && IsMnbRecoveryRequested(hash) && GetTime() < mMnbRecoveryRequests[hash].first) {
                LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- mnb=%s seen request\n", hash.ToString());
                if (mMnbRecoveryRequests[hash].second.count(pfrom->addr)) {
                    LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- mnb=%s seen request, addr=%s\n", hash.ToString(), pfrom->addr.ToString());
                    // do not allow node to send same mnb multiple times in recovery mode
                    mMnbRecoveryRequests[hash].second.erase(pfrom->addr);
                    // does it have newer lastPing?
                    if (mnb.lastPing.sigTime > mapSeenNoirnodeBroadcast[hash].second.lastPing.sigTime) {
                        // simulate Check
                        CNoirnode mnTemp = CNoirnode(mnb);
                        mnTemp.Check();
                        LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- mnb=%s seen request, addr=%s, better lastPing: %d min ago, projected mn state: %s\n", hash.ToString(), pfrom->addr.ToString(), (GetTime() - mnb.lastPing.sigTime) / 60, mnTemp.GetStateString());
                        if (mnTemp.IsValidStateForAutoStart(mnTemp.nActiveState)) {
                            // this node thinks it's a good one
                            LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- noirnode=%s seen good\n", mnb.vin.prevout.ToStringShort());
                            mMnbRecoveryGoodReplies[hash].push_back(mnb);
                        }
                    }
                }
            }
            return true;
        }
        mapSeenNoirnodeBroadcast.insert(std::make_pair(hash, std::make_pair(GetTime(), mnb)));

        LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- noirnode=%s new\n", mnb.vin.prevout.ToStringShort());

        if (!mnb.SimpleCheck(nDos)) {
            LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- SimpleCheck() failed, noirnode=%s\n", mnb.vin.prevout.ToStringShort());
            return false;
        }

        // search Noirnode list
        CNoirnode *pmn = Find(mnb.vin);
        if (pmn) {
            CNoirnodeBroadcast mnbOld = mapSeenNoirnodeBroadcast[CNoirnodeBroadcast(*pmn).GetHash()].second;
            if (!mnb.Update(pmn, nDos)) {
                LogPrint("noirnode", "CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- Update() failed, noirnode=%s\n", mnb.vin.prevout.ToStringShort());
                return false;
            }
            if (hash != mnbOld.GetHash()) {
                mapSeenNoirnodeBroadcast.erase(mnbOld.GetHash());
            }
        }
    } // end of LOCK(cs);

    if(mnb.CheckOutpoint(nDos)) {
        Add(mnb);
        noirnodeSync.AddedNoirnodeList();
        // if it matches our Noirnode privkey...
        if(fNoirNode && mnb.pubKeyNoirnode == activeNoirnode.pubKeyNoirnode) {
            mnb.nPoSeBanScore = -NOIRNODE_POSE_BAN_MAX_SCORE;
            if(mnb.nProtocolVersion == PROTOCOL_VERSION || mnb.nProtocolVersion == 90048) {
                // ... and PROTOCOL_VERSION, then we've been remotely activated ...
                LogPrintf("CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- Got NEW Noirnode entry: noirnode=%s  sigTime=%lld  addr=%s\n",
                            mnb.vin.prevout.ToStringShort(), mnb.sigTime, mnb.addr.ToString());
                activeNoirnode.ManageState();
            } else {
                // ... otherwise we need to reactivate our node, do not add it to the list and do not relay
                // but also do not ban the node we get this message from
                LogPrintf("CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- wrong PROTOCOL_VERSION, re-activate your MN: message nProtocolVersion=%d  PROTOCOL_VERSION=%d\n", mnb.nProtocolVersion, PROTOCOL_VERSION);
                return false;
            }
        }
        mnb.RelayNoirNode();
    } else {
        LogPrintf("CNoirnodeMan::CheckMnbAndUpdateNoirnodeList -- Rejected Noirnode entry: %s  addr=%s\n", mnb.vin.prevout.ToStringShort(), mnb.addr.ToString());
        return false;
    }

    return true;
}

void CNoirnodeMan::UpdateLastPaid()
{
    LOCK(cs);
    if(fLiteMode) return;
    if(!pCurrentBlockIndex) {
        // LogPrintf("CNoirnodeMan::UpdateLastPaid, pCurrentBlockIndex=NULL\n");
        return;
    }

    static bool IsFirstRun = true;
    // Do full scan on first run or if we are not a noirnode
    // (MNs should update this info on every block, so limited scan should be enough for them)
    int nMaxBlocksToScanBack = (IsFirstRun || !fNoirNode) ? mnpayments.GetStorageLimit() : LAST_PAID_SCAN_BLOCKS;

    LogPrint("mnpayments", "CNoirnodeMan::UpdateLastPaid -- nHeight=%d, nMaxBlocksToScanBack=%d, IsFirstRun=%s\n",
                             pCurrentBlockIndex->nHeight, nMaxBlocksToScanBack, IsFirstRun ? "true" : "false");

    BOOST_FOREACH(CNoirnode& mn, vNoirnodes) {
        mn.UpdateLastPaid(pCurrentBlockIndex, nMaxBlocksToScanBack);
    }

    // every time is like the first time if winners list is not synced
    IsFirstRun = !noirnodeSync.IsWinnersListSynced();
}

void CNoirnodeMan::CheckAndRebuildNoirnodeIndex()
{
    LOCK(cs);

    if(GetTime() - nLastIndexRebuildTime < MIN_INDEX_REBUILD_TIME) {
        return;
    }

    if(indexNoirnodes.GetSize() <= MAX_EXPECTED_INDEX_SIZE) {
        return;
    }

    if(indexNoirnodes.GetSize() <= int(vNoirnodes.size())) {
        return;
    }

    indexNoirnodesOld = indexNoirnodes;
    indexNoirnodes.Clear();
    for(size_t i = 0; i < vNoirnodes.size(); ++i) {
        indexNoirnodes.AddNoirnodeVIN(vNoirnodes[i].vin);
    }

    fIndexRebuilt = true;
    nLastIndexRebuildTime = GetTime();
}

void CNoirnodeMan::UpdateWatchdogVoteTime(const CTxIn& vin)
{
    LOCK(cs);
    CNoirnode* pMN = Find(vin);
    if(!pMN)  {
        return;
    }
    pMN->UpdateWatchdogVoteTime();
    nLastWatchdogVoteTime = GetTime();
}

bool CNoirnodeMan::IsWatchdogActive()
{
    LOCK(cs);
    // Check if any noirnodes have voted recently, otherwise return false
    return (GetTime() - nLastWatchdogVoteTime) <= NOIRNODE_WATCHDOG_MAX_SECONDS;
}

void CNoirnodeMan::CheckNoirnode(const CTxIn& vin, bool fForce)
{
    LOCK(cs);
    CNoirnode* pMN = Find(vin);
    if(!pMN)  {
        return;
    }
    pMN->Check(fForce);
}

void CNoirnodeMan::CheckNoirnode(const CPubKey& pubKeyNoirnode, bool fForce)
{
    LOCK(cs);
    CNoirnode* pMN = Find(pubKeyNoirnode);
    if(!pMN)  {
        return;
    }
    pMN->Check(fForce);
}

int CNoirnodeMan::GetNoirnodeState(const CTxIn& vin)
{
    LOCK(cs);
    CNoirnode* pMN = Find(vin);
    if(!pMN)  {
        return CNoirnode::NOIRNODE_NEW_START_REQUIRED;
    }
    return pMN->nActiveState;
}

int CNoirnodeMan::GetNoirnodeState(const CPubKey& pubKeyNoirnode)
{
    LOCK(cs);
    CNoirnode* pMN = Find(pubKeyNoirnode);
    if(!pMN)  {
        return CNoirnode::NOIRNODE_NEW_START_REQUIRED;
    }
    return pMN->nActiveState;
}

bool CNoirnodeMan::IsNoirnodePingedWithin(const CTxIn& vin, int nSeconds, int64_t nTimeToCheckAt)
{
    LOCK(cs);
    CNoirnode* pMN = Find(vin);
    if(!pMN) {
        return false;
    }
    return pMN->IsPingedWithin(nSeconds, nTimeToCheckAt);
}

void CNoirnodeMan::SetNoirnodeLastPing(const CTxIn& vin, const CNoirnodePing& mnp)
{
    LOCK(cs);
    CNoirnode* pMN = Find(vin);
    if(!pMN)  {
        return;
    }
    pMN->lastPing = mnp;
    mapSeenNoirnodePing.insert(std::make_pair(mnp.GetHash(), mnp));

    CNoirnodeBroadcast mnb(*pMN);
    uint256 hash = mnb.GetHash();
    if(mapSeenNoirnodeBroadcast.count(hash)) {
        mapSeenNoirnodeBroadcast[hash].second.lastPing = mnp;
    }
}

void CNoirnodeMan::UpdatedBlockTip(const CBlockIndex *pindex)
{
    pCurrentBlockIndex = pindex;
    LogPrint("noirnode", "CNoirnodeMan::UpdatedBlockTip -- pCurrentBlockIndex->nHeight=%d\n", pCurrentBlockIndex->nHeight);

    CheckSameAddr();

    if(fNoirNode) {
        // normal wallet does not need to update this every block, doing update on rpc call should be enough
        UpdateLastPaid();
    }
}

void CNoirnodeMan::NotifyNoirnodeUpdates()
{
    // Avoid double locking
    bool fNoirnodesAddedLocal = false;
    bool fNoirnodesRemovedLocal = false;
    {
        LOCK(cs);
        fNoirnodesAddedLocal = fNoirnodesAdded;
        fNoirnodesRemovedLocal = fNoirnodesRemoved;
    }

    if(fNoirnodesAddedLocal) {
//        governance.CheckNoirnodeOrphanObjects();
//        governance.CheckNoirnodeOrphanVotes();
    }
    if(fNoirnodesRemovedLocal) {
//        governance.UpdateCachesAndClean();
    }

    LOCK(cs);
    fNoirnodesAdded = false;
    fNoirnodesRemoved = false;
}
