///
/// \file   TObject2JsonServer.cxx
/// \author Adam Wegrzynek
///

// TObject2Json
#include "TObject2JsonFactory.h"
#include <boost/program_options.hpp>

using o2::quality_control::tobject_to_json::TObject2JsonFactory;

int main(int argc, char *argv[])
{
  boost::program_options::variables_map vm;
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("backend", boost::program_options::value<std::string>()->required(), "Backend URL, eg.: mysql://<loign>:<password>@<hostname>:<port>/<database>")
    ("zeromq-server", boost::program_options::value<std::string>()->required(), "ZeroMQ server endpoint, eg.: tcp://<host>:<port>")
  ;
  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);
  } catch (...) {
    std::cout << desc << std::endl;
    return 1;
  }
  std::string backend = vm["backend"].as<std::string>();
  std::string zeromq = vm["zeromq-server"].as<std::string>();

  auto converter = TObject2JsonFactory::Get(backend, zeromq);
  converter->start();
  return 0;
}
