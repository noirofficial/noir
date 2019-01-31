// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activenoirnode.h"
#include "noirnode.h"
#include "noirnode-sync.h"
#include "noirnodeman.h"
#include "protocol.h"

extern CWallet *pwalletMain;

// Keep track of the active Noirnode
CActiveNoirnode activeNoirnode;

void CActiveNoirnode::ManageState() {
    LogPrint("noirnode", "CActiveNoirnode::ManageState -- Start\n");
    if (!fNoirNode) {
        LogPrint("noirnode", "CActiveNoirnode::ManageState -- Not a noirnode, returning\n");
        return;
    }

    if (Params().NetworkIDString() != CBaseChainParams::REGTEST && !noirnodeSync.IsBlockchainSynced()) {
        nState = ACTIVE_NOIRNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveNoirnode::ManageState -- %s: %s\n", GetStateString(), GetStatus());
        return;
    }

    if (nState == ACTIVE_NOIRNODE_SYNC_IN_PROCESS) {
        nState = ACTIVE_NOIRNODE_INITIAL;
    }

    LogPrint("noirnode", "CActiveNoirnode::ManageState -- status = %s, type = %s, pinger enabled = %d\n",
             GetStatus(), GetTypeString(), fPingerEnabled);

    if (eType == NOIRNODE_UNKNOWN) {
        ManageStateInitial();
    }

    if (eType == NOIRNODE_REMOTE) {
        ManageStateRemote();
    } else if (eType == NOIRNODE_LOCAL) {
        // Try Remote Start first so the started local noirnode can be restarted without recreate noirnode broadcast.
        ManageStateRemote();
        if (nState != ACTIVE_NOIRNODE_STARTED)
            ManageStateLocal();
    }

    SendNoirnodePing();
}

std::string CActiveNoirnode::GetStateString() const {
    switch (nState) {
        case ACTIVE_NOIRNODE_INITIAL:
            return "INITIAL";
        case ACTIVE_NOIRNODE_SYNC_IN_PROCESS:
            return "SYNC_IN_PROCESS";
        case ACTIVE_NOIRNODE_INPUT_TOO_NEW:
            return "INPUT_TOO_NEW";
        case ACTIVE_NOIRNODE_NOT_CAPABLE:
            return "NOT_CAPABLE";
        case ACTIVE_NOIRNODE_STARTED:
            return "STARTED";
        default:
            return "UNKNOWN";
    }
}

std::string CActiveNoirnode::GetStatus() const {
    switch (nState) {
        case ACTIVE_NOIRNODE_INITIAL:
            return "Node just started, not yet activated";
        case ACTIVE_NOIRNODE_SYNC_IN_PROCESS:
            return "Sync in progress. Must wait until sync is complete to start Noirnode";
        case ACTIVE_NOIRNODE_INPUT_TOO_NEW:
            return strprintf("Noirnode input must have at least %d confirmations",
                             Params().GetConsensus().nNoirnodeMinimumConfirmations);
        case ACTIVE_NOIRNODE_NOT_CAPABLE:
            return "Not capable noirnode: " + strNotCapableReason;
        case ACTIVE_NOIRNODE_STARTED:
            return "Noirnode successfully started";
        default:
            return "Unknown";
    }
}

std::string CActiveNoirnode::GetTypeString() const {
    std::string strType;
    switch (eType) {
        case NOIRNODE_UNKNOWN:
            strType = "UNKNOWN";
            break;
        case NOIRNODE_REMOTE:
            strType = "REMOTE";
            break;
        case NOIRNODE_LOCAL:
            strType = "LOCAL";
            break;
        default:
            strType = "UNKNOWN";
            break;
    }
    return strType;
}

