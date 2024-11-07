import logging
import time
import unittest
from importlib import import_module

from qcrepocleaner.Ccdb import Ccdb
from tests import test_utils
from tests.test_utils import CCDB_TEST_URL

one_per_run =  import_module(".1_per_run", "qcrepocleaner.rules") # file names should not start with a number...

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
        self.ccdb = Ccdb(CCDB_TEST_URL)
        self.path = "qc/TST/MO/repo/test"
        self.run = 124321
        self.extra = {}

    def test_1_per_run(self):
        """
        6 runs of 10 versions, versions 1 minute apart
        grace period of 15 minutes
        Preserved: 14 at the end (grace period), 6 for the runs, but 2 are in both sets --> 14+6-2=18 preserved
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_1_per_run"
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [10, 10, 10, 10, 10, 10], [0, 0, 0, 0, 0, 0], 123)

        objects_versions = self.ccdb.get_versions_list(test_path)
        created = len(objects_versions)

        stats = one_per_run.process(self.ccdb, test_path, 15, 1, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 42)
        self.assertEqual(stats["preserved"], 18)
        self.assertEqual(created, stats["deleted"] + stats["preserved"])

        objects_versions = self.ccdb.get_versions_list(test_path)
        self.assertEqual(len(objects_versions), 18)

    def test_1_per_run_period(self):
        """
        6 runs of 10 versions each, versions 1 minute apart
        no grace period
        acceptance period is only the 38 minutes in the middle
        preserved: 6 runs + 11 first and 11 last, with an overlap of 2 --> 26
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_1_per_run_period"
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [10, 10, 10, 10, 10, 10], [0, 0, 0, 0, 0, 0], 123)
        current_timestamp = int(time.time() * 1000)

        stats = one_per_run.process(self.ccdb, test_path, 0, current_timestamp - 49 * 60 * 1000,
                                    current_timestamp - 11 * 60 * 1000, self.extra)
        self.assertEqual(stats["deleted"], 34)
        self.assertEqual(stats["preserved"], 26)

        objects_versions = self.ccdb.get_versions_list(test_path)
        self.assertEqual(len(objects_versions), 26)

if __name__ == '__main__':
    unittest.main()
