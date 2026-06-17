import csv
import time
import logging
from constants import CMD_EXIT_DEBUG, CMD_OVERRIDE_PWM_DUTY

logger = logging.getLogger("FanController.TestSuite")


class Tester:
    def __init__(self, usb_manager, telemetry_provider):
        self.usb = usb_manager
        self.get_latest_telemetry = telemetry_provider

    def run_all(self):
        logger.info("Starting host-driven automated test cases...")
        all_passed = True
        test_records = []

        try:
            # --- TEST CASE 1: SENSOR READING ---
            logger.info("[TEST] Step 1: Checking Temperature Sensor Snapshot...")
            time.sleep(0.2)

            latest_tlm = self.get_latest_telemetry()
            temp = latest_tlm.get("temp")
            is_valid = latest_tlm.get("is_valid", True)

            step1_passed = False
            details = ""
            measured_str = "No Variable Detected"

            if temp is None:
                details = "No temperature variable detected in telemetry packet."
            elif 10.0 <= temp <= 100.0 and is_valid is True:
                step1_passed = True
                measured_str = f"{temp:.2f} °C (Valid: {is_valid})"
                logger.info(
                    f"  -> [PASS]: Temperature is working. Value: {measured_str}"
                )
            else:
                details = (
                    f"Range/Validity breach. Value: {temp:.2f} °C, Valid: {is_valid}"
                )
                measured_str = f"{temp:.2f} °C (Valid: {is_valid})"
                logger.error(f"  -> [FAIL]: {details}")

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

            # --- TEST CASE 2: PWM 100% ---
            logger.info("[TEST] Step 2: Overriding PWM to 100%...")
            self.usb.send_command(
                cmd_id=CMD_OVERRIDE_PWM_DUTY, value=100
            )
            time.sleep(5.0)

            rpm = self.get_latest_telemetry().get("rpm", 0)
            step2_passed = 1200 < rpm < 1600
            details = (
                ""
                if step2_passed
                else f"RPM outside full throttle spec! Got: {rpm} RPM"
            )
            (
                logger.info(f"  -> [PASS]: RPM within constraints: {rpm} RPM")
                if step2_passed
                else logger.error(f"  -> [FAIL]: {details}")
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

            # --- TEST CASE 3: PWM 20% ---
            logger.info("[TEST] Step 3: Reducing PWM to 20%...")
            self.usb.send_command(cmd_id=CMD_OVERRIDE_PWM_DUTY, value=20)
            time.sleep(5.0)

            rpm = self.get_latest_telemetry().get("rpm", 0)
            step3_passed = rpm > 0
            details = (
                ""
                if step3_passed
                else f"Fan engine stalled under low duty cycle! Got: {rpm} RPM"
            )
            (
                logger.info(f"  -> [PASS]: Rotation remains active: {rpm} RPM")
                if step3_passed
                else logger.error(f"  -> [FAIL]: {details}")
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

            # --- TEST CASE 4: PWM 0% ---
            logger.info("[TEST] Step 4: Cutting PWM output entirely (0%)...")
            self.usb.send_command(cmd_id=CMD_OVERRIDE_PWM_DUTY, value=0)
            time.sleep(5.0)

            rpm = self.get_latest_telemetry().get("rpm", 0)
            step4_passed = rpm == 0
            details = (
                ""
                if step4_passed
                else f"Residual spin-down torque tracking failure. Got: {rpm} RPM"
            )
            (
                logger.info("  -> [PASS]: Rotor brought to a complete rest.")
                if step4_passed
                else logger.error(f"  -> [FAIL]: {details}")
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

            if all_passed:
                logger.info("=== [SUCCESS]: All Host Automated Test Cases Passed! ===")
            else:
                logger.warning("=== [FAILURE]: One or more test cases failed! ===")

        except Exception as e:
            logger.error(
                f"Unexpected exception inside execution loop: {e}", exc_info=True
            )
        finally:
            self._write_csv_report(test_records)
            logger.info("[TEST] Forcing system safely back to regular monitoring...")
            if self.usb.is_connected:
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
            logger.info(f"Run summary written out to logfile: '{filename}'")
        except Exception as csv_err:
            logger.error(f"Failed to output test compilation log to CSV: {csv_err}")
