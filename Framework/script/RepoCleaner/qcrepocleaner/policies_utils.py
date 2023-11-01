from datetime import datetime
from datetime import timedelta
import logging
from typing import DefaultDict, List

from qcrepocleaner.Ccdb import Ccdb, ObjectVersion

logger = logging  # default logger


def in_grace_period(version: ObjectVersion, delay: int):
    return version.createdAtDt >= datetime.now() - timedelta(minutes=delay)


def get_run(v: ObjectVersion) -> str:
    run = "none"
    if "Run" in v.metadata:
        run = str(v.metadata['Run'])
    elif "RunNumber" in v.metadata:
        run = str(v.metadata['RunNumber'])
    return run


def group_versions(ccdb, object_path, period_pass, versions_buckets_dict: DefaultDict[str, List[ObjectVersion]]):
    # Find all the runs and group the versions (by run or by a combination of multiple attributes)
    versions = ccdb.getVersionsList(object_path)
    logger.debug(f"group_versions: found {len(versions)} versions")
    for v in versions:
        logger.debug(f"Assigning {v} to a bucket")
        run = get_run(v)
        period_name = v.metadata.get("PeriodName") or ""
        pass_name = v.metadata.get("PassName") or ""
        key = run + period_name + pass_name if period_pass else run
        versions_buckets_dict[key].append(v)
