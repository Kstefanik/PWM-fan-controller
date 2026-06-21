import threading
import customtkinter as ctk
from usb import UsbManager
from self_test import Tester
from constants import (
    CMD_GET_TELEMETRY,
    CMD_SET_TARGET_TEMP,
    CMD_ENTER_DEBUG,
    CMD_EXIT_DEBUG,
    CMD_SELF_TEST,
    CMD_CALIBRATE_PID,
    
    SYSTEM_STATE_NORMAL,
    SYSTEM_STATE_DEBUG_IDLE,
    SYSTEM_STATE_SELF_TEST,
    SYSTEM_STATE_PID_CAL,
)

SYSTEM_STATE_MAP = {
    SYSTEM_STATE_NORMAL: "Normal",
    SYSTEM_STATE_DEBUG_IDLE: "Debug Idle",
    SYSTEM_STATE_SELF_TEST: "Self Test",
    SYSTEM_STATE_PID_CAL: "PID Calibration",
}


class GuiManager(ctk.CTk):
    def __init__(self, usb_manager: UsbManager):
        super().__init__()
        self.usb = usb_manager

        # Window Configuration
        self.title("PWM Fan Controller")
        self.geometry("850x440")
        ctk.set_appearance_mode("Dark")
        ctk.set_default_color_theme("blue")

        # State Machine variables
        self._last_known_connection_state = None
        self.current_system_state = SYSTEM_STATE_NORMAL
        self.latest_telemetry = {}

        # Grid Setup
        self.grid_rowconfigure(1, weight=1)
        self.grid_columnconfigure(0, weight=1)
        self.grid_columnconfigure(1, weight=1)

        self._create_connection_status_banner()
        self._create_left_frame()
        self._create_right_frame()

        # Lifecycle Management
        self.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.monitor_connection_health()
        self.auto_request_telemetry()
        self.poll_telemetry_queue()

    def _create_connection_status_banner(self):
        self.lbl_status_banner = ctk.CTkLabel(
            self,
            text="Searching for fan controller hardware...",
            fg_color="#D32F2F",
            text_color="white",
            font=("Arial", 12, "bold"),
            height=30,
        )
        self.lbl_status_banner.grid(row=0, column=0, columnspan=2, sticky="ew")

    def _create_left_frame(self):
        self.left_frame = ctk.CTkFrame(self)
        self.left_frame.grid(row=1, column=0, padx=(20, 10), pady=20, sticky="nsew")
        self.left_frame.grid_columnconfigure((0, 1), weight=1)

        ctk.CTkLabel(
            self.left_frame, text="Telemetry", font=("Arial", 18, "bold")
        ).grid(row=0, column=0, columnspan=2, pady=(15, 10))

        self.telemetry_fields = {
            "temp": ("Temperature:", "--- °C"),
            "rpm": ("Fan Speed:", "--- RPM"),
            "duty": ("PWM Duty:", "--- %"),
            "target_temp": ("Current Target:", "--- °C"),
            "system_state": ("Operation Mode:", "---"),
        }
        self.telemetry_labels = {}

        current_row = 1
        for key, (label_text, default_val) in self.telemetry_fields.items():
            ctk.CTkLabel(self.left_frame, text=label_text, font=("Arial", 14)).grid(
                row=current_row, column=0, padx=20, pady=5, sticky="w"
            )
            lbl_val = ctk.CTkLabel(
                self.left_frame, text=default_val, font=("Arial", 14, "bold")
            )
            lbl_val.grid(row=current_row, column=1, padx=20, pady=5, sticky="e")
            self.telemetry_labels[key] = lbl_val
            current_row += 1

        self.lbl_slider_val = ctk.CTkLabel(
            self.left_frame, text="Selected Target: 65.0 °C", font=("Arial", 13)
        )
        self.lbl_slider_val.grid(
            row=current_row, column=0, columnspan=2, padx=20, pady=(20, 0), sticky="w"
        )
        current_row += 1

        self.slider_temp = ctk.CTkSlider(
            self.left_frame,
            from_=15.0,
            to=80.0,
            number_of_steps=650,
            command=self.on_slider_move,
        )
        self.slider_temp.set(65.0)
        self.slider_temp.grid(
            row=current_row, column=0, padx=(20, 10), pady=(5, 20), sticky="ew"
        )

        self.btn_set_temp = ctk.CTkButton(
            self.left_frame,
            text="Set Temperature",
            command=self.send_set_target_temp_cmd,
        )
        self.btn_set_temp.grid(
            row=current_row, column=1, padx=(10, 20), pady=(5, 20), sticky="ew"
        )

    def _create_right_frame(self):
        self.right_frame = ctk.CTkFrame(self)
        self.right_frame.grid(row=1, column=1, padx=(10, 20), pady=20, sticky="nsew")
        self.right_frame.grid_columnconfigure(0, weight=1)

        ctk.CTkLabel(
            self.right_frame, text="Debug Mode", font=("Arial", 18, "bold")
        ).grid(row=0, column=0, pady=(15, 10))

        self.btn_toggle_debug = ctk.CTkButton(
            self.right_frame,
            text="Enter Debug Mode",
            font=("Arial", 13, "bold"),
            command=self.toggle_debug_mode,
        )
        self.btn_toggle_debug.grid(row=1, column=0, padx=20, pady=10, sticky="ew")

        self.btn_self_test = ctk.CTkButton(
            self.right_frame,
            text="Run Self Test",
            font=("Arial", 13, "bold"),
            command=self.run_self_test,
        )
        self.btn_self_test.grid(row=2, column=0, padx=20, pady=10, sticky="ew")

        # Disabled PID calibration feature not implemented on zephyr device side

        # self.btn_pid_cal = ctk.CTkButton(
        #     self.right_frame,
        #     text="Calibrate PID",
        #     font=("Arial", 13, "bold"),
        #     command=self.run_pid_calibration,
        # )
        # self.btn_pid_cal.grid(row=3, column=0, padx=20, pady=10, sticky="ew")

    def auto_request_telemetry(self):
        if self.usb.is_connected:
            self.usb.send_command(CMD_GET_TELEMETRY)
        self.after(200, self.auto_request_telemetry)

    def poll_telemetry_queue(self):
        while True:
            data = self.usb.get_received_data()
            if not data:
                break

            if data.get("msg_type") == "telemetry" and self.usb.is_connected:
                self.latest_telemetry = data
                for key in self.telemetry_labels.keys():
                    if key not in data:
                        continue

                    val = data.get(key)
                    if "temp" in key:
                        fmt = f"{val:.2f} °C"
                    elif key == "rpm":
                        fmt = f"{val} RPM"
                    elif key == "duty":
                        fmt = f"{val} %"
                    elif key == "system_state":
                        state_int = int(val)
                        fmt = SYSTEM_STATE_MAP.get(state_int, f"Unknown ({state_int})")
                        if state_int != self.current_system_state:
                            self.current_system_state = state_int
                            self.update_widget_states()
                    else:
                        fmt = str(val)

                    self.telemetry_labels[key].configure(text=fmt)

        self.after(50, self.poll_telemetry_queue)

    def _reset_telemetry_strings(self):
        for key, (_, default_val) in self.telemetry_fields.items():
            self.telemetry_labels[key].configure(text=default_val)
        self.latest_telemetry = {}

    def monitor_connection_health(self):
        current_state = self.usb.is_connected
        if current_state != self._last_known_connection_state:
            self._last_known_connection_state = current_state
            if current_state:
                self.lbl_status_banner.configure(
                    text=f"CONNECTED: {self.usb.current_port}", fg_color="#2E7D32"
                )
            else:
                self.lbl_status_banner.configure(
                    text="DISCONNECTED: Searching for controller...", fg_color="#D32F2F"
                )
                self.current_system_state = SYSTEM_STATE_NORMAL
                self._reset_telemetry_strings()

            self.update_widget_states()

        self.after(200, self.monitor_connection_health)

    def update_widget_states(self):
        # action_buttons = [self.btn_toggle_debug, self.btn_self_test, self.btn_pid_cal]
        action_buttons = [self.btn_toggle_debug, self.btn_self_test]
        default_blue = ["#3B8ED0", "#1F6AA5"]

        if not self.usb.is_connected:
            for widget in [self.slider_temp, self.btn_set_temp] + action_buttons:
                widget.configure(state="disabled")
            self.btn_toggle_debug.configure(
                text="Enter Debug Mode", fg_color=default_blue
            )
            self.btn_self_test.configure(text="Run Self Test", fg_color=default_blue)

            # Disabled PID calibration feature not implemented on zephyr device side

            # self.btn_pid_cal.configure(text="Calibrate PID", fg_color=default_blue)
            return

        control_state = (
            "normal" if self.current_system_state == SYSTEM_STATE_NORMAL else "disabled"
        )
        for widget in [self.slider_temp, self.btn_set_temp]:
            widget.configure(state=control_state)

        if self.current_system_state == SYSTEM_STATE_NORMAL:
            self.btn_toggle_debug.configure(
                state="normal",
                text="Enter Debug Mode",
                fg_color="#1976D2",
                hover_color="#1565C0",
            )
            self.btn_self_test.configure(
                state="disabled", text="Run Self Test", fg_color=default_blue
            )

            # Disabled PID calibration feature not implemented on zephyr device side

            # self.btn_pid_cal.configure(
            #     state="disabled", text="Calibrate PID", fg_color=default_blue
            # )

        elif self.current_system_state == SYSTEM_STATE_DEBUG_IDLE:
            self.btn_toggle_debug.configure(
                state="normal",
                text="Exit Debug Mode",
                fg_color="#C62828",
                hover_color="#B71C1C",
            )
            self.btn_self_test.configure(
                state="normal",
                text="Run Self Test",
                fg_color=default_blue,
                hover_color=["#2B73B9", "#17548C"],
            )

            # Disabled PID calibration feature not implemented on zephyr device side

            # self.btn_pid_cal.configure(
            #     state="normal",
            #     text="Calibrate PID",
            #     fg_color=default_blue,
            #     hover_color=["#2B73B9", "#17548C"],
            # )

        elif self.current_system_state == SYSTEM_STATE_SELF_TEST:
            self.btn_toggle_debug.configure(
                state="disabled", text="Enter Debug Mode", fg_color=default_blue
            )
            self.btn_self_test.configure(
                state="disabled", text="Self Test Running...", fg_color="#E65100"
            )

            # Disabled PID calibration feature not implemented on zephyr device side

            # self.btn_pid_cal.configure(
            #     state="disabled", text="Calibrate PID", fg_color=default_blue
            # )

        elif self.current_system_state == SYSTEM_STATE_PID_CAL:
            self.btn_toggle_debug.configure(
                state="disabled", text="Enter Debug Mode", fg_color=default_blue
            )
            self.btn_self_test.configure(
                state="disabled", text="Run Self Test", fg_color=default_blue
            )
            
            # Disabled PID calibration feature not implemented on zephyr device side

            # self.btn_pid_cal.configure(
            #     state="disabled", text="PID Calibrating...", fg_color="#2E7D32"
            # )

    def toggle_debug_mode(self):
        if not self.usb.is_connected:
            return
        if self.current_system_state == SYSTEM_STATE_DEBUG_IDLE:
            self.usb.send_command(cmd_id=CMD_EXIT_DEBUG)
        else:
            self.usb.send_command(cmd_id=CMD_ENTER_DEBUG)

    def run_self_test(self):
        if (
            self.usb.is_connected
            and self.current_system_state == SYSTEM_STATE_DEBUG_IDLE
        ):
            self.usb.send_command(cmd_id=CMD_SELF_TEST)

            tester = Tester(
                usb_manager=self.usb, telemetry_provider=lambda: self.latest_telemetry
            )
            threading.Thread(
                target=tester.run_all, name="Self test thread", daemon=True
            ).start()

    def run_pid_calibration(self):
        if (
            self.usb.is_connected
            and self.current_system_state == SYSTEM_STATE_DEBUG_IDLE
        ):
            self.usb.send_command(cmd_id=CMD_CALIBRATE_PID)

    def on_slider_move(self, value):
        self.lbl_slider_val.configure(text=f"Selected Target: {value:.1f} °C")

    def send_set_target_temp_cmd(self):
        self.usb.send_command(
            cmd_id=CMD_SET_TARGET_TEMP, value=round(float(self.slider_temp.get()), 1)
        )

    def on_closing(self):
        self.usb.stop()
        self.destroy()
