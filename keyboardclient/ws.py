import asyncio
import websockets
import json

SAMPLE_PATTERNS =     [
        [
            [
                {
                    "Time": 0.0,
                    "Value": 0.0
                },
                {
                    "Time": 1.0,
                    "Value": 1.0,
                    "CurveControl1X": 0.18770942705386642,
                    "CurveControl1Y": 0.44849774497488687,
                    "CurveControl2X": 0.5228377439416259,
                    "CurveControl2Y": 0.7832831878803149
                },
                {
                    "Time": 2.0,
                    "Value": 0.0,
                    "CurveControl1X": 0.18770942705386642,
                    "CurveControl1Y": 0.44849774497488687,
                    "CurveControl2X": 0.5228377439416259,
                    "CurveControl2Y": 0.7832831878803149
                }
            ]
        ]
    ]

async def send_test_json(uri):
    async with websockets.connect(uri) as websocket:
        print(f"Connected to {uri}")

        # Create a test JSON message
        test_message = {
            "type": 4,
            "data": SAMPLE_PATTERNS
        }

        # Convert the dictionary to a JSON string
        json_message = json.dumps(test_message)

        # Send the JSON message to the WebSocket server
        await websocket.send(json_message)
        print(f"Sent: {json_message}")


        # Create a test JSON message
        test_message = {
            "type": 1,
            "data": 0
        }

        # Convert the dictionary to a JSON string
        json_message = json.dumps(test_message)

        # Send the JSON message to the WebSocket server
        await websocket.send(json_message)
        print(f"Sent: {json_message}")
        # Receive a response (if any) and print it
        await websocket.close()
        # try:
        #     response = await websocket.recv()
        #     print(f"Received: {response}")
        # except websockets.exceptions.ConnectionClosed:
        #     print("Connection closed")

async def main():
    uri = "ws://192.168.68.104:7032/ws"  # Change this to your ESP32's IP address
    await send_test_json(uri)

if __name__ == "__main__":
    asyncio.run(main())
