#include "activenoirnode.h"
#include "darksend.h"
#include "init.h"
#include "main.h"
#include "noirnode-payments.h"
#include "noirnode-sync.h"
#include "noirnodeconfig.h"
#include "noirnodeman.h"
#include "rpc/server.h"
#include "util.h"
#include "utilmoneystr.h"
#include "net.h"

#include <fstream>
#include <iomanip>
#include <univalue.h>

void EnsureWalletIsUnlocked();

UniValue privatesend(const UniValue &params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
                "privatesend \"command\"\n"
                        "\nArguments:\n"
                        "1. \"command\"        (string or set of strings, required) The command to execute\n"
                        "\nAvailable commands:\n"
                        "  start       - Start mixing\n"
                        "  stop        - Stop mixing\n"
                        "  reset       - Reset mixing\n"
        );

    if (params[0].get_str() == "start") {
        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        if (fNoirNode)
            return "Mixing is not supported from noirnodes";

        fEnablePrivateSend = true;
        bool result = darkSendPool.DoAutomaticDenominating();
        return "Mixing " +
               (result ? "started successfully" : ("start failed: " + darkSendPool.GetStatus() + ", will retry"));
    }

    if (params[0].get_str() == "stop") {
        fEnablePrivateSend = false;
        return "Mixing was stopped";
    }

    if (params[0].get_str() == "reset") {
        darkSendPool.ResetPool();
        return "Mixing was reset";
    }

    return "Unknown command, please see \"help privatesend\"";
}

UniValue getpoolinfo(const UniValue &params, bool fHelp) {
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                "getpoolinfo\n"
                        "Returns an object containing mixing pool related information.\n");

    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("state", darkSendPool.GetStateString()));
//    obj.push_back(Pair("mixing_mode",       fPrivateSendMultiSession ? "multi-session" : "normal"));
    obj.push_back(Pair("queue", darkSendPool.GetQueueSize()));
    obj.push_back(Pair("entries", darkSendPool.GetEntriesCount()));
    obj.push_back(Pair("status", darkSendPool.GetStatus()));

    if (darkSendPool.pSubmittedToNoirnode) {
        obj.push_back(Pair("outpoint", darkSendPool.pSubmittedToNoirnode->vin.prevout.ToStringShort()));
        obj.push_back(Pair("addr", darkSendPool.pSubmittedToNoirnode->addr.ToString()));
    }

    if (pwalletMain) {
        obj.push_back(Pair("keys_left", pwalletMain->nKeysLeftSinceAutoBackup));
        obj.push_back(Pair("warnings", pwalletMain->nKeysLeftSinceAutoBackup < PRIVATESEND_KEYS_THRESHOLD_WARNING
                                       ? "WARNING: keypool is almost depleted!" : ""));
    }

    return obj;
}


