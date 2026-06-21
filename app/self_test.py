import csv
import time
import logging
from constants import CMD_EXIT_DEBUG, CMD_OVERRIDE_PWM_DUTY, CMD_SET_TARGET_TEMP

logger = logging.getLogger("FanController.TestSuite")


class Tester:
    def __init__(self, usb_manager, telemetry_provider):
        self.usb = usb_manager
        self.get_latest_telemetry = telemetry_provider

    def run_all(self):
        logger.info("running tests...")
        all_passed = True
        test_records = []

        try:
            # test 1 - temp sensor check
            logger.info("[TEST] step 1: checking temp sensor...")
            time.sleep(0.2)
            latest_tlm = self.get_latest_telemetry()
            temp = latest_tlm.get("temp")
            is_valid = latest_tlm.get("is_valid", True)
            step1_passed = False
            details = ""
            measured_str = "no reading"

            if temp is None:
                details = "temp missing from telemetry packet"
            elif 10.0 <= temp <= 100.0 and is_valid is True:
                step1_passed = True
                measured_str = f"{temp:.2f} C (valid={is_valid})"
                logger.info(
                    f" -> [PASS]: temp ok, got: {measured_str}"
                )
            else:
                details = (
                    f"out of range. got: {temp:.2f} C, valid={is_valid}"
                )
                measured_str = f"{temp:.2f} C (valid={is_valid})"
                logger.error(f" -> [FAIL]: {details}")

            test_records.append(
                self._create_record(
                    "Temperature Sensor Snapshot",
                    step1_passed,
                    measured_str,
                    "10.0 <= Temp <= 100.0",
                    details,
                )
            )
            if not step1_passed:
                all_passed = False

            # test 2 - PWM 100%
            logger.info("[TEST] step 2: setting PWM to 100%...")
            self.usb.send_command(cmd_id=CMD_OVERRIDE_PWM_DUTY, value=100)
            time.sleep(5.0)
            rpm = self.get_latest_telemetry().get("rpm", 0)
            step2_passed = 1200 < rpm < 1600
            details = (
                ""
                if step2_passed
                else f"rpm out of range at 100%: {rpm} RPM"
            )
            (
                logger.info(f" -> [PASS]: rpm ok: {rpm} RPM")
                if step2_passed
                else logger.error(f" -> [FAIL]: {details}")
            )
            test_records.append(
                self._create_record(
                    "PWM 100% Full Throttle",
                    step2_passed,
                    f"{rpm} RPM",
                    "1200 < RPM < 1600",
                    details,
                )
            )
            if not step2_passed:
                all_passed = False

            # test 3 - PWM 20%
            logger.info("[TEST] step 3: setting PWM to 20%...")
            self.usb.send_command(cmd_id=CMD_OVERRIDE_PWM_DUTY, value=20)
            time.sleep(5.0)
            rpm = self.get_latest_telemetry().get("rpm", 0)
            step3_passed = rpm > 0
            details = (
                ""
                if step3_passed
                else f"fan stalled at 20%: {rpm} RPM"
            )
            (
                logger.info(f" -> [PASS]: fan still spinning: {rpm} RPM")
                if step3_passed
                else logger.error(f" -> [FAIL]: {details}")
            )
            test_records.append(
                self._create_record(
                    "PWM 20% Low Throttle",
                    step3_passed,
                    f"{rpm} RPM",
                    "RPM > 0",
                    details,
                )
            )
            if not step3_passed:
                all_passed = False

            # test 4 - PWM 0%
            logger.info("[TEST] step 4: setting PWM to 0%...")
            self.usb.send_command(cmd_id=CMD_OVERRIDE_PWM_DUTY, value=0)
            time.sleep(5.0)
            rpm = self.get_latest_telemetry().get("rpm", 0)
            step4_passed = rpm == 0
            details = (
                ""
                if step4_passed
                else f"fan didn't stop: {rpm} RPM"
            )
            (
                logger.info(" -> [PASS]: fan stopped.")
                if step4_passed
                else logger.error(f" -> [FAIL]: {details}")
            )
            test_records.append(
                self._create_record(
                    "PWM 0% Stop Verification",
                    step4_passed,
                    f"{rpm} RPM",
                    "RPM == 0",
                    details,
                )
            )
            if not step4_passed:
                all_passed = False

            # test 5 - step response curve (disabled for now)
            # logger.info(
            #     "[TEST] step 5: logging step response curve data..."
            # )
            # # PID in Zephyr only works in SYSTEM_STATE_NORMAL,
            # # right now we're in SYSTEM_STATE_SELF_TEST so need to switch states
            # logger.info(" -> exiting self-test so PID kicks in...")
            # self.usb.send_command(
            #     cmd_id=CMD_EXIT_DEBUG
            # )  # SELF_TEST -> DEBUG_IDLE
            # time.sleep(0.5)
            # self.usb.send_command(
            #     cmd_id=CMD_EXIT_DEBUG
            # )  # DEBUG_IDLE -> NORMAL
            # time.sleep(1.0)  # wait for control_mgr to settle in NORMAL
            #
            # # PID is running now, set the target temp
            # self.usb.send_command(cmd_id=CMD_SET_TARGET_TEMP, value=65.0)
            # duration = 300.0       # how long to collect data, seconds (5 min)
            # poll_interval = 1.0    # sample rate
            # step_data = []
            # logger.info(
            #     f"collecting data for {int(duration)}s, please wait..."
            # )
            # start_time = time.time()
            # while True:
            #     elapsed = time.time() - start_time
            #     if elapsed > duration:
            #         break
            #     tlm = self.get_latest_telemetry()
            #     step_data.append(
            #         {
            #             "Time (s)": round(elapsed, 2),
            #             "Temperature (C)": tlm.get("temp", 0.0),
            #             "Target Temp (C)": tlm.get("target_temp", 0.0),
            #             "PWM Duty (%)": tlm.get("duty", 0),
            #             "RPM": tlm.get("rpm", 0),
            #         }
            #     )
            #     time.sleep(poll_interval)
            #
            # step_csv_filename = (
            #     f"step_response_data_{time.strftime('%Y%m%d_%H%M%S')}.csv"
            # )
            # step5_passed = False
            # details = ""
            # try:
            #     step_fields = [
            #         "Time (s)",
            #         "Temperature (C)",
            #         "Target Temp (C)",
            #         "PWM Duty (%)",
            #         "RPM",
            #     ]
            #     with open(
            #         step_csv_filename, mode="w", newline="", encoding="utf-8"
            #     ) as step_file:
            #         writer = csv.DictWriter(step_file, fieldnames=step_fields)
            #         writer.writeheader()
            #         writer.writerows(step_data)
            #     logger.info(
            #         f" -> [PASS]: data saved to '{step_csv_filename}'"
            #     )
            #     step5_passed = True
            #     details = f"saved {len(step_data)} samples to {step_csv_filename}"
            # except Exception as e:
            #     logger.error(f" -> [FAIL]: CSV write failed: {e}")
            #     details = f"CSV error: {e}"
            #
            # test_records.append(
            #     self._create_record(
            #         "Step Response Curve",
            #         step5_passed,
            #         "Data Logged",
            #         f"{int(duration)}s Run",
            #         details,
            #     )
            # )
            # if not step5_passed:
            #     all_passed = False

            # done
            if all_passed:
                logger.info("=== [OK]: all tests passed ===")
            else:
                logger.warning("=== [FAIL]: something went wrong ===")

        except Exception as e:
            logger.error(
                f"unexpected exception: {e}", exc_info=True
            )
        finally:
            self._write_csv_report(test_records)
            logger.info("[TEST] putting system back to normal mode...")
            if self.usb.is_connected:
                self.usb.send_command(cmd_id=CMD_EXIT_DEBUG)
                self.usb.send_command(cmd_id=CMD_EXIT_DEBUG)

    def _create_record(self, name, passed, measured, expected, notes):
        return {
            "Timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "Test Name": name,
            "Status": "PASS" if passed else "FAIL",
            "Measured Value": measured,
            "Expected Bounds": expected,
            "Notes": notes,
        }

    def _write_csv_report(self, records):
        if not records:
            return
        filename = f"test_report_{time.strftime('%Y%m%d_%H%M%S')}.csv"
        try:
            fieldnames = [
                "Timestamp",
                "Test Name",
                "Status",
                "Measured Value",
                "Expected Bounds",
                "Notes",
            ]
            with open(filename, mode="w", newline="", encoding="utf-8") as csv_file:
                writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
                writer.writeheader()
                writer.writerows(records)
            logger.info(f"report saved to '{filename}'")
        except Exception as csv_err:
            logger.error(f"couldn't save report: {csv_err}")
