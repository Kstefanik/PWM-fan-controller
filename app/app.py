# import sys
# import serial
# import serial.tools.list_ports
# from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QVBoxLayout, QHBoxLayout, QLabel
# from PyQt5.QtCore import QTimer, Qt
# from PyQt5.QtGui import QPixmap

# def find_stm32_port():
#     # Numery VID/PID zgodne z Twoim prj_3.conf[cite: 4]
#     MY_VID = 0x2FE3
#     MY_PID = 0x0001
#     ports = serial.tools.list_ports.comports()
#     for port in ports:
#         if port.vid == MY_VID and port.pid == MY_PID:
#             print(f"--- Znaleziono STM32 na porcie: {port.device} ---")
#             return port.device
#     return None

# # --- Próba otwarcia portu na starcie ---
# PORT = find_stm32_port()
# ser = None

# if PORT:
#     try:
#         # Konfiguracja portu szeregowego[cite: 1]
#         ser = serial.Serial(PORT, 115200, timeout=0.01)
#         ser.dtr = True
#         ser.rts = True
#     except Exception as e:
#         print(f"Błąd otwierania portu: {e}")
# else:
#     print("OSTRZEŻENIE: STM32 nieodnaleziony. GUI uruchomione w trybie podglądu.")

# class App(QWidget):
#     def __init__(self):
#         super().__init__()
#         self.led_state = False

#         self.setWindowTitle("STM32 - Sterownik Wiatraka Endorfy")
#         self.resize(650, 300)

#         # Układ poziomy: Lewo (Obraz) | Prawo (Kontrolki)
#         main_layout = QHBoxLayout()
        
#         # --- SEKCJA LEWA: GRAFIKA (WebP) ---
#         self.image_label = QLabel()
#         pixmap = QPixmap("wiatrak.webp") 
        
#         if pixmap.isNull():
#             self.image_label.setText("BRAK PLIKU\nwiatrak.webp")
#             self.image_label.setStyleSheet("color: red; font-weight: bold;")
#         else:
#             # Skalowanie z zachowaniem proporcji[cite: 2]
#             self.image_label.setPixmap(pixmap.scaled(250, 250, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        
#         self.image_label.setAlignment(Qt.AlignCenter)
        
#         # --- SEKCJA PRAWA: KONTROLKI ---
#         controls_layout = QVBoxLayout()

#         self.label_title = QLabel("KONTROLA URZĄDZENIA")
#         self.label_title.setStyleSheet("font-weight: bold; font-size: 16px; color: #2c3e50;")
#         self.label_title.setAlignment(Qt.AlignCenter)

#         # Status połączenia i przycisku[cite: 1, 5]
#         status_init = "PRZYCISK: PUŚCZONY" if ser else "STATUS: ROZŁĄCZONO"
#         self.status_label = QLabel(status_init)
#         self.status_label.setAlignment(Qt.AlignCenter)
#         self.status_label.setStyleSheet("""
#             font-size: 18px; 
#             color: #7f8c8d; 
#             background-color: #ecf0f1; 
#             border-radius: 10px; 
#             padding: 20px;
#             border: 1px solid #bdc3c7;
#         """)

#         self.btn_led = QPushButton("Przełącz LED / Wiatrak")
#         self.btn_led.setFixedHeight(50)
#         self.btn_led.setStyleSheet("font-weight: bold; font-size: 14px;")
#         self.btn_led.clicked.connect(self.toggle_led)

#         controls_layout.addWidget(self.label_title)
#         controls_layout.addSpacing(10)
#         controls_layout.addWidget(self.status_label)
#         controls_layout.addSpacing(20)
#         controls_layout.addWidget(self.btn_led)

#         # Łączymy sekcje
#         main_layout.addWidget(self.image_label, stretch=1)
#         main_layout.addLayout(controls_layout, stretch=1)
        
#         self.setLayout(main_layout)

#         # Timer do sprawdzania danych co 30ms[cite: 1]
#         self.timer = QTimer()
#         self.timer.timeout.connect(self.read_serial_data)
#         self.timer.start(30)

