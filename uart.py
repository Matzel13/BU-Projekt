import serial
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
        else:
            time.sleep(0.1)
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
    print("Serial port closed.")
