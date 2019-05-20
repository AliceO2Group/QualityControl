from datetime import datetime
from datetime import timedelta
import logging

from Ccdb import Ccdb, ObjectVersion


def process(ccdb: Ccdb, object_path: str, delay: int):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    Objects who have been created recently are spared (delay is expressed in minutes).
    This specific policy, 1_per_hour, operates like this : take the first record, 
    delete everything for the next hour, find the next one, extend validity of the 
    previous record to match the next one, loop.

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''
    
    logging.debug(f"Plugin 1_per_hour processing {object_path}")

    versions = ccdb.getVersionsList(object_path)

    last_preserved: ObjectVersion = None
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    update_list: List[ObjectVersion] = []
    for v in versions:
        if last_preserved == None or last_preserved.validFromAsDatetime < v.validFromAsDatetime - timedelta(hours=1):
            # first extend validity of the previous preserved (should we take into account the run ?)
            if last_preserved != None:
                ccdb.updateValidity(last_preserved, last_preserved.validFrom, str(int(v.validFrom) - 1))
                update_list.append(last_preserved)
            last_preserved = v
            preservation_list.append(v)
        else:
            deletion_list.append(v)
            if v.validFromAsDatetime < datetime.now() - timedelta(minutes=delay):
                logging.debug(f"not in the grace period, we delete {v}")
                ccdb.deleteVersion(v)

    logging.debug("deleted : ")
    for v in deletion_list:
        logging.debug(f"   {v}")

    logging.debug("preserved : ")
    for v in preservation_list:
        logging.debug(f"   {v}")

    logging.debug("updated : ")
    for v in update_list:
        logging.debug(f"   {v}")

    return {"deleted" : len(deletion_list), "preserved": len(preservation_list), "updated" : len(update_list)}


def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "asdfasdf/example", 60)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
