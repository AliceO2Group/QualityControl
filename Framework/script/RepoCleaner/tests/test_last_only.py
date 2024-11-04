import logging
import time
import unittest

from qcrepocleaner.Ccdb import Ccdb, ObjectVersion
from qcrepocleaner.rules import last_only
from tests import test_utils


class TestLastOnly(unittest.TestCase):
    """
    This test pushes data to the CCDB and then run the Rule last_only and then check.
    It does it for several use cases.
    One should truncate /qc/TST/MO/repo/test before running it.
    """

    thirty_minutes = 1800000
    one_hour = 3600000
    in_ten_years = 1975323342000
    one_minute = 60000

    def setUp(self):
        self.ccdb = Ccdb('http://128.142.249.62:8080') # ccdb-test but please use IP to avoid DNS alerts
        self.extra = {}
        self.path = "qc/TST/MO/repo/test"
        self.run = 124321


    def test_last_only(self):
        """
        60 versions
        grace period of 30 minutes
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_last_only"
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [60], [0], 123)

        stats = last_only.process(self.ccdb, test_path, 30, 1, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 31) # 31 because between the time we produced the 60 versions and now, there is a shift
        self.assertEqual(stats["preserved"], 29)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 29)


    def test_last_only_period(self):
        """
        60 versions
        no grace period
        only 20 minutes in the middle are in the period
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_last_only_period"
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [60], [0], 123)
        current_timestamp = int(time.time() * 1000)

        stats = last_only.process(self.ccdb, test_path, 0, current_timestamp-40*60*1000, current_timestamp-20*60*1000, self.extra)
        self.assertEqual(stats["deleted"], 20)
        self.assertEqual(stats["preserved"], 40)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 40)


if __name__ == '__main__':
    unittest.main()
