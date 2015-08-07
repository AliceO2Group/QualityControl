//
// Created by flpprotodev on 7/10/15.
//

#ifndef QUALITY_CONTROL_QCINFOLOGGER_H
#define QUALITY_CONTROL_QCINFOLOGGER_H

#include "TaskInterface.h"
#include <iostream>
#include <InfoLogger.hxx>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

using namespace std;

class QcInfoLogger : public AliceO2::InfoLogger::InfoLogger
{

  public:

    static QcInfoLogger &GetInstance()
    {
      // Guaranteed to be destroyed. Instantiated on first use
      static QcInfoLogger foo;// *foo = new QcInfoLogger;
      return foo;
    }

  private:

    QcInfoLogger()
    {
      // TODO configure the QC infologger, e.g. proper facility
      *this << "QC infologger initialized" << AliceO2::InfoLogger::InfoLogger::endm;
    }

    virtual ~QcInfoLogger()
    {

    }

    // Disallow copying
    QcInfoLogger &operator=(const QcInfoLogger &) = delete;
    QcInfoLogger(const QcInfoLogger &) = delete;

};

}
}
}

#endif //QUALITY_CONTROL_BASICTASK_H
