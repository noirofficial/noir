// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activenoirnode.h"
#include "consensus/validation.h"
#include "darksend.h"
#include "init.h"
//#include "governance.h"
#include "noirnode.h"
#include "noirnode-payments.h"
#include "noirnode-sync.h"
#include "noirnodeman.h"
#include "util.h"

#include <boost/lexical_cast.hpp>


CNoirnode::CNoirnode() :
    vin(),
    addr(),
    pubKeyCollateralAddress(),
    pubKeyNoirnode(),
    lastPing(),
    vchSig(),
    sigTime(GetAdjustedTime()),
    nLastDsq(0),
    nTimeLastChecked(0),
    nTimeLastPaid(0),
    nTimeLastWatchdogVote(0),
    nActiveState(NOIRNODE_ENABLED),
    nCacheCollateralBlock(0),
    nBlockLastPaid(0),
    nProtocolVersion(PROTOCOL_VERSION),
    nPoSeBanScore(0),
    nPoSeBanHeight(0),
    fAllowMixingTx(true),
    fUnitTest(false) {}

CNoirnode::CNoirnode(CService addrNew, CTxIn vinNew, CPubKey pubKeyCollateralAddressNew, CPubKey pubKeyNoirnodeNew, int nProtocolVersionIn) :
    vin(vinNew),
    addr(addrNew),
    pubKeyCollateralAddress(pubKeyCollateralAddressNew),
    pubKeyNoirnode(pubKeyNoirnodeNew),
    lastPing(),
    vchSig(),
    sigTime(GetAdjustedTime()),
    nLastDsq(0),
    nTimeLastChecked(0),
    nTimeLastPaid(0),
    nTimeLastWatchdogVote(0),
    nActiveState(NOIRNODE_ENABLED),
    nCacheCollateralBlock(0),
    nBlockLastPaid(0),
    nProtocolVersion(nProtocolVersionIn),
    nPoSeBanScore(0),
    nPoSeBanHeight(0),
    fAllowMixingTx(true),
    fUnitTest(false) {}

CNoirnode::CNoirnode(const CNoirnode &other) :
    vin(other.vin),
    addr(other.addr),
    pubKeyCollateralAddress(other.pubKeyCollateralAddress),
    pubKeyNoirnode(other.pubKeyNoirnode),
    lastPing(other.lastPing),
    vchSig(other.vchSig),
    sigTime(other.sigTime),
    nLastDsq(other.nLastDsq),
    nTimeLastChecked(other.nTimeLastChecked),
    nTimeLastPaid(other.nTimeLastPaid),
    nTimeLastWatchdogVote(other.nTimeLastWatchdogVote),
    nActiveState(other.nActiveState),
    nCacheCollateralBlock(other.nCacheCollateralBlock),
    nBlockLastPaid(other.nBlockLastPaid),
    nProtocolVersion(other.nProtocolVersion),
    nPoSeBanScore(other.nPoSeBanScore),
    nPoSeBanHeight(other.nPoSeBanHeight),
    fAllowMixingTx(other.fAllowMixingTx),
    fUnitTest(other.fUnitTest) {}

CNoirnode::CNoirnode(const CNoirnodeBroadcast &mnb) :
    vin(mnb.vin),
    addr(mnb.addr),
    pubKeyCollateralAddress(mnb.pubKeyCollateralAddress),
    pubKeyNoirnode(mnb.pubKeyNoirnode),
    lastPing(mnb.lastPing),
    vchSig(mnb.vchSig),
    sigTime(mnb.sigTime),
    nLastDsq(0),
    nTimeLastChecked(0),
    nTimeLastPaid(0),
    nTimeLastWatchdogVote(mnb.sigTime),
    nActiveState(mnb.nActiveState),
    nCacheCollateralBlock(0),
    nBlockLastPaid(0),
    nProtocolVersion(mnb.nProtocolVersion),
    nPoSeBanScore(0),
    nPoSeBanHeight(0),
    fAllowMixingTx(true),
    fUnitTest(false) {}

//CSporkManager sporkManager;
//
// When a new noirnode broadcast is sent, update our information
//
bool CNoirnode::UpdateFromNewBroadcast(CNoirnodeBroadcast &mnb) {
    if (mnb.sigTime <= sigTime && !mnb.fRecovery) return false;

    pubKeyNoirnode = mnb.pubKeyNoirnode;
    sigTime = mnb.sigTime;
    vchSig = mnb.vchSig;
    nProtocolVersion = mnb.nProtocolVersion;
    addr = mnb.addr;
    nPoSeBanScore = 0;
    nPoSeBanHeight = 0;
    nTimeLastChecked = 0;
    int nDos = 0;
    if (mnb.lastPing == CNoirnodePing() || (mnb.lastPing != CNoirnodePing() && mnb.lastPing.CheckAndUpdate(this, true, nDos))) {
        lastPing = mnb.lastPing;
        mnodeman.mapSeenNoirnodePing.insert(std::make_pair(lastPing.GetHash(), lastPing));
    }
    // if it matches our Noirnode privkey...
    if (fNoirNode && pubKeyNoirnode == activeNoirnode.pubKeyNoirnode) {
        nPoSeBanScore = -NOIRNODE_POSE_BAN_MAX_SCORE;
        if (nProtocolVersion == PROTOCOL_VERSION) {
            // ... and PROTOCOL_VERSION, then we've been remotely activated ...
            activeNoirnode.ManageState();
        } else {
            // ... otherwise we need to reactivate our node, do not add it to the list and do not relay
            // but also do not ban the node we get this message from
            LogPrintf("CNoirnode::UpdateFromNewBroadcast -- wrong PROTOCOL_VERSION, re-activate your MN: message nProtocolVersion=%d  PROTOCOL_VERSION=%d\n", nProtocolVersion, PROTOCOL_VERSION);
            return false;
        }
    }
    return true;
}

