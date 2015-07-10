//
// Created by flpprotodev on 7/9/15.
//

#ifndef QUALITY_CONTROL_EXCEPTIONS_H
#define QUALITY_CONTROL_EXCEPTIONS_H

#include <boost/exception/all.hpp>
#include <exception>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

// Definitions of error_info structures to store extra info the boost way.
typedef boost::error_info<struct errinfo_object_name_, std::string> errinfo_object_name;

struct ExceptionBase : virtual std::exception, virtual boost::exception
{
};

struct ObjectNotFoundError : virtual ExceptionBase
{
  virtual const char *what() const noexcept override
  {
    return "Object not found error";
  }
};

// we could define further exceptions this way :
//    struct io_error: virtual exception_base { };
//    struct file_error: virtual io_error { };
//    struct read_error: virtual io_error { };
//    struct file_read_error: virtual file_error, virtual read_error { };
// Note that we don't add members because boost exceptions allow adding arbitrary stuff any ways.
// See above for adding specific information but in a generic way.

}
}
}

#endif //QUALITY_CONTROL_EXCEPTIONS_H
