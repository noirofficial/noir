// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double fSigcheckVerificationFactor = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64 nTimeLastCheckpoint;
        int64 nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (      0, uint256("0x23911212a525e3d149fcad6c559c8b17f1e8326a272a75ff9bb315c8d96433ef"))
	(    160, uint256("0x8789d38fb146f4fbbc2057019944eab4320c4f36a6ac8d5128a9c7ac01773784"))
	(   2857, uint256("0x9c88967c9070fc8271478fb554b4201ced511b5264018dfcf42211837ecb4965"))
        (   5000, uint256("0x3bdd20e0a597ac1c30ef2aa335474d52addc4fe06850cbc9079bd76e77b0ef63"))
	(   10000, uint256("0xa10b137abce234ed21c2d25e64f12975f56ddc1f3ec74a3fb72bd436ec5731b0"))
	(   19285, uint256("0xab45de4e6e33b9ef32402c30195e23602dab87c1213c7f898dfd76054e5b55df"))
	(   23246, uint256("0xa1b05dc07d80ffcefa1eb7590b4a45d0ded23d5f6ca0824531a11aea3838200f"))
	(   27962, uint256("0x2dbe39206eaa0c0f12683f3fde9a9d51a0e8700be6c8f393d881870e8810e4d4"))
	(   27982, uint256("0xf288dccbf7593f7c98988a69868382607e2aadb86942958ed37a25f32279505d"))
	(   27996, uint256("0xfed132c4ef97a1512b41662fbfd1a6c9c8edb1a04b577813d572abf24070bfa1"))	
	(   30643, uint256("0xcb2482dc59053afccb109d986ab060e9920e25be384dbc162d15975c08928133"))
	(   31716, uint256("0x43aabee008f19b9f403aa3f510c1ae8eb364b23fad9b35dc326b6a33bcb11379"))
	(   38232, uint256("0x7e30df00017ff5fcee8f09686e2879dc5912f2422e5ba51fc1285a6d54379f18"))
	(   43002, uint256("0x36cc81be0ba984d308a4b37c3b799d714436f79b76a0469a8abb12f1730431bb"))
	(   66120, uint256("0x2a25932779f36adfb18829df71d89e0443680706921a9febd5bbfc72c3de0a53"))
	(   70561, uint256("0xb68502e11080bab864e516fb289685b7a389fe5f23843f85167774fd74a0cd52"))
	(  100013, uint256("0x356eb4cf425ff78a2d6657784cfcd504dfbe1113a477c5f23caaf2e67636b6f6"))
	(  100980, uint256("0x568b5969a6c473d9d63b0e68e7f054efbc254c3201872d177985549aaa7bc9f9"))
    ( 192630, uint256("0x271b4a537db0e02b0011ecf85c96e70a92fc47c33e4ce4ee1024e0abcde919d2"))
    ( 202380, uint256("0x5b1e1682e11dec8b3e5d658b2f6fed0147274fd503041398d2203ad87b2e3e6a"))
		
	;
    static const CCheckpointData data = {
        &mapCheckpoints,
        1511417378, // * UNIX timestamp of last checkpoint block
        157916,      // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        762         // * estimated number of transactions per day after checkpoint //1152.0
    };

    static MapCheckpoints mapCheckpointsTestnet = 
        boost::assign::map_list_of
        (     0, uint256("0x6283b7fafca969a803f6f539f5e8fb1a4f8a28fc1ec2106ad35b39354a4647e5"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1478117690,
        1,
        3456.0
    };

    const CCheckpointData &Checkpoints() {
        if (fTestNet)
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!GetBoolArg("-checkpoints", true))
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex) {
        if (pindex==NULL)
            return 0.0;

        int64 nNow = time(NULL);

        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (!GetBoolArg("-checkpoints", true))
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