#     def toggle_led(self):
#         if ser is None:
#             print("Błąd: Nie wykryto STM32. Podłącz płytkę!")
#             return

#         try:
#             # Wysyłanie komend tekstowych do STM32[cite: 5]
#             if self.led_state:
#                 ser.write(b"LED:0\r\n")
#                 self.led_state = False
#             else:
#                 ser.write(b"LED:1\r\n")
#                 self.led_state = True
#             ser.flush()
#         except Exception as e:
#             print(f"Błąd komunikacji: {e}")

#     def read_serial_data(self):
#         if ser is None:
#             return

#         try:
#             # Odbieranie statusu przycisku przez ZBus -> USB CDC
#             if ser.in_waiting > 0:
#                 line = ser.readline().decode('utf-8', errors='ignore').strip()
                
#                 if line.startswith("BTN:"):
#                     state = line.split(":")[1]
#                     if state == "1":
#                         self.status_label.setText("WCIŚNIĘTY!")
#                         self.status_label.setStyleSheet("font-size: 18px; color: white; background-color: #e74c3c; font-weight: bold; border-radius: 10px; padding: 20px;")
#                     else:
#                         self.status_label.setText("PUŚCZONY")
#                         self.status_label.setStyleSheet("font-size: 18px; color: #7f8c8d; background-color: #ecf0f1; border-radius: 10px; padding: 20px;")
#         except Exception:
#             pass

# if __name__ == "__main__":
#     app = QApplication(sys.argv)
#     window = App()
#     window.show()
#     sys.exit(app.exec_())








# import sys
# import serial
# import serial.tools.list_ports
# from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QVBoxLayout, QHBoxLayout, QLabel
# from PyQt5.QtCore import QTimer, Qt
# from PyQt5.QtGui import QPixmap

# # Stałe identyfikacyjne z Twojego prj_3.conf
# MY_VID = 0x2FE3
# MY_PID = 0x0001

# class App(QWidget):
#     def __init__(self):
#         super().__init__()
#         self.ser = None
#         self.led_state = False

#         self.setWindowTitle("STM32 - Sterownik Endorfy (Auto-Connect)")
#         self.resize(700, 350)

#         # --- Interfejs GUI ---
#         main_layout = QHBoxLayout()
        
#         # LEWO: Obrazek
#         self.image_label = QLabel()
#         pixmap = QPixmap("wiatrak.webp") 
#         if not pixmap.isNull():
#             self.image_label.setPixmap(pixmap.scaled(250, 250, Qt.KeepAspectRatio, Qt.SmoothTransformation))
#         self.image_label.setAlignment(Qt.AlignCenter)
        
#         # PRAWO: Kontrolki
#         controls_layout = QVBoxLayout()
        
#         # Mały wskaźnik połączenia na górze
#         self.conn_label = QLabel("STATUS: SZUKAM STM32...")
#         self.conn_label.setAlignment(Qt.AlignCenter)
#         self.conn_label.setStyleSheet("font-size: 12px; color: #7f8c8d; background-color: #ecf0f1; padding: 5px;")

#         # DUŻY WSKAŹNIK PRZYCISKU (to ten co ma być czerwony)
#         self.btn_status_label = QLabel("PRZYCISK: CZEKAM...")
#         self.btn_status_label.setAlignment(Qt.AlignCenter)
#         self.btn_status_label.setStyleSheet("""
#             font-size: 20px; 
#             color: #7f8c8d; 
#             background-color: #ecf0f1; 
#             border-radius: 10px; 
#             padding: 20px;
#             font-weight: bold;
#         """)

#         self.btn_led = QPushButton("Przełącz LED / Wiatrak")
#         self.btn_led.setFixedHeight(50)
#         self.btn_led.clicked.connect(self.toggle_led)

#         controls_layout.addWidget(self.conn_label)
#         controls_layout.addWidget(self.btn_status_label)
#         controls_layout.addSpacing(10)
#         controls_layout.addWidget(self.btn_led)
        