UniValue noirnode(const UniValue &params, bool fHelp) {
    std::string strCommand;
    if (params.size() >= 1) {
        strCommand = params[0].get_str();
    }

    if (strCommand == "start-many")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "DEPRECATED, please use start-all instead");

    if (fHelp ||
        (strCommand != "start" && strCommand != "start-alias" && strCommand != "start-all" &&
         strCommand != "start-missing" &&
         strCommand != "start-disabled" && strCommand != "list" && strCommand != "list-conf" && strCommand != "count" &&
         strCommand != "debug" && strCommand != "current" && strCommand != "winner" && strCommand != "winners" &&
         strCommand != "genkey" &&
         strCommand != "connect" && strCommand != "outputs" && strCommand != "status"))
        throw std::runtime_error(
                "noirnode \"command\"...\n"
                        "Set of commands to execute noirnode related actions\n"
                        "\nArguments:\n"
                        "1. \"command\"        (string or set of strings, required) The command to execute\n"
                        "\nAvailable commands:\n"
                        "  count        - Print number of all known noirnodes (optional: 'ps', 'enabled', 'all', 'qualify')\n"
                        "  current      - Print info on current noirnode winner to be paid the next block (calculated locally)\n"
                        "  debug        - Print noirnode status\n"
                        "  genkey       - Generate new noirnodeprivkey\n"
                        "  outputs      - Print noirnode compatible outputs\n"
                        "  start        - Start local Hot noirnode configured in dash.conf\n"
                        "  start-alias  - Start single remote noirnode by assigned alias configured in noirnode.conf\n"
                        "  start-<mode> - Start remote noirnodes configured in noirnode.conf (<mode>: 'all', 'missing', 'disabled')\n"
                        "  status       - Print noirnode status information\n"
                        "  list         - Print list of all known noirnodes (see noirnodelist for more info)\n"
                        "  list-conf    - Print noirnode.conf in JSON format\n"
                        "  winner       - Print info on next noirnode winner to vote for\n"
                        "  winners      - Print list of noirnode winners\n"
        );

    if (strCommand == "list") {
        UniValue newParams(UniValue::VARR);
        // forward params but skip "list"
        for (unsigned int i = 1; i < params.size(); i++) {
            newParams.push_back(params[i]);
        }
        return noirnodelist(newParams, fHelp);
    }

    if (strCommand == "connect") {
        if (params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Noirnode address required");

        std::string strAddress = params[1].get_str();

        CService addr = CService(strAddress);

        CNode *pnode = ConnectNode(CAddress(addr, NODE_NETWORK), NULL);
        if (!pnode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Couldn't connect to noirnode %s", strAddress));

        return "successfully connected";
    }

    if (strCommand == "count") {
        if (params.size() > 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Too many parameters");

        if (params.size() == 1)
            return mnodeman.size();

        std::string strMode = params[1].get_str();

        if (strMode == "ps")
            return mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION);

        if (strMode == "enabled")
            return mnodeman.CountEnabled();

        int nCount;
        mnodeman.GetNextNoirnodeInQueueForPayment(true, nCount);

        if (strMode == "qualify")
            return nCount;

        if (strMode == "all")
            return strprintf("Total: %d (PS Compatible: %d / Enabled: %d / Qualify: %d)",
                             mnodeman.size(), mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION),
                             mnodeman.CountEnabled(), nCount);
    }

    if (strCommand == "current" || strCommand == "winner") {
        int nCount;
        int nHeight;
        CNoirnode *winner = NULL;
        {
            LOCK(cs_main);
            nHeight = chainActive.Height() + (strCommand == "current" ? 1 : 10);
        }
        mnodeman.UpdateLastPaid();
        winner = mnodeman.GetNextNoirnodeInQueueForPayment(nHeight, true, nCount);
        if (!winner) return "unknown";

        UniValue obj(UniValue::VOBJ);

        obj.push_back(Pair("height", nHeight));
        obj.push_back(Pair("IP:port", winner->addr.ToString()));
        obj.push_back(Pair("protocol", (int64_t) winner->nProtocolVersion));
        obj.push_back(Pair("vin", winner->vin.prevout.ToStringShort()));
        obj.push_back(Pair("payee", CBitcoinAddress(winner->pubKeyCollateralAddress.GetID()).ToString()));
        obj.push_back(Pair("lastseen", (winner->lastPing == CNoirnodePing()) ? winner->sigTime :
                                       winner->lastPing.sigTime));
        obj.push_back(Pair("activeseconds", (winner->lastPing == CNoirnodePing()) ? 0 :
                                            (winner->lastPing.sigTime - winner->sigTime)));
        obj.push_back(Pair("nBlockLastPaid", winner->nBlockLastPaid));
        return obj;
    }

    if (strCommand == "debug") {
        if (activeNoirnode.nState != ACTIVE_NOIRNODE_INITIAL || !noirnodeSync.IsBlockchainSynced())
            return activeNoirnode.GetStatus();

        CTxIn vin;
        CPubKey pubkey;
        CKey key;

        if (!pwalletMain || !pwalletMain->GetNoirnodeVinAndKeys(vin, pubkey, key))
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               "Missing noirnode input, please look at the documentation for instructions on noirnode creation");

        return activeNoirnode.GetStatus();
    }

    if (strCommand == "start") {
        if (!fNoirNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "You must set noirnode=1 in the configuration");

        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        if (activeNoirnode.nState != ACTIVE_NOIRNODE_STARTED) {
            activeNoirnode.nState = ACTIVE_NOIRNODE_INITIAL; // TODO: consider better way
            activeNoirnode.ManageState();
        }

        return activeNoirnode.GetStatus();
    }

    if (strCommand == "start-alias") {
        if (params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Please specify an alias");

        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        std::string strAlias = params[1].get_str();

        bool fFound = false;

        UniValue statusObj(UniValue::VOBJ);
        statusObj.push_back(Pair("alias", strAlias));

        BOOST_FOREACH(CNoirnodeConfig::CNoirnodeEntry mne, noirnodeConfig.getEntries()) {
            if (mne.getAlias() == strAlias) {
                fFound = true;
                std::string strError;
                CNoirnodeBroadcast mnb;

                bool fResult = CNoirnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(),
                                                            mne.getOutputIndex(), strError, mnb);
                statusObj.push_back(Pair("result", fResult ? "successful" : "failed"));
                if (fResult) {
                    mnodeman.UpdateNoirnodeList(mnb);
                    mnb.RelayNoirNode();
                } else {
                    LogPrintf("Start-alias: errorMessage = %s\n", strError);
                    statusObj.push_back(Pair("errorMessage", strError));
                }
                mnodeman.NotifyNoirnodeUpdates();
                break;
            }
        }

        if (!fFound) {
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Could not find alias in config. Verify with list-conf."));
        }

//        LogPrintf("start-alias: statusObj=%s\n", statusObj);

        return statusObj;

    }

    if (strCommand == "start-all" || strCommand == "start-missing" || strCommand == "start-disabled") {
        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        if ((strCommand == "start-missing" || strCommand == "start-disabled") &&
            !noirnodeSync.IsNoirnodeListSynced()) {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                               "You can't use this command until noirnode list is synced");
        }

        int nSuccessful = 0;
        int nFailed = 0;

        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH(CNoirnodeConfig::CNoirnodeEntry mne, noirnodeConfig.getEntries()) {
            std::string strError;

            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CNoirnode *pmn = mnodeman.Find(vin);
            CNoirnodeBroadcast mnb;

            if (strCommand == "start-missing" && pmn) continue;
            if (strCommand == "start-disabled" && pmn && pmn->IsEnabled()) continue;

            bool fResult = CNoirnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(),
                                                        mne.getOutputIndex(), strError, mnb);

            UniValue statusObj(UniValue::VOBJ);
            statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", fResult ? "successful" : "failed"));

            if (fResult) {
                nSuccessful++;
                mnodeman.UpdateNoirnodeList(mnb);
                mnb.RelayNoirNode();
            } else {
                nFailed++;
                statusObj.push_back(Pair("errorMessage", strError));
            }

            resultsObj.push_back(Pair("status", statusObj));
        }
        mnodeman.NotifyNoirnodeUpdates();

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall",
                                 strprintf("Successfully started %d noirnodes, failed to start %d, total %d",
                                           nSuccessful, nFailed, nSuccessful + nFailed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    if (strCommand == "genkey") {
        CKey secret;
        secret.MakeNewKey(false);

        return CBitcoinSecret(secret).ToString();
    }

    if (strCommand == "list-conf") {
        UniValue resultObj(UniValue::VOBJ);

        BOOST_FOREACH(CNoirnodeConfig::CNoirnodeEntry mne, noirnodeConfig.getEntries()) {
            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CNoirnode *pmn = mnodeman.Find(vin);

            std::string strStatus = pmn ? pmn->GetStatus() : "MISSING";

            UniValue mnObj(UniValue::VOBJ);
            mnObj.push_back(Pair("alias", mne.getAlias()));
            mnObj.push_back(Pair("address", mne.getIp()));
            mnObj.push_back(Pair("privateKey", mne.getPrivKey()));
            mnObj.push_back(Pair("txHash", mne.getTxHash()));
            mnObj.push_back(Pair("outputIndex", mne.getOutputIndex()));
            mnObj.push_back(Pair("status", strStatus));
            resultObj.push_back(Pair("noirnode", mnObj));
        }

        return resultObj;
    }

    if (strCommand == "outputs") {
        // Find possible candidates
        std::vector <COutput> vPossibleCoins;
        pwalletMain->AvailableCoins(vPossibleCoins, true, NULL, false, ONLY_1000);

        UniValue obj(UniValue::VOBJ);
        BOOST_FOREACH(COutput & out, vPossibleCoins)
        {
            obj.push_back(Pair(out.tx->GetHash().ToString(), strprintf("%d", out.i)));
        }

        return obj;

    }

    if (strCommand == "status") {
        if (!fNoirNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a noirnode");

        UniValue mnObj(UniValue::VOBJ);

        mnObj.push_back(Pair("vin", activeNoirnode.vin.ToString()));
        mnObj.push_back(Pair("service", activeNoirnode.service.ToString()));

        CNoirnode mn;
        if (mnodeman.Get(activeNoirnode.vin, mn)) {
            mnObj.push_back(Pair("payee", CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString()));
        }

        mnObj.push_back(Pair("status", activeNoirnode.GetStatus()));
        return mnObj;
    }

    if (strCommand == "winners") {
        int nHeight;
        {
            LOCK(cs_main);
            CBlockIndex *pindex = chainActive.Tip();
            if (!pindex) return NullUniValue;

            nHeight = pindex->nHeight;
        }

        int nLast = 10;
        std::string strFilter = "";

        if (params.size() >= 2) {
            nLast = atoi(params[1].get_str());
        }

        if (params.size() == 3) {
            strFilter = params[2].get_str();
        }

        if (params.size() > 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'noirnode winners ( \"count\" \"filter\" )'");

        UniValue obj(UniValue::VOBJ);

        for (int i = nHeight - nLast; i < nHeight + 20; i++) {
            std::string strPayment = GetRequiredPaymentsString(i);
            if (strFilter != "" && strPayment.find(strFilter) == std::string::npos) continue;
            obj.push_back(Pair(strprintf("%d", i), strPayment));
        }

        return obj;
    }

    return NullUniValue;
}

UniValue noirnodelist(const UniValue &params, bool fHelp) {
    std::string strMode = "status";
    std::string strFilter = "";

    if (params.size() >= 1) strMode = params[0].get_str();
    if (params.size() == 2) strFilter = params[1].get_str();

    if (fHelp || (
            strMode != "activeseconds" && strMode != "addr" && strMode != "full" &&
            strMode != "lastseen" && strMode != "lastpaidtime" && strMode != "lastpaidblock" &&
            strMode != "protocol" && strMode != "payee" && strMode != "rank" && strMode != "qualify" &&
            strMode != "status")) {
        throw std::runtime_error(
                "noirnodelist ( \"mode\" \"filter\" )\n"
                        "Get a list of noirnodes in different modes\n"
                        "\nArguments:\n"
                        "1. \"mode\"      (string, optional/required to use filter, defaults = status) The mode to run list in\n"
                        "2. \"filter\"    (string, optional) Filter results. Partial match by outpoint by default in all modes,\n"
                        "                                    additional matches in some modes are also available\n"
                        "\nAvailable modes:\n"
                        "  activeseconds  - Print number of seconds noirnode recognized by the network as enabled\n"
                        "                   (since latest issued \"noirnode start/start-many/start-alias\")\n"
                        "  addr           - Print ip address associated with a noirnode (can be additionally filtered, partial match)\n"
                        "  full           - Print info in format 'status protocol payee lastseen activeseconds lastpaidtime lastpaidblock IP'\n"
                        "                   (can be additionally filtered, partial match)\n"
                        "  lastpaidblock  - Print the last block height a node was paid on the network\n"
                        "  lastpaidtime   - Print the last time a node was paid on the network\n"
                        "  lastseen       - Print timestamp of when a noirnode was last seen on the network\n"
                        "  payee          - Print Dash address associated with a noirnode (can be additionally filtered,\n"
                        "                   partial match)\n"
                        "  protocol       - Print protocol of a noirnode (can be additionally filtered, exact match))\n"
                        "  rank           - Print rank of a noirnode based on current block\n"
                        "  qualify        - Print qualify status of a noirnode based on current block\n"
                        "  status         - Print noirnode status: PRE_ENABLED / ENABLED / EXPIRED / WATCHDOG_EXPIRED / NEW_START_REQUIRED /\n"
                        "                   UPDATE_REQUIRED / POSE_BAN / OUTPOINT_SPENT (can be additionally filtered, partial match)\n"
        );
    }

    if (strMode == "full" || strMode == "lastpaidtime" || strMode == "lastpaidblock") {
        mnodeman.UpdateLastPaid();
    }

    UniValue obj(UniValue::VOBJ);
    if (strMode == "rank") {
        std::vector <std::pair<int, CNoirnode>> vNoirnodeRanks = mnodeman.GetNoirnodeRanks();
        BOOST_FOREACH(PAIRTYPE(int, CNoirnode) & s, vNoirnodeRanks)
        {
            std::string strOutpoint = s.second.vin.prevout.ToStringShort();
            if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) continue;
            obj.push_back(Pair(strOutpoint, s.first));
        }
    } else {
        std::vector <CNoirnode> vNoirnodes = mnodeman.GetFullNoirnodeVector();
        BOOST_FOREACH(CNoirnode & mn, vNoirnodes)
        {
            std::string strOutpoint = mn.vin.prevout.ToStringShort();
            if (strMode == "activeseconds") {
                if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strOutpoint, (int64_t)(mn.lastPing.sigTime - mn.sigTime)));
            } else if (strMode == "addr") {
                std::string strAddress = mn.addr.ToString();
                if (strFilter != "" && strAddress.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos)
                    continue;
                obj.push_back(Pair(strOutpoint, strAddress));
            } else if (strMode == "full") {
                std::ostringstream streamFull;
                streamFull << std::setw(18) <<
                           mn.GetStatus() << " " <<
                           mn.nProtocolVersion << " " <<
                           CBitcoinAddress(mn.pubKeyCollateralAddress.GetID()).ToString() << " " <<
                           (int64_t) mn.lastPing.sigTime << " " << std::setw(8) <<
                           (int64_t)(mn.lastPing.sigTime - mn.sigTime) << " " << std::setw(10) <<
                           mn.GetLastPaidTime() << " " << std::setw(6) <<
                           mn.GetLastPaidBlock() << " " <<
                           mn.addr.ToString();
                std::string strFull = streamFull.str();
                if (strFilter != "" && strFull.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos)
                    continue;
                obj.push_back(Pair(strOutpoint, strFull));
            } else if (strMode == "lastpaidblock") {
                if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strOutpoint, mn.GetLastPaidBlock()));
            } else if (strMode == "lastpaidtime") {
                if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strOutpoint, mn.GetLastPaidTime()));
            } else if (strMode == "lastseen") {
                if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strOutpoint, (int64_t) mn.lastPing.sigTime));
            } else if (strMode == "payee") {
                CBitcoinAddress address(mn.pubKeyCollateralAddress.GetID());
                std::string strPayee = address.ToString();
                if (strFilter != "" && strPayee.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos)
                    continue;
                obj.push_back(Pair(strOutpoint, strPayee));
            } else if (strMode == "protocol") {
                if (strFilter != "" && strFilter != strprintf("%d", mn.nProtocolVersion) &&
                    strOutpoint.find(strFilter) == std::string::npos)
                    continue;
                obj.push_back(Pair(strOutpoint, (int64_t) mn.nProtocolVersion));
            } else if (strMode == "status") {
                std::string strStatus = mn.GetStatus();
                if (strFilter != "" && strStatus.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos)
                    continue;
                obj.push_back(Pair(strOutpoint, strStatus));
            } else if (strMode == "qualify") {
                int nBlockHeight;
                {
                    LOCK(cs_main);
                    CBlockIndex *pindex = chainActive.Tip();
                    if (!pindex) return NullUniValue;

                    nBlockHeight = pindex->nHeight;
                }
                int nMnCount = mnodeman.CountEnabled();
                char* reasonStr = mnodeman.GetNotQualifyReason(mn, nBlockHeight, true, nMnCount);
                std::string strOutpoint = mn.vin.prevout.ToStringShort();
                if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strOutpoint, (reasonStr != NULL) ? reasonStr : "true"));
            }
        }
    }
    return obj;
}

