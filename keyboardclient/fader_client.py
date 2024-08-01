import asyncio
import json
import logging
import threading
import time

import websockets
from common import Commandable, KeyMapEntry, CustomWebSocketClientProtocol


class FaderClient(Commandable):
    COMMAND_START_PATTERN = 0x01
    COMMAND_SET_GAIN = 0x02
    COMMAND_SET_SPEED = 0x03
    COMMAND_SET_PATTERNS = 0x04
    COMMAND_ADD_PATTERN = 0x05
    COMMAND_CLEAR_PATTERNS = 0x06
    COMMAND_SET_MULTIPLIER = 0x07
    COMMAND_STOP_PATTERN = 0x08

    RETRY_DELAY = 2
    
    def __init__(self, host, port, command_queue):
        self.host = host
        self.port = port
        self.command_queue = command_queue
        self.websocket = None
        self.patterns = json.load(open('patterns.json'))
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
            await self.send_command((FaderClient.COMMAND_SET_PATTERNS, {}))
            for pattern in self.patterns:
                await self.send_command((FaderClient.COMMAND_ADD_PATTERN, pattern))
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

    @staticmethod
    def parse_command(raw_command: str, multipliers: dict) -> KeyMapEntry:
        parts = raw_command.split(':')
        if len(parts) != 2:
            raise ValueError(f"Invalid command format: {raw_command}")
        command_type, command_data = parts
        if command_type == "once":
            return KeyMapEntry("ewctrl", (FaderClient.COMMAND_START_PATTERN, {"name": command_data, "loop": False}))
        elif command_type == "hold":
            return KeyMapEntry("ewctrl", (FaderClient.COMMAND_START_PATTERN, {"name": command_data, "loop": True}), (8, {"name": command_data}))
        elif command_type == "multiplier_raw":
            return KeyMapEntry("ewctrl", (FaderClient.COMMAND_SET_MULTIPLIER, json.loads(command_data)))
        elif command_type == "multiplier":
            return KeyMapEntry("ewctrl", (FaderClient.COMMAND_SET_MULTIPLIER, multipliers[command_data]))
        elif command_type == "speed":
            if command_data in ["-", "+"]:
                return KeyMapEntry("ewctrl", (FaderClient.COMMAND_SET_SPEED, {"speed": command_data}))
            try:
                return KeyMapEntry("ewctrl", (FaderClient.COMMAND_SET_SPEED, {"speed": float(command_data)}))
            except ValueError:
                raise ValueError(f"Invalid speed value: {command_data}")
        raise ValueError(f"Invalid command type: {command_type}")