#         main_layout.addWidget(self.image_label)
#         main_layout.addLayout(controls_layout)
#         self.setLayout(main_layout)

#         # --- TIMER: Sprawdza port i dane ---
#         self.timer = QTimer()
#         self.timer.timeout.connect(self.manage_connection)
#         self.timer.start(50) # Szybkie sprawdzanie dla płynności przycisku

#     def find_stm32_port(self):
#         ports = serial.tools.list_ports.comports()
#         for port in ports:
#             if port.vid == MY_VID and port.pid == MY_PID:
#                 return port.device
#         return None

#     def manage_connection(self):
#         if self.ser is None or not self.ser.is_open:
#             port = self.find_stm32_port()
#             if port:
#                 try:
#                     self.ser = serial.Serial(port, 115200, timeout=0.01)
#                     self.ser.dtr = True
#                     self.ser.rts = True
#                     self.conn_label.setText(f"POŁĄCZONO: {port}")
#                     self.conn_label.setStyleSheet("color: white; background-color: #2ecc71;")
#                 except Exception:
#                     self.ser = None
#             else:
#                 self.conn_label.setText("STATUS: BRAK STM32 (PODŁĄCZ KABEL)")
#                 self.conn_label.setStyleSheet("color: white; background-color: #e67e22;")
#                 self.btn_status_label.setText("PRZYCISK: ROZŁĄCZONO")
#                 self.btn_status_label.setStyleSheet("font-size: 20px; color: #7f8c8d; background-color: #ecf0f1; border-radius: 10px; padding: 20px;")
#         else:
#             self.read_serial_data()

#     def read_serial_data(self):
#         try:
#             if self.ser.in_waiting > 0:
#                 line = self.ser.readline().decode('utf-8', errors='ignore').strip()
#                 # LOGIKA CZERWONEGO KOMUNIKATU
#                 if line.startswith("BTN:"):
#                     state = line.split(":")[1]
#                     if state == "1":
#                         self.btn_status_label.setText("PRZYCISK: WCIŚNIĘTY!")
#                         self.btn_status_label.setStyleSheet("""
#                             font-size: 20px; color: white; background-color: #e74c3c; 
#                             font-weight: bold; border-radius: 10px; padding: 20px;
#                         """)
#                     else:
#                         self.btn_status_label.setText("PRZYCISK: PUŚCZONY")
#                         self.btn_status_label.setStyleSheet("""
#                             font-size: 20px; color: #7f8c8d; background-color: #ecf0f1; 
#                             border-radius: 10px; padding: 20px;
#                         """)
#         except Exception:
#             self.ser = None # Jeśli odepniesz kabel, wraca do szukania

#     def toggle_led(self):
#         if self.ser and self.ser.is_open:
#             try:
#                 cmd = b"LED:1\r\n" if not self.led_state else b"LED:0\r\n"
#                 self.ser.write(cmd)
#                 self.ser.flush()
#                 self.led_state = not self.led_state
#             except Exception:
#                 self.ser = None

# if __name__ == "__main__":
#     app = QApplication(sys.argv)
#     window = App()
#     window.show()
#     sys.exit(app.exec_())








# import sys
# import serial
# import serial.tools.list_ports
# from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSlider
# from PyQt5.QtCore import QTimer, Qt

# MY_VID = 0x2FE3
# MY_PID = 0x0001

# class App(QWidget):
#     def __init__(self):
#         super().__init__()
#         self.ser = None
#         self.setWindowTitle("STM32 - Sterownik z Suwakiem")
#         self.resize(400, 300)

#         layout = QVBoxLayout()

#         self.conn_label = QLabel("STATUS: SZUKAM STM32...")
#         layout.addWidget(self.conn_label)

#         # Wskaźnik przycisku
#         self.btn_status = QLabel("PRZYCISK: CZEKAM...")
#         self.btn_status.setStyleSheet("background-color: #ecf0f1; padding: 20px; font-weight: bold;")
#         self.btn_status.setAlignment(Qt.AlignCenter)
#         layout.addWidget(self.btn_status)

