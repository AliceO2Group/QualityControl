from Ccdb import Ccdb, ObjectVersion
from datetime import timedelta
from datetime import datetime
import logging


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
    '''
    
    logging.debug(f"Plugin 1_per_hour processing {object_path}")

    versions = ccdb.getVersionsList(object_path)

    last_preserved: ObjectVersion = None
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    for v in versions:
        if last_preserved == None or last_preserved.validFrom < v.validFrom - timedelta(hours=1):
            # first extend validity of the previous preserved (should we take into account the run ?)
            if last_preserved != None:
                ccdb.updateValidity(last_preserved, last_preserved.validFromTimestamp, str(int(v.validFromTimestamp) - 1))
            last_preserved = v
            preservation_list.append(v)
        else:
            deletion_list.append(v)
            if v.validFrom < datetime.now() - timedelta(minutes=delay):
                logging.debug(f"not in the grace period, we delete {v}")
                ccdb.deleteVersion(v)

    logging.debug("deleted : ")
    for v in deletion_list:
        logging.debug(f"   {v}")

    logging.debug("preserved : ")
    for v in preservation_list:
        logging.debug(f"   {v}")

def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "asdfasdf/example", 60)

if __name__== "__main__": # to be able to run the test code above when not imported.
    main()