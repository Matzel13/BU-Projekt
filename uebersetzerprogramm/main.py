import customtkinter as ctk  # GUI elements
import serial.tools.list_ports  # List available COM ports
import serial  # Serial communication
import concurrent.futures  # Manage concurrent tasks
import threading  # Handle background threads
import keyboard  # Simulate keypresses
import json  # Save/load key mappings
import time  # Manage delays
import os  # File and system interactions


# Variable for debug output
debug = True

# 4x4 Key Layout
key_layout = [
    [f"{chr(65 + row)}{col + 1}" for col in range(4)] for row in range(4)
]

# Initialize a 4x4 list to hold label references for the keys
labels = [[None for _ in range(4)] for _ in range(4)]


# Global variables for data processing
current_module = []
key_mappings = {}
modules = {}

# GUI Info window
info_window = None

# Text to display in the keybinds info window
key_mapping_info = """
Combine Keybinds by using '+'

ctrl = Control
alt = Alt
shift = Shift
enter = Return
space = Space
backspace = BackSpace
delete = Delete
esc = Escape
tab = Tab
capslock = Caps_Lock
home = Home
end = End
pageup = Page_Up
pagedown = Page_Down
up = Up
down = Down
left = Left
right = Right
win = Windows
f1 = F1
f2 = F2
f3 = F3
f4 = F4
f5 = F5
f6 = F6
f7 = F7
f8 = F8
f9 = F9
f10 = F10
f11 = F11
f12 = F12
""".strip()

# Function for normalizing common user inputs
def normalize_key_binding(binding):
    mapping = {
        "ctrl": "Control",
        "alt": "Alt",
        "shift": "Shift",
        "enter": "Return",
        "space": "Space",
        "backspace": "BackSpace",
        "delete": "Delete",
        "esc": "Escape",
        "tab": "Tab",
        "capslock": "Caps_Lock",
        "home": "Home",
        "end": "End",
        "pageup": "Page_Up",
        "pagedown": "Page_Down",
        "up": "Up",
        "down": "Down",
        "left": "Left",
        "right": "Right",
        "win": "Windows",
        "f1": "F1",
        "f2": "F2",
        "f3": "F3",
        "f4": "F4",
        "f5": "F5",
        "f6": "F6",
        "f7": "F7",
        "f8": "F8",
        "f9": "F9",
        "f10": "F10",
        "f11": "F11",
        "f12": "F12",
    }
    parts = binding.split("+")
    normalized_parts = [mapping.get(part.lower(), part) for part in parts]
    return "+".join(normalized_parts)


def find_keypad_port(com_ports, expected_message="INIT"):
    # Nested function to check if a given port contains the expected message
    def check_port(port_info):
        print(f"Check port: {port_info.device}")  # Debug: Log the port being checked
        try:
            # Open the serial port with specified parameters
            with serial.Serial(
                port_info.device, baudrate=9600, timeout=2
            ) as ser:
                # Read and decode
                data = ser.read(100).decode(errors="ignore")
                # Check if "INIT" is received
                if expected_message in data:
                    return port_info.device
        except (serial.SerialException, UnicodeDecodeError):
            pass
        return None

    # Use thread pool to check multiple ports at the same time
    with concurrent.futures.ThreadPoolExecutor() as executor:
        # Create a dictionary of future tasks for checking ports
        futures = {
            executor.submit(check_port, port): port for port in com_ports
        }
        # Process completed tasks as they finish
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            if result:  # If a port returns a valid device then return
                return result
    return None



available_ports = list(serial.tools.list_ports.comports()) # Get all available Ports
keypad_port = find_keypad_port(available_ports) # Find the Keypad among all ports


# This function maps data about the function a string (name)
def set_function_name(function):
    function_map = {1: "Keypad", 2: "Audiomodul"}
    return function_map.get(function)

# This function sets a given module to the currently selected module and updates the gui
def switch_module(module):
    global current_module, key_mappings  # Declare global variables for tracking
    current_module = module  # Update current module

    # Update the module dropdown
    module_dropdown.configure(values=list(modules.keys()))
    if (modules):
        module_var.set(current_module)  # Set the dropdown selection to the current module
    else:
        module_var.set("No modules")  # Set the dropdown selection to the current module

    # Initialize key mappings for the module if they do not exist
    # May be duplicate Code. Needs to be tested:
    # if module not in key_mappings:
    #   key_mappings[module] = {key: "" for row in key_layout for key in row}  # Create a mapping

    # Update the display
    update_display(is_initializing=False)