#         # SUWAK (Slider) do ramki kolegi
#         layout.addWidget(QLabel("Moc wiatraka (Parametr D):"))
#         self.slider = QSlider(Qt.Horizontal)
#         self.slider.setRange(0, 100)
#         self.slider.valueChanged.connect(self.send_frame)
#         layout.addWidget(self.slider)

#         self.setLayout(layout)

#         self.timer = QTimer()
#         self.timer.timeout.connect(self.manage_connection)
#         self.timer.start(50)

#     def manage_connection(self):
#         if self.ser is None or not self.ser.is_open:
#             ports = serial.tools.list_ports.comports()
#             for port in ports:
#                 if port.vid == MY_VID and port.pid == MY_PID:
#                     try:
#                         self.ser = serial.Serial(port.device, 115200, timeout=0.01)
#                         self.conn_label.setText(f"POŁĄCZONO: {port.device}")
#                         self.conn_label.setStyleSheet("color: green;")
#                     except: pass
#         else:
#             self.read_data()

#     def send_frame(self):
#         if self.ser and self.ser.is_open:
#             val = self.slider.value()
#             # Format kolegi: T:temp, R:rpm, D:duty, TRG:target
#             frame = f"T:0.0,R:0,D:{val}.0,TRG:0.0\n"
#             self.ser.write(frame.encode())

#     def read_data(self):
#         try:
#             if self.ser.in_waiting > 0:
#                 line = self.ser.readline().decode().strip()
#                 if "BTN:1" in line:
#                     self.btn_status.setStyleSheet("background-color: red; color: white;")
#                 elif "BTN:0" in line:
#                     self.btn_status.setStyleSheet("background-color: #ecf0f1; color: black;")
#         except: self.ser = None

# if __name__ == "__main__":
#     app = QApplication(sys.argv)
#     window = App()
#     window.show()
#     sys.exit(app.exec_())







# import sys
# import serial
# import serial.tools.list_ports
# from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QSlider
# from PyQt5.QtCore import QTimer, Qt
# from PyQt5.QtGui import QPixmap

# MY_VID = 0x2FE3
# MY_PID = 0x0001

# class App(QWidget):
#     def __init__(self):
#         super().__init__()
#         self.ser = None
#         self.setWindowTitle("STM32 - Sterownik Endorfy")
#         self.resize(700, 400)

#         main_layout = QHBoxLayout()
        
#         # LEWO: Przywrócona grafika[cite: 13]
#         self.image_label = QLabel()
#         pixmap = QPixmap("wiatrak.webp") 
#         if not pixmap.isNull():
#             self.image_label.setPixmap(pixmap.scaled(300, 300, Qt.KeepAspectRatio, Qt.SmoothTransformation))
#         else:
#             self.image_label.setText("BRAK PLIKU wiatrak.webp")
#         main_layout.addWidget(self.image_label)

#         # PRAWO: Kontrolki
#         controls_layout = QVBoxLayout()
        
#         self.conn_label = QLabel("STATUS: SZUKAM STM32...")
#         controls_layout.addWidget(self.conn_label)

#         self.btn_status = QLabel("PRZYCISK: CZEKAM...")
#         self.btn_status.setStyleSheet("font-size: 18px; background-color: #ecf0f1; padding: 20px; border-radius: 10px;")
#         self.btn_status.setAlignment(Qt.AlignCenter)
#         controls_layout.addWidget(self.btn_status)

#         controls_layout.addSpacing(20)
#         controls_layout.addWidget(QLabel("Moc wiatraka (Suwak):"))
#         self.slider = QSlider(Qt.Horizontal)
#         self.slider.setRange(0, 100)
#         self.slider.valueChanged.connect(self.send_frame)
#         controls_layout.addWidget(self.slider)

#         main_layout.addLayout(controls_layout)
#         self.setLayout(main_layout)

#         self.timer = QTimer()
#         self.timer.timeout.connect(self.manage_connection)
#         self.timer.start(50)

