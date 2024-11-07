import logging
import time
import unittest

from qcrepocleaner.Ccdb import Ccdb
from qcrepocleaner.rules import multiple_per_run
from tests import test_utils
from tests.test_utils import CCDB_TEST_URL


class TestMultiplePerRunDeleteFirstLast(unittest.TestCase):
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
        self.ccdb = Ccdb(CCDB_TEST_URL) # ccdb-test but please use IP to avoid DNS alerts
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
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [150], [22*60], 123)
        objectsBefore = self.ccdb.get_versions_list(test_path)

        stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
                                       to_timestamp=self.in_ten_years, extra_params=self.extra)
        objectsAfter = self.ccdb.get_versions_list(test_path)

        self.assertEqual(stats["deleted"], 147)
        self.assertEqual(stats["preserved"], 3)
        self.assertEqual(stats["updated"], 0)

        self.assertEqual(objectsAfter[0].valid_from, objectsBefore[1].valid_from)
        self.assertEqual(objectsAfter[2].valid_from, objectsBefore[-2].valid_from)

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
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [150, 150], [3*60, 20*60], 123)

        stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
                                       to_timestamp=self.in_ten_years, extra_params=self.extra)

        self.assertEqual(stats["deleted"], 147)
        self.assertEqual(stats["preserved"], 3+150)
        self.assertEqual(stats["updated"], 0)

    # def test_5_runs(self):
    #     """
    #     1 hour Run - 1h - 2 hours Run - 2h - 3h10 run - 3h10 - 4 hours run - 4 hours - 5 hours run - 5 h
    #     All more than 24 hours
    #     Expected output: 2 + 3 + 4 + 4 + 5
    #     """
    #     logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
    #                         datefmt='%d-%b-%y %H:%M:%S')
    #     logging.getLogger().setLevel(int(10))
    #
    #     # Prepare data
    #     test_path = self.path + "/test_5_runs"
    #     test_utils.clean_data(self.ccdb, test_path)
    #     test_utils.prepare_data(self.ccdb, test_path, [1*60, 2*60, 3*60+10, 4*60, 5*60],
    #                       [60, 120, 190, 240, 24*60], 123)
    #
    #     stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
    #                                    to_timestamp=self.in_ten_years, extra_params=self.extra)
    #     self.assertEqual(stats["deleted"], 60+120+190+240+300-18)
    #     self.assertEqual(stats["preserved"], 18)
    #     self.assertEqual(stats["updated"], 0)
    #
    #     # and now re-run it to make sure we preserve the state
    #     stats = multiple_per_run.process(self.ccdb, test_path, delay=60*24, from_timestamp=1,
    #                                       to_timestamp=self.in_ten_years, extra_params=self.extra)
    #
    #     self.assertEqual(stats["deleted"], 0)
    #     self.assertEqual(stats["preserved"], 18)
    #     self.assertEqual(stats["updated"], 0)

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
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [1], [25*60], 123)

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
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [2], [25*60], 123)

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
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [30,30, 30], [120,120,25*60], 123)

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
        test_utils.clean_data(self.ccdb, test_path)
        test_utils.prepare_data(self.ccdb, test_path, [70, 70, 70], [6*60, 6*60, 25*60], 55555)


if __name__ == '__main__':
    unittest.main()
