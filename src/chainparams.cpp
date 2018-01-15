// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"
#include "libzerocoin/bitcoin_bignum/bignum.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"
#include "arith_uint256.h"


static CBlock CreateGenesisBlock(const char *pszTimestamp, const CScript &genesisOutputScript, uint32_t nTime, uint32_t nNonce,
                   uint32_t nBits, int32_t nVersion, const CAmount &genesisReward,
                   std::vector<unsigned char> extraNonce) {
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
//    CScriptNum csn = CScriptNum(4);
//    std::cout << "CScriptNum(4):" << csn.GetHex();
//    CBigNum cbn = CBigNum(4);
//    std::cout << "CBigNum(4):" << cbn.GetHex();

    txNew.vin[0].scriptSig = CScript() << nBits << CBigNum(4).getvch() << std::vector < unsigned
    char >
    ((const unsigned char *) pszTimestamp, (const unsigned char *) pszTimestamp + strlen(pszTimestamp)) << extraNonce;

    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetHex("0x0");
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount &genesisReward,
                   std::vector<unsigned char> extraNonce) {
//    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    //btzc: zoin timestamp
    const char *pszTimestamp = "We don’t operate on leaks - Obama 2 Nov 2016";
    const CScript genesisOutputScript = CScript();
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward,
                              extraNonce);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.nMinNFactor = 10;
        consensus.nMaxNFactor = 30;
        //nVertcoinStartTime
        consensus.nChainStartTime = 1389306217;
        consensus.BIP34Height = 227931;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
//      static const int64 nInterval = nTargetTimespan / nTargetSpacing;
        consensus.nPowTargetTimespan = 150 * 3; // 7.5 minutes between retargets
        consensus.nPowTargetSpacing = 150; // 2.5 minute blocks
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1475020800; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1479168000; // November 15th, 2016.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1510704000; // November 15th, 2017.

        // The best chain should have at least this much work.
