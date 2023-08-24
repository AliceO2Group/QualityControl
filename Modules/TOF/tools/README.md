# checkEff.C
# void checkEff(const char *fn1="QC.root", const char *fn2="QC.root", long ts=1660430519000);
# this macro allow to check on QC the efficiency strip by strip using tracks with pT > 2 GeV/c
# strip efficiencies are also corrected for all the features anchored in MC: acceptance/dead channels, decoding errors, problematic channels from calibrations 
# after correction we expect efficiency > 60% for all strips enabled -> this would guarantee we are able to anchor
# ARGUMENTS: file with digit QC, file with matching QC (different only in MC), SOR timestamp for the calibration object
