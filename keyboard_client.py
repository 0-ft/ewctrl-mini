import socket
import time
import yaml
from pynput import keyboard
import pyudev
import threading
import queue
import logging

# Constants
HOST = '192.168.4.1'  # Replace with the actual IP address of the ESP32 hotspot
PORT = 80

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class KeyboardCommander:
    def __init__(self, host, port, keymap_file):
        self.host = host
        self.port = port
        self.keymap = self.load_keymap(keymap_file)
        self.sock = None
        self.command_queue = queue.Queue(maxsize=3)  # Limit the queue to 3 commands
        self.connection_thread = threading.Thread(target=self.manage_connection, daemon=True)
        self.connection_thread.start()
        self.context = pyudev.Context()
        self.monitor = pyudev.Monitor.from_netlink(self.context)
        self.observer = pyudev.MonitorObserver(self.monitor, self.usb_event, name='usb-observer')
        self.observer.start()

    def load_keymap(self, filename):
        with open(filename, 'r') as file:
            return yaml.safe_load(file)

    def send_command(self, command_type, command_data):
        start_marker = 0x00
        type_byte = command_type.to_bytes(1, 'big')
        data_bytes = command_data.to_bytes(2, 'big')
        message = bytes([start_marker]) + type_byte + data_bytes
        self.sock.sendall(message)
        logging.info(f"Sent command: Type={command_type}, Data={command_data}")

    def on_press(self, key):
        try:
            key_str = key.char  # Single character keys
        except AttributeError:
            key_str = str(key)  # Special keys

        logging.debug(f"Key pressed: {key_str}")

        if key_str in self.keymap:
            event = self.keymap[key_str]
            if not self.command_queue.full():
                self.command_queue.put(event)
            else:
                logging.warning("Command queue is full. Dropping command.")
        else:
            logging.info(f"Key {key_str} not mapped to any event.")

    def manage_connection(self):
        while True:
            if self.sock is None:
                self.connect_to_server()
            else:
                try:
                    event = self.command_queue.get(timeout=1)
                    self.send_command(event['type'], event['data'])
                except queue.Empty:
                    continue
                except (socket.error, BrokenPipeError):
                    logging.error("Connection lost. Reconnecting...")
                    self.sock.close()
                    self.sock = None

    def connect_to_server(self):
        while self.sock is None:
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.settimeout(10)  # Set timeout to handle network delays
                logging.info(f"Connecting to {self.host}:{self.port}")
                self.sock.connect((self.host, self.port))
                logging.info("Connected to server.")
            except socket.error as e:
                logging.error(f"Connection failed: {e}. Retrying immediately...")
                self.sock = None

    def usb_event(self, action, device):
        logging.debug(f"USB event detected: Action={action}, Device={device}")
        if 'ID_INPUT_KEYBOARD' in device:
            if action == 'add':
                logging.info(f"Keyboard connected: {device.device_node}")
            elif action == 'remove':
                logging.info(f"Keyboard disconnected: {device.device_node}")

    def start(self):
        with keyboard.Listener(on_press=self.on_press) as listener:
            listener.join()

def main():
    client = KeyboardCommander(HOST, PORT, 'keymap.yaml')
    client.start()

if __name__ == "__main__":
    main()
