import logging
import time
import unittest
from datetime import timedelta, date, datetime

from Ccdb import Ccdb, ObjectVersion
from rules import production


class TestProduction(unittest.TestCase):
    """
    This test pushes data to the CCDB and then run the Rule Production and then check.
    It does it for several use cases.
    One should truncate /qc/TST/MO/repo/test before running it.
    """

    def setUp(self):
        self.ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
        self.extra = {"delay_first_trimming": "30", "period_btw_versions_first": "10", "delay_final_trimming": "60",
                      "period_btw_versions_final": "60"}
        self.path = "qc/TST/MO/repo/test"
        self.run = 124321

    def test_start_run(self):
        """
        Ongoing run (25).
        0'-25': not trimmed
        Expected output: nothing trimmed, then change parameter delay_first_trimming to 5' and trim
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_start_run"
        self.prepare_data(test_path, 25, 30, True, 60, False)

        production.eor_dict.pop(int(self.run), None)
        stats = production.process(self.ccdb, test_path, 30, self.extra)
        self.assertEqual(stats["deleted"], 0)
        self.assertEqual(stats["preserved"], 26)
        self.assertEqual(stats["updated"], 0)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 26)

        self.extra = {"delay_first_trimming": "5", "period_btw_versions_first": "10", "delay_final_trimming": "60",
                      "period_btw_versions_final": "15"}
        stats = production.process(self.ccdb, test_path, 30, self.extra)
        self.assertEqual(stats["deleted"], 19)
        self.assertEqual(stats["preserved"], 7)
        self.assertEqual(stats["updated"], 1)

    def test_mid_run(self):
        """
        Ongoing run (1h30).
        0-30' : already trimmed
        30-90': not trimmed
        Expected output: 0-60' trimmed
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_mid_run"
        self.prepare_data(test_path, 90)

        production.eor_dict.pop(int(self.run), None)
        stats = production.process(self.ccdb, test_path, 30, self.extra)
        self.assertEqual(stats["deleted"], 28)
        self.assertEqual(stats["preserved"], 35)
        self.assertEqual(stats["updated"], 3)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 35)
        self.assertTrue("trim1" in objects_versions[0].metadata)

    def test_run_finished(self):
        """
        Finished run (3h10).
        0'-190': not trimmed
        running after delay for final trimming
        Expected output: 5 versions : SOR, EOR, 2 at 1 hour interval
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_run_finished"
        self.prepare_data(test_path, 290, 190, False, 0, True)

        production.eor_dict[int(self.run)] = datetime.now() - timedelta(minutes=100)
        stats = production.process(self.ccdb, test_path, 30, self.extra)
        self.assertEqual(stats["deleted"], 15)
        self.assertEqual(stats["preserved"], 4)
        self.assertEqual(stats["updated"], 3)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 4)
        self.assertTrue("trim1" not in objects_versions[0].metadata)
        self.assertTrue("preservation" in objects_versions[0].metadata)

    def prepare_data(self, path, minutes_since_sor, duration_first_part=30, skip_first_part=False,
                     minutes_second_part=60, skip_second_part=False):
        """
        Prepare a data set starting `minutes_since_sor` in the past.
        The data is layed out in two parts
          1. `duration_first_part` minutes already trimmed data (every 10 minutes)
          2. `minutes_second_part` minutes untrimmed
        Each part can be skipped with the respective parameter.
        Depending how far in the past one starts, different outputs of the production rule are expected.
        If `minutes_since_sor` is shorter than the run, we only create data in the past, never in the future.
        """

        current_timestamp = int(time.time() * 1000)
        sor = current_timestamp - minutes_since_sor * 60 * 1000
        data = {'part': 'part'}
        metadata = {'RunNumber': str(self.run), 'trim1': 'true'}
        cursor = sor

        # 1 version every 10 minutes starting at sor and finishing 30 minutes after with trim1 flag set
        if not skip_first_part:
            for x in range(int(round(duration_first_part / 10))):
                from_ts = cursor + x * 10 * 60 * 1000
                if from_ts > current_timestamp:
                    return
                to_ts = from_ts + 24 * 60 * 60 * 1000  # a day
                version_info = ObjectVersion(path=path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
                self.ccdb.putVersion(version=version_info, data=data)
            cursor = cursor + duration_first_part * 60 * 1000

        # 1 version every 1 minutes starting after and lasting `minutes_second_part` minutes
        if not skip_second_part:
            current_timestamp = int(time.time() * 1000)
            metadata = {'RunNumber': str(self.run)}
            for x in range(minutes_second_part):
                from_ts = cursor + x * 60 * 1000
                if from_ts > current_timestamp:
                    return
                to_ts = from_ts + 24 * 60 * 60 * 1000  # a day
                version_info = ObjectVersion(path=path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
                self.ccdb.putVersion(version=version_info, data=data)


if __name__ == '__main__':
    unittest.main()
