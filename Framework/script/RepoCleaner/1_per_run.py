from datetime import datetime
from datetime import timedelta
import logging
from collections import defaultdict
from Ccdb import Ccdb, ObjectVersion
import dryable

def process(ccdb: Ccdb, object_path: str, delay: int):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    Objects who have been created recently are spared (delay is expressed in minutes).
    This specific policy, 1_per_run, keeps only the most recent version for a given run based on the validity_from.

    It is implemented like this :
    Go through all objects, if a run is set add the object to the corresponding map element.
    Go through the map, for each run keep the most recent object and delete the rest.

    TODO: how to handle the validity period ?

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''
    
    logging.debug(f"Plugin 1_per_run processing {object_path}")

    versions = ccdb.getVersionsList(object_path)

    last_preserved: ObjectVersion = None
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    update_list: List[ObjectVersion] = []
    runs_dict: DefaultDict[str, List[ObjectVersion]] = defaultdict(list)

    # Find all the runs and group the versions
    for v in versions:
        print(f"Processing {v}")

        if "Run" in v.metadata:
            print(f"Run : {v.metadata['Run']}")
            runs_dict[v.metadata['Run']].append(v)

    logging.debug(f"Number of runs : {len(runs_dict)}")

    # Dispatch the versions to deletion and preservation lists
    for r, run_versions in runs_dict.items():
        logging.debug(f"- run {r}")

        freshest: ObjectVersion = None
        for v in run_versions:
            logging.debug(f"  - version {v}")
            if freshest is None or freshest.validFromAsDatetime < v.validFromAsDatetime:
                if freshest != None:
                    deletion_list.append(freshest)
                freshest = v
            else:
                deletion_list.append(v)
        preservation_list.append(freshest)

    # actual deletion
    for d in deletion_list:
        if d.validFromAsDatetime < datetime.now() - timedelta(minutes=delay):
            logging.debug(f"not in the grace period, we delete {d}")
            ccdb.deleteVersion(d)
        else:
            logging.debug(f"in the grace period, we spare {d}")

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
