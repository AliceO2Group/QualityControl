///
/// \file   InfrastructureGenerator.h
/// \author Piotr Konopka
///

#ifndef QC_CORE_INFRASTRUCTUREGENERATOR_H
#define QC_CORE_INFRASTRUCTUREGENERATOR_H

#include <string>
#include <Framework/WorkflowSpec.h>

namespace o2::quality_control
{
namespace core
{

/// \brief A factory class which can generate QC topologies given a configuration file.
///
/// A factory class which can generate QC topologies given a configuration file (example in Framework/basic.json and
/// Framework/example-default.json). As QC topologies will be spread on both processing chain machines and dedicated
/// QC servers, a _local_ vs. _remote_ distinction was introduced. Tasks which are _local_ should have taskRunners
/// placed on FLP or EPN machines and their results should be merged and checked on QC servers. The 'remote' option
/// means, that full QC chain should be located on remote (QC) machines. For the laptop development, use 'remote' tasks
/// and generateRemoteInfrastructure() to obtain the full topology in one go.
///
/// \author Piotr Konopka
class InfrastructureGenerator
{
 public:
  InfrastructureGenerator() = delete;

  /// \brief Generates the local part of the QC infrastructure for a specified host.
  ///
  /// Generates the local part of the QC infrastructure for a specified host - taskRunners which are declared in the
  /// configuration to be 'local'.
  ///
  /// \param configurationSource - full path to configuration file, preceded with the backend (f.e. "json://")
  /// \param host - name of the machine
  /// \return generated local QC workflow
  static framework::WorkflowSpec generateLocalInfrastructure(std::string configurationSource, std::string host);

  /// \brief Generates the local part of the QC infrastructure for a specified host.
  ///
  /// Generates the local part of the QC infrastructure for a specified host - taskRunners which are declared in the
  /// configuration to be 'local'.
  ///
  /// \param workflow - existing workflow where QC infrastructure should be placed
  /// \param configurationSource - full path to configuration file, preceded with the backend (f.e. "json://")
  /// \param host - name of the machine
  /// \return generated local QC workflow
  static void generateLocalInfrastructure(framework::WorkflowSpec& workflow, std::string configurationSource, std::string host);

  /// \brief Generates the remote part of the QC infrastructure.
  ///
  /// Generates the remote part of the QC infrastructure - mergers and checkers for 'local' tasks and full QC chain for
  /// 'remote' tasks.
  ///
  /// \param configurationSource - full path to configuration file, preceded with the backend (f.e. "json://")
  /// \return generated remote QC workflow
  static o2::framework::WorkflowSpec generateRemoteInfrastructure(std::string configurationSource);

  /// \brief Generates the remote part of the QC infrastructure.
  ///
  /// Generates the remote part of the QC infrastructure - mergers and checkers for 'local' tasks and full QC chain for
  /// 'remote' tasks.
  ///
  /// \param workflow - existing workflow where QC infrastructure should be placed
  /// \param configurationSource - full path to configuration file, preceded with the backend (f.e. "json://")
  /// \return generated remote QC workflow
  static void generateRemoteInfrastructure(framework::WorkflowSpec& workflow, std::string configurationSource);
};

} // namespace core

// exposing the class above as a main QC interface, syntactic sugar

inline framework::WorkflowSpec generateLocalInfrastructure(std::string configurationSource, std::string host)
{
  return core::InfrastructureGenerator::generateLocalInfrastructure(configurationSource, host);
}

inline void generateLocalInfrastructure(framework::WorkflowSpec& workflow, std::string configurationSource, std::string host)
{
  core::InfrastructureGenerator::generateLocalInfrastructure(workflow, configurationSource, host);
}

inline framework::WorkflowSpec generateRemoteInfrastructure(std::string configurationSource)
{
  return core::InfrastructureGenerator::generateRemoteInfrastructure(configurationSource);
}

inline void generateRemoteInfrastructure(framework::WorkflowSpec& workflow, std::string configurationSource)
{
  core::InfrastructureGenerator::generateRemoteInfrastructure(workflow, configurationSource);
}

} // namespace o2::quality_control

#endif //QC_CORE_INFRASTRUCTUREGENERATOR_H