//        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000003418b3ccbe5e93bcb39b43");
        consensus.nMinimumChainWorka = uint256S("0x0000000000000000000000000000000000000000000000000708f98bf623f02e");

            /**
             * The message start string is designed to be unlikely to occur in normal data.
             * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
           `  * a large 32-bit integer with any alignment.
             */
        //btzc: update zoin pchMessage
        pchMessageStart[0] = 0xf5;
        pchMessageStart[1] = 0x03;
        pchMessageStart[2] = 0xa9;
        pchMessageStart[3] = 0x51;
        nDefaultPort = 8255;
        nPruneAfterHeight = 100000;
        /**
         * btzc: zoin init genesis block
         * nBits = 0x1e0ffff0
         * nTime = 1414776286
         * nNonce = 142392
         * genesisReward = 0 * COIN
         * nVersion = 2
         * extraNonce
         */
        std::vector<unsigned char> extraNonce(4);
        extraNonce[0] = 0x00;
        extraNonce[1] = 0x00;
        extraNonce[2] = 0x00;
        extraNonce[3] = 0x00;

        genesis = CreateGenesisBlock(1478117691, 104780, 520159231, 2, 0 * COIN, extraNonce);
        //const std::string s = genesis.GetHash().ToString();
        //std::cout << "zoin new genesis hash: " << genesis.GetHash().ToString() << std::endl;
        //std::cout << "zoin new hashMerkleRoot hash: " << genesis.hashMerkleRoot.ToString() << std::endl;
        consensus.hashGenesisBlock = genesis.GetHash();
        //btzc: update main zoin hashGenesisBlock and hashMerkleRoot

        assert(consensus.hashGenesisBlock ==
               uint256S("0x23911212a525e3d149fcad6c559c8b17f1e8326a272a75ff9bb315c8d96433ef"));
        assert(genesis.hashMerkleRoot ==
               uint256S("0x4f193d83c304ebd3bf2319611cbb84f26af7960f23d06dd243b6c93ebf4d7797"));
        //btzc: update zoin cdnsseeddata`
        vSeeds.push_back(CDNSSeedData("node11.zoinofficial.com", "node11.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node1.zoinofficial.com", "node1.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node2.zoinofficial.com", "node2.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node3.zoinofficial.com", "node3.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node4.zoinofficial.com", "node4.zoinofficial.com", false));
        //vSeeds.push_back(CDNSSeedData("node5.zoinofficial.com", "node5.zoinofficial.com", false));

        /*
        vSeeds.push_back(CDNSSeedData("node1.zoinofficial.com", "node1.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node2.zoinofficial.com", "node2.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node3.zoinofficial.com", "node3.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node4.zoinofficial.com", "node4.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node5.zoinofficial.com", "node5.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node6.zoinofficial.com", "node6.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node7.zoinofficial.com", "node7.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node8.zoinofficial.com", "node8.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node9.zoinofficial.com", "node9.zoinofficial.com", false));
        vSeeds.push_back(CDNSSeedData("node10.zoinofficial.com", "node10.zoinofficial.com", false)); */
        // Note that of those with the service bits flag, most only support a subset of possible options
        base58Prefixes[PUBKEY_ADDRESS] = std::vector < unsigned char > (1, 80);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector < unsigned char > (1, 7);
        base58Prefixes[SECRET_KEY] = std::vector < unsigned char > (1, 208);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container < std::vector < unsigned char > > ();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container < std::vector < unsigned char > > ();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (0, uint256S("0x23911212a525e3d149fcad6c559c8b17f1e8326a272a75ff9bb315c8d96433ef"))
            (160, uint256S("0x8789d38fb146f4fbbc2057019944eab4320c4f36a6ac8d5128a9c7ac01773784"))
            (2857, uint256S("0x9c88967c9070fc8271478fb554b4201ced511b5264018dfcf42211837ecb4965"))
            (5000, uint256S("0x3bdd20e0a597ac1c30ef2aa335474d52addc4fe06850cbc9079bd76e77b0ef63"))
            (10000, uint256S("0xa10b137abce234ed21c2d25e64f12975f56ddc1f3ec74a3fb72bd436ec5731b0"))
            (19285, uint256S("0xab45de4e6e33b9ef32402c30195e23602dab87c1213c7f898dfd76054e5b55df"))
            (23246, uint256S("0xa1b05dc07d80ffcefa1eb7590b4a45d0ded23d5f6ca0824531a11aea3838200f"))
            (27962, uint256S("0x2dbe39206eaa0c0f12683f3fde9a9d51a0e8700be6c8f393d881870e8810e4d4"))
            (27982, uint256S("0xf288dccbf7593f7c98988a69868382607e2aadb86942958ed37a25f32279505d"))
            (27996, uint256S("0xfed132c4ef97a1512b41662fbfd1a6c9c8edb1a04b577813d572abf24070bfa1"))
            (30643, uint256S("0xcb2482dc59053afccb109d986ab060e9920e25be384dbc162d15975c08928133"))
            (31716, uint256S("0x43aabee008f19b9f403aa3f510c1ae8eb364b23fad9b35dc326b6a33bcb11379"))
            (38232, uint256S("0x7e30df00017ff5fcee8f09686e2879dc5912f2422e5ba51fc1285a6d54379f18"))
            (43002, uint256S("0x36cc81be0ba984d308a4b37c3b799d714436f79b76a0469a8abb12f1730431bb"))
            (66120, uint256S("0x2a25932779f36adfb18829df71d89e0443680706921a9febd5bbfc72c3de0a53"))
            (70561, uint256S("0xb68502e11080bab864e516fb289685b7a389fe5f23843f85167774fd74a0cd52"))
            (100013, uint256S("0x356eb4cf425ff78a2d6657784cfcd504dfbe1113a477c5f23caaf2e67636b6f6"))
            (100980, uint256S("0x568b5969a6c473d9d63b0e68e7f054efbc254c3201872d177985549aaa7bc9f9"))
            (192630, uint256S("0x271b4a537db0e02b0011ecf85c96e70a92fc47c33e4ce4ee1024e0abcde919d2"))
            (202380, uint256S("0x5b1e1682e11dec8b3e5d658b2f6fed0147274fd503041398d2203ad87b2e3e6a")),
            1511417378, // * UNIX timestamp of last checkpoint block
            157916,    // * total number of transactions between genesis and last checkpoint
            //   (the tx=... number in the SetBestChain debug.log lines)
            762.0     // * estimated number of transactions per day after checkpoint
        };
    }
};