# This is the main communication function between this script and the main controller (the hardware) via COM-Port
def read_from_com_port(serial_connection):
    global module_count, modules, current_module, key_mappings
    address = None 
    data = None 
    pressed_keys = []  # List to store all keys that are pressed

    while True:
        try:
            # Check for data waiting in serial port
            if serial_connection.in_waiting > 0:
                # Read and decode a line of data from the serial connection
                message = (
                    serial_connection.readline().decode(errors="ignore").strip()
                )
                if message:
                    # START signal
                    if message == "START":
                        if debug: print("START")
                        address = None
                        data = None
                        pressed_keys = []  # Reset the pressed keys list

                    # END signal
                    elif message == "END":
                        if debug: print("END")
                        # Process pressed keys if address and keys are set
                        if address is not None and pressed_keys:
                            module_name = f"{function_name} (Address {address})"
                            if module_name:
                                for key in pressed_keys:
                                    # Check if the key has a mapping for the current module
                                    if key in key_mappings[module_name]:
                                        press = key_mappings[module_name][key] # Saves the mapped key for the pressed Button
                                        if press:
                                            if debug: print(
                                                f"Key {key} on {module_name} activated."
                                            )
                                            # Simulate the press of the mapped key or keys
                                            keyboard.press_and_release(press)
                                        else:
                                            if debug: print("No binding assigned.")
                                    else:
                                        if debug: print(
                                            f"Invalid key {key} for {module_name}."
                                        )
                            else:
                                if debug: print(f"Invalid address: {address}")
                        else:
                            if debug: print("Error: Missing address or keys!")

                    # LIST signal to update module information
                    elif message == "LIST":
                        if debug: print("LIST")
                        new_modules = {}
                        while True:
                            # Read module information until END is received
                            module_message = (
                                serial_connection.readline()
                                .decode(errors="ignore")
                                .strip()
                            )
                            if module_message == "END":
                                if debug: print("END")
                                break
                            
                            # If the first message is a digit, then its the module address
                            elif module_message.isdigit() and int(module_message) in range(2,8):
                                address = int(module_message)
                                if debug: print(f"{address} (Address)")
                                # Read function associated with the address
                                function_message = (
                                    serial_connection.readline()
                                    .decode(errors="ignore")
                                    .strip()
                                )
                                 # If the second message is a digit, then its the function number 
                                if function_message.isdigit():
                                    function = int(function_message)
                                    if debug: print(f"{function} (Function)")
                                    function_name = set_function_name(function) # Get the function name from the function number
                                    # Safety measure: If there is a function name then set the module name
                                    if function_name:
                                        module_name = f"{function_name} (Address {address})"
                                        new_modules[module_name] = {}
                                        # Initialize key mappings if not already present (May be duplicate code -> switch_module function)
                                        if module_name not in key_mappings:
                                            key_mappings[module_name] = {
                                                key: ""
                                                for row in key_layout
                                                for key in row
                                            }
                                            if debug: print(
                                                f"Key mappings created for {module_name}."
                                            )
                                    else:
                                        print(
                                            f"Unknown function {function} for Address {address}."
                                        )

                        # Update current module and module list if needed
                        if not current_module and new_modules:
                            current_module = sorted(
                                new_modules.keys(), reverse=True
                            )[0]
                        module_count = len(new_modules)

                        
                        # Update the current module only if there are changes in the modules
                        if new_modules != modules:
                            modules = new_modules  # Replace old modules with new modules
                            module_count = len(modules)  # Update module count
                            
                            # Sort the module names and pick the first one as the current module
                            if (new_modules):
                             current_module = sorted(
                                new_modules.keys(), reverse=True
                            )[0]
                            else:
                                print("No Modules")
                            
                            # Switch to the updated current module
                            switch_module(current_module)
                            if debug: print(f"Module count: {module_count}")

                    # Handle messages (Data) that are not "END" or "INIT"
                    elif message not in ("END", "INIT"):
                        try:
                            if int(message) in range(2, 8):
                                address = int(message)
                                if debug: print(f"{address} (Address)")
                        except ValueError:
                            data = message
                            if debug: print(f"{data} (Data)")
                            if address is not None:
                                pressed_keys.append(data)

                    # Small delay to prevent overwhelming the serial connection
                    time.sleep(0.05)

        except serial.SerialException as e:
            print(f"Error reading from COM port: {e}")
            break 
        except Exception as e:
            print(f"Unexpected error: {e}")
            break



if keypad_port:
    print(f"Start reading PORT: {keypad_port}")

    # Open serial connection
    ser = serial.Serial(keypad_port, 9600, timeout=1)

    # Create a thread to handle reading from the COM port
    read_thread = threading.Thread(
        target=read_from_com_port,  # Function to run in the thread
        args=(ser,),  # Pass serial connection as argument
        daemon=True  # Set thread as a daemon to terminate with the main program
    )
    read_thread.start()  # Start thread
else:
    print("No controller connected!")