UniValue listnoirnodes(const UniValue& params, bool fHelp) {
    std::string strFilter = "";

    if (params.size() == 1) strFilter = params[0].get_str();

    if (fHelp || (params.size() > 1))
        throw runtime_error(
            "listnoirnodes ( \"filter\" )\n"
            "\nGet a ranked list of noirnodes\n"

            "\nArguments:\n"
            "1. \"filter\"    (string, optional) Filter search text. Partial match by txhash, status, or addr.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"rank\": n,           (numeric) Noirnode Rank (or 0 if not enabled)\n"
            "    \"txhash\": \"hash\",  (string) Collateral transaction hash\n"
            "    \"outidx\": n,         (numeric) Collateral transaction output index\n"
            "    \"status\": s,         (string) Status (ENABLED/EXPIRED/REMOVE/etc)\n"
            "    \"addr\": \"addr\",    (string) Noirnode address\n"
            "    \"version\": v,        (numeric) Noirnode protocol version\n"
            "    \"lastseen\": ttt,     (numeric) The time in seconds since epoch (Jan 1 1970 GMT) of the last seen\n"
            "    \"activetime\": ttt,   (numeric) The time in seconds since epoch (Jan 1 1970 GMT) noirnode has been active\n"
            "    \"lastpaid\": ttt,     (numeric) The time in seconds since epoch (Jan 1 1970 GMT) noirnode was last paid\n"
            "  }\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n" +
            HelpExampleCli("listnoirnodes", "") + HelpExampleRpc("listnoirnodes", ""));

    UniValue ret(UniValue::VARR);
    int nHeight;
    {
        LOCK(cs_main);
        CBlockIndex* pindex = chainActive.Tip();
        if(!pindex) return 0;
        nHeight = pindex->nHeight;
    }
    std::vector<pair<int, CNoirnode> > vNoirnodeRanks = mnodeman.GetNoirnodeRanks(nHeight);
    BOOST_FOREACH(PAIRTYPE(int, CNoirnode) & s, vNoirnodeRanks) {
        UniValue obj(UniValue::VOBJ);
        std::string strVin = s.second.vin.prevout.ToStringShort();
        std::string strTxHash = s.second.vin.prevout.hash.ToString();
        uint32_t oIdx = s.second.vin.prevout.n;

        CNoirnode* mn = mnodeman.Find(s.second.vin);

        if (mn != NULL) {
            if (strFilter != "" && strTxHash.find(strFilter) == string::npos &&
                    mn->GetStatus().find(strFilter) == string::npos &&
                    CBitcoinAddress(mn->pubKeyCollateralAddress.GetID()).ToString().find(strFilter) == string::npos) continue;

            std::string strStatus = mn->GetStatus();
            std::string strHost;
            int port;
            SplitHostPort(mn->addr.ToString(), port, strHost);
            CNetAddr node = CNetAddr(strHost);
            std::string strNetwork = GetNetworkName(node.GetNetwork());

            obj.push_back(Pair("rank", (strStatus == "ENABLED" ? s.first : 0)));
            obj.push_back(Pair("network", strNetwork));
            obj.push_back(Pair("txhash", strTxHash));
            obj.push_back(Pair("outidx", (uint64_t)oIdx));
            obj.push_back(Pair("status", strStatus));
            obj.push_back(Pair("addr", CBitcoinAddress(mn->pubKeyCollateralAddress.GetID()).ToString()));
            obj.push_back(Pair("version", mn->nProtocolVersion));
            obj.push_back(Pair("lastseen", (int64_t)mn->lastPing.sigTime));
            obj.push_back(Pair("activetime", (int64_t)(mn->lastPing.sigTime - mn->sigTime)));
            obj.push_back(Pair("lastpaid", (int64_t)mn->GetLastPaidTime()));

            ret.push_back(obj);
        }
    }

    return ret;
}