bool CActiveNoirnode::SendNoirnodePing() {
    if (!fPingerEnabled) {
        LogPrint("noirnode",
                 "CActiveNoirnode::SendNoirnodePing -- %s: noirnode ping service is disabled, skipping...\n",
                 GetStateString());
        return false;
    }

    if (!mnodeman.Has(vin)) {
        strNotCapableReason = "Noirnode not in noirnode list";
        nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
        LogPrintf("CActiveNoirnode::SendNoirnodePing -- %s: %s\n", GetStateString(), strNotCapableReason);
        return false;
    }

    CNoirnodePing mnp(vin);
    if (!mnp.Sign(keyNoirnode, pubKeyNoirnode)) {
        LogPrintf("CActiveNoirnode::SendNoirnodePing -- ERROR: Couldn't sign Noirnode Ping\n");
        return false;
    }

    // Update lastPing for our noirnode in Noirnode list
    if (mnodeman.IsNoirnodePingedWithin(vin, NOIRNODE_MIN_MNP_SECONDS, mnp.sigTime)) {
        LogPrintf("CActiveNoirnode::SendNoirnodePing -- Too early to send Noirnode Ping\n");
        return false;
    }

    mnodeman.SetNoirnodeLastPing(vin, mnp);

    LogPrintf("CActiveNoirnode::SendNoirnodePing -- Relaying ping, collateral=%s\n", vin.ToString());
    mnp.Relay();

    return true;
}

