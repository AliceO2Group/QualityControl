from datetime import datetime
from datetime import timedelta
import logging
from collections import defaultdict
from qcrepocleaner.Ccdb import Ccdb, ObjectVersion
import dryable
from typing import Dict, List


logger = logging  # default logger


def in_grace_period(version: ObjectVersion, delay: int):
    return not (version.validFromAsDt < datetime.now() - timedelta(minutes=delay))


def get_run(v: ObjectVersion) -> str:
    run = "none"
    if "Run" in v.metadata:
        run = str(v.metadata['Run'])
    elif "RunNumber" in v.metadata:
        run = str(v.metadata['RunNumber'])
    return run


def process(ccdb: Ccdb, object_path: str, delay: int,  from_timestamp: int, to_timestamp: int, extra_params: Dict[str, str]):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    Objects which have been created recently are spared (delay is expressed in minutes).
    This specific policy, 1_per_run, keeps only the most recent version for a given run based on the validity_from.

    Config Parameters:
    - period_pass: Keep 1 version for a combination of run+pass+period if true.
    - delete_when_no_run: Versions without runs are preserved if delete_when_no_run is set to false (default).
      Otherwise only the last one is preserved.
    THEY CANNOT BE BOTH TRUE AT THE SAME TIME

    It is implemented like this :
    Map of buckets: run[+pass+period] -> list of versions
    Go through all objects: Add the object to the corresponding key (run[+pass+period])
    If delete_when_no_run is false, remove the versions without run from the map
    Go through the map: for each run (resp. run+pass+period) keep the most recent object and delete the rest.

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    :param from_timestamp: only objects created after this timestamp are considered.
    :param to_timestamp: only objects created before this timestamp are considered.
    :param extra_params: a dictionary containing extra parameters for this rule.
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''
    
    logger.debug(f"Plugin 1_per_run processing {object_path}")

    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    update_list: List[ObjectVersion] = []
    versions_buckets_dict: DefaultDict[str, List[ObjectVersion]] = defaultdict(list)

    # config parameters
    delete_when_no_run = (extra_params.get("delete_when_no_run", False) == True)
    logger.debug(f"delete_when_no_run : {delete_when_no_run}")
    period_pass = (extra_params.get("period_pass", False) == True)
    logger.debug(f"period_pass : {period_pass}")
    if delete_when_no_run == True and period_pass == True:
        logger.error(f"1_per_run does not allow both delete_when_no_run and period_pass to be on at the same time")
        return {"deleted": 0, "preserved": 0, "updated": 0}

    # Find all the runs and group the versions (by run or by a combination of multiple attributes)
    versions = ccdb.getVersionsList(object_path)
    for v in versions:
        logger.debug(f"Assigning {v} to a bucket")
        run = get_run(v)
        period_name = v.metadata.get("PeriodName") or ""
        pass_name = v.metadata.get("PassName") or ""
        key = run+period_name+pass_name if period_pass else run
        versions_buckets_dict[key].append(v)

    logger.debug(f"Number of buckets : {len(versions_buckets_dict)}")
    if not period_pass:
        logger.debug(f"Number of versions without runs : {len(versions_buckets_dict['none'])}")

    # if we should not touch the files with no runs, let's just remove them from the map
    if not delete_when_no_run:
        del versions_buckets_dict['none']

    # Dispatch the versions to deletion and preservation lists
    for bucket, run_versions in versions_buckets_dict.items():
        # logger.debug(f"- run {r}")

        freshest: ObjectVersion = None
        for v in run_versions:
            if freshest is None or freshest.validFromAsDt < v.validFromAsDt:
                if freshest is not None:
                    if in_grace_period(freshest, delay):
                        preservation_list.append(freshest)
                    else:
                        deletion_list.append(freshest)
                freshest = v
            else:
                if in_grace_period(freshest, delay):
                    preservation_list.append(v)
                else:
                    deletion_list.append(v)
        preservation_list.append(freshest)

    # Actual deletion
    logger.debug(f"Delete but preserve versions that are not in the period passed to the policy")
    temp_deletion_list: List[ObjectVersion] = []
    for v in deletion_list:
        if from_timestamp < v.validFrom < to_timestamp:  # in the allowed period
            temp_deletion_list.append(v)   # we will delete any ways
            ccdb.deleteVersion(v)
        else:
            preservation_list.append(v)    # we preserve
    deletion_list = temp_deletion_list

    logger.debug(f"deleted ({len(deletion_list)}) : ")
    for v in deletion_list:
        logger.debug(f"   {v}")

    logger.debug(f"preserved ({len(preservation_list)}) : ")
    for v in preservation_list:
        logger.debug(f"   {v}")

    logger.debug(f"updated ({len(update_list)}) : ")
    for v in update_list:
        logger.debug(f"   {v}")

    return {"deleted" : len(deletion_list), "preserved": len(preservation_list), "updated" : len(update_list)}


def main():
    logger.getLogger().setLevel(int(10))
    dryable.set( True )
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "qc/testRunCleanup", 0)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
