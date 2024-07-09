import asyncio
import websockets
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
import requests
import json
import socket

# Ports for different servers
SERVERS = [('192.168.0.20', 80), 7032]  # Example: WLED at a specific IP and fader device to be discovered

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class Commandable:
    def send_command(self, command: str):
        raise NotImplementedError("This method should be overridden by subclasses")
    
    def is_connected(self) -> bool:
        raise NotImplementedError("This method should be overridden by subclasses")

class FaderClient(Commandable):
    def __init__(self, host, port, command_queue):
        self.host = host
        self.port = port
        self.command_queue = command_queue
        self.websocket = None
        self.patterns = json.load(open('patterns.json'))
        self.connection_thread = threading.Thread(target=self.manage_connection, daemon=True)
        self.connection_thread.start()

    async def send_command(self, command: str):
        parts = command.split(',')
        if len(parts) != 2:
            logging.error(f"Invalid command format: {command}")
            return

        command_type = int(parts[0])
        command_data = parts[1]

        message = json.dumps({
            "type": command_type,
            "data": command_data
        })

        if self.websocket is not None and self.websocket.open:
            await self.websocket.send(message)
            logging.info(f"Sent command to {self.host}:{self.port} - {message}")

    async def send_patterns(self):
        if self.websocket is not None and self.websocket.open:
            message = json.dumps({
                "type": 4,
                "data": self.patterns
            })
            await self.websocket.send(message)
            logging.info(f"Sent patterns to {self.host}:{self.port}")

    async def connect_to_server(self):
        ws_url = f"ws://{self.host}:{self.port}/ws"
        logging.info(f"Connecting to WebSocket at {ws_url}")
        try:
            self.websocket = await websockets.connect(ws_url, max_size=None)
            logging.info(f"Connected to WebSocket server at {self.host}:{self.port}")
            await self.send_patterns()
            while True:
                command = self.command_queue.get()  # Use blocking get() from queue
                await self.send_command(command)
        except Exception as e:
            logging.error(f"Error connecting to WebSocket server: {e}")
            self.websocket = None

    def manage_connection(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        while True:
            if not self.websocket or not self.websocket.open:
                loop.run_until_complete(self.connect_to_server())
            time.sleep(1)  # Retry connection after a short delay

    def is_connected(self) -> bool:
        return self.websocket is not None and self.websocket.open

class WLEDClient(Commandable):
    RETRY_DELAY = 2  # Time in seconds to wait before retrying the fetch

    def __init__(self, host, port, command_queue):
        self.host = host
        self.port = port
        self.command_queue = command_queue
        self.presets = {}
        self.websocket = None
        self.connection_thread = threading.Thread(target=self.manage_connection, daemon=True)
        self.connection_thread.start()

    def fetch_presets(self):
        while True:
            try:
                url = f"http://{self.host}/presets.json"
                logging.info(f"Fetching presets from {url}")
                response = requests.get(url)
                if response.status_code == 200:
                    data = response.json()
                    self.presets = {v.get('n', f'Preset {k}'): k for k, v in data.items()}
                    logging.info(f"Fetched presets: {self.presets}")
                    break  # Exit the loop if fetching succeeds
                else:
                    logging.error(f"Failed to fetch presets: HTTP {response.status_code}")
            except Exception as e:
                logging.error(f"Error fetching presets: {e}")
            logging.info(f"Retrying in {self.RETRY_DELAY} seconds...")
            time.sleep(self.RETRY_DELAY)

    async def send_command(self, command: str):
        if command in self.presets:
            preset_id = self.presets[command]
            try:
                if self.websocket is not None and self.websocket.open:
                    message = json.dumps({"ps": preset_id})
                    logging.debug(f"Sending WebSocket message: {message}")
                    await self.websocket.send(message)
                    logging.info(f"Command sent successfully for preset {command} (ID: {preset_id})")
            except Exception as e:
                logging.error(f"Error sending WebSocket command: {e}")
                self.websocket = None
        else:
            logging.error(f"Preset {command} not found")

    async def connect_to_server(self):
        ws_url = f"ws://{self.host}:{self.port}/ws"
        logging.info(f"Connecting to WebSocket at {ws_url}")
        try:
            self.websocket = await websockets.connect(ws_url)
            logging.info(f"Connected to WebSocket server at {self.host}:{self.port}")
            while True:
                command = self.command_queue.get()  # Use blocking get() from queue
                await self.send_command(command)
        except Exception as e:
            logging.error(f"Error connecting to WebSocket server: {e}")
            self.websocket = None

    def manage_connection(self):
        self.fetch_presets()  # Fetch presets before managing the connection
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        while True:
            if not self.websocket or not self.websocket.open:
                loop.run_until_complete(self.connect_to_server())
            time.sleep(1)  # Retry connection after a short delay

    def is_connected(self) -> bool:
        return self.websocket is not None and self.websocket.open

class ServerManager:
    def __init__(self, servers):
        self.clients = {}
        self.command_queues = {}
        for server in servers:
            if isinstance(server, tuple):
                ip, port = server
                self.command_queues[port] = queue.Queue(maxsize=3)
                threading.Thread(target=self.manage_port_connection, args=(port, ip), daemon=True).start()
            else:
                port = server
                self.command_queues[port] = queue.Queue(maxsize=3)
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

    def manage_port_connection(self, port, ip=None):
        while True:
            if port not in self.clients or not self.clients[port].is_connected():
                server_ip = ip if ip else self.find_server(port)
                if server_ip:
                    if port == 80:
                        self.clients[port] = WLEDClient(server_ip, port, self.command_queues[port])
                    else:
                        self.clients[port] = FaderClient(server_ip, port, self.command_queues[port])
                    self.clients[port].connection_thread.join()
                else:
                    logging.debug(f"Could not find the server on the LAN for port {port}. Retrying...")
            time.sleep(0.5)  # Retry every half second

    def queue_command(self, port, command: str):
        if port in self.clients and self.clients[port].is_connected():
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
                                logging.debug(f"Key {key} event detected, state: {key_event.keystate}")
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
    server_manager = ServerManager(SERVERS)
    keyboard_commander = KeyboardCommander('keymap.csv', server_manager)
    keyboard_commander.start()

if __name__ == "__main__":
    main()
