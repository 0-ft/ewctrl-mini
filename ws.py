import asyncio
import websockets

async def send_commands(uri):
    async with websockets.connect(uri) as websocket:
        print(f"Connected to {uri}")

        async def send_command():
            while True:
                command = input("Enter command (type,data) or 'exit' to quit: ")
                if command.lower() == 'exit':
                    break
                try:
                    type_data = command.split(',')
                    if len(type_data) != 2:
                        print("Invalid command format. Use 'type,data'.")
                        continue
                    type_byte = int(type_data[0])
                    data = int(type_data[1])
                    message = bytes([0x00, type_byte, (data >> 8) & 0xFF, data & 0xFF])
                    await websocket.send(message)
                    print(f"Sent: {command}")
                except ValueError:
                    print("Invalid command format. Use integers for type and data.")
        
        async def receive_messages():
            while True:
                try:
                    response = await websocket.recv()
                    if response == "ACK":
                        print("Heartbeat acknowledged.")
                    else:
                        print(f"Received: {response}")
                except websockets.ConnectionClosed:
                    print("Connection closed")
                    break

        sender = asyncio.create_task(send_command())
        receiver = asyncio.create_task(receive_messages())

        await sender
        receiver.cancel()

async def main():
    uri = "ws://192.168.2.176/ws"  # Change this to your ESP32's IP address
    await send_commands(uri)

if __name__ == "__main__":
    asyncio.run(main())
