import serial

def main():
    # Configure the COM port and serial communication settings
    port = "COM12"  # Replace with your COM port (e.g., "COM3" on Windows or "/dev/ttyUSB0" on Linux)
    baud_rate = 9600  # Replace with the baud rate used by your device
    timeout = 1  # Timeout in seconds for reading from the serial port

    try:
        # Open the serial port
        with serial.Serial(port, baud_rate, timeout=timeout) as ser:
            print(f"Connected to {port} at {baud_rate} baud rate.")

            while True:
                # Read a line of text from the serial port
                raw_data = ser.read() 
                received_data = ser.readline().decode('utf-8').strip()  # Decode bytes to string and strip newlines
                if received_data:
                    print(f"Received: {received_data}")

                # Optional: Exit the loop if a specific command is received
                if received_data.lower() == "exit":
                    print("Exit command received. Closing the port.")
                    break
                print(f"Received: {raw_data}")

    except serial.SerialException as e:
        print(f"Error: {e}")

    except KeyboardInterrupt:
        print("Interrupted by user. Exiting...")

if __name__ == "__main__":
    main()
