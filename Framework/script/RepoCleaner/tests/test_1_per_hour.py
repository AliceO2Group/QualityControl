import logging
import time
import unittest
from importlib import import_module
from qcrepocleaner.Ccdb import Ccdb
from tests import test_utils
from tests.test_utils import CCDB_TEST_URL

one_per_hour =  import_module(".1_per_hour", "qcrepocleaner.rules") # file names should not start with a number...

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
        self.ccdb = Ccdb(CCDB_TEST_URL) # ccdb-test but please use IP to avoid DNS alerts
        self.path = "qc/TST/MO/repo/test"
        self.run = 124321
        self.extra = {}


    def test_1_per_hour(self):
        """
        120 versions
        grace period of 15 minutes
        First version is preserved (always). 14 are preserved during the grace period at the end.
        One more is preserved after 1 hour. --> 16 preserved
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_1_per_hour"
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [120], [0], 123)

        stats = one_per_hour.process(self.ccdb, test_path, 15, 1, self.in_ten_years, self.extra)
        logging.info(stats)
        self.assertEqual(stats["deleted"], 104)
        self.assertEqual(stats["preserved"], 16)

        objects_versions = self.ccdb.get_versions_list(test_path)
        self.assertEqual(len(objects_versions), 16)


    def test_1_per_hour_period(self):
        """
        120 versions.
        no grace period.
        period of acceptance: 1 hour in the middle.
        We have therefore 60 versions in the acceptance period.
        Only 1 of them, the one 1 hour after the first version in the set, will be preserved, the others are deleted.
        Thus, we have 59 deletion. Everything outside the acceptance period is kept.
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_1_per_hour_period"
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [120], [0], 123)
        current_timestamp = int(time.time() * 1000)

        stats = one_per_hour.process(self.ccdb, test_path, 15, current_timestamp-90*60*1000,
                                     current_timestamp-30*60*1000, self.extra)
        logging.info(stats)
        self.assertEqual(stats["deleted"], 59)
        self.assertEqual(stats["preserved"], 61)

        objects_versions = self.ccdb.get_versions_list(test_path)
        self.assertEqual(len(objects_versions), 61)


if __name__ == '__main__':
    unittest.main()