def update_display(is_initializing=True):
    global current_module, key_mappings

    # Update the display for each key in the 4x4 layout
    for row in range(4):
        for col in range(4):
            key = key_layout[row][col]  # Get key at the current position
            if is_initializing:
                # During display initialization, use global key mappings
                key_text = f"{key}\n({key_mappings.get(key, '')})"
            else:
                # For display updates, use module specific key mappings
                key_text = (
                    f"{key}\n({key_mappings[current_module].get(key, '')})"
                )
            # Update label text for the current key
            labels[row][col].configure(text=key_text)

    # Update COM port label
    if keypad_port:
        com_port_label.configure(text=f"COM Port: {keypad_port}")
    else:
        com_port_label.configure(text="COM Port: Not Connected")

    # Update module count label if not initializing
    if not is_initializing:
        if module_count:
            module_count_label.configure(text=f"Module count: {module_count}")
        else:
            module_count_label.configure(text="Module count: No modules")



def save_mapping():
    if modules:  # Check if modules are initialized
        try:
            # Open file "key_mappings.json" in write mode
            with open("key_mappings.json", "w") as f:
                # Save key_mappings dictionary to the file in JSON format
                json.dump(key_mappings, f, indent=4)
                update_display(is_initializing=False)
                print("Mappings successfully saved.")
        except Exception as e:
            print(f"An error occurred while saving mappings: {e}")
    else:
        print("Modules are not initialized. Nothing to save.")




def load_mapping():
    global current_module, modules, key_mappings

    # Check if the save file exists
    if not os.path.exists("key_mappings.json"):
        print("No save file found.")
        return

    # Open file "key_mappings.json" in read mode
    with open("key_mappings.json", "r") as f:
        loaded_data = json.load(f)

        # Merge loaded data with the existing key mappings
        for key in key_mappings.keys(): 
            if key in loaded_data:  
                for sub_key in key_mappings[key].keys(): 
                    if sub_key in loaded_data[key]: 
                        key_mappings[key][sub_key] = loaded_data[key][sub_key]

    # Update the module dropdown menu with the loaded keys
    # module_dropdown.configure(values=list(key_mappings.keys())) -> Might be unnecessary

    update_display(is_initializing=False)



def assign_key(row, col):
    global current_module, key_mappings 

    # Identify key based on its position in the key layout
    key = key_layout[row][col]
    
    # Retrieve the new binding from the entry field
    new_binding = entry_var.get()
    
    # If the binding is empty, do nothing
    if not new_binding:
        return
    
    # Normalize the key binding (important for basic keys like Control or Alt)
    normalized_binding = normalize_key_binding(new_binding)
    
    # Update the key mapping for the current module
    key_mappings[current_module][key] = normalized_binding
    
    update_display(is_initializing=False)



def reset_key(row, col):
    global current_module, key_mappings

    # Identify the key based on its position in the key layout
    key = key_layout[row][col]
    
    # Reset the mapping for the current module's key by setting it to an empty string
    key_mappings[current_module][key] = ""
    
    # Update the display to reflect the changes in key mappings
    update_display(is_initializing=False)



# Set appearance and color for GUI
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

# Create the main window
root = ctk.CTk()
root.title("Input-Mapper")

# Get the screen dimensions for positioning the window
screen_width = root.winfo_screenwidth()
screen_height = root.winfo_screenheight()

# Define window size
initial_width = 600
initial_height = 600

# Calculate position of the screens center
position_top = int((screen_height - initial_height) / 2)
position_left = int((screen_width - initial_width) / 2)
root.resizable(True, True)

# Set the initial geometry and position of the window
root.geometry(f"{initial_width}x{initial_height}+{position_left}+{position_top}")

# Configure grid layout of the root window
root.grid_rowconfigure(0, weight=1)  # Row for module selection
root.grid_rowconfigure(1, weight=10)  # Main frame for keys
root.grid_columnconfigure(0, weight=1)  # Single column layout

# Create the module selection frame in the first row
module_frame = ctk.CTkFrame(root)
module_frame.grid(row=0, column=0, sticky="nsew", pady=10)

# Add a label to the module selection
module_label = ctk.CTkLabel(module_frame, text="Select Module:")
module_label.pack(side="left", padx=5)

# Create a dropdown menu for selecting modules
module_var = ctk.StringVar(value="No modules")  # Default value
module_dropdown = ctk.CTkOptionMenu(
    module_frame,
    values=list(sorted(key_mappings.keys(), reverse=True)),
    command=switch_module,  # Function to call when a module is selected
    variable=module_var,  # Variable of current selection
)
module_dropdown.pack(side="left", padx=5)

# Create the main frame for key mappings in the second row
frame = ctk.CTkFrame(root)
frame.grid(row=1, column=0, sticky="nsew", pady=10)

