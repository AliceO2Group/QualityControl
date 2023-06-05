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

    thirty_minutes = 1800000
    one_hour = 3600000
    in_ten_years = 1975323342000
    one_minute = 60000

    def setUp(self):
        self.ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
        self.extra = {"delay_first_trimming": "30", "period_btw_versions_first": "10", "delay_final_trimming": "60",
                      "period_btw_versions_final": "60"}
        self.path = "qc/TST/MO/repo/test"
        self.run = 124321

    def test_start_run(self):
        """
        Ongoing run (25 minutes, 26 versions).
        0'-25': not trimmed
        Expected output: nothing trimmed, then change parameter delay_first_trimming to 5' and trim
        In the second processing, preserved: 5 at the end (delay_first_trimming) + 1 at the beginning + 1 after 10 min.
                                            --> 7
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_start_run"
        self.prepare_data(test_path, 25, 30, True, 60, False)

        production.eor_dict.pop(int(self.run), None)
        stats = production.process(self.ccdb, test_path, 30, 1, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 0)
        self.assertEqual(stats["preserved"], 26)
        self.assertEqual(stats["updated"], 0)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 26)

        self.extra = {"delay_first_trimming": "5", "period_btw_versions_first": "10", "delay_final_trimming": "60",
                      "period_btw_versions_final": "15"}
        stats = production.process(self.ccdb, test_path, 30, 1, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 19)
        self.assertEqual(stats["preserved"], 7)
        self.assertEqual(stats["updated"], 1)

    def test_start_run_period(self):
        """
        Ongoing run (25 minutes, 26 versions).
        0'-25': not trimmed
        two processings: the first one has a period that does not match anything in the run and the second has
        the period at SOR+5 and SOR+20
        Preserved 2nd processing: 6 at the end (delay_first_trimming and outside period) + 6 at the end (outside period)
                                    + the first in the period + 1 after 10 min
                                            --> 14
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_start_run_period"
        first_ts = self.prepare_data(test_path, 25, 30, True, 60, False)
        logging.getLogger().debug(f"first_ts : {first_ts}")

        # everything outside the period
        self.extra = {"delay_first_trimming": "5", "period_btw_versions_first": "10", "delay_final_trimming": "60",
                      "period_btw_versions_final": "15"}
        stats = production.process(self.ccdb, test_path, 30, 1, 10, self.extra)
        self.assertEqual(stats["deleted"], 0)
        self.assertEqual(stats["preserved"], 26)
        self.assertEqual(stats["updated"], 0)

        # with a period from 5 to 20
        self.extra = {"delay_first_trimming": "5", "period_btw_versions_first": "10", "delay_final_trimming": "60",
                      "period_btw_versions_final": "15"}
        stats = production.process(self.ccdb, test_path, 30, first_ts + self.one_minute * 5,
                                   first_ts + self.one_minute * 20, self.extra)
        self.assertEqual(stats["deleted"], 12)
        self.assertEqual(stats["preserved"], 14)
        self.assertEqual(stats["updated"], 1)


    def test_mid_run(self):
        """
        Ongoing run (1h30).
        0-30' : already trimmed
        30-90': not trimmed
        defaults params are used (delay_first_trimming = 30, period_btw_versions_first= 10)
        Expected output: 0-60' trimmed
        preserved: the 4 at the beginning already trimmed, 3 for the beginning of the run, 28 for the last 30 minutes
                    (delay_first_trimming)
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_mid_run"
        self.prepare_data(test_path, 90)

        production.eor_dict.pop(int(self.run), None)
        stats = production.process(self.ccdb, test_path, 30, 1, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 28)
        self.assertEqual(stats["preserved"], 35)
        self.assertEqual(stats["updated"], 3)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 35)
        self.assertTrue("trim1" in objects_versions[0].metadata)

    def test_mid_run_period(self):
        """
        Ongoing run (1h30).
        0-30' : already trimmed
        30-90': not trimmed
        only applies to the period: sOR+35 - SOR+80
        defaults params are used (delay_first_trimming = 30, period_btw_versions_first= 10)
        Expected output: 0-60' trimmed
        preserved: the 4 at the beginning already trimmed, 5 outside the period at the beginning,
                        3 for the beginning of the run, 29 for the last 30 minutes
                    (delay_first_trimming)
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_mid_run_period"
        first_ts = self.prepare_data(test_path, 90)
        logging.getLogger().debug(f"first_ts : {first_ts}")

        objects_versions = self.ccdb.getVersionsList(test_path)
        created = len(objects_versions)

        production.eor_dict.pop(int(self.run), None)
        stats = production.process(self.ccdb, test_path, 30, first_ts + 35 * self.one_minute, first_ts + 80 * self.one_minute, self.extra)
        self.assertEqual(stats["deleted"], 22)
        self.assertEqual(stats["preserved"], 41)
        self.assertEqual(stats["updated"], 2)
        self.assertEqual(created, stats["deleted"]+stats["preserved"])

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 41)
        self.assertTrue("trim1" in objects_versions[0].metadata)

    def test_run_finished(self):
        """
        Finished run (3h10).
        0'-190': trimmed
        running after delay for final trimming
        Expected output: 4 versions : SOR, EOR, 2 at 1 hour interval
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_run_finished"
        self.prepare_data(test_path, 290, 190, False, 0, True)

        production.eor_dict[int(self.run)] = datetime.now() - timedelta(minutes=100)
        stats = production.process(self.ccdb, test_path, 30, 1, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 15)
        self.assertEqual(stats["preserved"], 4)
        self.assertEqual(stats["updated"], 4)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 4)
        self.assertTrue("trim1" not in objects_versions[0].metadata)
        self.assertTrue("preservation" in objects_versions[0].metadata)

    def test_run_finished_period(self):
        """
        Finished run (3h10).
        0'-190': trimmed
        running after delay for final trimming
        Only the data after SOR+30 minutes is processed
        Expected output: 4 versions : SOR, EOR, 4  outside the period, 1 in the middle of the period
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_run_finished_period"
        first_ts = self.prepare_data(test_path, 290, 190, False, 0, True)
        logging.getLogger().debug(f"first_ts : {first_ts}")

        production.eor_dict[int(self.run)] = datetime.now() - timedelta(minutes=100)
        stats = production.process(self.ccdb, test_path, 30, first_ts+self.thirty_minutes, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 12)
        self.assertEqual(stats["preserved"], 7)
        self.assertEqual(stats["updated"], 3)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 7)
        logging.debug(f"{objects_versions[0].metadata}")
        self.assertTrue("trim1" in objects_versions[0].metadata) # we did not touch this data thus it is not processed
        self.assertTrue("trim1" not in objects_versions[6].metadata)
        self.assertTrue("preservation" in objects_versions[6].metadata)

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
        first_ts = 1975323342000

        # 1 version every 10 minutes starting at sor and finishing duration_first_part minutes after with trim1 flag set
        if not skip_first_part:
            for x in range(int(round(duration_first_part / 10))):
                from_ts = cursor + x * 10 * 60 * 1000
                if from_ts > current_timestamp:
                    return first_ts
                if first_ts > from_ts:
                    first_ts = from_ts
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
                    return first_ts
                if first_ts > from_ts:
                    first_ts = from_ts
                to_ts = from_ts + 24 * 60 * 60 * 1000  # a day
                version_info = ObjectVersion(path=path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
                self.ccdb.putVersion(version=version_info, data=data)

        return first_ts


if __name__ == '__main__':
    unittest.main()
