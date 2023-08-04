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
/// \file   RootFileStorage.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_ROOTFILESTORAGE_H
#define QUALITYCONTROL_ROOTFILESTORAGE_H

#include <string>
#include <map>
#include <variant>
#include <stack>
#include <vector>

class TFile;
class TDirectory;

namespace o2::quality_control::core
{

class MonitorObjectCollection;

/// \brief Manager for storage and retrieval of MonitorObjectCollections in TFiles
class RootFileStorage
{
 public:
  struct MonitorObjectCollectionNode;
  struct DirectoryNode {
    std::string fullPath{};
    std::string name{};
    std::map<std::string, std::variant<DirectoryNode, MonitorObjectCollectionNode>> children = {};
  };
  struct MonitorObjectCollectionNode {
    std::string fullPath{};
    std::string name{};
    MonitorObjectCollection* moc = nullptr;
  };

  enum class ReadMode {
    Read,
    Update
  };

  explicit RootFileStorage(const std::string& filePath, ReadMode);
  ~RootFileStorage();

  DirectoryNode readStructure(bool loadObjects = false) const;

  MonitorObjectCollection* readMonitorObjectCollection(const std::string& path) const;

  void storeIntegralMOC(MonitorObjectCollection* const moc);
  void storeMovingWindowMOC(MonitorObjectCollection* const moc);

 private:
  DirectoryNode readStructureImpl(TDirectory* currentDir, bool loadObjects) const;

 private:
  TFile* mFile = nullptr;
};

/// \brief walks over integral MOC paths in the alphabetical order of detectors and task names
class IntegralMocWalker
{
 public:
  explicit IntegralMocWalker(const RootFileStorage::DirectoryNode& rootNode);

  bool hasNextPath();

  std::string nextPath();

 private:
  using child_iterator = decltype(std::declval<RootFileStorage::DirectoryNode>().children.cbegin());
  std::vector<std::string> mOrder;
  std::vector<std::string>::const_iterator mPathIterator;
};

/// \brief walks over moving window MOC paths in the chronological order
class MovingWindowMocWalker
{
 public:
  explicit MovingWindowMocWalker(const RootFileStorage::DirectoryNode& rootNode);

  bool hasNextPath() const;

  std::string nextPath();

 private:
  using child_iterator = decltype(std::declval<RootFileStorage::DirectoryNode>().children.begin());
  std::multimap<uint64_t, std::string> mOrder;
  std::multimap<uint64_t, std::string>::const_iterator mPathIterator;
};

} // namespace o2::quality_control::core

#endif // QUALITYCONTROL_ROOTFILESTORAGE_H