#     def find_port(self):
#         for port in serial.tools.list_ports.comports():
#             if port.vid == MY_VID and port.pid == MY_PID:
#                 return port.device
#         return None

#     def manage_connection(self):
#         if self.ser is None or not self.ser.is_open:
#             port = self.find_port()
#             if port:
#                 try:
#                     self.ser = serial.Serial(port, 115200, timeout=0.01)
#                     self.conn_label.setText(f"POŁĄCZONO: {port}")
#                     self.conn_label.setStyleSheet("color: white; background-color: green;")
#                 except: pass
#         else:
#             self.read_data()

#     def send_frame(self):
#         if self.ser and self.ser.is_open:
#             val = self.slider.value()
#             # Ramka kolegi: D to moc[cite: 13]
#             frame = f"T:0.0,R:0,D:{val}.0,TRG:0.0\n"
#             self.ser.write(frame.encode())

#     def read_data(self):
#         try:
#             if self.ser.in_waiting > 0:
#                 line = self.ser.readline().decode('utf-8', errors='ignore').strip()
#                 if "BTN:1" in line:
#                     self.btn_status.setText("WCIŚNIĘTY!")
#                     self.btn_status.setStyleSheet("font-size: 18px; color: white; background-color: red; font-weight: bold;")
#                 elif "BTN:0" in line:
#                     self.btn_status.setText("PUŚCZONY")
#                     self.btn_status.setStyleSheet("font-size: 18px; color: black; background-color: #ecf0f1;")
#         except: self.ser = None

# if __name__ == "__main__":
#     app = QApplication(sys.argv)
#     window = App()
#     window.show()
#     sys.exit(app.exec_())




import sys
import serial
import serial.tools.list_ports
from PyQt5.QtWidgets import (QApplication, QWidget, QVBoxLayout, QHBoxLayout, 
                             QLabel, QSlider, QFrame)
from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtGui import QPixmap, QFont

MY_VID = 0x2FE3
MY_PID = 0x0001

