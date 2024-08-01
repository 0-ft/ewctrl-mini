import csv
import errno
import json
import logging
import select
import threading
import time

import pyudev
from evdev import InputDevice, categorize, ecodes, list_devices
from fader_client import FaderClient
from wled_client import WLEDClient


class KeyboardCommander:
    DEBOUNCE_TIME = 0.3  # Time in seconds to debounce udev events

    def __init__(self, server_manager, keymap_file, multipliers_file=None):
        self.multipliers = self.load_multipliers(multipliers_file) if multipliers_file else {}
        self.keymap = self.load_keymap(keymap_file)
        self.server_manager = server_manager
        self.devices = {}
        self.context = pyudev.Context()
        self.monitor = pyudev.Monitor.from_netlink(self.context)
        self.monitor.filter_by('input')
        self.observer = pyudev.MonitorObserver(self.monitor, self.handle_udev_event, name='monitor-observer')
        self.observer.start()
        self.debounce_timer = None
        self.debounce_lock = threading.Lock()
        self.update_keyboards()

    def load_multipliers(self, filename):
        multipliers = {}
        with open(filename, mode='r') as file:
            reader = csv.DictReader(file)
            for row in reader:
                name = row['name'].lower()
                multipliers[name] = json.loads(row['multiplier'])
        return multipliers

    def load_keymap(self, filename):
        keymap = {}
        with open(filename, mode='r') as file:
            reader = csv.DictReader(file)
            for row in reader:
                key = row['key'].lower()
                # keymap[key] = {
                #     'port': int(row['port']),
                #     'command': row['command']
                # }
                keymap[key] = []
                # print(row)
                if(row["ewctrl"]):
                    command = FaderClient.parse_command(row["ewctrl"], self.multipliers)
                    logging.info(f"Adding command: {command} for key {key}")
                    keymap[key].append(command)
                if("wled" in row and row["wled"]):
                    keymap[key].append(WLEDClient.parse_command(row["wled"]))
        return keymap

    def find_keyboards(self):
        devices = [InputDevice(path) for path in list_devices()]
        # print("DEVS", devices)
        keyboards = []
        for device in devices:
            logging.debug(f"Device found: {device.path}, Name: {device.name}")
            capabilities = device.capabilities().get(ecodes.EV_KEY, [])
            if capabilities and self.is_keyboard(capabilities):
                logging.debug(f"Keyboard found: {device.path}")
                keyboards.append(device)
        return keyboards

    def is_keyboard(self, capabilities):
        # Define key capabilities typical of keyboards to filter out mouse events
        keyboard_keys = {ecodes.KEY_A, ecodes.KEY_B, ecodes.KEY_C, ecodes.KEY_D, ecodes.KEY_E, ecodes.KEY_F, ecodes.KEY_G}
        return any(key in capabilities for key in keyboard_keys)

    def update_keyboards(self):
        keyboards = self.find_keyboards()
        connected_devices = []
        for device in keyboards:
            if device.fd not in self.devices:
                self.devices[device.fd] = device
                connected_devices.append(device.name)
        # Remove disconnected devices
        existing_fds = [device.fd for device in keyboards]
        for fd in list(self.devices.keys()):
            if fd not in existing_fds:
                logging.info(f"Removing device: {self.devices[fd].path}")
                del self.devices[fd]

        # Log the currently connected devices
        if connected_devices:
            logging.info(f"Connected devices: {', '.join(connected_devices)}")

    def handle_udev_event(self, action, device):
        logging.info(f"Udev event detected: {action}, {device}")
        with self.debounce_lock:
            if self.debounce_timer is not None:
                self.debounce_timer.cancel()
            self.debounce_timer = threading.Timer(self.DEBOUNCE_TIME, self.update_keyboards)
            self.debounce_timer.start()

    def handle_key_event(self, key_event):
            # logging.debug(f"Key event: {key_event}")
            if not hasattr(key_event, 'keycode'):
                return
            key = key_event.keycode
            if isinstance(key, list):
                key = key[0]  # Handle cases where keycode is a list
            key = key.lower()
            logging.debug(f"{key} {["up", "down", "hold"][key_event.keystate]} detected")

            if key_event.keystate not in [key_event.key_down, key_event.key_up]:
                return

            if key not in self.keymap:
                return

            mappings = self.keymap[key]
            for mapping in mappings:
                command = [mapping.on_up, mapping.on_down][key_event.keystate]
                if command is None:
                    continue
                self.server_manager.queue_command(mapping.target, command)
                logging.info(f"{key} {["up", "down"][key_event.keystate]} → queued command {command} for {mapping.target}")

            # targets = self.keymap[key]
            # for target in targets:
            #     # if target['target'] == "ewctrl":
            #     #     print(target["command"])
            #     #     play_type, command = target['command'].split("|")
            #     #     if play_type == "once" and key_event.keystate == key_event.key_down:
            #     #         self.server_manager.queue_command(target['target'], f"play|{target['command']}")
            #     #         logging.info(f"{key} down → queued command play|{target['command']} for {target['target']}")
            #     #     elif play_type == "hold":
            #     #         commands = {
            #     #             key_event.key_down: f"start|{command}",
            #     #             key_event.key_up: f"stop|{command}"
            #     #         }
            #     #         if key_event.keystate in commands:
            #     #             self.server_manager.queue_command(target['target'], commands[key_event.keystate])
            #     #             logging.info(f"{key} down → queued command {commands[key_event.keystate]} for {target['target']}")

            #     #     else:
            #     #         logging.error(f"Invalid play type: {play_type}")

            #     # elif target['target'] == "wled":
            #     if key_event.keystate in [key_event.key_down, key_event.key_up] :
            #         self.server_manager.queue_command(target['target'], (key_event.keystate, target['command']))
            #         logging.info(f"{key} down → queued command {(key_event.keystate, target['command'])} for {target['target']}")


    def start(self):
        logging.info("Starting to read events from the keyboards...")
        
        while True:
            if not self.devices:
                time.sleep(1)
                continue
            r, w, x = select.select(self.devices.keys(), [], [])
            for fd in r:
                device = self.devices.get(fd)
                if not device:
                    continue
                try:
                    for event in device.read():
                        if event.type == ecodes.EV_KEY:
                            key_event = categorize(event)
                            self.handle_key_event(key_event)
                except OSError as e:
                    if e.errno == errno.ENODEV:
                        logging.warning(f"Device {device.path} removed.")
                        del self.devices[fd]
