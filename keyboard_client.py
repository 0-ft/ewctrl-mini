import socket
import threading
import queue
import logging
import csv
from evdev import InputDevice, categorize, ecodes, list_devices
import os
import select
import subprocess
import time
import pyudev
import errno

# Ports for different servers
PORTS = [7032, 9165]

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class Commandable:
    def send_command(self, command: str):
        raise NotImplementedError("This method should be overridden by subclasses")

class FaderClient(Commandable):
    def __init__(self, host, port, command_queue):
        self.host = host
        self.port = port
        self.sock = None
        self.command_queue = command_queue
        self.connection_thread = threading.Thread(target=self.manage_connection, daemon=True)
        self.connection_thread.start()

    def send_command(self, command: str):
        parts = command.split(',')
        if len(parts) != 2:
            logging.error(f"Invalid command format: {command}")
            return

        command_type = int(parts[0])
        command_data = int(parts[1])

        start_marker = 0x00
        type_byte = command_type.to_bytes(1, 'big')
        data_bytes = command_data.to_bytes(2, 'big')
        message = bytes([start_marker]) + type_byte + data_bytes
        self.sock.sendall(message)
        logging.info(f"Sent command to {self.host}:{self.port} - Type={command_type}, Data={command_data}")

    def manage_connection(self):
        try:
            self.connect_to_server()
            while True:
                command = self.command_queue.get()
                self.send_command(command)
        except (socket.error, BrokenPipeError):
            logging.error(f"Connection to {self.host}:{self.port} lost.")
        finally:
            if self.sock:
                self.sock.close()
            logging.info(f"FaderClient connection thread to {self.host}:{self.port} terminating.")

    def connect_to_server(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 4096)  # Increase send buffer size
        self.sock.settimeout(5)  # Set timeout to handle network delays
        logging.info(f"Connecting to {self.host}:{self.port}")
        self.sock.connect((self.host, self.port))
        logging.info(f"Connected to server at {self.host}:{self.port}")

class ServerManager:
    def __init__(self, ports):
        self.ports = ports
        self.clients = {}
        self.command_queues = {port: queue.Queue(maxsize=3) for port in ports}
        for port in ports:
            threading.Thread(target=self.manage_port_connection, args=(port,), daemon=True).start()

    def get_lan_devices(self):
        result = subprocess.run(['arp', '-an'], capture_output=True, text=True)
        devices = []
        for line in result.stdout.splitlines():
            if line.startswith('?'):
                parts = line.split()
                if len(parts) >= 2:
                    devices.append(parts[1].strip('()'))
        return devices

    def is_port_open(self, ip, port):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(1)
            result = sock.connect_ex((ip, port))
            return result == 0

    def find_server(self, port):
        devices = self.get_lan_devices()
        logging.debug(f"Devices on LAN: {devices}")
        for device in devices:
            if self.is_port_open(device, port):
                logging.info(f"Server found at {device}:{port}")
                return device
        logging.debug(f"Server not found on the LAN for port {port}.")
        return None

    def manage_port_connection(self, port):
        while True:
            if port not in self.clients or not self.clients[port].sock:
                server_ip = self.find_server(port)
                if server_ip:
                    self.clients[port] = FaderClient(server_ip, port, self.command_queues[port])
                    self.clients[port].connection_thread.join()
                else:
                    logging.debug(f"Could not find the server on the LAN for port {port}. Retrying...")
            else:
                # If the client is already connected, check the connection status
                try:
                    self.clients[port].sock.send(b'')
                except (socket.error, BrokenPipeError):
                    logging.error(f"Connection to {self.clients[port].host}:{port} lost.")
                    del self.clients[port]
            time.sleep(0.5)  # Retry every half second

    def queue_command(self, port, command: str):
        if port in self.clients and self.clients[port].sock:
            if not self.command_queues[port].full():
                self.command_queues[port].put(command)
            else:
                logging.warning(f"Command queue for port {port} is full. Dropping command.")
        else:
            logging.warning(f"No active connection to server on port {port}. Command discarded.")

class KeyboardCommander:
    DEBOUNCE_TIME = 0.3  # Time in seconds to debounce udev events

    def __init__(self, keymap_file, server_manager):
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

    def load_keymap(self, filename):
        keymap = {}
        with open(filename, mode='r') as file:
            reader = csv.DictReader(file)
            for row in reader:
                key = row['key']
                keymap[key] = {
                    'port': int(row['port']),
                    'command': row['command']
                }
        return keymap

    def find_keyboards(self):
        devices = [InputDevice(path) for path in list_devices()]
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
                            logging.debug(f"Key event: {key_event}")
                            if hasattr(key_event, 'keycode'):
                                key = key_event.keycode
                                if isinstance(key, list):
                                    key = key[0]  # Handle cases where keycode is a list
                                key = key.lower()
                                logging.info(f"Key {key} event detected, state: {key_event.keystate}")
                                if key in self.keymap:
                                    if key_event.keystate == key_event.key_down:
                                        event = self.keymap[key]
                                        port = event['port']
                                        command = event['command']
                                        self.server_manager.queue_command(port, command)
                                        logging.info(f"Key {key} pressed, sent command {command} to port {port}")
                except OSError as e:
                    if e.errno == errno.ENODEV:
                        logging.warning(f"Device {device.path} removed.")
                        del self.devices[fd]

def main():
    server_manager = ServerManager(PORTS)
    keyboard_commander = KeyboardCommander('keymap.csv', server_manager)
    keyboard_commander.start()

if __name__ == "__main__":
    main()
