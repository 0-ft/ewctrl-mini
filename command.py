import socket
import time

# Constants
HOST = '192.168.4.1'  # Replace with the actual IP address of the ESP32 hotspot
PORT = 80

# Function to send commands
def send_command(sock, command_type, command_data):
    start_marker = 0x00
    type_byte = command_type.to_bytes(1, 'big')
    data_bytes = command_data.to_bytes(2, 'big')
    message = bytes([start_marker]) + type_byte + data_bytes
    sock.sendall(message)
    print(f"Sent command: Type={command_type}, Data={command_data}")
    time.sleep(0.1)  # Small delay to allow processing

# Interactive mode function
def interactive_mode(sock):
    print("Interactive mode. Enter your commands in the format 'type data' (without quotes). Type 'exit' to quit.")
    while True:
        user_input = input("Enter command: ")
        if user_input.lower() == 'exit':
            break
        try:
            command_type, command_data = map(int, user_input.split())
            send_command(sock, command_type, command_data)
        except ValueError:
            print("Invalid command format. Use 'type data' (without quotes).")

# Main function
def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(10)  # Set timeout to handle network delays
        print(f"Connecting to {HOST}:{PORT}")
        sock.connect((HOST, PORT))
        print("Connected to server.")
        interactive_mode(sock)

if __name__ == "__main__":
    main()