class App(QWidget):
    def __init__(self):
        super().__init__()
        self.ser = None
        self.current_trg = 40.0  # Domyślna temp. docelowa
        
        self.setWindowTitle("Endorfy Fan Control Panel")
        self.resize(850, 500)
        self.setStyleSheet("background-color: #1a1a1a; color: white; font-family: 'Segoe UI';")

        main_layout = QHBoxLayout()
        
        # --- LEWO: Grafika ---
        self.image_label = QLabel()
        pixmap = QPixmap("wiatrak.webp") 
        if not pixmap.isNull():
            self.image_label.setPixmap(pixmap.scaled(350, 350, Qt.KeepAspectRatio, Qt.SmoothTransformation))
        else:
            self.image_label.setText("FAN IMAGE")
        main_layout.addWidget(self.image_label)

        # --- PRAWO: Kontrolki ---
        controls_layout = QVBoxLayout()
        
        # Status połączenia
        self.conn_label = QLabel("SZUKAM STEROWNIKA...")
        self.conn_label.setStyleSheet("background-color: #333; padding: 5px; border-radius: 5px;")
        self.conn_label.setAlignment(Qt.AlignCenter)
        controls_layout.addWidget(self.conn_label)

        # Panel Odczytów (T, R, D - nadpisywane przez STM32)
        data_panel = QFrame()
        data_panel.setStyleSheet("background-color: #262626; border-radius: 10px; padding: 10px;")
        data_layout = QVBoxLayout(data_panel)
        
        self.label_t = QLabel("🌡️ Temperatura: -- °C")
        self.label_r = QLabel("🔄 Obroty: -- RPM")
        self.label_d = QLabel("⚡ Moc (PWM): -- %")
        
        for lbl in [self.label_t, self.label_r, self.label_d]:
            lbl.setStyleSheet("font-size: 16px; margin: 5px;")
            data_layout.addWidget(lbl)
        
        controls_layout.addWidget(data_panel)

        # Sekcja Suwaka (TRG - to pole kontroluje PC)
        controls_layout.addSpacing(20)
        self.trg_display = QLabel("🎯 Temperatura Docelowa: 40.0 °C")
        self.trg_display.setStyleSheet("font-weight: bold; color: #00ff88; font-size: 14px;")
        controls_layout.addWidget(self.trg_display)

        # Suwak: Zakres 20.0 - 80.0 z krokiem 0.2
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(100, 400) 
        self.slider.setValue(200) # Start na 40.0
        self.slider.valueChanged.connect(self.update_target)
        controls_layout.addWidget(self.slider)

        # Przycisk bezpieczeństwa (widok z STM32)
        self.btn_status = QLabel("PRZYCISK BEZPIECZEŃSTWA: OK")
        self.btn_status.setStyleSheet("background-color: #2ecc71; color: white; padding: 15px; border-radius: 10px; margin-top: 10px;")
        self.btn_status.setAlignment(Qt.AlignCenter)
        controls_layout.addWidget(self.btn_status)

        main_layout.addLayout(controls_layout)
        self.setLayout(main_layout)

        self.timer = QTimer()
        self.timer.timeout.connect(self.manage_connection)
        self.timer.start(50)

    def update_target(self):
        """PC nadpisuje wartość TRG i wysyła ją do STM32"""
        self.current_trg = self.slider.value() / 5.0
        self.trg_display.setText(f"🎯 Temperatura Docelowa: {self.current_trg:.1f} °C")
        self.send_frame()

    def find_port(self):
        for port in serial.tools.list_ports.comports():
            if port.vid == MY_VID and port.pid == MY_PID:
                return port.device
        return None

    def manage_connection(self):
        if self.ser is None or not self.ser.is_open:
            port = self.find_port()
            if port:
                try:
                    self.ser = serial.Serial(port, 115200, timeout=0.01)
                    self.conn_label.setText(f"POŁĄCZONO: {port}")
                    self.conn_label.setStyleSheet("background-color: #2ecc71; color: white;")
                except: pass
        else:
            self.read_data()

    def send_frame(self):
        """PC -> STM32: Wysyłamy pełną ramkę, ale tylko TRG ma nową wartość"""
        if self.ser and self.ser.is_open:
            # Utrzymujemy format ramki kolegi: T, R i D na zero (bo to pola dla STM32)
            frame = f"T:0.00,R:0,D:0.0,TRG:{self.current_trg:.2f}\n"
            self.ser.write(frame.encode())

    def read_data(self):
        """STM32 -> PC: Odbieramy ramkę i nadpisujemy tylko T, R, D oraz BTN"""
        try:
            if self.ser.in_waiting > 0:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                
                parts = line.split(',')
                for part in parts:
                    # Nadpisujemy pola pomiarowe
                    if part.startswith("T:"):
                        self.label_t.setText(f"🌡️ Temperatura: {part.split(':')[1]} °C")
                    elif part.startswith("R:"):
                        self.label_r.setText(f"🔄 Obroty: {part.split(':')[1]} RPM")
                    elif part.startswith("D:"):
                        self.label_d.setText(f"⚡ Moc (PWM): {part.split(':')[1]} %")
                    
                    # Obsługa przycisku (nadpisuje kolor i tekst labela BTN)
                    elif part.startswith("BTN:"):
                        state = part.split(':')[1]
                        if "1" in state:
                            self.btn_status.setText("STOP AWARYJNY!")
                            self.btn_status.setStyleSheet("background-color: red; color: white; font-weight: bold; border-radius: 10px;")
                        else:
                            self.btn_status.setText("PRZYCISK BEZPIECZEŃSTWA: OK")
                            self.btn_status.setStyleSheet("background-color: #2ecc71; color: white; border-radius: 10px;")
                
                # UWAGA: Pole TRG w odebranej ramce ignorujemy, 
                # żeby suwak na PC był jedynym źródłem prawdy dla nastawy.
        except: 
            self.ser = None
            self.conn_label.setText("STATUS: ROZŁĄCZONO")
            self.conn_label.setStyleSheet("background-color: #e74c3c;")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = App()
    window.show()
    sys.exit(app.exec_())