//
// Deterministically calculate a given "score" for a Noirnode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
arith_uint256 CNoirnode::CalculateScore(const uint256 &blockHash) {
    uint256 aux = ArithToUint256(UintToArith256(vin.prevout.hash) + vin.prevout.n);

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << blockHash;
    arith_uint256 hash2 = UintToArith256(ss.GetHash());

    CHashWriter ss2(SER_GETHASH, PROTOCOL_VERSION);
    ss2 << blockHash;
    ss2 << aux;
    arith_uint256 hash3 = UintToArith256(ss2.GetHash());

    return (hash3 > hash2 ? hash3 - hash2 : hash2 - hash3);
}

void CNoirnode::Check(bool fForce) {
    LOCK(cs);

    if (ShutdownRequested()) return;

    if (!fForce && (GetTime() - nTimeLastChecked < NOIRNODE_CHECK_SECONDS)) return;
    nTimeLastChecked = GetTime();

    LogPrint("noirnode", "CNoirnode::Check -- Noirnode %s is in %s state\n", vin.prevout.ToStringShort(), GetStateString());

    //once spent, stop doing the checks
    if (IsOutpointSpent()) return;

    int nHeight = 0;
    if (!fUnitTest) {
        TRY_LOCK(cs_main, lockMain);
        if (!lockMain) return;

        CCoins coins;
        if (!pcoinsTip->GetCoins(vin.prevout.hash, coins) ||
                (unsigned int) vin.prevout.n >= coins.vout.size() ||
                coins.vout[vin.prevout.n].IsNull()) {
            nActiveState = NOIRNODE_OUTPOINT_SPENT;
            LogPrint("noirnode", "CNoirnode::Check -- Failed to find Noirnode UTXO, noirnode=%s\n", vin.prevout.ToStringShort());
            return;
        }

        nHeight = chainActive.Height();
    }

    if (IsPoSeBanned()) {
        if (nHeight < nPoSeBanHeight) return; // too early?
        // Otherwise give it a chance to proceed further to do all the usual checks and to change its state.
        // Noirnode still will be on the edge and can be banned back easily if it keeps ignoring mnverify
        // or connect attempts. Will require few mnverify messages to strengthen its position in mn list.
        LogPrintf("CNoirnode::Check -- Noirnode %s is unbanned and back in list now\n", vin.prevout.ToStringShort());
        DecreasePoSeBanScore();
    } else if (nPoSeBanScore >= NOIRNODE_POSE_BAN_MAX_SCORE) {
        nActiveState = NOIRNODE_POSE_BAN;
        // ban for the whole payment cycle
        nPoSeBanHeight = nHeight + mnodeman.size();
        LogPrintf("CNoirnode::Check -- Noirnode %s is banned till block %d now\n", vin.prevout.ToStringShort(), nPoSeBanHeight);
        return;
    }

    int nActiveStatePrev = nActiveState;
    bool fOurNoirnode = fNoirNode && activeNoirnode.pubKeyNoirnode == pubKeyNoirnode;

    // noirnode doesn't meet payment protocol requirements ...
    bool fRequireUpdate = nProtocolVersion < mnpayments.GetMinNoirnodePaymentsProto() ||
                          // or it's our own node and we just updated it to the new protocol but we are still waiting for activation ...
                          (fOurNoirnode && nProtocolVersion < PROTOCOL_VERSION);

    if (fRequireUpdate) {
        nActiveState = NOIRNODE_UPDATE_REQUIRED;
        if (nActiveStatePrev != nActiveState) {
            LogPrint("noirnode", "CNoirnode::Check -- Noirnode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
        }
        return;
    }

    // keep old noirnodes on start, give them a chance to receive updates...
    bool fWaitForPing = !noirnodeSync.IsNoirnodeListSynced() && !IsPingedWithin(NOIRNODE_MIN_MNP_SECONDS);

    if (fWaitForPing && !fOurNoirnode) {
        // ...but if it was already expired before the initial check - return right away
        if (IsExpired() || IsWatchdogExpired() || IsNewStartRequired()) {
            LogPrint("noirnode", "CNoirnode::Check -- Noirnode %s is in %s state, waiting for ping\n", vin.prevout.ToStringShort(), GetStateString());
            return;
        }
    }

    // don't expire if we are still in "waiting for ping" mode unless it's our own noirnode
    if (!fWaitForPing || fOurNoirnode) {

        if (!IsPingedWithin(NOIRNODE_NEW_START_REQUIRED_SECONDS)) {
            nActiveState = NOIRNODE_NEW_START_REQUIRED;
            if (nActiveStatePrev != nActiveState) {
                LogPrint("noirnode", "CNoirnode::Check -- Noirnode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
            }
            return;
        }

        bool fWatchdogActive = noirnodeSync.IsSynced() && mnodeman.IsWatchdogActive();
        bool fWatchdogExpired = (fWatchdogActive && ((GetTime() - nTimeLastWatchdogVote) > NOIRNODE_WATCHDOG_MAX_SECONDS));

        //        LogPrint("noirnode", "CNoirnode::Check -- outpoint=%s, nTimeLastWatchdogVote=%d, GetTime()=%d, fWatchdogExpired=%d\n",
        //                vin.prevout.ToStringShort(), nTimeLastWatchdogVote, GetTime(), fWatchdogExpired);

        if (fWatchdogExpired) {
            nActiveState = NOIRNODE_WATCHDOG_EXPIRED;
            if (nActiveStatePrev != nActiveState) {
                LogPrint("noirnode", "CNoirnode::Check -- Noirnode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
            }
            return;
        }

        if (!IsPingedWithin(NOIRNODE_EXPIRATION_SECONDS)) {
            nActiveState = NOIRNODE_EXPIRED;
            if (nActiveStatePrev != nActiveState) {
                LogPrint("noirnode", "CNoirnode::Check -- Noirnode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
            }
            return;
        }
    }

    if (lastPing.sigTime - sigTime < NOIRNODE_MIN_MNP_SECONDS) {
        nActiveState = NOIRNODE_PRE_ENABLED;
        if (nActiveStatePrev != nActiveState) {
            LogPrint("noirnode", "CNoirnode::Check -- Noirnode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
        }
        return;
    }

    nActiveState = NOIRNODE_ENABLED; // OK
    if (nActiveStatePrev != nActiveState) {
        LogPrint("noirnode", "CNoirnode::Check -- Noirnode %s is in %s state now\n", vin.prevout.ToStringShort(), GetStateString());
    }
}

bool CNoirnode::IsValidNetAddr() {
    return IsValidNetAddr(addr);
}

bool CNoirnode::IsValidForPayment() {
    if (nActiveState == NOIRNODE_ENABLED) {
        return true;
    }
    //    if(!sporkManager.IsSporkActive(SPORK_14_REQUIRE_SENTINEL_FLAG) &&
    //       (nActiveState == NOIRNODE_WATCHDOG_EXPIRED)) {
    //        return true;
    //    }

    return false;
}

bool CNoirnode::IsValidNetAddr(CService addrIn) {
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return Params().NetworkIDString() == CBaseChainParams::REGTEST ||
           (addrIn.IsIPv4() && IsReachable(addrIn) && addrIn.IsRoutable());
}

noirnode_info_t CNoirnode::GetInfo() {
    noirnode_info_t info;
    info.vin = vin;
    info.addr = addr;
    info.pubKeyCollateralAddress = pubKeyCollateralAddress;
    info.pubKeyNoirnode = pubKeyNoirnode;
    info.sigTime = sigTime;
    info.nLastDsq = nLastDsq;
    info.nTimeLastChecked = nTimeLastChecked;
    info.nTimeLastPaid = nTimeLastPaid;
    info.nTimeLastWatchdogVote = nTimeLastWatchdogVote;
    info.nTimeLastPing = lastPing.sigTime;
    info.nActiveState = nActiveState;
    info.nProtocolVersion = nProtocolVersion;
    info.fInfoValid = true;
    return info;
}

std::string CNoirnode::StateToString(int nStateIn) {
    switch (nStateIn) {
    case NOIRNODE_PRE_ENABLED:
        return "PRE_ENABLED";
    case NOIRNODE_ENABLED:
        return "ENABLED";
    case NOIRNODE_EXPIRED:
        return "EXPIRED";
    case NOIRNODE_OUTPOINT_SPENT:
        return "OUTPOINT_SPENT";
    case NOIRNODE_UPDATE_REQUIRED:
        return "UPDATE_REQUIRED";
    case NOIRNODE_WATCHDOG_EXPIRED:
        return "WATCHDOG_EXPIRED";
    case NOIRNODE_NEW_START_REQUIRED:
        return "NEW_START_REQUIRED";
    case NOIRNODE_POSE_BAN:
        return "POSE_BAN";
    default:
        return "UNKNOWN";
    }
}

std::string CNoirnode::GetStateString() const {
    return StateToString(nActiveState);
}

std::string CNoirnode::GetStatus() const {
    // TODO: return smth a bit more human readable here
    return GetStateString();
}

std::string CNoirnode::ToString() const {
    std::string str;
    str += "noirnode{";
    str += addr.ToString();
    str += " ";
    str += std::to_string(nProtocolVersion);
    str += " ";
    str += vin.prevout.ToStringShort();
    str += " ";
    str += CBitcoinAddress(pubKeyCollateralAddress.GetID()).ToString();
    str += " ";
    str += std::to_string(lastPing == CNoirnodePing() ? sigTime : lastPing.sigTime);
    str += " ";
    str += std::to_string(lastPing == CNoirnodePing() ? 0 : lastPing.sigTime - sigTime);
    str += " ";
    str += std::to_string(nBlockLastPaid);
    str += "}\n";
    return str;
}

int CNoirnode::GetCollateralAge() {
    int nHeight;
    {
        TRY_LOCK(cs_main, lockMain);
        if (!lockMain || !chainActive.Tip()) return -1;
        nHeight = chainActive.Height();
    }

    if (nCacheCollateralBlock == 0) {
        int nInputAge = GetInputAge(vin);
        if (nInputAge > 0) {
            nCacheCollateralBlock = nHeight - nInputAge;
        } else {
            return nInputAge;
        }
    }

    return nHeight - nCacheCollateralBlock;
}

void CNoirnode::UpdateLastPaid(const CBlockIndex *pindex, int nMaxBlocksToScanBack) {
    if (!pindex) {
        LogPrintf("CNoirnode::UpdateLastPaid pindex is NULL\n");
        return;
    }

    const CBlockIndex *BlockReading = pindex;

    CScript mnpayee = GetScriptForDestination(pubKeyCollateralAddress.GetID());
    LogPrint("noirnode", "CNoirnode::UpdateLastPaidBlock -- searching for block with payment to %s\n", vin.prevout.ToStringShort());

    LOCK(cs_mapNoirnodeBlocks);

    for (int i = 0; BlockReading && BlockReading->nHeight > nBlockLastPaid && i < nMaxBlocksToScanBack; i++) {
        //        LogPrintf("mnpayments.mapNoirnodeBlocks.count(BlockReading->nHeight)=%s\n", mnpayments.mapNoirnodeBlocks.count(BlockReading->nHeight));
        //        LogPrintf("mnpayments.mapNoirnodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(mnpayee, 2)=%s\n", mnpayments.mapNoirnodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(mnpayee, 2));
        if (mnpayments.mapNoirnodeBlocks.count(BlockReading->nHeight) &&
                mnpayments.mapNoirnodeBlocks[BlockReading->nHeight].HasPayeeWithVotes(mnpayee, 2)) {
            // LogPrintf("i=%s, BlockReading->nHeight=%s\n", i, BlockReading->nHeight);
            CBlock block;
            if (!ReadBlockFromDisk(block, BlockReading, Params().GetConsensus())) // shouldn't really happen
            {
                LogPrintf("ReadBlockFromDisk failed\n");
                continue;
            }

            CAmount nNoirnodePayment = GetNoirnodePayment(BlockReading->nHeight, block.vtx[0].GetValueOut());

            BOOST_FOREACH(CTxOut txout, block.vtx[0].vout)
            if (mnpayee == txout.scriptPubKey && nNoirnodePayment == txout.nValue) {
                nBlockLastPaid = BlockReading->nHeight;
                nTimeLastPaid = BlockReading->nTime;
                LogPrint("noirnode", "CNoirnode::UpdateLastPaidBlock -- searching for block with payment to %s -- found new %d\n", vin.prevout.ToStringShort(), nBlockLastPaid);
                return;
            }
        }

        if (BlockReading->pprev == NULL) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    // Last payment for this noirnode wasn't found in latest mnpayments blocks
    // or it was found in mnpayments blocks but wasn't found in the blockchain.
    // LogPrint("noirnode", "CNoirnode::UpdateLastPaidBlock -- searching for block with payment to %s -- keeping old %d\n", vin.prevout.ToStringShort(), nBlockLastPaid);
}

bool CNoirnodeBroadcast::Create(std::string strService, std::string strKeyNoirnode, std::string strTxHash, std::string strOutputIndex, std::string &strErrorRet, CNoirnodeBroadcast &mnbRet, bool fOffline) {
    LogPrintf("CNoirnodeBroadcast::Create\n");
    CTxIn txin;
    CPubKey pubKeyCollateralAddressNew;
    CKey keyCollateralAddressNew;
    CPubKey pubKeyNoirnodeNew;
    CKey keyNoirnodeNew;
    //need correct blocks to send ping
    if (!fOffline && !noirnodeSync.IsBlockchainSynced()) {
        strErrorRet = "Sync in progress. Must wait until sync is complete to start Noirnode";
        LogPrintf("CNoirnodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    //TODO
    if (!darkSendSigner.GetKeysFromSecret(strKeyNoirnode, keyNoirnodeNew, pubKeyNoirnodeNew)) {
        strErrorRet = strprintf("Invalid noirnode key %s", strKeyNoirnode);
        LogPrintf("CNoirnodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    if (!pwalletMain->GetNoirnodeVinAndKeys(txin, pubKeyCollateralAddressNew, keyCollateralAddressNew, strTxHash, strOutputIndex)) {
        strErrorRet = strprintf("Could not allocate txin %s:%s for noirnode %s", strTxHash, strOutputIndex, strService);
        LogPrintf("CNoirnodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    CService service = CService(strService);
    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (service.GetPort() != mainnetDefaultPort) {
            strErrorRet = strprintf("Invalid port %u for noirnode %s, only %d is supported on mainnet.", service.GetPort(), strService, mainnetDefaultPort);
            LogPrintf("CNoirnodeBroadcast::Create -- %s\n", strErrorRet);
            return false;
        }
    } else if (service.GetPort() == mainnetDefaultPort) {
        strErrorRet = strprintf("Invalid port %u for noirnode %s, %d is the only supported on mainnet.", service.GetPort(), strService, mainnetDefaultPort);
        LogPrintf("CNoirnodeBroadcast::Create -- %s\n", strErrorRet);
        return false;
    }

    return Create(txin, CService(strService), keyCollateralAddressNew, pubKeyCollateralAddressNew, keyNoirnodeNew, pubKeyNoirnodeNew, strErrorRet, mnbRet);
}

bool CNoirnodeBroadcast::Create(CTxIn txin, CService service, CKey keyCollateralAddressNew, CPubKey pubKeyCollateralAddressNew, CKey keyNoirnodeNew, CPubKey pubKeyNoirnodeNew, std::string &strErrorRet, CNoirnodeBroadcast &mnbRet) {
    // wait for reindex and/or import to finish
    if (fImporting || fReindex) return false;

    LogPrint("noirnode", "CNoirnodeBroadcast::Create -- pubKeyCollateralAddressNew = %s, pubKeyNoirnodeNew.GetID() = %s\n",
             CBitcoinAddress(pubKeyCollateralAddressNew.GetID()).ToString(),
             pubKeyNoirnodeNew.GetID().ToString());


    CNoirnodePing mnp(txin);
    if (!mnp.Sign(keyNoirnodeNew, pubKeyNoirnodeNew)) {
        strErrorRet = strprintf("Failed to sign ping, noirnode=%s", txin.prevout.ToStringShort());
        LogPrintf("CNoirnodeBroadcast::Create -- %s\n", strErrorRet);
        mnbRet = CNoirnodeBroadcast();
        return false;
    }

    mnbRet = CNoirnodeBroadcast(service, txin, pubKeyCollateralAddressNew, pubKeyNoirnodeNew, PROTOCOL_VERSION);

    if (!mnbRet.IsValidNetAddr()) {
        strErrorRet = strprintf("Invalid IP address, noirnode=%s", txin.prevout.ToStringShort());
        LogPrintf("CNoirnodeBroadcast::Create -- %s\n", strErrorRet);
        mnbRet = CNoirnodeBroadcast();
        return false;
    }

    mnbRet.lastPing = mnp;
    if (!mnbRet.Sign(keyCollateralAddressNew)) {
        strErrorRet = strprintf("Failed to sign broadcast, noirnode=%s", txin.prevout.ToStringShort());
        LogPrintf("CNoirnodeBroadcast::Create -- %s\n", strErrorRet);
        mnbRet = CNoirnodeBroadcast();
        return false;
    }

    return true;
}

bool CNoirnodeBroadcast::SimpleCheck(int &nDos) {
    nDos = 0;

    // make sure addr is valid
    if (!IsValidNetAddr()) {
        LogPrintf("CNoirnodeBroadcast::SimpleCheck -- Invalid addr, rejected: noirnode=%s  addr=%s\n",
                  vin.prevout.ToStringShort(), addr.ToString());
        return false;
    }

    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("CNoirnodeBroadcast::SimpleCheck -- Signature rejected, too far into the future: noirnode=%s\n", vin.prevout.ToStringShort());
        nDos = 1;
        return false;
    }

    // empty ping or incorrect sigTime/unknown blockhash
    if (lastPing == CNoirnodePing() || !lastPing.SimpleCheck(nDos)) {
        // one of us is probably forked or smth, just mark it as expired and check the rest of the rules
        nActiveState = NOIRNODE_EXPIRED;
    }

    if (nProtocolVersion < mnpayments.GetMinNoirnodePaymentsProto()) {
        LogPrintf("CNoirnodeBroadcast::SimpleCheck -- ignoring outdated Noirnode: noirnode=%s  nProtocolVersion=%d\n", vin.prevout.ToStringShort(), nProtocolVersion);
        return false;
    }

    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(pubKeyCollateralAddress.GetID());

    if (pubkeyScript.size() != 25) {
        LogPrintf("CNoirnodeBroadcast::SimpleCheck -- pubKeyCollateralAddress has the wrong size\n");
        nDos = 100;
        return false;
    }

    CScript pubkeyScript2;
    pubkeyScript2 = GetScriptForDestination(pubKeyNoirnode.GetID());

    if (pubkeyScript2.size() != 25) {
        LogPrintf("CNoirnodeBroadcast::SimpleCheck -- pubKeyNoirnode has the wrong size\n");
        nDos = 100;
        return false;
    }

    if (!vin.scriptSig.empty()) {
        LogPrintf("CNoirnodeBroadcast::SimpleCheck -- Ignore Not Empty ScriptSig %s\n", vin.ToString());
        nDos = 100;
        return false;
    }

    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (addr.GetPort() != mainnetDefaultPort) return false;
    } else if (addr.GetPort() == mainnetDefaultPort) return false;

    return true;
}

bool CNoirnodeBroadcast::Update(CNoirnode *pmn, int &nDos) {
    nDos = 0;

    if (pmn->sigTime == sigTime && !fRecovery) {
        // mapSeenNoirnodeBroadcast in CNoirnodeMan::CheckMnbAndUpdateNoirnodeList should filter legit duplicates
        // but this still can happen if we just started, which is ok, just do nothing here.
        return false;
    }

    // this broadcast is older than the one that we already have - it's bad and should never happen
    // unless someone is doing something fishy
    if (pmn->sigTime > sigTime) {
        LogPrintf("CNoirnodeBroadcast::Update -- Bad sigTime %d (existing broadcast is at %d) for Noirnode %s %s\n",
                  sigTime, pmn->sigTime, vin.prevout.ToStringShort(), addr.ToString());
        return false;
    }

    pmn->Check();

    // noirnode is banned by PoSe
    if (pmn->IsPoSeBanned()) {
        LogPrintf("CNoirnodeBroadcast::Update -- Banned by PoSe, noirnode=%s\n", vin.prevout.ToStringShort());
        return false;
    }

    // IsVnAssociatedWithPubkey is validated once in CheckOutpoint, after that they just need to match
    if (pmn->pubKeyCollateralAddress != pubKeyCollateralAddress) {
        LogPrintf("CNoirnodeBroadcast::Update -- Got mismatched pubKeyCollateralAddress and vin\n");
        nDos = 33;
        return false;
    }

    if (!CheckSignature(nDos)) {
        LogPrintf("CNoirnodeBroadcast::Update -- CheckSignature() failed, noirnode=%s\n", vin.prevout.ToStringShort());
        return false;
    }

    // if ther was no noirnode broadcast recently or if it matches our Noirnode privkey...
    if (!pmn->IsBroadcastedWithin(NOIRNODE_MIN_MNB_SECONDS) || (fNoirNode && pubKeyNoirnode == activeNoirnode.pubKeyNoirnode)) {
        // take the newest entry
        LogPrintf("CNoirnodeBroadcast::Update -- Got UPDATED Noirnode entry: addr=%s\n", addr.ToString());
        if (pmn->UpdateFromNewBroadcast((*this))) {
            pmn->Check();
            RelayNoirNode();
        }
        noirnodeSync.AddedNoirnodeList();
    }

    return true;
}

bool CNoirnodeBroadcast::CheckOutpoint(int &nDos) {
    // we are a noirnode with the same vin (i.e. already activated) and this mnb is ours (matches our Noirnode privkey)
    // so nothing to do here for us
    if (fNoirNode && vin.prevout == activeNoirnode.vin.prevout && pubKeyNoirnode == activeNoirnode.pubKeyNoirnode) {
        return false;
    }

    if (!CheckSignature(nDos)) {
        LogPrintf("CNoirnodeBroadcast::CheckOutpoint -- CheckSignature() failed, noirnode=%s\n", vin.prevout.ToStringShort());
        return false;
    }

    {
        TRY_LOCK(cs_main, lockMain);
        if (!lockMain) {
            // not mnb fault, let it to be checked again later
            LogPrint("noirnode", "CNoirnodeBroadcast::CheckOutpoint -- Failed to aquire lock, addr=%s", addr.ToString());
            mnodeman.mapSeenNoirnodeBroadcast.erase(GetHash());
            return false;
        }

        CCoins coins;
        if (!pcoinsTip->GetCoins(vin.prevout.hash, coins) ||
                (unsigned int) vin.prevout.n >= coins.vout.size() ||
                coins.vout[vin.prevout.n].IsNull()) {
            LogPrint("noirnode", "CNoirnodeBroadcast::CheckOutpoint -- Failed to find Noirnode UTXO, noirnode=%s\n", vin.prevout.ToStringShort());
            return false;
        }
        if (coins.vout[vin.prevout.n].nValue != NOIRNODE_COIN_REQUIRED * COIN) {
            LogPrint("noirnode", "CNoirnodeBroadcast::CheckOutpoint -- Noirnode UTXO should have 25000 NOI, noirnode=%s\n", vin.prevout.ToStringShort());
            return false;
        }
        if (chainActive.Height() - coins.nHeight + 1 < Params().GetConsensus().nNoirnodeMinimumConfirmations) {
            LogPrintf("CNoirnodeBroadcast::CheckOutpoint -- Noirnode UTXO must have at least %d confirmations, noirnode=%s\n",
                      Params().GetConsensus().nNoirnodeMinimumConfirmations, vin.prevout.ToStringShort());
            // maybe we miss few blocks, let this mnb to be checked again later
            mnodeman.mapSeenNoirnodeBroadcast.erase(GetHash());
            return false;
        }
    }

    LogPrint("noirnode", "CNoirnodeBroadcast::CheckOutpoint -- Noirnode UTXO verified\n");

    // make sure the vout that was signed is related to the transaction that spawned the Noirnode
    //  - this is expensive, so it's only done once per Noirnode
    if (!darkSendSigner.IsVinAssociatedWithPubkey(vin, pubKeyCollateralAddress)) {
        LogPrintf("CNoirnodeMan::CheckOutpoint -- Got mismatched pubKeyCollateralAddress and vin\n");
        nDos = 33;
        return false;
    }

    // verify that sig time is legit in past
    // should be at least not earlier than block when 25000 NOI tx got nNoirnodeMinimumConfirmations
    uint256 hashBlock = uint256();
    CTransaction tx2;
    GetTransaction(vin.prevout.hash, tx2, Params().GetConsensus(), hashBlock, true);
    {
        LOCK(cs_main);
        BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end() && (*mi).second) {
            CBlockIndex *pMNIndex = (*mi).second; // block for 25000 NOI tx -> 1 confirmation
            CBlockIndex *pConfIndex = chainActive[pMNIndex->nHeight + Params().GetConsensus().nNoirnodeMinimumConfirmations - 1]; // block where tx got nNoirnodeMinimumConfirmations
            if (pConfIndex->GetBlockTime() > sigTime) {
                LogPrintf("CNoirnodeBroadcast::CheckOutpoint -- Bad sigTime %d (%d conf block is at %d) for Noirnode %s %s\n",
                          sigTime, Params().GetConsensus().nNoirnodeMinimumConfirmations, pConfIndex->GetBlockTime(), vin.prevout.ToStringShort(), addr.ToString());
                return false;
            }
        }
    }

    return true;
}

bool CNoirnodeBroadcast::Sign(CKey &keyCollateralAddress) {
    std::string strError;
    std::string strMessage;

    sigTime = GetAdjustedTime();

    strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                 pubKeyCollateralAddress.GetID().ToString() + pubKeyNoirnode.GetID().ToString() +
                 boost::lexical_cast<std::string>(nProtocolVersion);

    if (!darkSendSigner.SignMessage(strMessage, vchSig, keyCollateralAddress)) {
        LogPrintf("CNoirnodeBroadcast::Sign -- SignMessage() failed\n");
        return false;
    }

    if (!darkSendSigner.VerifyMessage(pubKeyCollateralAddress, vchSig, strMessage, strError)) {
        LogPrintf("CNoirnodeBroadcast::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CNoirnodeBroadcast::CheckSignature(int &nDos) {
    std::string strMessage;
    std::string strError = "";
    nDos = 0;

    strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) +
                 pubKeyCollateralAddress.GetID().ToString() + pubKeyNoirnode.GetID().ToString() +
                 boost::lexical_cast<std::string>(nProtocolVersion);

    LogPrint("noirnode", "CNoirnodeBroadcast::CheckSignature -- strMessage: %s  pubKeyCollateralAddress address: %s  sig: %s\n", strMessage, CBitcoinAddress(pubKeyCollateralAddress.GetID()).ToString(), EncodeBase64(&vchSig[0], vchSig.size()));

    if (!darkSendSigner.VerifyMessage(pubKeyCollateralAddress, vchSig, strMessage, strError)) {
        LogPrintf("CNoirnodeBroadcast::CheckSignature -- Got bad Noirnode announce signature, error: %s\n", strError);
        nDos = 100;
        return false;
    }

    return true;
}

void CNoirnodeBroadcast::RelayNoirNode() {
    LogPrintf("CNoirnodeBroadcast::RelayNoirNode\n");
    CInv inv(MSG_NOIRNODE_ANNOUNCE, GetHash());
    RelayInv(inv);
}

CNoirnodePing::CNoirnodePing(CTxIn &vinNew) {
    LOCK(cs_main);
    if (!chainActive.Tip() || chainActive.Height() < 12) return;

    vin = vinNew;
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
    sigTime = GetAdjustedTime();
    vchSig = std::vector < unsigned char > ();
}

bool CNoirnodePing::Sign(CKey &keyNoirnode, CPubKey &pubKeyNoirnode) {
    std::string strError;
    std::string strNoirNodeSignMessage;

    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if (!darkSendSigner.SignMessage(strMessage, vchSig, keyNoirnode)) {
        LogPrintf("CNoirnodePing::Sign -- SignMessage() failed\n");
        return false;
    }

    if (!darkSendSigner.VerifyMessage(pubKeyNoirnode, vchSig, strMessage, strError)) {
        LogPrintf("CNoirnodePing::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool CNoirnodePing::CheckSignature(CPubKey &pubKeyNoirnode, int &nDos) {
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";
    nDos = 0;

    if (!darkSendSigner.VerifyMessage(pubKeyNoirnode, vchSig, strMessage, strError)) {
        LogPrintf("CNoirnodePing::CheckSignature -- Got bad Noirnode ping signature, noirnode=%s, error: %s\n", vin.prevout.ToStringShort(), strError);
        nDos = 33;
        return false;
    }
    return true;
}

bool CNoirnodePing::SimpleCheck(int &nDos) {
    // don't ban by default
    nDos = 0;

    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("CNoirnodePing::SimpleCheck -- Signature rejected, too far into the future, noirnode=%s\n", vin.prevout.ToStringShort());
        nDos = 1;
        return false;
    }

    {
    //        LOCK(cs_main);
        AssertLockHeld(cs_main);
        BlockMap::iterator mi = mapBlockIndex.find(blockHash);
        if (mi == mapBlockIndex.end()) {
            LogPrint("noirnode", "CNoirnodePing::SimpleCheck -- Noirnode ping is invalid, unknown block hash: noirnode=%s blockHash=%s\n", vin.prevout.ToStringShort(), blockHash.ToString());
            // maybe we stuck or forked so we shouldn't ban this node, just fail to accept this ping
            // TODO: or should we also request this block?
            return false;
        }
    }
    LogPrint("noirnode", "CNoirnodePing::SimpleCheck -- Noirnode ping verified: noirnode=%s  blockHash=%s  sigTime=%d\n", vin.prevout.ToStringShort(), blockHash.ToString(), sigTime);
    return true;
}

bool CNoirnodePing::CheckAndUpdate(CNoirnode *pmn, bool fFromNewBroadcast, int &nDos) {
    // don't ban by default
    nDos = 0;

    if (!SimpleCheck(nDos)) {
        return false;
    }

    if (pmn == NULL) {
        LogPrint("noirnode", "CNoirnodePing::CheckAndUpdate -- Couldn't find Noirnode entry, noirnode=%s\n", vin.prevout.ToStringShort());
        return false;
    }

    if (!fFromNewBroadcast) {
        if (pmn->IsUpdateRequired()) {
            LogPrint("noirnode", "CNoirnodePing::CheckAndUpdate -- noirnode protocol is outdated, noirnode=%s\n", vin.prevout.ToStringShort());
            return false;
        }

        if (pmn->IsNewStartRequired()) {
            LogPrint("noirnode", "CNoirnodePing::CheckAndUpdate -- noirnode is completely expired, new start is required, noirnode=%s\n", vin.prevout.ToStringShort());
            return false;
        }
    }

    {
        LOCK(cs_main);
        BlockMap::iterator mi = mapBlockIndex.find(blockHash);
        if ((*mi).second && (*mi).second->nHeight < chainActive.Height() - 24) {
            LogPrintf("CNoirnodePing::CheckAndUpdate -- Noirnode ping is invalid, block hash is too old: noirnode=%s  blockHash=%s\n", vin.prevout.ToStringShort(), blockHash.ToString());
            // nDos = 1;
            return false;
        }
    }

    LogPrint("noirnode", "CNoirnodePing::CheckAndUpdate -- New ping: noirnode=%s  blockHash=%s  sigTime=%d\n", vin.prevout.ToStringShort(), blockHash.ToString(), sigTime);

    // LogPrintf("mnping - Found corresponding mn for vin: %s\n", vin.prevout.ToStringShort());
    // update only if there is no known ping for this noirnode or
    // last ping was more then NOIRNODE_MIN_MNP_SECONDS-60 ago comparing to this one
    if (pmn->IsPingedWithin(NOIRNODE_MIN_MNP_SECONDS - 60, sigTime)) {
        LogPrint("noirnode", "CNoirnodePing::CheckAndUpdate -- Noirnode ping arrived too early, noirnode=%s\n", vin.prevout.ToStringShort());
        //nDos = 1; //disable, this is happening frequently and causing banned peers
        return false;
    }

    if (!CheckSignature(pmn->pubKeyNoirnode, nDos)) return false;

    // so, ping seems to be ok

    // if we are still syncing and there was no known ping for this mn for quite a while
    // (NOTE: assuming that NOIRNODE_EXPIRATION_SECONDS/2 should be enough to finish mn list sync)
    if (!noirnodeSync.IsNoirnodeListSynced() && !pmn->IsPingedWithin(NOIRNODE_EXPIRATION_SECONDS / 2)) {
        // let's bump sync timeout
        LogPrint("noirnode", "CNoirnodePing::CheckAndUpdate -- bumping sync timeout, noirnode=%s\n", vin.prevout.ToStringShort());
        noirnodeSync.AddedNoirnodeList();
    }

    // let's store this ping as the last one
    LogPrint("noirnode", "CNoirnodePing::CheckAndUpdate -- Noirnode ping accepted, noirnode=%s\n", vin.prevout.ToStringShort());
    pmn->lastPing = *this;

    // and update noirnodeman.mapSeenNoirnodeBroadcast.lastPing which is probably outdated
    CNoirnodeBroadcast mnb(*pmn);
    uint256 hash = mnb.GetHash();
    if (mnodeman.mapSeenNoirnodeBroadcast.count(hash)) {
        mnodeman.mapSeenNoirnodeBroadcast[hash].second.lastPing = *this;
    }

    pmn->Check(true); // force update, ignoring cache
    if (!pmn->IsEnabled()) return false;

    LogPrint("noirnode", "CNoirnodePing::CheckAndUpdate -- Noirnode ping acceepted and relayed, noirnode=%s\n", vin.prevout.ToStringShort());
    Relay();

    return true;
}

void CNoirnodePing::Relay() {
    CInv inv(MSG_NOIRNODE_PING, GetHash());
    RelayInv(inv);
}

//void CNoirnode::AddGovernanceVote(uint256 nGovernanceObjectHash)
//{
//    if(mapGovernanceObjectsVotedOn.count(nGovernanceObjectHash)) {
//        mapGovernanceObjectsVotedOn[nGovernanceObjectHash]++;
//    } else {
//        mapGovernanceObjectsVotedOn.insert(std::make_pair(nGovernanceObjectHash, 1));
//    }
//}

//void CNoirnode::RemoveGovernanceObject(uint256 nGovernanceObjectHash)
//{
//    std::map<uint256, int>::iterator it = mapGovernanceObjectsVotedOn.find(nGovernanceObjectHash);
//    if(it == mapGovernanceObjectsVotedOn.end()) {
//        return;
//    }
//    mapGovernanceObjectsVotedOn.erase(it);
//}

void CNoirnode::UpdateWatchdogVoteTime() {
    LOCK(cs);
    nTimeLastWatchdogVote = GetTime();
}

/**
*   FLAG GOVERNANCE ITEMS AS DIRTY
*
*   - When noirnode come and go on the network, we must flag the items they voted on to recalc it's cached flags
*
*/
//void CNoirnode::FlagGovernanceItemsAsDirty()
//{
//    std::vector<uint256> vecDirty;
//    {
//        std::map<uint256, int>::iterator it = mapGovernanceObjectsVotedOn.begin();
//        while(it != mapGovernanceObjectsVotedOn.end()) {
//            vecDirty.push_back(it->first);
//            ++it;
//        }
//    }
//    for(size_t i = 0; i < vecDirty.size(); ++i) {
//        noirnodeman.AddDirtyGovernanceObjectHash(vecDirty[i]);
//    }
//}