# Configure the 4x4 grid layout
frame.grid_rowconfigure(tuple(range(4)), weight=1)
frame.grid_columnconfigure(tuple(range(4)), weight=1)

# Loop through rows and columns to build the grid
for row in range(4):
    for col in range(4):
        # Get the key name from the key layout
        key = key_layout[row][col]
        
        # Create a frame for this cell and position it
        label_frame = ctk.CTkFrame(frame)
        label_frame.grid(row=row, column=col, padx=5, pady=5, sticky="nsew")
        
        # Create a label to display the key name and binding (initially empty)
        labels[row][col] = ctk.CTkLabel(
            label_frame,
            text=f"{key}\n()",
            corner_radius=5,
        )
        labels[row][col].pack(expand=True, fill="both", padx=5, pady=5)
        
        # Create frame for "Set" and "Reset" buttons
        button_frame = ctk.CTkFrame(label_frame)
        button_frame.pack(fill="x", pady=2)
        
        # Configure the frame to have two columns with equal width
        button_frame.grid_columnconfigure((0, 1), weight=1)
        
        # Create a "Set" button
        button = ctk.CTkButton(
            button_frame,
            text="Set",
            # Function to call on press
            command=lambda r=row, c=col: assign_key(r, c), 
            fg_color="#5A9BD8", 
            height=30,
            width=50,
        )
        button.grid(row=0, column=0, padx=(2, 1), pady=(2, 0), sticky="ew")
        
        # Create a "Reset" button
        reset_button = ctk.CTkButton(
            button_frame,
            text="Reset",
             # Function to call on press
            command=lambda r=row, c=col: reset_key(r, c),
            fg_color="#FF7F7F",
            height=30,
            width=50,
        )
        reset_button.grid(row=0, column=1, padx=(1, 2), pady=(2, 0), sticky="ew")


# Create frame to hold key binding entry field and label
entry_frame = ctk.CTkFrame(root)
entry_frame.grid(row=2, column=0, sticky="nsew", pady=10)

# Create a label as explaination for the entry field
entry_label = ctk.CTkLabel(entry_frame, text="Key Binding:")
entry_label.pack(side="left", padx=5)

# Store user input
entry_var = ctk.StringVar()
entry_field = ctk.CTkEntry(entry_frame, textvariable=entry_var)
entry_field.pack(side="left", padx=5)


def open_info_window():
    global info_window

    # Check if the window already exists and is open
    if info_window is not None and info_window.winfo_exists():
        return

    # Create a new window for key bindings info
    info_window = ctk.CTkToplevel()
    info_window.title("Key Bindings Info")  
    info_window.geometry("400x300")  

    # Calculate position to open info window on the right of the main window
    window_width = 400
    window_height = 600
    x = root.winfo_x() + root.winfo_width()
    y = root.winfo_y()
    info_window.geometry(f"{window_width}x{window_height}+{x}+{y}")

    # Add a scrollable frame
    info_frame = ctk.CTkScrollableFrame(info_window)
    info_frame.pack(fill="both", expand=True, padx=10, pady=10)

    # Add a label to display key mapping information
    info_label = ctk.CTkLabel(
        info_frame,
        text=key_mapping_info.strip(), 
        justify="left", 
        anchor="w",  
        fg_color=None, 
    )
    info_label.pack(fill="both", expand=True, padx=5, pady=5)


# Create a button to open the info window
info_button = ctk.CTkButton( 
    entry_frame,
    text="Info", 
    command=open_info_window, 
    fg_color="#5A9BD8", 
    corner_radius=5,
)
info_button.pack(side="left", padx=5)  

# Create a frame for control labels
control_frame = ctk.CTkFrame(root) 
control_frame.grid(row=4, column=0, sticky="nsew", pady=10)

# Control label for COM port status
com_port_label = ctk.CTkLabel(  
    control_frame, text="COM Port: Not Connected", anchor="w" 
)
com_port_label.grid(row=0, column=0, sticky="w", padx=10, pady=10) 

 # Control label for module count
module_count_label = ctk.CTkLabel( 
    control_frame, text=f"Module count: No modules", anchor="w"
)
module_count_label.grid(row=0, column=1, sticky="w", padx=10, pady=10) 

control_frame = ctk.CTkFrame(root)  
control_frame.grid(row=3, column=0, sticky="nsew", pady=10)

# Save button
save_button = ctk.CTkButton( 
    control_frame, text="Save Mapping", command=save_mapping 
)
save_button.grid(row=0, column=0, padx=10) 

# Load button
load_button = ctk.CTkButton( 
    control_frame, text="Load Mapping", command=load_mapping
)
load_button.grid(row=0, column=1, padx=10)

# Button alignment
control_frame.grid_columnconfigure(0, weight=1)
control_frame.grid_columnconfigure(1, weight=1)

update_display(is_initializing=True)
root.mainloop()