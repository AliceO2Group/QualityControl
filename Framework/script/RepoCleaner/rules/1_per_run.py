from datetime import datetime
from datetime import timedelta
import logging
from collections import defaultdict
from Ccdb import Ccdb, ObjectVersion
import dryable
from typing import Dict

def in_grace_period(version: ObjectVersion, delay: int):
    return not (version.validFromAsDatetime < datetime.now() - timedelta(minutes=delay))

def process(ccdb: Ccdb, object_path: str, delay: int, extra_params: Dict[str, str]):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    Objects which have been created recently are spared (delay is expressed in minutes).
    This specific policy, 1_per_run, keeps only the most recent version for a given run based on the validity_from.

    It is implemented like this :
    Go through all objects: if a run is set, add the object to the corresponding map element.
    Go through the map: for each run keep the most recent object and delete the rest.
    Files without runs are preserved if delete_when_no_run is set to false. Otherwise only the last one is preserved.

    TODO: how to handle the validity period ?

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    :param extra_params: a dictionary containing extra parameters for this rule.
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''
    
    logging.debug(f"Plugin 1_per_run processing {object_path}")

    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    update_list: List[ObjectVersion] = []
    runs_dict: DefaultDict[str, List[ObjectVersion]] = defaultdict(list)

    delete_when_no_run = (extra_params.get("delete_when_no_run", False) == True)
    logging.debug(f"delete_when_no_run : {delete_when_no_run}")

    # Find all the runs and group the versions
    versions = ccdb.getVersionsList(object_path)
    for v in versions:
        logging.debug(f"Processing {v}")
        if "Run" in v.metadata:
            runs_dict[v.metadata['Run']].append(v)
        else:
            runs_dict[-1].append(v) # the ones with no run specified

    logging.debug(f"Number of runs : {len(runs_dict)}")
    logging.debug(f"Number of versions without runs : {len(runs_dict[-1])}")

    # if we should not touch the files with no runs, let's just remove them from the map
    if not delete_when_no_run:
        del runs_dict[-1]

    # Dispatch the versions to deletion and preservation lists
    for r, run_versions in runs_dict.items():
        # logging.debug(f"- run {r}")

        freshest: ObjectVersion = None
        for v in run_versions:
            # logging.debug(f"  - version {v}")
            if freshest is None or freshest.validFromAsDatetime < v.validFromAsDatetime:
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

    # actual deletion
    for d in deletion_list:
        ccdb.deleteVersion(d)

    logging.debug(f"deleted ({len(deletion_list)}) : ")
    for v in deletion_list:
        logging.debug(f"   {v}")

    logging.debug(f"preserved ({len(preservation_list)}) : ")
    for v in preservation_list:
        logging.debug(f"   {v}")

    logging.debug(f"updated ({len(update_list)}) : ")
    for v in update_list:
        logging.debug(f"   {v}")

    return {"deleted" : len(deletion_list), "preserved": len(preservation_list), "updated" : len(update_list)}


def main():
    logging.getLogger().setLevel(int(10))
    dryable.set( True )
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "qc/testRunCleanup", 0)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
