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

///
/// \file   HelperGraph.h
/// \author Jakub Muszyński jakub.milosz.muszynski@cern.ch
/// \brief Graph helper

#ifndef QC_MODULE_FIT_FITHELPERGRAPH_H
#define QC_MODULE_FIT_FITHELPERGRAPH_H

#include <memory>
#include <string>
#include <type_traits>

#include <TGraph.h>
#include "QualityControl/QcInfoLogger.h"

namespace o2::quality_control_modules::fit::helper
{

/// \brief Factory that forwards ctor arguments to the chosen GraphType.
/// \example
///   auto g = makeGraph<TGraphErrors>("name","title",n,x,y,ex,ey);
/// \author Jakub Muszyński jakub.milosz.muszynski@cern.ch
template <typename GraphType, typename... CtorArgs>
inline auto makeGraph(const std::string& name,
                      const std::string& title,
                      CtorArgs&&... args)
{
  static_assert(std::is_base_of_v<TObject, GraphType>,
                "GraphType must inherit from TObject / ROOT graph class");
  auto ptr = std::make_unique<GraphType>(std::forward<CtorArgs>(args)...);
  ptr->SetNameTitle(name.c_str(), title.c_str());
  return ptr;
}

/// \brief Publishes graph through the QC ObjectsManager.
/// \author Jakub Muszyński jakub.milosz.muszynski@cern.ch
template <typename GraphType,
          typename ManagerType,
          typename PublicationPolicyType,
          typename... CtorArgs>
inline auto registerGraph(ManagerType manager,
                          PublicationPolicyType publicationPolicy,
                          const std::string& defaultDrawOption,
                          const std::string& name,
                          const std::string& title,
                          CtorArgs&&... args)
{
  auto ptrGraph = makeGraph<GraphType>(name, title,
                                       std::forward<CtorArgs>(args)...);
  manager->startPublishing(ptrGraph.get(), publicationPolicy);

  if (!defaultDrawOption.empty()) {
    manager->setDefaultDrawOptions(ptrGraph.get(), defaultDrawOption);
  }
  ILOG(Info, Support) << "Registered graph \"" << name
                      << "\" with publication policy " << int(publicationPolicy)
                      << ENDM;
  return ptrGraph;
}

} // namespace o2::quality_control_modules::fit::helper
#endif // QC_MODULE_FIT_FITHELPERGRAPH_H
