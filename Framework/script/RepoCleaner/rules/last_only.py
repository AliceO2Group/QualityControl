from datetime import datetime
from datetime import timedelta
import logging
from typing import Dict

from Ccdb import Ccdb, ObjectVersion


def process(ccdb: Ccdb, object_path: str, delay: int, extra_params: Dict[str, str]):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    
    Only the last version of each object is preserved. Grace period is respected.

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    :param extra_params: a dictionary containing extra parameters (unused in this rule)
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''
    
    logging.debug(f"Plugin last_only processing {object_path}")

    versions = ccdb.getVersionsList(object_path)

    earliest: ObjectVersion = None
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    # find the earliest
    for v in versions:
        if earliest == None or v.validFromAsDatetime > earliest.validFromAsDatetime:
            earliest = v
    logging.debug(f"earliest : {earliest}")
            
    # delete the non-earliest if we are not in the grace period
    for v in versions:
        if v == earliest:
            preservation_list.append(v)
            continue
        
        if v.validFromAsDatetime < datetime.now() - timedelta(minutes=delay):
            deletion_list.append(v)
            ccdb.deleteVersion(v)
        else:
            preservation_list.append(v)

    logging.debug("deleted : ")
    for v in deletion_list:
        logging.debug(f"   {v}")

    logging.debug("preserved : ")
    for v in preservation_list:
        logging.debug(f"   {v}")
        
    return {"deleted" : len(deletion_list), "preserved": len(preservation_list)}

    
def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "asdfasdf/example", 60)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
