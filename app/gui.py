import customtkinter as ctk
from usb import UsbManager


class GuiManager(ctk.CTk):

    def __init__(self, usb_manager: UsbManager):
        super().__init__()
        self.usb = usb_manager

        self.title("PWM fan controller")
        self.geometry("450x430")
        ctk.set_appearance_mode("Dark")
        ctk.set_default_color_theme("blue")

        self.grid_rowconfigure(0, weight=1)
        self.grid_rowconfigure(1, weight=1)
        self.grid_columnconfigure(0, weight=1)

        self._create_telemetry_frame()
        self._create_control_frame()

        self.protocol("WM_DELETE_WINDOW", self.on_closing)

        self.poll_telemetry_queue()

    def _create_telemetry_frame(self):
        self.telemetry_frame = ctk.CTkFrame(self)
        self.telemetry_frame.grid(
            row=0, column=0, padx=20, pady=(20, 10), sticky="nsew"
        )
        self.telemetry_frame.grid_columnconfigure((0, 1), weight=1)

        self.lbl_title = ctk.CTkLabel(
            self.telemetry_frame, text="Telemetry", font=("Arial", 18, "bold")
        )
        self.lbl_title.grid(row=0, column=0, columnspan=2, pady=10)

        self.lbl_temp = self._make_telemetry_row("Temperature:", "--- °C", 1)
        self.lbl_rpm = self._make_telemetry_row("Fan Speed:", "--- RPM", 2)
        self.lbl_duty = self._make_telemetry_row("PWM Duty:", "--- %", 3)
        self.lbl_target = self._make_telemetry_row("Current Target:", "--- °C", 4)

    def _make_telemetry_row(self, label_text, default_val, row_idx):
        lbl_name = ctk.CTkLabel(
            self.telemetry_frame, text=label_text, font=("Arial", 14)
        )
        lbl_name.grid(row=row_idx, column=0, padx=20, pady=5, sticky="w")

        lbl_val = ctk.CTkLabel(
            self.telemetry_frame, text=default_val, font=("Arial", 14, "bold")
        )
        lbl_val.grid(row=row_idx, column=1, padx=20, pady=5, sticky="e")
        return lbl_val

    def _create_control_frame(self):
        self.control_frame = ctk.CTkFrame(self)
        self.control_frame.grid(row=1, column=0, padx=20, pady=(10, 20), sticky="nsew")
        self.control_frame.grid_columnconfigure(0, weight=2)
        self.control_frame.grid_columnconfigure(1, weight=1)

        self.lbl_slider_val = ctk.CTkLabel(
            self.control_frame, text="Selected: 40.0 °C", font=("Arial", 13)
        )
        self.lbl_slider_val.grid(row=0, column=0, padx=10, pady=(10, 0), sticky="w")

        self.slider_temp = ctk.CTkSlider(
            self.control_frame,
            from_=15.0,
            to=80.0,
            number_of_steps=650,
            command=self.on_slider_move,
        )

        # Default slider value: 40.0 C
        self.slider_temp.set(40.0)
        self.slider_temp.grid(row=1, column=0, padx=10, pady=(0, 20), sticky="ew")

        self.btn_set_temp = ctk.CTkButton(
            self.control_frame, text="Set Temperature", command=self.send_temp
        )
        self.btn_set_temp.grid(row=1, column=1, padx=10, pady=(0, 20), sticky="ew")

        """ Debug Mode """
        self.btn_debug = ctk.CTkButton(
            self.control_frame,
            text="Enter Debug Mode",
            fg_color="#D32F2F",
            hover_color="#B71C1C",
            command=self.send_debug_cmd,
        )
        self.btn_debug.grid(
            row=2, column=0, columnspan=2, padx=10, pady=10, sticky="ew"
        )

    def poll_telemetry_queue(self):
        while True:
            data = self.usb.get_received_data()
            if not data:
                break

            self.lbl_temp.configure(text=f"{data['temp']:.2f} °C")
            self.lbl_rpm.configure(text=f"{data['rpm']} RPM")
            self.lbl_duty.configure(text=f"{data['duty']} %")
            self.lbl_target.configure(text=f"{data['target_temp']:.2f} °C")

        self.after(50, self.poll_telemetry_queue)

    def on_slider_move(self, value):
        self.lbl_slider_val.configure(text=f"Selected: {value:.1f} °C")

    def send_temp(self):
        target_temp = round(float(self.slider_temp.get()), 1)
        self.usb.send_command(cmd_id=1, value=target_temp)

    def send_debug_cmd(self):
        self.usb.send_command(cmd_id=2, value=None)

    def on_closing(self):
        print("Shutting down USB thread...")
        self.usb.stop()
        self.destroy()
