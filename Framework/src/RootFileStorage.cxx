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
/// \file   RootFileStorage.cxx
/// \author Piotr Konopka
///

#include "QualityControl/RootFileStorage.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObjectCollection.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/ValidityInterval.h"

#include <TFile.h>
#include <TKey.h>
#include <TIterator.h>
#include <TDirectory.h>
#include <filesystem>
#include <stack>

namespace o2::quality_control::core
{

constexpr auto integralsDirectoryName = "int";
constexpr auto movingWindowsDirectoryName = "mw";

RootFileStorage::RootFileStorage(const std::string& filePath, ReadMode readMode)
{
  switch (readMode) {
    case ReadMode::Update:
      mFile = new TFile(filePath.c_str(), "UPDATE");
      break;
    case ReadMode::Read:
    default:
      mFile = new TFile(filePath.c_str(), "READ");
  }
  if (mFile->IsZombie()) {
    throw std::runtime_error("File '" + filePath + "' is zombie.");
  }
  if (!mFile->IsOpen()) {
    throw std::runtime_error("Failed to open the file: " + filePath);
  }
  if (readMode == ReadMode::Update && !mFile->IsWritable()) {
    throw std::runtime_error("File '" + filePath + "' is not writable.");
  }
  ILOG(Info) << "Output file '" << filePath << "' successfully open." << ENDM;
}

RootFileStorage::DirectoryNode RootFileStorage::readStructure(bool loadObjects) const
{
  return readStructureImpl(mFile, loadObjects);
}

RootFileStorage::DirectoryNode RootFileStorage::readStructureImpl(TDirectory* currentDir, bool loadObjects) const
{
  auto fullPath = currentDir->GetPath();
  auto pathToPos = std::strstr(fullPath, ":/");
  if (pathToPos == nullptr) {
    ILOG(Error, Support) << "Could not extract path to node in string '" << currentDir->GetPath() << "', skipping" << ENDM;
    return {};
  }
  DirectoryNode currentNode{ pathToPos + 2, currentDir->GetName() };

  TIter nextKey(currentDir->GetListOfKeys());
  TKey* key;
  while ((key = (TKey*)nextKey())) {
    if (!loadObjects && std::strcmp(key->GetClassName(), MonitorObjectCollection::Class_Name()) == 0) {
      std::string mocPath = currentNode.fullPath + std::filesystem::path::preferred_separator + key->GetName();
      currentNode.children[key->GetName()] = MonitorObjectCollectionNode{ mocPath, key->GetName() };
      continue;
    }

    ILOG(Debug, Devel) << "Getting the value for key '" << key->GetName() << "'" << ENDM;
    auto* value = currentDir->Get(key->GetName());
    if (value == nullptr) {
      ILOG(Error) << "Could not get the value '" << key->GetName() << "', skipping." << ENDM;
      continue;
    }
    if (auto moc = dynamic_cast<MonitorObjectCollection*>(value)) {
      moc->postDeserialization();
      std::string mocPath = currentNode.fullPath + std::filesystem::path::preferred_separator + key->GetName();
      currentNode.children[moc->GetName()] = MonitorObjectCollectionNode{ mocPath, key->GetName(), moc };
      ILOG(Debug, Support) << "Read object '" << moc->GetName() << "' in path '" << currentNode.fullPath << "'" << ENDM;
    } else if (auto childDir = dynamic_cast<TDirectory*>(value)) {
      currentNode.children[key->GetName()] = readStructureImpl(childDir, loadObjects);
    } else {
      ILOG(Warning, Support) << "Could not cast the node to MonitorObjectCollection nor TDirectory, skipping." << ENDM;
      delete value;
      continue;
    }
  }

  return currentNode;
}

MonitorObjectCollection* RootFileStorage::readMonitorObjectCollection(const std::string& path) const
{
  auto storedTObj = mFile->Get(path.c_str());
  if (storedTObj == nullptr) {
    ILOG(Error, Ops) << "Could not read object '" << path << "'" << ENDM;
    return nullptr;
  }
  auto storedMOC = dynamic_cast<MonitorObjectCollection*>(storedTObj);
  if (storedMOC == nullptr) {
    ILOG(Error, Ops) << "Could not cast the stored object to MonitorObjectCollection" << ENDM;
    delete storedTObj;
  }
  return storedMOC;
}

RootFileStorage::~RootFileStorage()
{
  if (mFile != nullptr) {
    if (mFile->IsOpen()) {
      ILOG(Info, Support) << "Closing file '" << mFile->GetName() << "'." << ENDM;
      mFile->Write();
      mFile->Close();
    }
    delete mFile;
  }
}

void deleteTDirectory(TDirectory* d)
{
  if (d != nullptr) {
    d->Write();
    d->Close();
    delete d;
  }
}

std::unique_ptr<TDirectory, void (*)(TDirectory*)> getOrCreateDirectory(TDirectory* parentDir, const char* dirName)
{
  auto dir = std::unique_ptr<TDirectory, void (*)(TDirectory*)>(parentDir->GetDirectory(dirName), deleteTDirectory);
  if (dir == nullptr) {
    ILOG(Debug, Support) << "Creating a new directory '" << dirName << "'." << ENDM;
    dir = std::unique_ptr<TDirectory, void (*)(TDirectory*)>(parentDir->mkdir(dirName), deleteTDirectory);
  }
  return dir;
}

validity_time_t earliestValidFrom(const MonitorObjectCollection* moc)
{
  validity_time_t earliest = std::numeric_limits<validity_time_t>::max();
  for (const auto& obj : *moc) {
    if (auto mo = dynamic_cast<MonitorObject*>(obj)) {
      earliest = std::min(mo->getValidity().getMin(), earliest);
    }
  }
  return earliest;
}

bool validObjectValidities(const MonitorObjectCollection* moc)
{
  for (const auto& obj : *moc) {
    if (auto mo = dynamic_cast<MonitorObject*>(obj); mo->getValidity().isInvalid()) {
      return false;
    }
  }
  return true;
}

// fixme we should not have to change the name!
void RootFileStorage::storeIntegralMOC(MonitorObjectCollection* const moc)
{
  const auto& mocStorageName = moc->getTaskName();
  if (mocStorageName.empty()) {
    ILOG(Error, Support) << "taskName empty, not storing MOC '" << moc->GetName() << "' for detector '" << moc->getDetector() << "'" << ENDM;
    return;
  }
  moc->SetName(mocStorageName.c_str());
  // directory level: int
  auto integralDir = getOrCreateDirectory(mFile, integralsDirectoryName);
  if (integralDir == nullptr) {
    ILOG(Error, Support) << "Could not create the directory '" << integralsDirectoryName << "', skipping." << ENDM;
    return;
  }

  // directory level: int/DET
  auto detector = moc->getDetector();
  auto detDir = getOrCreateDirectory(integralDir.get(), detector.c_str());
  if (detDir == nullptr) {
    ILOG(Error, Support) << "Could not create directory '" << detector << "', skipping." << ENDM;
    return;
  }

  // directory level: int/DET/TASK
  ILOG(Debug, Support) << "Checking for existing objects in the file." << ENDM;
  int nbytes = 0;
  auto storedMOC = std::unique_ptr<MonitorObjectCollection>(detDir->Get<MonitorObjectCollection>(mocStorageName.c_str()));
  if (storedMOC != nullptr) {
    storedMOC->postDeserialization();
    ILOG(Info, Support) << "Merging objects for task '" << detector << "/" << moc->getTaskName() << "' with the existing ones in the file." << ENDM;
    storedMOC->merge(moc);
    nbytes = detDir->WriteObject(storedMOC.get(), storedMOC->GetName(), "Overwrite");
  } else {
    ILOG(Info, Support) << "Storing objects for task '" << detector << "/" << moc->getTaskName() << "' in the file." << ENDM;
    nbytes = detDir->WriteObject(moc, moc->GetName(), "Overwrite");
  }
  ILOG(Info, Support) << "Integrated objects '" << moc->GetName() << "' have been stored in the file (" << nbytes << " bytes)." << ENDM;
}

void RootFileStorage::storeMovingWindowMOC(MonitorObjectCollection* const moc)
{
  if (moc->GetEntries() == 0) {
    ILOG(Warning, Support) << "The provided MonitorObjectCollection '" << moc->GetName() << "' is empty, will not store." << ENDM;
    return;
  }
  if (!validObjectValidities(moc)) {
    // this should not happen, because we have a protection in MonitorObjectCollection::cloneMovingWindow() against it.
    // thus, we should raise some concern if this occurs anyway.
    ILOG(Warning, Ops) << "The provided MonitorObjectCollection '" << moc->GetName() << "' contains at least one object with invalid validity!!!" << ENDM;
  }
  // directory level: mw
  auto mwDir = getOrCreateDirectory(mFile, movingWindowsDirectoryName);
  if (mwDir == nullptr) {
    ILOG(Error, Support) << "Could not create the directory '" << movingWindowsDirectoryName << "', skipping." << ENDM;
    return;
  }

  // directory level: mw/DET
  auto detector = moc->getDetector();
  auto detDir = getOrCreateDirectory(mwDir.get(), detector.c_str());
  if (detDir == nullptr) {
    ILOG(Error, Support) << "Could not create directory '" << detector << "', skipping." << ENDM;
    return;
  }

  // directory level: mw/DET/TASK
  auto taskDir = getOrCreateDirectory(detDir.get(), moc->getTaskName().c_str());
  if (taskDir == nullptr) {
    ILOG(Error, Support) << "Could not create directory '" << moc->getTaskName() << "', skipping." << ENDM;
    return;
  }

  // directory level: mw/DET/TASK/<mw_start_time>
  auto mocStorageName = std::to_string(earliestValidFrom(moc));
  moc->SetName(mocStorageName.c_str());
  ILOG(Info, Support) << "Checking for existing moving windows '" << mocStorageName << "' for task '" << detector << "/" << moc->getTaskName() << "' in the file." << ENDM;
  int nbytes = 0;
  auto storedMOC = std::unique_ptr<MonitorObjectCollection>(taskDir->Get<MonitorObjectCollection>(mocStorageName.c_str()));
  if (storedMOC != nullptr) {
    storedMOC->postDeserialization();
    ILOG(Info, Support) << "Merging moving windows '" << moc->GetName() << "' for task '" << moc->getDetector() << "/" << moc->getTaskName() << "' with the existing one in the file." << ENDM;
    storedMOC->merge(moc);
    nbytes = taskDir->WriteObject(storedMOC.get(), storedMOC->GetName(), "Overwrite");
  } else {
    ILOG(Info, Support) << "Storing moving windows '" << moc->GetName() << "' for task '" << moc->getDetector() << "/" << moc->getTaskName() << "' in the file." << ENDM;
    nbytes = taskDir->WriteObject(moc, moc->GetName(), "Overwrite");
  }
  ILOG(Info, Support) << "Moving windows '" << moc->GetName() << "' for task '" << detector << "/" << moc->getTaskName() << "' has been stored in the file (" << nbytes << " bytes)." << ENDM;
}

IntegralMocWalker::IntegralMocWalker(const RootFileStorage::DirectoryNode& rootNode)
{
  auto integralDirIt = rootNode.children.find(integralsDirectoryName);
  if (integralDirIt == rootNode.children.end()) {
    mPathIterator = mOrder.cbegin();
    return;
  }
  if (!std::holds_alternative<RootFileStorage::DirectoryNode>(integralDirIt->second)) {
    mPathIterator = mOrder.cbegin();
    return;
  }
  const auto& integralMocNode = std::get<RootFileStorage::DirectoryNode>(integralDirIt->second);
  std::stack<std::pair<const RootFileStorage::DirectoryNode&, child_iterator>> stack{};
  stack.push({ integralMocNode, integralMocNode.children.cbegin() });

  while (!stack.empty()) {
    auto& [currentNode, childIt] = stack.top();
    if (childIt == currentNode.children.end()) {
      // move to the next child of the parent node
      stack.pop();
      if (!stack.empty()) {
        stack.top().second++;
      }
    } else if (std::holds_alternative<RootFileStorage::DirectoryNode>(childIt->second)) {
      // move to a child of the current node
      const auto& childNode = std::get<RootFileStorage::DirectoryNode>(childIt->second);
      stack.push({ childNode, childNode.children.cbegin() });
    } else if (std::holds_alternative<RootFileStorage::MonitorObjectCollectionNode>(childIt->second)) {
      // move to the next child in the currentNode and return a path
      const auto& childNode = std::get<RootFileStorage::MonitorObjectCollectionNode>(childIt->second);
      ++childIt;
      mOrder.push_back(childNode.fullPath);
    } else {
      // unrecognized child node, move to the next child in the currentNode
      ++childIt;
    }
  }
  mPathIterator = mOrder.cbegin();
}

bool IntegralMocWalker::hasNextPath()
{
  return mPathIterator != mOrder.cend();
}

std::string IntegralMocWalker::nextPath()
{
  if (hasNextPath()) {
    return *mPathIterator++;
  }
  return {};
}

MovingWindowMocWalker::MovingWindowMocWalker(const RootFileStorage::DirectoryNode& rootNode)
{
  auto movingWindowDirIt = rootNode.children.find(movingWindowsDirectoryName);
  if (movingWindowDirIt == rootNode.children.end()) {
    mPathIterator = mOrder.cbegin();
    return;
  }
  if (!std::holds_alternative<RootFileStorage::DirectoryNode>(movingWindowDirIt->second)) {
    mPathIterator = mOrder.cbegin();
    return;
  }
  auto movingWindowMocNode = std::get<RootFileStorage::DirectoryNode>(movingWindowDirIt->second);

  std::stack<std::pair<const RootFileStorage::DirectoryNode&, child_iterator>> stack{};
  stack.push({ movingWindowMocNode, movingWindowMocNode.children.begin() });

  // we walk over all the MOCs in the tree and we save them in chronological order
  while (!stack.empty()) {
    auto& [currentNode, childIt] = stack.top();

    if (childIt == currentNode.children.end()) {
      // move to the next child of the parent node
      stack.pop();
      if (!stack.empty()) {
        stack.top().second++;
      }
    } else if (std::holds_alternative<RootFileStorage::DirectoryNode>(childIt->second)) {
      // move to a child of the current node
      auto& childNode = std::get<RootFileStorage::DirectoryNode>(childIt->second);
      stack.push({ childNode, childNode.children.begin() });
    } else if (std::holds_alternative<RootFileStorage::MonitorObjectCollectionNode>(childIt->second)) {
      // move to the next child in the currentNode and return a path
      auto timestamp = std::stoull(childIt->first);
      auto& childNode = std::get<RootFileStorage::MonitorObjectCollectionNode>(childIt->second);
      mOrder.emplace(timestamp, childNode.fullPath);
      childIt++;
    } else {
      // unrecognized child node, move to the next child in the currentNode
      childIt++;
    }
  }
  mPathIterator = mOrder.cbegin();
}

bool MovingWindowMocWalker::hasNextPath() const
{
  return mPathIterator != mOrder.cend();
}

std::string MovingWindowMocWalker::nextPath()
{
  if (hasNextPath()) {
    return (mPathIterator++)->second;
  }
  return {};
}

} // namespace o2::quality_control::core