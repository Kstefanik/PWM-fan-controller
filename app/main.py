import sys
import serial
import serial.tools.list_ports
from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QVBoxLayout, QLabel
from PyQt5.QtCore import QTimer, Qt

def find_stm32_port():
    MY_VID = 0x2FE3
    MY_PID = 0x0001
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if port.vid == MY_VID and port.pid == MY_PID:
            print(f"--- Znaleziono STM32 na porcie: {port.device} ---")
            return port.device
    return None

PORT = find_stm32_port()
if PORT is None:
    print("BŁĄD: Nie znaleziono STM32!")
    sys.exit(1)

# Otwieramy port
ser = serial.Serial(PORT, 115200, timeout=0.01)
ser.dtr = True
ser.rts = True

class App(QWidget):
    def __init__(self):
        super().__init__()
        self.led_state = False

        # --- Interfejs GUI ---
        self.setWindowTitle("STM32 - Kontrola Przycisku")
        self.resize(400, 200)

        # Tytuł
        self.label_title = QLabel("Status Niebieskiego Przycisku")
        self.label_title.setAlignment(Qt.AlignCenter)
        self.label_title.setStyleSheet("font-weight: bold; font-size: 14px;")

        # Dynamiczny status przycisku
        self.status_label = QLabel("PRZYCISK: PUŚCZONY")
        self.status_label.setAlignment(Qt.AlignCenter)
        self.status_label.setStyleSheet("""
            font-size: 24px; 
            color: #7f8c8d; 
            background-color: #ecf0f1; 
            border-radius: 10px; 
            padding: 20px;
        """)

        # Przycisk do LED (nadal działa!)
        self.btn_led = QPushButton("Przełącz LED na płytce")
        self.btn_led.setFixedHeight(40)
        self.btn_led.clicked.connect(self.toggle_led)

        # Układanie elementów
        layout = QVBoxLayout()
        layout.addWidget(self.label_title)
        layout.addSpacing(10)
        layout.addWidget(self.status_label)
        layout.addSpacing(20)
        layout.addWidget(self.btn_led)
        self.setLayout(layout)

        # --- TIMER (Odbieranie danych) ---
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial_data)
        self.timer.start(30) # Sprawdzaj co 30ms

    def toggle_led(self):
        try:
            if self.led_state:
                ser.write(b"LED:0\r\n")
                self.led_state = False
            else:
                ser.write(b"LED:1\r\n")
                self.led_state = True
            ser.flush()
        except Exception as e:
            print(f"Błąd wysyłania: {e}")

    def read_serial_data(self):
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if line.startswith("BTN:"):
                    state = line.split(":")[1]
                    if state == "1":
                        self.status_label.setText("PRZYCISK: WCIŚNIĘTY!")
                        self.status_label.setStyleSheet("""
                            font-size: 24px; 
                            color: white; 
                            background-color: #e74c3c; 
                            font-weight: bold;
                            border-radius: 10px; 
                            padding: 20px;
                        """)
                    else:
                        self.status_label.setText("PRZYCISK: PUŚCZONY")
                        self.status_label.setStyleSheet("""
                            font-size: 24px; 
                            color: #7f8c8d; 
                            background-color: #ecf0f1; 
                            border-radius: 10px; 
                            padding: 20px;
                        """)
        except Exception as e:
            pass

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = App()
    window.show()
    sys.exit(app.exec_())

# TEST
