import asyncio
import json
import logging
import threading
import time

import requests
import websockets
from common import Commandable, KeyMapEntry


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

    @staticmethod
    def parse_keymap(raw_command: str) -> KeyMapEntry:
        return KeyMapEntry("wled", raw_command)
