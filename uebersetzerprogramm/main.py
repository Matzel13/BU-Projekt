# Bugs:
# - Tastatur/ Controller wird nicht gefunden wenn das Programm schon läuft aber der Controller noch nicht angeschlossen ist
# - Nur ein Savefile für alle Module, was passiert bei ungleicher modulanzahl; 3 gespeichert aber nur 2 angeschlossen
# - Info fenster evtl. togglen

import customtkinter as ctk
import serial.tools.list_ports
import concurrent.futures
import threading
import keyboard
import serial
import json
import time

key_layout = [
    [f"{chr(65 + row)}{col + 1}" for col in range(4)] for row in range(4)
]

current_module = []
key_mappings = {}
modules = {}


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
    def check_port(port_info):
        print(f"Prüfe Port: {port_info.device}")  # Debug-Ausgabe
        try:
            with serial.Serial(
                port_info.device, baudrate=9600, timeout=2
            ) as ser:
                data = ser.read(100).decode(errors="ignore")
                if expected_message in data:
                    return port_info.device
        except (serial.SerialException, UnicodeDecodeError):
            pass
        return None

    with concurrent.futures.ThreadPoolExecutor() as executor:
        futures = {
            executor.submit(check_port, port): port for port in com_ports
        }
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            if result:
                return result
    return None


def address_to_function(function):
    function_map = {1: "Keypad", 2: "Audiomodul"}
    return function_map.get(function)


available_ports = list(serial.tools.list_ports.comports())
keypad_port = find_keypad_port(available_ports)


def switch_module(module):
    global current_module, key_mappings
    current_module = module

    module_dropdown.configure(values=list(modules.keys()))
    module_var.set(current_module)

    if module not in key_mappings:
        key_mappings[module] = {key: "" for row in key_layout for key in row}
    update_display(is_initializing = False)



def read_from_com_port(serial_connection):
    global module_count, modules, current_module, key_mappings
    address = None
    data = None
    pressed_keys = []  # List to store pressed keys

    while True:
        try:
            if serial_connection.in_waiting > 0:
                message = (
                    serial_connection.readline().decode(errors="ignore").strip()
                )
                if message:
                    if message == "START":
                        print("START")
                        address = None
                        data = None
                        pressed_keys = []

                    elif message == "END":
                        print("END")
                        if address is not None and pressed_keys:
                            module_name = f"{function_name} (Address {address})"
                            if module_name:
                                for key in pressed_keys:
                                    if key in key_mappings[module_name]:
                                        press = key_mappings[module_name][key]
                                        if press:
                                            print(
                                                f"Key {key} on {module_name} activated."
                                            )
                                            keyboard.press_and_release(press)
                                        else:
                                            print("No binding assigned.")
                                    else:
                                        print(
                                            f"Invalid key {key} for {module_name}."
                                        )
                            else:
                                print(f"Invalid address: {address}")
                        else:
                            print("Error: Missing address or keys!")

                    elif message == "LIST":
                        print("LIST")
                        new_modules = ({})  
                        while True:
                            module_message = (
                                serial_connection.readline()
                                .decode(errors="ignore")
                                .strip()
                            )
                            if module_message == "END":
                                print("END")
                                break
                            elif module_message.isdigit():
                                address = int(module_message)
                                print(f"Address {address} received.")
                                function_message = (
                                    serial_connection.readline()
                                    .decode(errors="ignore")
                                    .strip()
                                )
                                if function_message.isdigit():
                                    function = int(function_message)
                                    print(f"Function {function} received.")
                                    function_name = address_to_function(
                                        function
                                    )
                                    if function_name:
                                        module_name = f"{function_name} (Address {address})"
                                        new_modules[module_name] = {}
                                        print(f"Module {module_name} in list.")
                                        # Dynamically create key mappings for the module
                                        if module_name not in key_mappings:
                                            key_mappings[module_name] = {
                                                key: ""
                                                for row in key_layout
                                                for key in row
                                            }
                                            print(
                                                f"Key mappings created for {module_name}."
                                            )
                                    else:
                                        print(
                                            f"Unknown function {function} for Address {address}."
                                        )

                        if not current_module and new_modules:
                            current_module = sorted(
                                new_modules.keys(), reverse=True
                            )[0]
                        module_count = len(new_modules)

                        # switch_module(current_module)

                        # Check if the module list has changed
                        if new_modules != modules:
                            modules = new_modules
                            module_count = len(modules)
                            current_module = sorted(
                                new_modules.keys(), reverse=True
                            )[0]

                            print(f"Module Count: {module_count}")
                            switch_module(current_module)

                    elif message not in ("END", "INIT"):
                        try:
                            address = int(message)
                            print(f"A: {address} received.")
                        except ValueError:
                            data = message
                            print(f"D: {data} received.")
                            if address is not None:
                                pressed_keys.append(data)
                    time.sleep(0.1)

        except serial.SerialException as e:
            print(f"Error reading from COM port: {e}")
            break
        except Exception as e:
            print(f"Unexpected error: {e}")
            break


