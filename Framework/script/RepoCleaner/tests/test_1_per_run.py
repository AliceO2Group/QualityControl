import logging
import time
import unittest
from datetime import timedelta, date, datetime

from Ccdb import Ccdb, ObjectVersion
from rules import last_only
import os
import sys
import importlib


def import_path(path):  # needed because o2-qc-repo-cleaner has no suffix
    module_name = os.path.basename(path).replace('-', '_')
    spec = importlib.util.spec_from_loader(
        module_name,
        importlib.machinery.SourceFileLoader(module_name, path)
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    sys.modules[module_name] = module
    return module


one_per_run = import_path("../qcrepocleaner/rules/1_per_run.py")


class Test1PerRun(unittest.TestCase):
    """
    This test pushes data to the CCDB and then run the Rule test_1_per_run and then check.
    It does it for several use cases.
    One should truncate /qc/TST/MO/repo/test before running it.
    """

    thirty_minutes = 1800000
    one_hour = 3600000
    in_ten_years = 1975323342000
    one_minute = 60000

    def setUp(self):
        self.ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
        self.path = "qc/TST/MO/repo/test"
        self.run = 124321
        self.extra = {}

    def test_1_per_run(self):
        """
        60 versions, 1 minute apart
        6 runs
        grace period of 15 minutes
        Preserved: 14 at the end (grace period), 6 for the runs, but 2 are in both sets --> 14+6-2=18 preserved
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_1_per_run"
        self.prepare_data(test_path, 60)

        objects_versions = self.ccdb.getVersionsList(test_path)
        created = len(objects_versions)

        stats = one_per_run.process(self.ccdb, test_path, 15, 1, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 42)
        self.assertEqual(stats["preserved"], 18)
        self.assertEqual(created, stats["deleted"] + stats["preserved"])

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 18)

    def test_1_per_run_period(self):
        """
        60 versions 1 minute apart
        6 runs
        no grace period
        acceptance period is only the 38 minutes in the middle
        preserved: 6 runs + 11 first and 11 last, with an overlap of 2 --> 26
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_1_per_run_period"
        self.prepare_data(test_path, 60)
        current_timestamp = int(time.time() * 1000)

        stats = one_per_run.process(self.ccdb, test_path, 0, current_timestamp - 49 * 60 * 1000,
                                    current_timestamp - 11 * 60 * 1000, self.extra)
        self.assertEqual(stats["deleted"], 34)
        self.assertEqual(stats["preserved"], 26)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 26)

    def prepare_data(self, path, since_minutes):
        """
        Prepare a data set starting `since_minutes` in the past.
        1 version per minute, 1 run every 10 versions
        """

        current_timestamp = int(time.time() * 1000)
        data = {'part': 'part'}
        run = 1234
        counter = 0

        for x in range(since_minutes + 1):
            counter = counter + 1
            from_ts = current_timestamp - x * 60 * 1000
            to_ts = current_timestamp
            metadata = {'RunNumber': str(run)}
            version_info = ObjectVersion(path=path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
            self.ccdb.putVersion(version=version_info, data=data)
            if x % 10 == 0:
                run = run + 1

        logging.debug(f"counter : {counter}")


if __name__ == '__main__':
    unittest.main()
