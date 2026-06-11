import threading
import queue
import time
import serial
from cobs import cobs
import cbor2
import struct


class UsbManager:
    def __init__(
        self,
        port: str = "/dev/ttyACM0",
        baud: int = 115200,
        rx_callback=None,
    ):
        self.port = port
        self.baud = baud
        self.on_data_received = rx_callback

        self._rx_queue = queue.Queue()
        self._tx_queue = queue.Queue()

        self.stop_signal = threading.Event()
        self.thread = None

    def start(self):
        self.stop_signal.clear()
        self.thread = threading.Thread(target=self._loop, daemon=True)
        self.thread.start()

    def stop(self):
        self.stop_signal.set()
        if self.thread:
            self.thread.join(timeout=1)

    def send_command(self, cmd_id: int, value=None):
        """
        Queues a command for sending over USB.

        Args:
            cmd_id (int): 1 for set_temp, 2 for debug.
            value: float for temp, None for debug.
        """
        if not isinstance(cmd_id, int):
            raise TypeError("Command ID must be an integer")
        self._tx_queue.put((cmd_id, value))

    def get_received_data(self):
        try:
            return self._rx_queue.get_nowait()
        except queue.Empty:
            return None

    def _build_packet(self, cmd_id, value) -> bytes:
        # set_target_temp
        if cmd_id == 1:
            f32_bytes = struct.pack(">f", float(value))
            cbor_float32 = bytes([0xFA]) + f32_bytes
            return bytes([0xA1, 0x01]) + cbor_float32

        # enter_debug_mode
        elif cmd_id == 2:  
            return bytes([0xA1, 0x02, 0xF6])

        else:
            return cbor2.dumps({cmd_id: value})

    def _loop(self):
        ser = None
        try:
            ser = serial.Serial(self.port, self.baud, timeout=0.1, dsrdtr=True)
            time.sleep(0.2)
        except Exception as e:
            print(f"Error opening port: {e}")
            return

        rx_buffer = bytearray()

        while not self.stop_signal.is_set():

            """Transmitting data"""
            try:
                while not self._tx_queue.empty():
                    cmd_id, value = self._tx_queue.get_nowait()

                    tx_cbor_data = self._build_packet(cmd_id, value)

                    tx_cobs_data = cobs.encode(tx_cbor_data)
                    tx_packet = tx_cobs_data + b"\x00"

                    ser.write(tx_packet)
                    ser.flush()
            except Exception as e:
                print(f"Encoding / TX Error: {e}")

            """ Receiving data """
            try:
                if ser.in_waiting > 0:
                    rx_buffer.extend(ser.read(ser.in_waiting))

                    while b"\x00" in rx_buffer:
                        rx_packet, rx_buffer = rx_buffer.split(b"\x00", 1)

                        if not rx_packet:
                            continue

                        try:
                            rx_cbor_data = cobs.decode(rx_packet)
                            rx_data = cbor2.loads(rx_cbor_data)

                            if isinstance(rx_data, list) and len(rx_data) == 4:
                                rx_decoded_data = {
                                    "temp": float(rx_data[0]),
                                    "rpm": int(rx_data[1]),
                                    "duty": int(rx_data[2]),
                                    "target_temp": float(rx_data[3]),
                                }
                                self._rx_queue.put(rx_decoded_data)
                                if self.on_data_received:
                                    self.on_data_received()
                            else:
                                print(f"Unexpected telemetry format: {rx_data}")

                        except Exception as e:
                            print(f"Packet parse Error: {e}")

            except Exception as e:
                print(f"Decoding / RX Error: {e}")

            time.sleep(0.001)

        if ser and ser.is_open:
            ser.close()
