from usb import UsbManager
from gui import GuiManager

if __name__ == "__main__":
    usb_backend = UsbManager(port="/dev/ttyACM0", baud=115200)
    usb_backend.start()

    app = GuiManager(usb_manager=usb_backend)
    app.mainloop()