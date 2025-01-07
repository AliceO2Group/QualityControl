import logging
from datetime import datetime
from datetime import timedelta
from typing import Dict, List, Optional

from qcrepocleaner.Ccdb import Ccdb, ObjectVersion

logger = logging  # default logger


def process(ccdb: Ccdb, object_path: str, delay: int,  from_timestamp: int, to_timestamp: int, extra_params: Dict[str, str]):
    """
    Process this deletion rule on the object. We use the CCDB passed by argument.

    Only the last version of each object is preserved. Grace period is respected.

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period during which a new object is never deleted.
    :param from_timestamp: only objects created after this timestamp are considered.
    :param to_timestamp: only objects created before this timestamp are considered.
    :param extra_params: a dictionary containing extra parameters (unused in this rule)
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    """
    
    logger.debug(f"Plugin last_only processing {object_path}")

    versions = ccdb.get_versions_list(object_path)

    earliest: Optional[ObjectVersion] = None
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []
    # find the earliest
    for v in versions:
        if earliest is None or v.valid_from_as_dt > earliest.valid_from_as_dt:
            earliest = v
    logger.debug(f"earliest : {earliest}")

    # delete the non-earliest if we are not in the grace period
    for v in versions:
        logger.debug(f"{v} - {v.valid_from}")
        if v == earliest:
            preservation_list.append(v)
            continue


        if v.valid_from_as_dt < datetime.now() - timedelta(minutes=delay):  # grace period
            logger.debug(f"   not in the grace period")
            if from_timestamp < v.valid_from < to_timestamp:  # in the allowed period
                logger.debug(f"in the allowed period (from,to), we delete {v}")
                deletion_list.append(v)
                ccdb.delete_version(v)
                continue
        preservation_list.append(v)

    logger.debug(f"deleted ({len(deletion_list)}) : ")
    for v in deletion_list:
        logger.debug(f"   {v}")

    logger.debug(f"preserved ({len(preservation_list)}) : ")
    for v in preservation_list:
        logger.debug(f"   {v}")
        
    return {"deleted" : len(deletion_list), "preserved": len(preservation_list)}
