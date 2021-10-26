// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

//
// Struct that defines SAMPA Header
//
// A. Baldisseri (Feb. 2017)
//

#ifndef SAMPA_HEADERSTRUCT_H
#define SAMPA_HEADERSTRUCT_H

namespace Sampa
{
struct SampaHeaderStruct {
  unsigned long
    fHammingCode : 6,
    fHeaderParity : 1,
    fPkgType : 3,
    fNbOf10BitWords : 10,
    fChipAddress : 4,
    fChannelAddress : 5,
    fBunchCrossingCounter : 20,
    fPayloadParity : 1,
    fUnused : 14;
};

//    struct SampaHeaderStruct {
//        unsigned int fHammingCode          : 6;
//        unsigned int fHeaderParity         : 1;
//        unsigned int fPkgType              : 3;
//        unsigned int fNbOf10BitWords       : 10;
//        unsigned int fChipAddress          : 4;
//        unsigned int fChannelAddress       : 5;
//        unsigned int fBunchCrossingCounter : 20;
//        unsigned int fPayloadParity        : 1;
//    };

//    struct SampaHeaderStruct {
//
//        unsigned int fPayloadParity        : 1;
//        unsigned int fBunchCrossingCounter : 20;
//        unsigned int fChannelAddress       : 5;
//        unsigned int fChipAddress          : 4;
//        unsigned int fNbOf10BitWords       : 10;
//        unsigned int fPkgType              : 3;
//        unsigned int fHeaderParity         : 1;
//        unsigned int fHammingCode          : 6;
//
//
//    };

} // namespace Sampa

#endif // SAMPA_HEADERSTRUCT_H
