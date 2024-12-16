import serial
import keyboard  
import time

# Configure the serial port
ser = serial.Serial('COM15', 9600, timeout=1) 

print("Waiting for data from ESP32...")

try:
    while True:
        if ser.in_waiting > 0:  
            raw_data = ser.read(ser.in_waiting) 
            print(f"Received (raw bytes): {raw_data}")
            print(f"Received (hex): {raw_data.hex()}")

            # Simulate button presses based on received data
            if raw_data == b'\x01':  
                keyboard.press_and_release('a') 
                print("Simulated pressing 'A' key.")
            elif raw_data == b'\x02':  
                keyboard.press_and_release('space')  
                print("Simulated pressing 'Space' key.")
                
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
    print("Serial port closed.")
