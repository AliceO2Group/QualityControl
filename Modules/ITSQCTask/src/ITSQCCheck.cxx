///
/// \file   SkeletonCheck.cxx
/// \author Piotr Konopka
///

#include "ITSQCTask/ITSQCCheck.h"

// ROOT
#include <FairLogger.h>
#include <TH1.h>
#include <TPaveText.h>

using namespace std;

namespace o2
{
	namespace quality_control_modules
	{
		namespace itsqctask
		{

			ITSQCCheck::ITSQCCheck() {}

			ITSQCCheck::~ITSQCCheck() {}

			void ITSQCCheck::configure(std::string) {}

			Quality ITSQCCheck::check(const MonitorObject* mo)
			{
				Quality result = Quality::Null;


				LOG(INFO) << "Object Name = " << mo->getName(); 

				if (mo->getName() == "ChipProj") {
					auto* h = dynamic_cast<TH1D*>(mo->getObject());
					LOG(INFO) << "NBin = " << h->Integral(); 
					result = Quality::Good;

					for (int i = 0; i < h->GetNbinsX(); i++) {
						if (i > 0 && i < 8 && h->GetBinContent(i) == 0) {
							result = Quality::Bad;
							break;
						} else if ((i == 0 || i > 7) && h->GetBinContent(i) > 0) {
							result = Quality::Medium;
						}
					}
				}
				return result;
			}

			std::string ITSQCCheck::getAcceptedType() { return "TH1"; }

			void ITSQCCheck::beautify(MonitorObject* mo, Quality checkResult)
			{
				LOG(INFO) << "Object Name" << mo->getName(); 

				if (mo->getName() == "ChipProj") {
					auto* h = dynamic_cast<TH1D*>(mo->getObject());

					if (checkResult == Quality::Good) {
						h->SetFillColor(kGreen);
					} else if (checkResult == Quality::Bad) {
						LOG(INFO) << "Quality::Bad, setting to red";
						h->SetFillColor(kRed);
					} else if (checkResult == Quality::Medium) {
						LOG(INFO) << "Quality::medium, setting to orange";
						h->SetFillColor(kOrange);
					}
					h->SetLineColor(kBlack);
				}
			}

		} // namespace skeleton
	} // namespace quality_control_modules
} // namespace o2
