
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
