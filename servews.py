import asyncio
import websockets

async def handler(websocket, path):
    async for message in websocket:
        print(f"Received message: {message}")

async def main():
    server = await websockets.serve(handler, "localhost", 7032)
    print("WebSocket server started on ws://localhost:7032/ws")
    await server.wait_closed()

if __name__ == "__main__":
    asyncio.run(main())
