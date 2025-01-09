import serial
import keyboard  
import time

# Configure the serial port
ser = serial.Serial('COM12', 9600, timeout=1) 

print("Waiting for data from ESP32...")

try:
    while True:
        if ser.in_waiting > 0:  
            raw_data = ser.read(ser.in_waiting) 
            print(f"Received (raw bytes): {raw_data}")
            print(f"Received (hex): {raw_data.hex()}")
                
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
    print("Serial port closed.")
