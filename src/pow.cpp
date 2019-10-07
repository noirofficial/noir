// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"
#include "main.h"
#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"
#include <iostream>
#include "util.h"
#include "chainparams.h"
#include "libzerocoin/bitcoin_bignum/bignum.h"
#include "fixed.h"


static CBigNum bnProofOfWorkLimit(~arith_uint256(0) >> 16);

double GetDifficultyHelper(unsigned int nBits) {
    int nShift = (nBits >> 24) & 0xff;
    double dDiff = (double) 0x0000ffff / (double) (nBits & 0x00ffffff);

    while (nShift < 29) {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29) {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

static const int64_t nTargetSpacing = 180; // 2.5 minute blocks
static const int64_t nRetargetInterval = 3; // retargets every 3 blocks
static const int64_t nRetargetTimespan = nRetargetInterval * nTargetSpacing; // 7.5 minutes between retargets

static const int64_t nLookbackInterval = 6; // 6 blocks lookback for difficulty adjustment
static const int64_t nLookbackTimespan = nLookbackInterval * nTargetSpacing; // 15 minutes

static const int64_t LimUp = nLookbackTimespan * 100 / 106; // 6% up
static const int64_t LimDown = nLookbackTimespan * 106 / 100; // 6% down

//btzc, noir GetNextWorkRequired
unsigned int GetNextWorkRequired(const CBlockIndex *pindexLast, const CBlockHeader *pblock, const Consensus::Params &params) {

        unsigned int nProofOfWorkLimit = bnProofOfWorkLimit.GetCompact();
        bool fTestNet = Params().NetworkIDString() == CBaseChainParams::TESTNET;
        // Genesis block
        if (pindexLast == NULL)
            return nProofOfWorkLimit;

        // Testnet - min difficulty
        if (fTestNet){
            bnProofOfWorkLimit = CBigNum(~arith_uint256(0) >> 8);
            //return bnProofOfWorkLimit.GetCompact();
        }

        // Only change once per interval
        if ((pindexLast->nHeight+1) % nRetargetInterval != 0){
            return pindexLast->nBits;
        }

        // Go back by what we want to be nLookbackInterval blocks
        const CBlockIndex* pindexFirst = pindexLast;
        for (int i = 0; pindexFirst && i < nLookbackInterval; i++)
            pindexFirst = pindexFirst->pprev;
        if (!pindexFirst)
            return pindexLast->nBits;


        // Limit adjustment step
        int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
        //printf("  nActualTimespan = %ld before bounds\n", nActualTimespan);
        if (nActualTimespan < LimUp){
            //LogPrintf("diff up! \n");
            nActualTimespan = LimUp;
        }
        if (nActualTimespan > LimDown){
            //LogPrintf("diff down! \n");
            nActualTimespan = LimDown;
        }

        // Retarget
        CBigNum bnNew;
        bnNew.SetCompact(pindexLast->nBits);
        bnNew *= nActualTimespan;
        bnNew /= nLookbackTimespan;

        //printf("bnNew = %lld    bnProofOfWorkLimit = %ld\n", bnNew.GetCompact(), bnProofOfWorkLimit.GetCompact());
        if (bnNew > bnProofOfWorkLimit)
            bnNew = bnProofOfWorkLimit;

        return bnNew.GetCompact();
}

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, bool fProofOfStake)
{
    unsigned int nTargetLimit = UintToArith256(Params().GetConsensus().posLimit).GetCompact();

    // Genesis block or first proof-of-stake block
    if (pindexLast == NULL || pindexLast->nHeight == Params().GetConsensus().nLastPOWBlock)
        return UintToArith256(params.posLimit).GetCompact();

    const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);

    if (pindexPrev->pprev == NULL)
        return nTargetLimit; // first block
    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == NULL)
        return nTargetLimit; // second block

    return CalculateNextTargetRequired(pindexPrev, pindexPrevPrev->GetBlockTime(), params, fProofOfStake);
}

unsigned int CalculateNextTargetRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params, bool fProofOfStake)
{
    int64_t nTargetSpacing = Params().GetConsensus().nPowTargetSpacing;
    int64_t nActualSpacing = pindexLast->GetBlockTime() - nFirstBlockTime;

    // retarget with exponential moving toward target spacing
    const arith_uint256 bnTargetLimit = UintToArith256(Params().GetConsensus().posLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    int64_t nInterval = params.nPowTargetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);

    if (bnNew <= 0 || bnNew > bnTargetLimit)
        bnNew = bnTargetLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params &params, int nHeight) {
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget = arith_uint256().SetCompact(nBits);
    bool fTestNet = Params().NetworkIDString() == CBaseChainParams::TESTNET;

    // Testnet - min difficulty
    if (fTestNet){
        bnProofOfWorkLimit = CBigNum(~arith_uint256(0) >> 8);
    }
    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit)){
         LogPrintf("CPOW: Range Error\n %d", nHeight);
         return false;
    }

    if(nHeight == INT_MAX || nHeight < 233000)
        return true;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget){
        LogPrintf("CPOW: Amount Error %d\n", nHeight);
        return false;
    }
    return true;
}