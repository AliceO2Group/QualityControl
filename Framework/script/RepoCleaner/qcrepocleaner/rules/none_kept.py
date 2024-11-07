import logging
from typing import Dict, List

from qcrepocleaner import policies_utils
from qcrepocleaner.Ccdb import Ccdb, ObjectVersion

logger = logging  # default logger


def process(ccdb: Ccdb, object_path: str, delay: int,  from_timestamp: int, to_timestamp: int, extra_params: Dict[str, str]):
    """
    Process this deletion rule on the object. We use the CCDB passed by argument.

    This policy deletes everything after `delay` minutes.

    :param ccdb: the ccdb in which objects are cleaned up.
    :param object_path: path to the object, or pattern, to which a rule will apply.
    :param delay: the grace period in minutes during which a new object is never deleted.
    :param from_timestamp: only objects created after this timestamp are considered.
    :param to_timestamp: only objects created before this timestamp are considered.
    :param extra_params: a dictionary containing extra parameters (unused in this rule)
    :return a dictionary with the number of deleted, preserved and updated versions. Total = deleted+preserved.
    """
    
    logger.debug(f"Plugin 'none' processing {object_path}")

    versions = ccdb.get_versions_list(object_path)
    preservation_list: List[ObjectVersion] = []
    deletion_list: List[ObjectVersion] = []

    for v in versions:
        logger.debug(f"Processing version {v}")
        if policies_utils.in_grace_period(v, delay):
            logger.debug(f"     in grace period, skip this version")
            preservation_list.append(v)
        else:
            logger.debug(f"     not in the grace period")
            if from_timestamp < v.valid_from < to_timestamp:  # in the allowed period
                logger.debug(f"     in the allowed period (from,to), we delete it")
                deletion_list.append(v)
                ccdb.delete_version(v)
                continue

    logger.debug("deleted : ")
    for v in deletion_list:
        logger.debug(f"   {v}")

    logger.debug("preserved : ")
    for v in preservation_list:
        logger.debug(f"   {v}")

    return {"deleted": len(deletion_list), "preserved": len(preservation_list), "updated": 0}