# Start der Lesefunktion, falls ein COM-Port erkannt wurde
if keypad_port:
    print(f"Starte das Lesen vom COM-Port {keypad_port}...")
    ser = serial.Serial(keypad_port, 9600, timeout=1)

    # Thread für das kontinuierliche Lesen starten
    read_thread = threading.Thread(
        target=read_from_com_port, args=(ser,), daemon=True
    )
    read_thread.start()
else:
    print("Kein Controller verbunden. Nachrichten können nicht gelesen werden.")


def update_display(is_initializing=True):
    global current_module, key_mappings

    for row in range(4):
        for col in range(4):
            key = key_layout[row][col]
            if is_initializing:
                # Use global key_mappings for initialization
                key_text = f"{key}\n({key_mappings.get(key, '')})"
            else:
                # Use module-specific key_mappings for updating
                key_text = f"{key}\n({key_mappings[current_module].get(key, '')})"
            labels[row][col].configure(text=key_text)

    # Configure COM port label
    if keypad_port:
        com_port_label.configure(text=f"COM Port: {keypad_port}")
    else:
        com_port_label.configure(text="COM Port: Not Connected")

    # Configure module count label if updating (not initializing)
    if not is_initializing:
        if module_count:
            module_count_label.configure(text=f"Module Count: {module_count}")
        else:
            module_count_label.configure(text="Module Count: N/A")



def save_mapping():
    if modules:
        try:
            with open("key_mappings.json", "w") as f:
                json.dump(key_mappings, f, indent=4)  
                update_display(is_initializing=False)
                print("Mappings successfully saved.")
        except Exception as e:
            print(f"An error occurred while saving mappings: {e}")
    else:
        print("Modules are not initialized. Nothing to save.")


def load_mapping():
    global current_module, modules
    print(f"{modules}")
    with open("key_mappings.json", "r") as f:
        loaded_data = json.load(f)

        for key in key_mappings.keys():
            if key in loaded_data:  # Nur wenn der Schlüssel in beiden Dictionaries existiert
                for sub_key in key_mappings[key].keys():
                    if sub_key in loaded_data[key]:
                        key_mappings[key][sub_key] = loaded_data[key][sub_key]

        module_dropdown.configure(values=list(key_mappings.keys()))
    update_display(is_initializing = False)



def assign_key(row, col):
    global current_module, key_mappings
    key = key_layout[row][col]
    new_binding = entry_var.get()
    if not new_binding:
        return
    normalized_binding = normalize_key_binding(new_binding)

    key_mappings[current_module][key] = normalized_binding
    update_display(is_initializing = False)



def reset_key(row, col):
    global current_module, key_mappings
    key = key_layout[row][col]
    key_mappings[current_module][key] = ""
    update_display(is_initializing = False)



ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

root = ctk.CTk()
root.title("Input Mapper")

screen_width = root.winfo_screenwidth()
screen_height = root.winfo_screenheight()
initial_width = 600
initial_height = 600

position_top = int((screen_height - initial_height) / 2)
position_left = int((screen_width - initial_width) / 2)

root.resizable(True, True)

root.geometry(
    f"{initial_width}x{initial_height}+{position_left}+{position_top}"
)

root.grid_rowconfigure(0, weight=1)
root.grid_rowconfigure(1, weight=10)
root.grid_rowconfigure(2, weight=1)
root.grid_rowconfigure(3, weight=1)
root.grid_columnconfigure(0, weight=1)

module_frame = ctk.CTkFrame(root)
module_frame.grid(row=0, column=0, sticky="nsew", pady=10)

module_label = ctk.CTkLabel(module_frame, text="Select Module:")
module_label.pack(side="left", padx=5)

