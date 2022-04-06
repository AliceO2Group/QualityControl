from datetime import datetime
from datetime import timedelta
import logger
from typing import Dict

from Ccdb import Ccdb, ObjectVersion


logger = logging  # default logger


def process(ccdb: Ccdb, object_path: str, delay: int, #migration: bool,
            extra_params: Dict[str, str]):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    Objects who have been created recently are spared (delay is expressed in minutes).
    This specific policy, 1_per_hour, operates like this : take the first record, 
    delete everything for the next hour, find the next one, extend validity of the 
    previous record to match the next one, loop.

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    :param migration: whether the objects that have not been deleted should be migrated to the Grid. It does not apply to the objects spared because they are recent (see delay).
    :param extra_params: a dictionary containing extra parameters for this rule.
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''
    
    logger.debug(f"Plugin 1_per_hour processing {object_path}")

    versions = ccdb.getVersionsList(object_path)

    last_preserved: ObjectVersion = None
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    update_list: List[ObjectVersion] = []
    for v in versions:
        if last_preserved == None or last_preserved.validFromAsDt < v.validFromAsDt - timedelta(hours=1):
            # first extend validity of the previous preserved (should we take into account the run ?)
            if last_preserved != None:
                ccdb.updateValidity(last_preserved, last_preserved.validFrom, str(int(v.validFrom) - 1))
                update_list.append(last_preserved)
            last_preserved = v
        else:
            if v.validFromAsDt < datetime.now() - timedelta(minutes=delay):
                deletion_list.append(v)
                logger.debug(f"not in the grace period, we delete {v}")
                ccdb.deleteVersion(v)
            else:
                preservation_list.append(v)

    logger.debug("deleted : ")
    for v in deletion_list:
        logger.debug(f"   {v}")

    logger.debug("preserved : ")
    for v in preservation_list:
        logger.debug(f"   {v}")

    logger.debug("updated : ")
    for v in update_list:
        logger.debug(f"   {v}")

    return {"deleted" : len(deletion_list), "preserved": len(preservation_list), "updated" : len(update_list)}


def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "asdfasdf/example", 60)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
