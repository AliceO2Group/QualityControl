from datetime import datetime
from datetime import timedelta
import logging
from typing import Dict

from qcrepocleaner.Ccdb import Ccdb, ObjectVersion


logger = logging  # default logger


def process(ccdb: Ccdb, object_path: str, delay: int,  from_timestamp: int, to_timestamp: int, extra_params: Dict[str, str]):
    '''
    Process this deletion rule on the object. We use the CCDB passed by argument.
    
    This policy deletes everything after `delay` minutes.

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period in minutes during which a new object is never deleted.
    :param from_timestamp: only objects created after this timestamp are considered.
    :param to_timestamp: only objects created before this timestamp are considered.
    :param extra_params: a dictionary containing extra parameters (unused in this rule)
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    '''
    
    logger.debug(f"Plugin 'none' processing {object_path}")

    versions = ccdb.getVersionsList(object_path)
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []

    for v in versions:
        if v.validFromAsDt < datetime.now() - timedelta(minutes=delay):  # grace period
            logger.debug(f"not in the grace period")
            if from_timestamp < v.validFrom < to_timestamp:  # in the allowed period
                logger.debug(f"in the allowed period (from,to), we delete {v}")
                deletion_list.append(v)
                ccdb.deleteVersion(v)
                continue
        preservation_list.append(v)

    logger.debug("deleted : ")
    for v in deletion_list:
        logger.debug(f"   {v} - {v.validFrom}")

    logger.debug("preserved : ")
    for v in preservation_list:
        logger.debug(f"   {v} - {v.validFrom}")

    return {"deleted": len(deletion_list), "preserved": len(preservation_list), "updated": 0}
    
def main():
    ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
    process(ccdb, "asdfasdf/example", 60)


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
