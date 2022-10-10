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
/// \file    runFileMerger.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable which reads MonitorObjectCollections from files and creates a file with the merged result.

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/MonitorObjectCollection.h"

#include <string>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <TFile.h>
#include <TKey.h>
#include <TGrid.h>
#include <variant>

namespace bpo = boost::program_options;
using namespace o2::quality_control::core;

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct Node {
  std::string pathTo{};
  std::string name{};
  std::map<std::string, std::variant<Node, MonitorObjectCollection*>> children = {};

  std::string getFullPath() const { return pathTo + std::filesystem::path::preferred_separator + name; }
};

int main(int argc, const char* argv[])
{
  size_t filesRead = 0;
  try {
    bpo::options_description desc{ "Options" };
    desc.add_options()                                                                                                                                                         //
      ("help,h", "Help message")                                                                                                                                               //
      ("enable-alien", bpo::bool_switch()->default_value(false), "Connect to alien before accessing input files.")                                                             //
      ("exit-on-error", bpo::bool_switch()->default_value(false), "Makes the executable exit if any of the input files could not be read.")                                    //
      ("output-file", bpo::value<std::string>()->default_value("merged.root"), "File path to store the merged results, if the file exists, it will be merged with new files.") //
      ("input-files-list", bpo::value<std::string>()->default_value(""), "Path to a file containing a list of input files (row by row)")                                       //
      ("input-files", bpo::value<std::vector<std::string>>()->multitoken(), "Space-separated file paths which should be merged.")                                              //
      ("exclude-directories", bpo::value<std::vector<std::string>>()->multitoken(), "Space-separated directories which should be excluded when merging files.");

    bpo::variables_map vm;
    store(bpo::command_line_parser(argc, argv).options(desc).run(), vm);
    notify(vm);

    QcInfoLogger::setFacility("runFileMerger");

    if (vm.count("help")) {
      ILOG(Info, Support) << desc << ENDM;
      return 0;
    } else if (vm.count("input-files") > 0 && !vm["input-files-list"].as<std::string>().empty()) {
      ILOG(Error, Support) << "One should use either --input-files-list or --input-files, but not both." << ENDM;
      return 1;
    } else if (vm.count("input-files") == 0 && vm["input-files-list"].as<std::string>().empty()) {
      ILOG(Error, Support) << "No input files were provided. Use either --input-files-list or --input-files." << ENDM;
      return 1;
    }

    if (vm.count("help")) {
      // no infologger here, because the message is too long.
      std::cout << desc << std::endl;
      return 0;
    }

    std::vector<std::string> inputFilePaths;
    if (vm.count("input-files") > 0) {
      inputFilePaths = vm["input-files"].as<std::vector<std::string>>();
    } else if (auto inputFileList = vm["input-files-list"].as<std::string>(); !inputFileList.empty()) {
      std::ifstream file(inputFileList);
      if (!file.is_open()) {
        ILOG(Error, Support) << "Could not open the file with input list: " << inputFileList << ENDM;
        return 1;
      }
      for (std::string line; std::getline(file, line);) {
        inputFilePaths.push_back(line);
      }
    }

    if (vm["enable-alien"].as<bool>()) {
      ILOG(Info, Support) << "Connecting to alien" << ENDM;
      TGrid::Connect("alien:");
    }

    auto excludedDirectories = vm.count("exclude-directories") > 0 ? vm["exclude-directories"].as<std::vector<std::string>>() : std::vector<std::string>();
    if (!excludedDirectories.empty()) {
      ILOG(Info, Support) << "Will skip the following directories inside input files:";
      for (const auto& dir : excludedDirectories) {
        ILOG(Info, Support) << " " << dir;
      }
      ILOG(Info, Support) << ENDM;
    }

    auto handleError = vm["exit-on-error"].as<bool>()
                         ? [](const std::string& message) { throw std::runtime_error(message); }
                         : [](const std::string& message) { ILOG(Error, Support) << message << ENDM; };

    auto outputFilePath = vm["output-file"].as<std::string>();
    auto outputFile = new TFile(outputFilePath.c_str(), "UPDATE");
    if (outputFile->IsZombie()) {
      throw std::runtime_error("File '" + outputFilePath + "' is zombie.");
    }
    if (!outputFile->IsOpen()) {
      throw std::runtime_error("Failed to open the file: " + outputFilePath);
    }
    if (!outputFile->IsWritable()) {
      throw std::runtime_error("File '" + outputFilePath + "' is not writable.");
    }
    ILOG(Debug) << "Output file '" << outputFilePath << "' successfully open." << ENDM;

    // Unlike in RootFileSink and RootFileSource, where we assume that the latter only supports the output of the first,
    // here we have more relaxed assumptions and try to recursively merge everything, regardless of the directory structure.
    // This is because we might have to change the structure again when we support moving windows, so we might save some work
    // in the future.
    // We choose to keep the merged file structure in memory and merge everything we can before storing in a file.
    // If this becomes too memory-hungry, it could be rewritten to store anything merge immediately at the cost of
    // more I/O operations.
    Node mergedTree;
    for (const auto& inputFilePath : inputFilePaths) {
      auto* file = TFile::Open(inputFilePath.c_str(), "READ");
      if (file == nullptr) {
        handleError("File handler for '" + inputFilePath + "' is nullptr.");
        continue;
      }
      if (file->IsZombie()) {
        handleError("File '" + inputFilePath + "' is zombie.");
        continue;
      }
      if (!file->IsOpen()) {
        handleError("Failed to open the file: " + inputFilePath);
        continue;
      }
      ILOG(Debug) << "Input file '" << inputFilePath << "' successfully open." << ENDM;

      std::function<void(TDirectory*, Node&, const std::vector<std::string>&)> mergeRecursively;
      mergeRecursively = [&](TDirectory* fileNode, Node& memoryNode, const std::vector<std::string>& excludedDirectories = {}) {
        if (fileNode == nullptr) {
          ILOG(Error) << "Provided parentNode pointer is null, skipping." << ENDM;
          return;
        }
        TIter next(fileNode->GetListOfKeys());
        TKey* key;
        while ((key = (TKey*)next())) {
          // we look for exact matches here. we skip if there are no subdirectories
          if (std::find(excludedDirectories.begin(), excludedDirectories.end(), key->GetName()) != excludedDirectories.end()) {
            ILOG(Info, Support) << "Skipping '" << key->GetName() << "' as requested in the input arguments" << ENDM;
            continue;
          }
          // we check if we have to skip any subdirectories wrt where we are
          std::vector<std::string> excludedSubdirectories;
          for (const auto& excludedDirectory : excludedDirectories) {
            auto match = std::string(key->GetName()) + '/';
            if (excludedDirectory.find(match) == 0) {
              if (excludedDirectory.size() < match.size()) {
                ILOG(Warning, Support) << "Invalid exclusion path '" << excludedDirectory << "'" << ENDM;
                continue;
              }
              excludedSubdirectories.push_back(excludedDirectory.substr(match.size()));
            }
          }

          ILOG(Debug, Devel) << "Getting the value for key '" << key->GetName() << "'" << ENDM;
          auto* value = fileNode->Get(key->GetName());
          if (value == nullptr) {
            ILOG(Error) << "Could not get the value '" << key->GetName() << "', skipping." << ENDM;
            continue;
          }
          if (auto inputMOC = dynamic_cast<MonitorObjectCollection*>(value)) {
            inputMOC->postDeserialization();

            if (memoryNode.children.count(inputMOC->GetName()) == 0) {
              std::string mocPath = memoryNode.getFullPath() + std::filesystem::path::preferred_separator + key->GetName();

              auto mergedTObj = outputFile->Get(mocPath.c_str());
              if (mergedTObj != nullptr) {
                auto mergedMOC = dynamic_cast<MonitorObjectCollection*>(mergedTObj);
                if (mergedMOC == nullptr) {
                  handleError("Could not cast the merged object to MonitorObjectCollection, skipping.");
                  delete mergedMOC;
                  continue;
                }
                mergedMOC->postDeserialization();
                memoryNode.children[mergedMOC->GetName()] = mergedMOC;
                ILOG(Info) << "Read merged object '" << mergedMOC->GetName() << "'" << ENDM;
              }
            }

            if (memoryNode.children.count(inputMOC->GetName())) {
              try {
                std::get<MonitorObjectCollection*>(memoryNode.children[inputMOC->GetName()])->merge(inputMOC);
              } catch (...) {
                handleError("Failed to merge the Monitor Object Collection. Exception caught: " + boost::current_exception_diagnostic_information(true));
              }
              delete inputMOC;
            } else {
              memoryNode.children[inputMOC->GetName()] = inputMOC;
            }
          } else if (auto dir = dynamic_cast<TDirectory*>(value)) {
            auto name = dir->GetName();
            if (memoryNode.children.count(name) == 0) {
              memoryNode.children[name] = Node{ memoryNode.getFullPath(), name };
            }
            mergeRecursively(dir, std::get<Node>(memoryNode.children[name]), excludedSubdirectories);
          } else {
            handleError("Could not cast the node to MonitorObjectCollection nor TDirectory.");
            delete value;
            continue;
          }
        }
      };

      mergeRecursively(file, mergedTree, excludedDirectories);
      file->Close();
      delete file;
      filesRead++;
    }

    std::function<void(TDirectory*, const Node&)> storeRecursively;
    storeRecursively = [&](TDirectory* fout, const Node& memoryNode) {
      for (const auto& [name, value] : memoryNode.children) {
        std::visit(overloaded{
                     [&](const Node& node) {
                       auto* dir = fout->GetDirectory(node.name.c_str());
                       if (dir == nullptr) {
                         fout->mkdir(node.name.c_str());
                       }
                       dir = fout->GetDirectory(node.name.c_str());
                       if (dir == nullptr) {
                         handleError("Could not create directory '" + node.name + "' in path '" + node.pathTo + "'");
                       } else {
                         storeRecursively(dir, node);
                       }
                     },
                     [&](const MonitorObjectCollection* moc) {
                       fout->WriteObject(moc, moc->GetName(), "Overwrite");
                       delete moc;
                     } },
                   value);
      }
    };
    storeRecursively(outputFile, mergedTree);
    outputFile->Close();

  } catch (const bpo::error& ex) {
    ILOG(Error, Ops) << "Exception caught: " << ex.what() << ENDM;
    return 1;
  } catch (const boost::exception& ex) {
    ILOG(Error, Ops) << "Exception caught: " << boost::current_exception_diagnostic_information(true) << ENDM;
    return 1;
  }

  if (filesRead > 0) {
    ILOG(Info, Support) << "Successfully merged " << filesRead << " files into one." << ENDM;
  } else {
    ILOG(Info, Support) << "No files were merged." << ENDM;
  }
  return 0;
}