bool DecodeHexVecMnb(std::vector <CNoirnodeBroadcast> &vecMnb, std::string strHexMnb) {

    if (!IsHex(strHexMnb))
        return false;

    std::vector<unsigned char> mnbData(ParseHex(strHexMnb));
    CDataStream ssData(mnbData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ssData >> vecMnb;
    }
    catch (const std::exception &) {
        return false;
    }

    return true;
}

UniValue noirnodebroadcast(const UniValue &params, bool fHelp) {
    std::string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp ||
        (strCommand != "create-alias" && strCommand != "create-all" && strCommand != "decode" && strCommand != "relay"))
        throw std::runtime_error(
                "noirnodebroadcast \"command\"...\n"
                        "Set of commands to create and relay noirnode broadcast messages\n"
                        "\nArguments:\n"
                        "1. \"command\"        (string or set of strings, required) The command to execute\n"
                        "\nAvailable commands:\n"
                        "  create-alias  - Create single remote noirnode broadcast message by assigned alias configured in noirnode.conf\n"
                        "  create-all    - Create remote noirnode broadcast messages for all noirnodes configured in noirnode.conf\n"
                        "  decode        - Decode noirnode broadcast message\n"
                        "  relay         - Relay noirnode broadcast message to the network\n"
        );

    if (strCommand == "create-alias") {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        if (params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Please specify an alias");

        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        bool fFound = false;
        std::string strAlias = params[1].get_str();

        UniValue statusObj(UniValue::VOBJ);
        std::vector <CNoirnodeBroadcast> vecMnb;

        statusObj.push_back(Pair("alias", strAlias));

        BOOST_FOREACH(CNoirnodeConfig::CNoirnodeEntry
        mne, noirnodeConfig.getEntries()) {
            if (mne.getAlias() == strAlias) {
                fFound = true;
                std::string strError;
                CNoirnodeBroadcast mnb;

                bool fResult = CNoirnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(),
                                                            mne.getOutputIndex(), strError, mnb, true);

                statusObj.push_back(Pair("result", fResult ? "successful" : "failed"));
                if (fResult) {
                    vecMnb.push_back(mnb);
                    CDataStream ssVecMnb(SER_NETWORK, PROTOCOL_VERSION);
                    ssVecMnb << vecMnb;
                    statusObj.push_back(Pair("hex", HexStr(ssVecMnb.begin(), ssVecMnb.end())));
                } else {
                    statusObj.push_back(Pair("errorMessage", strError));
                }
                break;
            }
        }

        if (!fFound) {
            statusObj.push_back(Pair("result", "not found"));
            statusObj.push_back(Pair("errorMessage", "Could not find alias in config. Verify with list-conf."));
        }

        return statusObj;

    }

    if (strCommand == "create-all") {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        std::vector <CNoirnodeConfig::CNoirnodeEntry> mnEntries;
        mnEntries = noirnodeConfig.getEntries();

        int nSuccessful = 0;
        int nFailed = 0;

        UniValue resultsObj(UniValue::VOBJ);
        std::vector <CNoirnodeBroadcast> vecMnb;

        BOOST_FOREACH(CNoirnodeConfig::CNoirnodeEntry
        mne, noirnodeConfig.getEntries()) {
            std::string strError;
            CNoirnodeBroadcast mnb;

            bool fResult = CNoirnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(),
                                                        mne.getOutputIndex(), strError, mnb, true);

            UniValue statusObj(UniValue::VOBJ);
            statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", fResult ? "successful" : "failed"));

            if (fResult) {
                nSuccessful++;
                vecMnb.push_back(mnb);
            } else {
                nFailed++;
                statusObj.push_back(Pair("errorMessage", strError));
            }

            resultsObj.push_back(Pair("status", statusObj));
        }

        CDataStream ssVecMnb(SER_NETWORK, PROTOCOL_VERSION);
        ssVecMnb << vecMnb;
        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf(
                "Successfully created broadcast messages for %d noirnodes, failed to create %d, total %d",
                nSuccessful, nFailed, nSuccessful + nFailed)));
        returnObj.push_back(Pair("detail", resultsObj));
        returnObj.push_back(Pair("hex", HexStr(ssVecMnb.begin(), ssVecMnb.end())));

        return returnObj;
    }

    if (strCommand == "decode") {
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'noirnodebroadcast decode \"hexstring\"'");

        std::vector <CNoirnodeBroadcast> vecMnb;

        if (!DecodeHexVecMnb(vecMnb, params[1].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Noirnode broadcast message decode failed");

        int nSuccessful = 0;
        int nFailed = 0;
        int nDos = 0;
        UniValue returnObj(UniValue::VOBJ);

        BOOST_FOREACH(CNoirnodeBroadcast & mnb, vecMnb)
        {
            UniValue resultObj(UniValue::VOBJ);

            if (mnb.CheckSignature(nDos)) {
                nSuccessful++;
                resultObj.push_back(Pair("vin", mnb.vin.ToString()));
                resultObj.push_back(Pair("addr", mnb.addr.ToString()));
                resultObj.push_back(Pair("pubKeyCollateralAddress",
                                         CBitcoinAddress(mnb.pubKeyCollateralAddress.GetID()).ToString()));
                resultObj.push_back(Pair("pubKeyNoirnode", CBitcoinAddress(mnb.pubKeyNoirnode.GetID()).ToString()));
                resultObj.push_back(Pair("vchSig", EncodeBase64(&mnb.vchSig[0], mnb.vchSig.size())));
                resultObj.push_back(Pair("sigTime", mnb.sigTime));
                resultObj.push_back(Pair("protocolVersion", mnb.nProtocolVersion));
                resultObj.push_back(Pair("nLastDsq", mnb.nLastDsq));

                UniValue lastPingObj(UniValue::VOBJ);
                lastPingObj.push_back(Pair("vin", mnb.lastPing.vin.ToString()));
                lastPingObj.push_back(Pair("blockHash", mnb.lastPing.blockHash.ToString()));
                lastPingObj.push_back(Pair("sigTime", mnb.lastPing.sigTime));
                lastPingObj.push_back(
                        Pair("vchSig", EncodeBase64(&mnb.lastPing.vchSig[0], mnb.lastPing.vchSig.size())));

                resultObj.push_back(Pair("lastPing", lastPingObj));
            } else {
                nFailed++;
                resultObj.push_back(Pair("errorMessage", "Noirnode broadcast signature verification failed"));
            }

            returnObj.push_back(Pair(mnb.GetHash().ToString(), resultObj));
        }

        returnObj.push_back(Pair("overall", strprintf(
                "Successfully decoded broadcast messages for %d noirnodes, failed to decode %d, total %d",
                nSuccessful, nFailed, nSuccessful + nFailed)));

        return returnObj;
    }

    if (strCommand == "relay") {
        if (params.size() < 2 || params.size() > 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "noirnodebroadcast relay \"hexstring\" ( fast )\n"
                    "\nArguments:\n"
                    "1. \"hex\"      (string, required) Broadcast messages hex string\n"
                    "2. fast       (string, optional) If none, using safe method\n");

        std::vector <CNoirnodeBroadcast> vecMnb;

        if (!DecodeHexVecMnb(vecMnb, params[1].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Noirnode broadcast message decode failed");

        int nSuccessful = 0;
        int nFailed = 0;
        bool fSafe = params.size() == 2;
        UniValue returnObj(UniValue::VOBJ);

        // verify all signatures first, bailout if any of them broken
        BOOST_FOREACH(CNoirnodeBroadcast & mnb, vecMnb)
        {
            UniValue resultObj(UniValue::VOBJ);

            resultObj.push_back(Pair("vin", mnb.vin.ToString()));
            resultObj.push_back(Pair("addr", mnb.addr.ToString()));

            int nDos = 0;
            bool fResult;
            if (mnb.CheckSignature(nDos)) {
                if (fSafe) {
                    fResult = mnodeman.CheckMnbAndUpdateNoirnodeList(NULL, mnb, nDos);
                } else {
                    mnodeman.UpdateNoirnodeList(mnb);
                    mnb.RelayNoirNode();
                    fResult = true;
                }
                mnodeman.NotifyNoirnodeUpdates();
            } else fResult = false;

            if (fResult) {
                nSuccessful++;
                resultObj.push_back(Pair(mnb.GetHash().ToString(), "successful"));
            } else {
                nFailed++;
                resultObj.push_back(Pair("errorMessage", "Noirnode broadcast signature verification failed"));
            }

            returnObj.push_back(Pair(mnb.GetHash().ToString(), resultObj));
        }

        returnObj.push_back(Pair("overall", strprintf(
                "Successfully relayed broadcast messages for %d noirnodes, failed to relay %d, total %d", nSuccessful,
                nFailed, nSuccessful + nFailed)));

        return returnObj;
    }

    return NullUniValue;
}
