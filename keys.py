from evdev import InputDevice, categorize, ecodes, list_devices
import select
import logging

# Configure logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

def find_keyboards():
    devices = [InputDevice(path) for path in list_devices()]
    keyboards = []
    for device in devices:
        logging.info(f"Device found: {device.path}, Name: {device.name}")
        capabilities = device.capabilities().get(ecodes.EV_KEY, [])
        # Check if the device has key capabilities
        if capabilities:
            logging.info(f"Keyboard found: {device.path}")
            keyboards.append(device)
    if not keyboards:
        raise Exception("No keyboard found")
    return keyboards

def main():
    devices = find_keyboards()
    logging.info("Starting to read events from the keyboards...")
    
    device_fds = {dev.fd: dev for dev in devices}
    while True:
        r, w, x = select.select(device_fds.keys(), [], [])
        for fd in r:
            device = device_fds[fd]
            for event in device.read():
                if event.type == ecodes.EV_KEY:
                    key_event = categorize(event)
                    logging.debug(f"Key event: {key_event}")
                    if hasattr(key_event, 'keycode'):
                        key = key_event.keycode
                        if isinstance(key, list):
                            key = key[0]  # Handle cases where keycode is a list
                        key = key.lower()
                        logging.info(f"Key {key} event detected, state: {key_event.keystate}")

if __name__ == "__main__":
    main()
