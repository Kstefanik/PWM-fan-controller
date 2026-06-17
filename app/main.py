import logging
from usb import UsbManager
from gui import GuiManager

if __name__ == "__main__":
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] (%(threadName)s) %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )

    logging.info("Initializing PWM Fan Controller Host System Engine...")

    usb_backend = UsbManager(vendor_id=0x2FE3, product_id=0x0001, baud=115200)
    usb_backend.start()

    app = GuiManager(usb_manager=usb_backend)
    app.mainloop()
