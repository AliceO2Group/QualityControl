from collections import defaultdict
from datetime import datetime
from datetime import timedelta
import logging
from typing import Dict, DefaultDict, List

from qcrepocleaner.Ccdb import Ccdb, ObjectVersion
from qcrepocleaner.policies_utils import in_grace_period, group_versions

from qcrepocleaner import policies_utils

logger = logging  # default logger

def process(ccdb: Ccdb, object_path: str, delay: int,  from_timestamp: int, to_timestamp: int,
            extra_params: Dict[str, str]):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    Objects who have been created recently are spared (delay is expressed in minutes).

    This specific policy, multiple_per_run, operates like this : Keep the first and the last version of a run plus
    1 version every X minutes.

    Extra parameters:
      - migrate_to_EOS: Migrate the object to EOS. (default: false)
      - interval_between_versions: Period in minutes between the versions we will keep. (default: 90)
      - period_pass: Keep 1 version for a combination of run+pass+period if true. (default: false)

    It is implemented like this :
        Map of buckets: run[+pass+period] -> list of versions
        Go through all objects: Add the object to the corresponding key (run[+pass+period])
        Remove the empty run from the map (we ignore objects without a run)
        Go through the map: for each run (resp. run+pass+period)
            Get SOR (validity of first object)
            if SOR < now - delay
                do
                    keep first
                    delete everything for the next interval_between_versions
                until no more
                move the last one, from delete to preserve

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: delay since SOR after which we process the run altogether.
    :param from_timestamp: only objects created after this timestamp are considered.
    :param to_timestamp: only objects created before this timestamp are considered.
    :param extra_params: a dictionary containing extra parameters for this rule.
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''
    
    logger.debug(f"Plugin multiple_per_run processing {object_path}")

    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    update_list: List[ObjectVersion] = []
    versions_buckets_dict: DefaultDict[str, List[ObjectVersion]] = defaultdict(list)
    metadata_for_preservation = {'preservation': 'true'}

    # config parameters
    period_pass = (extra_params.get("period_pass", False) is True)
    logger.debug(f"period_pass : {period_pass}")
    interval_between_versions = int(extra_params.get("interval_between_versions", 90))
    logger.debug(f"interval_between_versions : {interval_between_versions}")
    migrate_to_EOS = (extra_params.get("migrate_to_EOS", False) is True)
    logger.debug(f"migrate_to_EOS : {migrate_to_EOS}")

    # Find all the runs and group the versions (by run or by a combination of multiple attributes)
    policies_utils.group_versions(ccdb, object_path, period_pass, versions_buckets_dict)

    # Remove the empty run from the map (we ignore objects without a run)
    logger.debug(f"Number of buckets : {len(versions_buckets_dict)}")
    if not period_pass:
        logger.debug(f"Number of versions without runs : {len(versions_buckets_dict['none'])}")
        del versions_buckets_dict['none']

    # for each run or combination of pass and run
    for bucket, run_versions in versions_buckets_dict.items():
        logger.debug(f"- bucket {bucket}")

        # Get SOR (validity of first object) and check if it is in the grace period and in the allowed period
        first_object = run_versions[0]
        if policies_utils.in_grace_period(first_object, delay):
            logger.debug(f"     in grace period, skip this bucket")
            preservation_list.extend(run_versions)
        elif not (from_timestamp < first_object.createdAt < to_timestamp):  # in the allowed period
            logger.debug(f"     not in the allowed period, skip this bucket")
            preservation_list.extend(run_versions)
        else:
            logger.debug(f"     not in the grace period")

            last_preserved: ObjectVersion = None
            for v in run_versions:
                logger.debug(f"process {v}")

                # first or next after the period, or last one --> preserve
                if last_preserved is None or \
                        last_preserved.createdAtDt < v.createdAtDt - timedelta(minutes=interval_between_versions) or \
                        v == run_versions[-1]:
                    logger.debug(f" --> preserve")
                    last_preserved = v
                    if migrate_to_EOS:
                        ccdb.updateValidity(v, v.validFrom, v.validTo, metadata_for_preservation)
                    preservation_list.append(last_preserved)
                else:  # in between period --> delete
                    logger.debug(f" --> delete")
                    deletion_list.append(v)
                    ccdb.deleteVersion(v)

    logger.debug(f"deleted ({len(deletion_list)}) : ")
    for v in deletion_list:
        logger.debug(f"   {v}")

    logger.debug(f"preserved ({len(preservation_list)}) : ")
    for v in preservation_list:
        logger.debug(f"   {v}")

    logger.debug(f"updated ({len(update_list)}) : ")
    for v in update_list:
        logger.debug(f"   {v}")

    return {"deleted": len(deletion_list), "preserved": len(preservation_list), "updated": len(update_list)}


def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "asdfasdf/example", 60)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
