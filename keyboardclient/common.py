# Ports for different servers
import logging

from websockets import WebSocketClientProtocol


SERVERS = {
    "ewctrl": 7032,
    # "wled": 80
}

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class Commandable:
    def send_command(self, command: str):
        raise NotImplementedError("This method should be overridden by subclasses")
    
    def is_connected(self) -> bool:
        raise NotImplementedError("This method should be overridden by subclasses")

class CustomWebSocketClientProtocol(WebSocketClientProtocol):
    async def ping(self, data=None):
        logging.info("Ping sent")
        return await super().ping(data)

class KeyMapEntry:
    def __init__(self, target: str, on_down: tuple, on_up: tuple = None):
        self.target = target
        self.on_down = on_down
        self.on_up = on_up

    def __str__(self):
        return f"KeyMapEntry({self.target}, {self.on_down}, {self.on_up})"