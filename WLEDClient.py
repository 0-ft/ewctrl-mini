import asyncio
import requests
import threading
import queue
import logging
import json
import time
import websockets

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class Commandable:
    def send_command(self, command: str):
        raise NotImplementedError("This method should be overridden by subclasses")

class WLEDClient(Commandable):
    RETRY_DELAY = 5  # Time in seconds to wait before retrying the fetch

    def __init__(self, host, port, command_queue):
        self.host = host
        self.port = port
        self.command_queue = command_queue
        self.sock = None
        self.presets = {}
        self.connection_thread = threading.Thread(target=self.manage_connection, daemon=True)
        self.connection_thread.start()
        self.fetch_presets()

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
            ws_url = f"ws://{self.host}/ws"
            logging.info(f"Connecting to WebSocket at {ws_url}")
            try:
                async with websockets.connect(ws_url) as websocket:
                    message = json.dumps({"ps": preset_id})
                    logging.info(f"Sending WebSocket message: {message}")
                    await websocket.send(message)
                    logging.info(f"Command sent successfully for preset {command} (ID: {preset_id})")
            except Exception as e:
                logging.error(f"Error sending WebSocket command: {e}")
        else:
            logging.error(f"Preset {command} not found")

    def manage_connection(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        while True:
            command = self.command_queue.get()
            loop.run_until_complete(self.send_command(command))

    def connect_to_server(self):
        # Placeholder for potential future implementation
        pass

# Sample test file for WLEDClient
if __name__ == "__main__":
    command_queue = queue.Queue(maxsize=3)
    host = '192.168.0.20'  # Replace with your WLED device IP
    port = 80  # Typically, WLED devices use port 80

    # Create WLEDClient instance
    wled_client = WLEDClient(host, port, command_queue)

    # Add some commands to the queue to test
    test_commands = ["Pulse 1", "Pulse 2", "Red Wavesin Pulse"]
    for cmd in test_commands:
        command_queue.put(cmd)
        time.sleep(5)

    # Allow some time for the commands to be processed
    time.sleep(5)
