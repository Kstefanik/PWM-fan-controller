import threading
import queue
import time
import struct
import logging
import serial
import serial.tools.list_ports
from cobs import cobs
import cbor2

from constants import (
    CMD_SET_TARGET_TEMP,
    _NO_ARG_COMMANDS,
    TLM_KEY_TEMP,
    TLM_KEY_RPM,
    TLM_KEY_DUTY,
    TLM_KEY_TARGET_TEMP,
    TLM_KEY_SYSTEM_STATE,
)

logger = logging.getLogger("FanController.UsbManager")

try:
    import termios

    SERIAL_ERRORS = (serial.SerialException, OSError, termios.error)
except ImportError:
    SERIAL_ERRORS = (serial.SerialException, OSError)


class UsbManager:
    def __init__(
        self, vendor_id: int = 0x6767, product_id: int = 0x0001, baud: int = 115200
    ):
        self.vid = vendor_id
        self.pid = product_id
        self.baud = baud

        self._rx_queue = queue.Queue()
        self._tx_queue = queue.Queue()

        self.stop_signal = threading.Event()
        self.thread = None

        self.is_connected = False
        self.current_port = None

    def start(self):
        self.stop_signal.clear()
        self.thread = threading.Thread(
            target=self._loop, name="UsbWorkerThread", daemon=True
        )
        self.thread.start()

    def stop(self):
        self.stop_signal.set()
        if self.thread:
            self.thread.join(timeout=1.5)

    def send_command(self, cmd_id: int, value=None):
        if not self.is_connected:
            return
        if not isinstance(cmd_id, int):
            raise TypeError("Command ID must be an integer")
        self._tx_queue.put((cmd_id, value))

    def get_received_data(self):
        try:
            return self._rx_queue.get_nowait()
        except queue.Empty:
            return None

    def find_device_port(self):
        ports = serial.tools.list_ports.comports()
        for port in ports:
            if port.vid == self.vid and port.pid == self.pid:
                return port.device
        return None

    def _build_packet(self, cmd_id: int, value) -> bytes:
        if cmd_id == CMD_SET_TARGET_TEMP:
            f32_bytes = struct.pack(">f", float(value))
            return bytes([0xA1, cmd_id, 0xFA]) + f32_bytes

        if cmd_id in _NO_ARG_COMMANDS:
            return bytes([0xA1, cmd_id, 0xF6])

        return cbor2.dumps({cmd_id: value})

    def _route_response(self, rx_data):
        if not isinstance(rx_data, dict):
            return

        tel = None
        if TLM_KEY_TEMP in rx_data and isinstance(rx_data[TLM_KEY_TEMP], dict):
            tel = rx_data[TLM_KEY_TEMP]
        elif TLM_KEY_TEMP in rx_data and isinstance(
            rx_data[TLM_KEY_TEMP], (int, float)
        ):
            tel = rx_data

        if tel is not None:
            decoded = {
                "msg_type": "telemetry",
                "temp": float(tel.get(TLM_KEY_TEMP, 0.0)),
                "rpm": int(tel.get(TLM_KEY_RPM, 0)),
                "duty": int(tel.get(TLM_KEY_DUTY, 0)),
                "target_temp": float(tel.get(TLM_KEY_TARGET_TEMP, 0.0)),
                "system_state": int(tel.get(TLM_KEY_SYSTEM_STATE, 0)),
            }
            self._rx_queue.put(decoded)
            return


    def _attempt_connection(self):
        port_name = self.find_device_port()
        if port_name is None:
            return None

        try:
            ser = serial.Serial(port_name, self.baud, timeout=0.1, dsrdtr=True)
            time.sleep(0.2)

            while not self._tx_queue.empty():
                try:
                    self._tx_queue.get_nowait()
                except queue.Empty:
                    break

            self.current_port = port_name
            self.is_connected = True
            logger.info(f"Successfully attached to device on {port_name}")
            return ser
        except Exception as e:
            logger.debug(f"Connection attempt to {port_name} failed: {e}")
            return None

    def _handle_tx(self, ser):
        while not self._tx_queue.empty():
            try:
                cmd_id, value = self._tx_queue.get_nowait()
                raw = self._build_packet(cmd_id, value)
                packet = cobs.encode(raw) + b"\x00"
                ser.write(packet)
            except queue.Empty:
                break
        ser.flush()

    def _handle_rx(self, ser, rx_buffer: bytearray):
        if ser.in_waiting > 0:
            rx_buffer.extend(ser.read(ser.in_waiting))
            while b"\x00" in rx_buffer:
                frame, remaining = rx_buffer.split(b"\x00", 1)
                rx_buffer[:] = remaining
                if not frame:
                    continue
                try:
                    rx_data = cbor2.loads(cobs.decode(frame))
                    self._route_response(rx_data)
                except Exception as parse_err:
                    logger.error(f"Frame Parse Error: {parse_err}")

    def _handle_disconnect(self, ser, error):
        logger.warning(f"Connection broken or hardware reset caught: {error}")
        self.is_connected = False
        self.current_port = None
        if ser:
            try:
                ser.close()
            except Exception:
                pass
        time.sleep(1.0)

    def _loop(self):
        rx_buffer = bytearray()
        ser = None

        while not self.stop_signal.is_set():
            if not self.is_connected:
                ser = self._attempt_connection()
                if ser is None:
                    time.sleep(1.0)
                    continue

            try:
                self._handle_tx(ser)
                self._handle_rx(ser, rx_buffer)
            except SERIAL_ERRORS as disconnect_error:
                self._handle_disconnect(ser, disconnect_error)
                ser = None

            time.sleep(0.001)

        if ser and ser.is_open:
            ser.close()
