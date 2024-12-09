import serial
import keyboard  # To simulate key presses
import time

# Configure the serial port
ser = serial.Serial('COM15', 9600, timeout=1)  # Adjust COM port and baud rate

print("Waiting for data from ESP32...")

try:
    while True:
        if ser.in_waiting > 0:  # Check if data is available
            raw_data = ser.read(ser.in_waiting)  # Read all available data
            print(f"Received (raw bytes): {raw_data}")
            print(f"Received (hex): {raw_data.hex()}")

            # Simulate button presses based on received data
            if raw_data == b'\x04':  # Example: if data is hex 0x01
                keyboard.press_and_release('backspace')  # Simulate pressing the 'A' key              
                print("Simulated pressing 'backspace' key.")
            elif raw_data == b'\x02':  # Example: if data is hex 0x02
                keyboard.press_and_release('space')  # Simulate pressing the Spacebar
                print("Simulated pressing 'Space' key.")
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
    print("Serial port closed.")