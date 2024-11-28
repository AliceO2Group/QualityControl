import logging
import time
from typing import List

from qcrepocleaner.Ccdb import ObjectVersion

CCDB_TEST_URL = 'http://128.142.249.62:8080'

def clean_data(ccdb, path):
    versions = ccdb.get_versions_list(path)
    for v in versions:
        ccdb.delete_version(v)


def prepare_data(ccdb, path, run_durations: List[int], time_till_next_run: List[int], first_run_number: int):
    """
    Prepare a data set populated with a number of runs.
    run_durations contains the duration of each of these runs in minutes
    time_till_next_run is the time between two runs in minutes.
    The first element of time_till_next_run is used to separate the first two runs.
    Both lists must have the same number of elements.
    """

    if len(run_durations) != len(time_till_next_run):
        logging.error(f"run_durations and time_till_next_run must have the same length")
        exit(1)

    total_duration = 0
    for a, b in zip(run_durations, time_till_next_run):
        total_duration += a + b
    logging.info(f"Total duration : {total_duration}")

    current_timestamp = int(time.time() * 1000)
    cursor = current_timestamp - total_duration * 60 * 1000
    first_ts = cursor
    data = {'part': 'part'}
    run = first_run_number

    for run_duration, time_till_next in zip(run_durations, time_till_next_run):
        metadata = {'RunNumber': str(run)}
        logging.debug(f"cursor: {cursor}")
        logging.debug(f"time_till_next: {time_till_next}")

        for i in range(run_duration):
            to_ts = cursor + 24 * 60 * 60 * 1000  # a day
            metadata2 = {**metadata, 'Created': str(cursor)}
            version_info = ObjectVersion(path=path, valid_from=cursor, valid_to=to_ts, metadata=metadata2,
                                         created_at=cursor)
            ccdb.put_version(version=version_info, data=data)
            cursor += 1 * 60 * 1000

        run += 1
        cursor += time_till_next * 60 * 1000

    return first_ts
