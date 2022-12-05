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

one_per_hour = import_path("../qcrepocleaner/rules/1_per_hour.py")

class Test1PerHour(unittest.TestCase):
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


    def test_1_per_hour(self):
        """
        60 versions, 2 minutes apart
        grace period of 15 minutes
        First version is preserved (always). 7 are preserved during the grace period at the end.
        One more is preserved after 1 hour. --> 9 preserved
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_1_per_hour"
        self.prepare_data(test_path, 60, 2)

        stats = one_per_hour.process(self.ccdb, test_path, 15, 1, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 51)
        self.assertEqual(stats["preserved"], 9)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 9)


    def test_1_per_hour_period(self):
        """
        60 versions, 2 minutes apart
        no grace period
        period of acceptance: 1 hour in the middle
        We have therefore 30 versions in the acceptance period.
        Only 1 of them, the one 1 hour after the first version in the set, will be preserved, the others are deleted.
        Thus we have 29 deletion. Everything outside the acceptance period is kept.
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_1_per_hour_period"
        self.prepare_data(test_path, 60, 2)
        current_timestamp = int(time.time() * 1000)
        logging.debug(f"{current_timestamp} - {datetime.today()}")

        objects_versions = self.ccdb.getVersionsList(test_path)
        created = len(objects_versions)

        stats = one_per_hour.process(self.ccdb, test_path, 15, current_timestamp-90*60*1000,
                                     current_timestamp-30*60*1000, self.extra)
        self.assertEqual(stats["deleted"], 29)
        self.assertEqual(stats["preserved"], 31)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 31)


    def prepare_data(self, path, number_versions, minutes_between):
        """
        Prepare a data set starting `since_minutes` in the past.
        1 version per minute
        """

        current_timestamp = int(time.time() * 1000)
        data = {'part': 'part'}
        run = 1234
        counter = 0

        for x in range(number_versions+1):
            counter = counter + 1
            from_ts = current_timestamp - minutes_between * x * 60 * 1000
            to_ts = current_timestamp
            metadata = {'RunNumber': str(run)}
            version_info = ObjectVersion(path=path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
            self.ccdb.putVersion(version=version_info, data=data)

        logging.debug(f"counter : {counter}")


if __name__ == '__main__':
    unittest.main()
