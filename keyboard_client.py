import asyncio
import websockets
from websockets.client import WebSocketClientProtocol
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
from readals import generate_patterns
# Ports for different servers
SERVERS = {
    "ewctrl": 7032,
    # "wled": 80
}

# Configure logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

class Commandable:
    def send_command(self, command: str):
        raise NotImplementedError("This method should be overridden by subclasses")
    
    def is_connected(self) -> bool:
        raise NotImplementedError("This method should be overridden by subclasses")

class CustomWebSocketClientProtocol(WebSocketClientProtocol):
    async def ping(self, data=None):
        logging.info("Ping sent")
        return await super().ping(data)

class FaderClient(Commandable):
    RETRY_DELAY = 2
    
    
    def __init__(self, host, port, command_queue):
        self.host = host
        self.port = port
        self.command_queue = command_queue
        self.websocket = None
        self.patterns = json.load(open('patterns_test.json'))
        self.connection_thread = threading.Thread(target=self.manage_connection, daemon=True)
        self.connection_thread.start()

    async def ws_recv_ack(self):
        # receive a single byte, 0 means OK, 1 means error & resend
        if self.websocket is not None and self.websocket.open:
            ack = await self.websocket.recv()
            print("Got ack", ack)
            return ack == "0"
        else:
            self.websocket = None

    async def ws_send_check(self, message):
        if self.websocket is not None and self.websocket.open:
            await self.websocket.send(message)
            return await self.ws_recv_ack()
        else:
            self.websocket = None
        return False

    async def ws_send_until_success(self, message):
        while not await self.ws_send_check(message):
            logging.error("Failed to send command, retrying...")
            time.sleep(0.05)

    async def send_command(self, command: tuple):
        command_type, command_data = command
        message = json.dumps({
            "type": command_type,
            "data": command_data
        })
        print(message)
        # try:
        #     pong_waiter = await self.websocket.ping()
        #     logging.info("sent ping...")
        #     await pong_waiter
        #     logging.info("pong received")
        # except Exception as e:
        #     logging.error(f"Error sending ping: {e}")
        #     self.websocket = None
        #     return
        if self.websocket is not None and self.websocket.open:
            # await self.ws_send_check(message)
            await self.websocket.send(message.replace(" ", ""))
            logging.info(f"Sent command to {self.host}:{self.port} - {message}")

    async def send_patterns(self):
        if self.websocket is not None and self.websocket.open:
            logging.info(f"sending {len(self.patterns)} patterns to {self.host}:{self.port}")

            # clear patterns
            await self.send_command((6, {}))
            for pattern in self.patterns:
                await self.send_command((5, pattern))
                time.sleep(0.15)
                logging.info(f"Sent a pattern to {self.host}:{self.port}")
            logging.info(f"Sent patterns to {self.host}:{self.port}")

    async def connect_to_server(self):
        ws_url = f"ws://{self.host}:{self.port}/ws"
        logging.info(f"Connecting to WebSocket at {ws_url}")
        try:
            self.websocket = await websockets.connect(ws_url, max_size=None, ping_interval=2, ping_timeout=2, create_protocol=CustomWebSocketClientProtocol)
            logging.info(f"Connected to WebSocket server at {self.host}:{self.port}")
            await self.send_patterns()
            # while True:
            #     command = self.command_queue.get()  # Use blocking get() from queue
            #     await self.send_command(command)
        except Exception as e:
            logging.error(f"Error connecting to WebSocket server: {e}")
            self.websocket = None

    async def read_commands(self):
        try:
            while True:
                command = self.command_queue.get()  # Use blocking get() from queue
                await self.send_command(command)
        except Exception as e:
            logging.error(f"Error connecting to WebSocket server: {e}")
            self.websocket = None

    def manage_connection(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        loop.run_until_complete(self.connect_to_server())
        loop.run_until_complete(self.read_commands())
        # while True:
        #     if not self.websocket or not self.websocket.open:
        #         loop.run_until_complete(self.connect_to_server())
        #     time.sleep(1)  # Retry connection after a short delay

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

    async def send_command(self, command: tuple):
        key_state, command = command
        if key_state != key_state.key_down:
            return
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
    def __init__(self):
        self.clients = {}
        self.command_queues = {}
        for name, port in SERVERS.items():
            self.command_queues[name] = queue.Queue(maxsize=10)
            threading.Thread(target=self.manage_port_connection, args=(name,), daemon=True).start()

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

    def manage_port_connection(self, name):
        while True:
            if name not in self.clients or not self.clients[name].is_connected():
                server_ip = self.find_server(SERVERS[name])
                if server_ip:
                    if name == "wled":
                        self.clients[name] = WLEDClient(server_ip, SERVERS[name], self.command_queues[name])
                    else:
                        self.clients[name] = FaderClient(server_ip, SERVERS[name], self.command_queues[name])
                    self.clients[name].connection_thread.join()
                else:
                    logging.debug(f"Could not find the server on the LAN for port {SERVERS[name]}. Retrying...")
            time.sleep(1)  # Retry connecting

    def queue_command(self, target_name, command: tuple):
        if target_name in self.clients and self.clients[target_name].is_connected():
            if not self.command_queues[target_name].full():
                try:
                    self.command_queues[target_name].put(command, block=False)
                except queue.Full:
                    logging.warning(f"Command queue for server {target_name} is full. Dropping command.")
            else:
                logging.warning(f"Command queue for server {target_name} is full. Dropping command.")
        else:
            logging.warning(f"No active connection to server {target_name}. Command discarded.")

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
                    # cmd = f"1,{row['ewctrl']}" if row
                    command_type, command_data = row['ewctrl'].split(':')
                    if command_type == "pattern":
                        command = (1, command_data)
                    elif command_type == "multiplier_raw":
                        command = (7, json.loads(command_data))
                    elif command_type == "multiplier":
                        command = (7, self.multipliers[command_data])
                    logging.info(f"Adding command: {command} for key {key}")
                    keymap[key].append({
                        'target': "ewctrl",
                        'command': command
                    })
                if("wled" in row and row["wled"]):
                    keymap[key].append({
                        'target': "wled",
                        'command': row['wled']
                    })
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
            if key not in self.keymap:
                return

            targets = self.keymap[key]
            for target in targets:
                # if target['target'] == "ewctrl":
                #     print(target["command"])
                #     play_type, command = target['command'].split("|")
                #     if play_type == "once" and key_event.keystate == key_event.key_down:
                #         self.server_manager.queue_command(target['target'], f"play|{target['command']}")
                #         logging.info(f"{key} down → queued command play|{target['command']} for {target['target']}")
                #     elif play_type == "hold":
                #         commands = {
                #             key_event.key_down: f"start|{command}",
                #             key_event.key_up: f"stop|{command}"
                #         }
                #         if key_event.keystate in commands:
                #             self.server_manager.queue_command(target['target'], commands[key_event.keystate])
                #             logging.info(f"{key} down → queued command {commands[key_event.keystate]} for {target['target']}")

                #     else:
                #         logging.error(f"Invalid play type: {play_type}")

                # elif target['target'] == "wled":
                if key_event.keystate in [key_event.key_down, key_event.key_up] :
                    self.server_manager.queue_command(target['target'], (key_event.keystate, target['command']))
                    logging.info(f"{key} down → queued command {(key_event.keystate, target['command'])} for {target['target']}")


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

def main():
    # generate_patterns("pridelx_3.als")
    # generate_patterns("/boot/ewctrl/lx.als")
    server_manager = ServerManager()
    keyboard_commander = KeyboardCommander(server_manager, 'map_test.csv', multipliers_file='multipliers_test.csv')
    # keyboard_commander = KeyboardCommander('/boot/ewctrl/patterns_map.csv', server_manager)
    keyboard_commander.start()

if __name__ == "__main__":
    main()
