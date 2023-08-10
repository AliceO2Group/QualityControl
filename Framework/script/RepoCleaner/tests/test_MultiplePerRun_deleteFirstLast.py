import logging
import time
import unittest
from datetime import timedelta, date, datetime
from typing import List

from qcrepocleaner.Ccdb import Ccdb, ObjectVersion
from qcrepocleaner.rules import multiple_per_run


class TestProduction(unittest.TestCase):
    """
    This test pushes data to the CCDB and then run the Rule Production and then check.
    It does it for several use cases.
    One should truncate /qc/TST/MO/repo/test before running it.
    """

    thirty_minutes = 1800000
    one_hour = 3600000
    in_ten_years = 1975323342000
    one_minute = 60000

    def setUp(self):
        self.ccdb = Ccdb('http://137.138.47.222:8080')
        self.extra = {"interval_between_versions": "90", "migrate_to_EOS": False, "delete_first_last": True}
        self.path = "qc/TST/MO/repo/test"

    def test_1_finished_run(self):
        """
        1 run of 2.5 hours finished 22 hours ago.
        Expected output: SOR, EOR, 1 in the middle
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_1_finished_run"
        self.prepare_data(test_path, [150], [22*60], 123)
        objectsBefore = self.ccdb.getVersionsList(test_path)

        stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
                                       to_timestamp=self.in_ten_years, extra_params=self.extra)
        objectsAfter = self.ccdb.getVersionsList(test_path)

        self.assertEqual(stats["deleted"], 147)
        self.assertEqual(stats["preserved"], 3)
        self.assertEqual(stats["updated"], 0)

        self.assertEqual(objectsAfter[0].validFrom, objectsBefore[1].validFrom)
        self.assertEqual(objectsAfter[2].validFrom, objectsBefore[-2].validFrom)

    def test_2_runs(self):
        """
        2 runs of 2.5 hours, separated by 3 hours, second finished 20h ago.
        Expected output: SOR, EOR, 1 in the middle for the first one, all for the second
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_2_runs"
        self.prepare_data(test_path, [150, 150], [3*60, 20*60], 123)

        stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
                                       to_timestamp=self.in_ten_years, extra_params=self.extra)

        self.assertEqual(stats["deleted"], 147)
        self.assertEqual(stats["preserved"], 3+150)
        self.assertEqual(stats["updated"], 0)

    def test_5_runs(self):
        """
        1 hour Run - 1h - 2 hours Run - 2h - 3h10 run - 3h10 - 4 hours run - 4 hours - 5 hours run - 5 h
        All more than 24 hours
        Expected output: 2 + 3 + 4 + 4 + 5
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_5_runs"
        self.prepare_data(test_path, [1*60, 2*60, 3*60+10, 4*60, 5*60],
                          [60, 120, 190, 240, 24*60], 123)

        stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
                                       to_timestamp=self.in_ten_years, extra_params=self.extra)
        self.assertEqual(stats["deleted"], 60+120+190+240+300-18)
        self.assertEqual(stats["preserved"], 18)
        self.assertEqual(stats["updated"], 0)
        
        # and now re-run it to make sure we preserve the state
        stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
                                          to_timestamp=self.in_ten_years, extra_params=self.extra)
        
        self.assertEqual(stats["deleted"], 0)
        self.assertEqual(stats["preserved"], 18)
        self.assertEqual(stats["updated"], 0)

    def test_run_one_object(self):
        """
        A run with a single object
        Expected output: keep the object
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_run_one_object"
        self.prepare_data(test_path, [1], [25*60], 123)

        stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
                                       to_timestamp=self.in_ten_years, extra_params=self.extra)

        self.assertEqual(stats["deleted"], 0)
        self.assertEqual(stats["preserved"], 1)
        self.assertEqual(stats["updated"], 0)

    def test_run_two_object(self):
        """
        A run with 2 objects
        Expected output: keep the 2 objects
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_run_two_object"
        self.prepare_data(test_path, [2], [25*60], 123)

        stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
                                       to_timestamp=self.in_ten_years, extra_params=self.extra)

        self.assertEqual(stats["deleted"], 0)
        self.assertEqual(stats["preserved"], 2)
        self.assertEqual(stats["updated"], 0)

    def test_3_runs_with_period(self):
        """
        3 runs more than 24h in the past but only the middle one starts in the period that is allowed.
        Expected output: second run is trimmed, not the other
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_3_runs_with_period"
        self.prepare_data(test_path, [30,30, 30], [120,120,25*60], 123)

        current_timestamp = int(time.time() * 1000)
        stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=current_timestamp-29*60*60*1000,
                                       to_timestamp=current_timestamp-26*60*60*1000, extra_params=self.extra)

        self.assertEqual(stats["deleted"], 28)
        self.assertEqual(stats["preserved"], 90-28)
        self.assertEqual(stats["updated"], 0)

    def test_asdf(self):
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))
        test_path = self.path + "/asdf"
        self.prepare_data(test_path, [70, 70, 70], [6*60, 6*60, 25*60], 55555)

    def prepare_data(self, path, run_durations: List[int], time_till_next_run: List[int], first_run_number: int):
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
                version_info = ObjectVersion(path=path, validFrom=cursor, validTo=to_ts, metadata=metadata2,
                                             createdAt=cursor)
                self.ccdb.putVersion(version=version_info, data=data)
                cursor += 1 * 60 * 1000

            run += 1
            cursor += time_till_next * 60 * 1000

        return first_ts


if __name__ == '__main__':
    unittest.main()