void CActiveNoirnode::ManageStateInitial() {
    LogPrint("noirnode", "CActiveNoirnode::ManageStateInitial -- status = %s, type = %s, pinger enabled = %d\n",
             GetStatus(), GetTypeString(), fPingerEnabled);

    // Check that our local network configuration is correct
    if (!fListen) {
        // listen option is probably overwritten by smth else, no good
        nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
        strNotCapableReason = "Noirnode must accept connections from outside. Make sure listen configuration option is not overwritten by some another parameter.";
        LogPrintf("CActiveNoirnode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }

    bool fFoundLocal = false;
    {
        LOCK(cs_vNodes);

        // First try to find whatever local address is specified by externalip option
        fFoundLocal = GetLocal(service) && CNoirnode::IsValidNetAddr(service);
        if (!fFoundLocal) {
            // nothing and no live connections, can't do anything for now
            if (vNodes.empty()) {
                nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
                strNotCapableReason = "Can't detect valid external address. Will retry when there are some connections available.";
                LogPrintf("CActiveNoirnode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
                return;
            }
            // We have some peers, let's try to find our local address from one of them
            BOOST_FOREACH(CNode * pnode, vNodes)
            {
                if (pnode->fSuccessfullyConnected && pnode->addr.IsIPv4()) {
                    fFoundLocal = GetLocal(service, &pnode->addr) && CNoirnode::IsValidNetAddr(service);
                    if (fFoundLocal) break;
                }
            }
        }
    }

    if (!fFoundLocal) {
        nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
        strNotCapableReason = "Can't detect valid external address. Please consider using the externalip configuration option if problem persists. Make sure to use IPv4 address only.";
        LogPrintf("CActiveNoirnode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }

    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (service.GetPort() != mainnetDefaultPort) {
            nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
            strNotCapableReason = strprintf("Invalid port: %u - only %d is supported on mainnet.", service.GetPort(), mainnetDefaultPort);
            LogPrintf("CActiveNoirnode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
    } else if (service.GetPort() == mainnetDefaultPort) {
        nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
        strNotCapableReason = strprintf("Invalid port: %u - %d is only supported on mainnet.", service.GetPort(), mainnetDefaultPort);
        LogPrintf("CActiveNoirnode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }

    LogPrintf("CActiveNoirnode::ManageStateInitial -- Checking inbound connection to '%s'\n", service.ToString());
    //TODO
    if (!ConnectNode(CAddress(service, NODE_NETWORK), NULL, false, true)) {
        nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
        strNotCapableReason = "Could not connect to " + service.ToString();
        LogPrintf("CActiveNoirnode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }

    // Default to REMOTE
    eType = NOIRNODE_REMOTE;

    // Check if wallet funds are available
    if (!pwalletMain) {
        LogPrintf("CActiveNoirnode::ManageStateInitial -- %s: Wallet not available\n", GetStateString());
        return;
    }

    if (pwalletMain->IsLocked()) {
        LogPrintf("CActiveNoirnode::ManageStateInitial -- %s: Wallet is locked\n", GetStateString());
        return;
    }

    if (pwalletMain->GetBalance() < NOIRNODE_COIN_REQUIRED * COIN) {
        LogPrintf("CActiveNoirnode::ManageStateInitial -- %s: Wallet balance is < 1000 XZC\n", GetStateString());
        return;
    }

    // Choose coins to use
    CPubKey pubKeyCollateral;
    CKey keyCollateral;

    // If collateral is found switch to LOCAL mode

    if (pwalletMain->GetNoirnodeVinAndKeys(vin, pubKeyCollateral, keyCollateral)) {
        eType = NOIRNODE_LOCAL;
    }

    LogPrint("noirnode", "CActiveNoirnode::ManageStateInitial -- End status = %s, type = %s, pinger enabled = %d\n",
             GetStatus(), GetTypeString(), fPingerEnabled);
}

void CActiveNoirnode::ManageStateRemote() {
    LogPrint("noirnode",
             "CActiveNoirnode::ManageStateRemote -- Start status = %s, type = %s, pinger enabled = %d, pubKeyNoirnode.GetID() = %s\n",
             GetStatus(), fPingerEnabled, GetTypeString(), pubKeyNoirnode.GetID().ToString());

    mnodeman.CheckNoirnode(pubKeyNoirnode);
    noirnode_info_t infoMn = mnodeman.GetNoirnodeInfo(pubKeyNoirnode);
    if (infoMn.fInfoValid) {
        if (infoMn.nProtocolVersion != PROTOCOL_VERSION) {
            nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
            strNotCapableReason = "Invalid protocol version";
            LogPrintf("CActiveNoirnode::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
        if (service != infoMn.addr) {
            nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
            // LogPrintf("service: %s\n", service.ToString());
            // LogPrintf("infoMn.addr: %s\n", infoMn.addr.ToString());
            strNotCapableReason = "Broadcasted IP doesn't match our external address. Make sure you issued a new broadcast if IP of this noirnode changed recently.";
            LogPrintf("CActiveNoirnode::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
        if (!CNoirnode::IsValidStateForAutoStart(infoMn.nActiveState)) {
            nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
            strNotCapableReason = strprintf("Noirnode in %s state", CNoirnode::StateToString(infoMn.nActiveState));
            LogPrintf("CActiveNoirnode::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
        if (nState != ACTIVE_NOIRNODE_STARTED) {
            LogPrintf("CActiveNoirnode::ManageStateRemote -- STARTED!\n");
            vin = infoMn.vin;
            service = infoMn.addr;
            fPingerEnabled = true;
            nState = ACTIVE_NOIRNODE_STARTED;
        }
    } else {
        nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
        strNotCapableReason = "Noirnode not in noirnode list";
        LogPrintf("CActiveNoirnode::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
    }
}

void CActiveNoirnode::ManageStateLocal() {
    LogPrint("noirnode", "CActiveNoirnode::ManageStateLocal -- status = %s, type = %s, pinger enabled = %d\n",
             GetStatus(), GetTypeString(), fPingerEnabled);
    if (nState == ACTIVE_NOIRNODE_STARTED) {
        return;
    }

    // Choose coins to use
    CPubKey pubKeyCollateral;
    CKey keyCollateral;

    if (pwalletMain->GetNoirnodeVinAndKeys(vin, pubKeyCollateral, keyCollateral)) {
        int nInputAge = GetInputAge(vin);
        if (nInputAge < Params().GetConsensus().nNoirnodeMinimumConfirmations) {
            nState = ACTIVE_NOIRNODE_INPUT_TOO_NEW;
            strNotCapableReason = strprintf(_("%s - %d confirmations"), GetStatus(), nInputAge);
            LogPrintf("CActiveNoirnode::ManageStateLocal -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }

        {
            LOCK(pwalletMain->cs_wallet);
            pwalletMain->LockCoin(vin.prevout);
        }

        CNoirnodeBroadcast mnb;
        std::string strError;
        if (!CNoirnodeBroadcast::Create(vin, service, keyCollateral, pubKeyCollateral, keyNoirnode,
            pubKeyNoirnode, strError, mnb)) {
            nState = ACTIVE_NOIRNODE_NOT_CAPABLE;
            strNotCapableReason = "Error creating mastenode broadcast: " + strError;
            LogPrintf("CActiveNoirnode::ManageStateLocal -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }

        fPingerEnabled = true;
        nState = ACTIVE_NOIRNODE_STARTED;

        //update to noirnode list
        LogPrintf("CActiveNoirnode::ManageStateLocal -- Update Noirnode List\n");
        mnodeman.UpdateNoirnodeList(mnb);
        mnodeman.NotifyNoirnodeUpdates();

        //send to all peers
        LogPrintf("CActiveNoirnode::ManageStateLocal -- Relay broadcast, vin=%s\n", vin.ToString());
        mnb.RelayNoirNode();
    }
}
