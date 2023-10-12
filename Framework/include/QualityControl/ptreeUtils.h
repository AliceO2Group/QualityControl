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
/// \file   ptreeUtils.h
/// \author Barthelemy von Haller
///

#ifndef QC_PTREE_UTILS_H
#define QC_PTREE_UTILS_H

#include <iostream>
#include <Common/Exceptions.h>
#include <boost/property_tree/ptree.hpp>
using boost::property_tree::ptree;

using namespace std;
namespace o2::quality_control::core
{

// Freely inspired from write_json_helper in boost
template <class Ptree>
void mergeInto(const Ptree& pt, ptree& parent, const std::string& fullPath , int indent)
{
  typedef typename Ptree::key_type::value_type Ch;
  typedef typename std::basic_string<Ch> Str;

  // Value or object or array
  if (indent > 0 && pt.empty()) { // Handle value
    Str data = pt.template get_value<Str>();
    parent.add(fullPath, data);
  } else if (indent > 0 && pt.count(Str()) == pt.size()) { // Handle array
    typename Ptree::const_iterator it = pt.begin();
    for (; it != pt.end(); ++it) {
      ptree object;
      // this check is to avoid problems when there is a node with data and we try to add children to it (forbidden in json)
      if(parent.get_child_optional(fullPath+".") && !parent.get_child(fullPath+".").data().empty()) {
        cerr << "could not add item to array '" + fullPath + ".' because it already contains some data." << endl;
      } else {
        parent.add_child(fullPath+".", it->second);
      }
    }
  } else { // Handle object
    typename Ptree::const_iterator it = pt.begin();
    for (; it != pt.end(); ++it) {
      std::string newFullPath = fullPath.empty() ? it->first : fullPath + "." + it->first;
      mergeInto(it->second, parent, newFullPath, indent + 1);
    }
  }
}

template <class Ptree>
void mergeInto(const Ptree& pt, ptree& destination)
{
  mergeInto(pt, destination, "", 1);
}

} // namespace o2::quality_control::core

#endif // QC_PTREE_UTILS_H
