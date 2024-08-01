import logging
import queue
import socket
import subprocess
import threading
import time

from als import generate_patterns
from common import SERVERS
from fader_client import FaderClient
from key_control import KeyboardCommander
from wled_client import WLEDClient


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

    def is_port_open(self, ip: str, port: int):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(1)
            result = sock.connect_ex((ip, port))
            return result == 0

    def find_server(self, port: int):
        devices = self.get_lan_devices()
        logging.debug(f"Devices on LAN: {devices}")
        for device in devices:
            if self.is_port_open(device, port):
                logging.info(f"Server found at {device}:{port}")
                return device
        logging.debug(f"Server not found on the LAN for port {port}.")
        return None

    def manage_port_connection(self, name: str):
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

    def queue_command(self, target_name: str, command: tuple):
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

def main():
    generate_patterns("data/ew4lx_1.als")
    # generate_patterns("data/pridelx_3.als")
    # generate_patterns("/boot/ewctrl/lx.als")
    server_manager = ServerManager()
    keyboard_commander = KeyboardCommander(server_manager, 'data/map_ewlx4.csv', multipliers_file='data/multipliers_test.csv')
    # keyboard_commander = KeyboardCommander('/boot/ewctrl/patterns_map.csv', server_manager)
    keyboard_commander.start()

if __name__ == "__main__":
    main()