static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.nMinNFactor = 10;
        consensus.nMaxNFactor = 30;
        consensus.nChainStartTime = 1389306217;
        consensus.BIP34Height = 21111;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 150 * 3; // 7.5 minutes between retargets
        consensus.nPowTargetSpacing = 150; // 2.5 minute blocks
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1456790400; // March 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1462060800; // May 1st 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1493596800; // May 1st 2017

        // The best chain should have at least this much work.
        consensus.nMinimumChainWorka = uint256S("0x0000000000000000000000000000000000000000000000000708f98bf623f02e");

        pchMessageStart[0] = 0xae;
        pchMessageStart[1] = 0x5d;
        pchMessageStart[2] = 0xbf;
        pchMessageStart[3] = 0x09;

        nDefaultPort = 28168;
        nPruneAfterHeight = 1000;
        /**
          * btzc: testnet params
          * nTime: 1478117690
          * nNonce: 1620571
          */

        std::vector<unsigned char> extraNonce(4);
        extraNonce[0] = 0x00;
        extraNonce[1] = 0x00;
        extraNonce[2] = 0x00;
        extraNonce[3] = 0x00;

        genesis = CreateGenesisBlock(1478117690, 177, 536936447, 2, 0 * COIN, extraNonce);
        std::cout << "zoin test new genesis hash: " << genesis.GetHash().ToString() << std::endl;
        std::cout << "zoin test new hashMerkleRoot hash: " << genesis.hashMerkleRoot.ToString() << std::endl;
        consensus.hashGenesisBlock = genesis.GetHash();

        //btzc: update testnet zoin hashGenesisBlock and hashMerkleRoot
        assert(consensus.hashGenesisBlock ==
               uint256S("0x6283b7fafca969a803f6f539f5e8fb1a4f8a28fc1ec2106ad35b39354a4647e5"));
        assert(genesis.hashMerkleRoot ==
               uint256S("0x04ff9bc3453a83687a95daf2342eceedac19dd73e356569704533aae02e9d6a9"));
        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        // zoin test seeds
        vSeeds.push_back(CDNSSeedData("92.247.116.44", "92.247.116.44", false));

        //vSeeds.push_back(CDNSSeedData("92.247.116.44", "92.247.116.44", true));
//        vSeeds.push_back(CDNSSeedData("petertodd.org", "seed.tbtc.petertodd.org", true));
//        vSeeds.push_back(CDNSSeedData("bluematt.me", "testnet-seed.bluematt.me"));
//        vSeeds.push_back(CDNSSeedData("bitcoin.schildbach.de", "testnet-seed.bitcoin.schildbach.de"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector < unsigned char > (1, 65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector < unsigned char > (1, 178);
        base58Prefixes[SECRET_KEY] = std::vector < unsigned char > (1, 193);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container < std::vector < unsigned char > > ();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container < std::vector < unsigned char > > ();
        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;


        checkpointData = (CCheckpointData) {
                boost::assign::map_list_of
                        (0, uint256S("0x")),
                        1478117690,
                        0,
                        100.0
        };

    }
};

static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
        consensus.BIP34Hash = uint256();
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 60 * 60; // 60 minutes between retargets
        consensus.nPowTargetSpacing = 10 * 60; // 10 minute blocks
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWorka = uint256S("0x00");

        pchMessageStart[0] = 0xae;
        pchMessageStart[1] = 0x5d;
        pchMessageStart[2] = 0xbf;
        pchMessageStart[3] = 0x09;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;

        /**
          * btzc: testnet params
          * nTime: 1478117690
          * nNonce: 177
          */
        std::vector<unsigned char> extraNonce(4);
        extraNonce[0] = 0x00;
        extraNonce[1] = 0x00;
        extraNonce[2] = 0x00;
        extraNonce[3] = 0x00;
        genesis = CreateGenesisBlock(1478117690, 177, 536936447, 2, 0 * COIN, extraNonce);
        consensus.hashGenesisBlock = genesis.GetHash();
        //btzc: update regtest zoin hashGenesisBlock and hashMerkleRoot
//        std::cout << "zoin regtest genesisBlock hash: " << consensus.hashGenesisBlock.ToString() << std::endl;
//        std::cout << "zoin regtest hashMerkleRoot hash: " << genesis.hashMerkleRoot.ToString() << std::endl;
        //btzc: update testnet zoin hashGenesisBlock and hashMerkleRoot
        assert(consensus.hashGenesisBlock ==
               uint256S("0x6283b7fafca969a803f6f539f5e8fb1a4f8a28fc1ec2106ad35b39354a4647e5"));
        assert(genesis.hashMerkleRoot ==
               uint256S("0x04ff9bc3453a83687a95daf2342eceedac19dd73e356569704533aae02e9d6a9"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData) {
                boost::assign::map_list_of
                        (0, uint256S("0x23911212a525e3d149fcad6c559c8b17f1e8326a272a75ff9bb315c8d96433ef")),
                0,
                0,
                0
        };
        base58Prefixes[PUBKEY_ADDRESS] = std::vector < unsigned char > (1, 65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector < unsigned char > (1, 178);
        base58Prefixes[SECRET_KEY] = std::vector < unsigned char > (1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container < std::vector < unsigned char > > ();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container < std::vector < unsigned char > > ();
    }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout) {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
};

static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams &Params(const std::string &chain) {
    if (chain == CBaseChainParams::MAIN)
        return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
        return testNetParams;
    else if (chain == CBaseChainParams::REGTEST)
        return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string &network) {
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout) {
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}
 