module_var = ctk.StringVar(value="N/A")
module_dropdown = ctk.CTkOptionMenu(
    module_frame,
    values=list(sorted(key_mappings.keys(), reverse=True)),
    command=switch_module,
    variable=module_var,
)
module_dropdown.pack(side="left", padx=5)


frame = ctk.CTkFrame(root)
frame.grid(row=1, column=0, sticky="nsew", pady=10)

frame.grid_rowconfigure(tuple(range(4)), weight=1)
frame.grid_columnconfigure(tuple(range(4)), weight=1)

labels = [[None for _ in range(4)] for _ in range(4)]
for row in range(4):
    for col in range(4):
        key = key_layout[row][col]
        label_frame = ctk.CTkFrame(frame)

        label_frame.grid(row=row, column=col, padx=5, pady=5, sticky="nsew")

        labels[row][col] = ctk.CTkLabel(
            label_frame,
            text=f"{key}\n()",
            corner_radius=5,
        )
        labels[row][col].pack(expand=True, fill="both", padx=5, pady=5)

        button_frame = ctk.CTkFrame(label_frame)
        button_frame.pack(fill="x", pady=2)

        button_frame.grid_columnconfigure((0, 1), weight=1)

        button = ctk.CTkButton(
            button_frame,
            text="Set",
            command=lambda r=row, c=col: assign_key(r, c),
            fg_color="#5A9BD8",
            height=30,
            width=50,
        )
        button.grid(row=0, column=0, padx=(2, 1), pady=(2, 0), sticky="ew")

        reset_button = ctk.CTkButton(
            button_frame,
            text="Reset",
            command=lambda r=row, c=col: reset_key(r, c),
            fg_color="#FF7F7F",
            height=30,
            width=50,
        )
        reset_button.grid(
            row=0, column=1, padx=(1, 2), pady=(2, 0), sticky="ew"
        )


entry_frame = ctk.CTkFrame(root)
entry_frame.grid(row=2, column=0, sticky="nsew", pady=10)

entry_label = ctk.CTkLabel(entry_frame, text="Key Binding:")
entry_label.pack(side="left", padx=5)

entry_var = ctk.StringVar()
entry_field = ctk.CTkEntry(entry_frame, textvariable=entry_var)
entry_field.pack(side="left", padx=5)


info_window = None


def open_info_window():

    global info_window

    if info_window is not None and info_window.winfo_exists():
        return
    info_window = ctk.CTkToplevel()
    info_window.title("Key Bindings Info")
    info_window.geometry("400x300")

    window_width = 400
    window_height = 600
    x = root.winfo_x() + root.winfo_width()
    y = root.winfo_y()

    info_window.geometry(f"{window_width}x{window_height}+{x}+{y}")

    info_frame = ctk.CTkScrollableFrame(info_window)
    info_frame.pack(fill="both", expand=True, padx=10, pady=10)

    info_label = ctk.CTkLabel(
        info_frame,
        text=key_mapping_info.strip(),
        justify="left",
        anchor="w",
        fg_color=None,
    )
    info_label.pack(fill="both", expand=True, padx=5, pady=5)


info_button = ctk.CTkButton(
    entry_frame,
    text="Info",
    command=open_info_window,
    fg_color="#5A9BD8",
    corner_radius=5,
)
info_button.pack(side="left", padx=5)

key_mapping_info = """
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


control_frame = ctk.CTkFrame(root)
control_frame.grid(row=4, column=0, sticky="nsew", pady=10)

com_port_label = ctk.CTkLabel(
    control_frame, text="COM Port: Not Connected", anchor="w"
)
com_port_label.grid(row=0, column=0, sticky="w", padx=10, pady=10)

module_count_label = ctk.CTkLabel(
    control_frame, text=f"Module count: N/A", anchor="w"
)
module_count_label.grid(row=0, column=1, sticky="w", padx=10, pady=10)


control_frame = ctk.CTkFrame(root)
control_frame.grid(row=3, column=0, sticky="nsew", pady=10)

save_button = ctk.CTkButton(
    control_frame, text="Save Mapping", command=save_mapping
)
save_button.grid(row=0, column=0, padx=10)

load_button = ctk.CTkButton(
    control_frame, text="Load Mapping", command=load_mapping
)
load_button.grid(row=0, column=1, padx=10)

control_frame.grid_columnconfigure(0, weight=1)
control_frame.grid_columnconfigure(1, weight=1)


update_display(is_initializing = True)
root.mainloop()
