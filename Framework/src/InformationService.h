// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
//

///
/// \author bvonhall
/// \file InformationService.h
///


#ifndef PROJECT_INFORMATIONSERVICE_H
#define PROJECT_INFORMATIONSERVICE_H

#include "FairMQDevice.h"

/// \brief Collect the list of objects published by all the tasks and make it available to clients.
///
/// The InformationService receives the list of objects published by each task.
/// It keeps an up to date list and send it upon request to clients. It also publishes updates when
/// new information comes from the tasks.
class InformationService : public FairMQDevice
{
  public:
    InformationService();
    virtual ~InformationService();

  protected:
    bool HandleData(FairMQMessagePtr&, int);

  private:
    std::map<std::string,std::vector<std::string>> mCache;
};

#endif //PROJECT_INFORMATIONSERVICE_H
