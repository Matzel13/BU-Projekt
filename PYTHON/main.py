# Bugs:
# - Tastatur/ Controller wird nicht gefunden wenn das Programm schon läuft aber der Controller noch nicht angeschlossen ist
# - Nur ein Savefile für alle Module, was passiert bei ungleicher modulanzahl; 3 gespeichert aber nur 2 angeschlossen
# - Info fenster evtl. togglen


# START
# Adresse 2-7
# Daten
# ENDE

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
key_mappings = {"Module 1": {key: "" for row in key_layout for key in row}}
current_module = "Module 1"

# habe die Reihenfolge der Module gedreht!
def address_to_module(address):
    module_map = {
        7: "Module 1",
        6: "Module 2",
        5: "Module 3",
        4: "Module 4",
        3: "Module 5",
        2: "Module 6",
    }
    return module_map.get(address, None)


def address_to_function(function):
    function_map = {1: "Keyboard", 2: "Audiomodul"}
    return function_map.get(function, None)


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


def find_keyboard_port(port_info, expected_message="INIT"):
    try:
        with serial.Serial(port_info.device, baudrate=9600, timeout=2) as ser:
            print(f"Prüfe Port: {port_info.device}")
            data = ser.read(100).decode(errors="ignore")
            if expected_message in data:
                return port_info.device
    except serial.SerialException as e:
        print(f"Fehler beim Zugriff auf {port_info.device}: {e}")
    except UnicodeDecodeError as e:
        print(f"Fehler beim Dekodieren von Daten auf {port_info.device}: {e}")
    return None


def find_keyboard(com_ports, expected_message="INIT"):
    with concurrent.futures.ThreadPoolExecutor() as executor:
        futures = [
            executor.submit(find_keyboard_port, port_info, expected_message)
            for port_info in com_ports
        ]

        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            if result:
                return result
    return None


available_ports = list(serial.tools.list_ports.comports())
keyboard_port = find_keyboard(available_ports)


def read_from_com_port(serial_connection):
    global module_count
    address = None
    data = None
    pressed_keys = []  # Liste zum Speichern der gedrückten Tasten
    modules = []  # Liste zum Speichern der angeschlossenen Module

    while True:
        try:
            if serial_connection.in_waiting > 0:
                message = (
                    serial_connection.readline().decode(errors="ignore").strip()
                )
                if message:
                    print(f"Nachricht empfangen: {message}")

                    # Überprüfen, ob es sich um "START" handelt
                    if message == "START":
                        print(
                            "Start-Nachricht empfangen, warte auf Adresse und Tasten..."
                        )
                        address = None
                        data = None
                        pressed_keys = (
                            []
                        )  # Liste zurücksetzen, wenn ein neuer Block beginnt
                    elif message == "END":
                        print("Ende-Nachricht empfangen.")
                        # Wenn sowohl Adresse als auch Daten vorhanden sind, dann verarbeiten
                        if address is not None and pressed_keys:
                            module = address_to_module(address)
                            if module:
                                print(f"Modul {module} empfangen.")
                                # Nacheinander alle gesammelten Tasten ausführen
                                for key in pressed_keys:
                                    if key in key_mappings[module]:
                                        press = key_mappings[module][key]
                                        if press:
                                            print(
                                                f"Taste {key} auf {module} aktiviert."
                                            )
                                            keyboard.press_and_release(
                                                f"{press}"
                                            )
                                        else:
                                            print("Keine Belegung")
                                    else:
                                        print(
                                            f"Ungültige Taste {key} für {module}."
                                        )
                            else:
                                print(f"Ungültige Adresse: {address}")
                        else:
                            print(
                                "Fehler: Adresse oder keine Tasten vorhanden!"
                            )

                    elif message == "LIST":
                        print(
                            "LIST-Nachricht empfangen. Lese angeschlossene Module..."
                        )
                        modules = []  # Liste der Module zurücksetzen
                        # Jetzt warten wir auf die Module-Daten
                        while True:
                            module_message = (
                                serial_connection.readline()
                                .decode(errors="ignore")
                                .strip()
                            )
                            if module_message == "END":
                                print("Ende der LIST-Nachricht empfangen.")
                                break
                            elif module_message.isdigit():
                                # Wenn die Nachricht eine Zahl (Adresse) ist
                                address = int(module_message)
                                print(f"Adresse {address} empfangen.")
                                # Nächste Zeile ist die Funktion des Moduls
                                function_message = (
                                    serial_connection.readline()
                                    .decode(errors="ignore")
                                    .strip()
                                )
                                if function_message.isdigit():
                                    function = int(function_message)
                                    print(f"Funktion {function} empfangen.")
                                    function_name = address_to_function(
                                        function
                                    )
                                    if function_name:
                                        modules.append((address, function_name))
                                        print(
                                            f"Modul {function_name} bei Adresse {address} hinzugefügt."
                                        )
                                    else:
                                        print(
                                            f"Unbekannte Funktion {function} für Adresse {address}."
                                        )
                            time.sleep(
                                0.1
                            )  # Leichte Pause, um CPU-Auslastung zu reduzieren
                        module_count = len(modules)
                        print(f"Module Count: {module_count}")
                        update_display()

                    elif message != "INIT":
                        # Wenn es keine START, END oder LIST Nachricht ist, dann könnte es Adresse oder Daten sein
                        try:
                            # Versuchen, die Nachricht als Adresse zu interpretieren
                            address = int(message)
                            print(f"Adresse {address} empfangen.")
                        except ValueError:
                            # Falls es sich nicht um eine Adresse handelt, nehmen wir an, dass es sich um Daten handelt
                            data = message
                            print(f"Daten {data} empfangen.")
                            if address is not None:
                                pressed_keys.append(
                                    data
                                )  # Die Taste zur Liste der gedrückten Tasten hinzufügen

                    time.sleep(
                        0.1
                    )  # Leichte Pause, um CPU-Auslastung zu reduzieren
        except serial.SerialException as e:
            print(f"Fehler beim Lesen vom COM-Port: {e}")
            break
        except Exception as e:
            print(f"Unerwarteter Fehler: {e}")
            break


# Start der Lesefunktion, falls ein COM-Port erkannt wurde
if keyboard_port:
    print(f"Starte das Lesen vom COM-Port {keyboard_port}...")
    ser = serial.Serial(keyboard_port, 9600, timeout=1)

    # Thread für das kontinuierliche Lesen starten
    read_thread = threading.Thread(
        target=read_from_com_port, args=(ser,), daemon=True
    )
    read_thread.start()
else:
    print("Kein Controller verbunden. Nachrichten können nicht gelesen werden.")


if keyboard_port:
    print(f"Controller verbunden an: {keyboard_port}")

else:
    print("Controller konnte nicht erkannt werden.")


def update_display():
    for row in range(4):
        for col in range(4):
            key = key_layout[row][col]
            labels[row][col].configure(
                text=f"{key}\n({key_mappings[current_module].get(key, '')})"
            )

    if keyboard_port:
        com_port_label.configure(text=f"COM Port: {keyboard_port}")
    else:
        com_port_label.configure(text="COM Port: Not Connected")

    update_remove_button_state()


def save_mapping():
    with open("key_mappings.json", "w") as f:
        json.dump(key_mappings, f)
    update_display()


def load_mapping():
    global key_mappings, current_module
    with open("key_mappings.json", "r") as f:
        loaded_data = json.load(f)
        key_mappings = loaded_data

    module_dropdown.configure(values=list(key_mappings.keys()))

    if current_module not in key_mappings:
        current_module = list(key_mappings.keys())[0]
    module_var.set(current_module)
    update_display()


def assign_key(row, col):
    key = key_layout[row][col]
    new_binding = entry_var.get()
    if not new_binding:
        return
    normalized_binding = normalize_key_binding(new_binding)
    key_mappings[current_module][key] = normalized_binding
    update_display()


def reset_key(row, col):
    key = key_layout[row][col]
    key_mappings[current_module][key] = ""
    update_display()


def switch_module(module):
    global current_module
    current_module = module
    if module not in key_mappings:
        key_mappings[module] = {key: "" for row in key_layout for key in row}
    update_display()


ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

root = ctk.CTk()
root.title("Keypad Mapper")

screen_width = root.winfo_screenwidth()
screen_height = root.winfo_screenheight()
initial_width = 600
initial_height = 600

position_top = int((screen_height - initial_height) / 2)
position_left = int((screen_width - initial_width) / 2)

root.resizable(False, False)

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

module_var = ctk.StringVar(value="Module 1")
module_dropdown = ctk.CTkOptionMenu(
    module_frame,
    variable=module_var,
    values=list(key_mappings.keys()),
    command=switch_module,
)
module_dropdown.pack(side="left", padx=5)


def add_module():
    if len(key_mappings) < 7:
        new_module = f"Module {len(key_mappings) + 1}"
        key_mappings[new_module] = {
            key: "" for row in key_layout for key in row
        }
        module_dropdown.configure(values=list(key_mappings.keys()))
        module_var.set(new_module)
        switch_module(new_module)

    update_add_module_button_state()


def update_add_module_button_state():
    if len(key_mappings) >= 7:
        add_module_button.configure(state="disabled", fg_color="#D3D3D3")
    else:
        add_module_button.configure(state="normal", fg_color="#5A9BD8")


add_module_button = ctk.CTkButton(
    module_frame, text="Add Module", command=add_module
)
add_module_button.pack(side="left", padx=5)


def remove_module():
    global current_module
    if current_module == "Module 1":
        return

    module_index = list(key_mappings.keys()).index(current_module)
    del key_mappings[current_module]

    updated_modules = list(key_mappings.keys())
    module_dropdown.configure(values=updated_modules)

    if module_index > 0:
        current_module = updated_modules[module_index - 1]
    else:
        current_module = updated_modules[0]

    module_var.set(current_module)
    update_add_module_button_state()
    update_display()


remove_module_button = ctk.CTkButton(
    module_frame,
    text="Remove Module",
    command=remove_module,
    fg_color="#FF7F7F",
)
remove_module_button.pack(side="left", padx=5)


def update_remove_button_state():
    if len(key_mappings) == 1 or current_module == "Module 1":
        remove_module_button.configure(state="disabled", fg_color="#D3D3D3")
    else:
        remove_module_button.configure(state="normal", fg_color="#FF7F7F")


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
            text=f"{key}\n({key_mappings[current_module][key]})",
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

    screen_width = info_window.winfo_screenwidth()
    screen_height = info_window.winfo_screenheight()

    window_width = 400
    window_height = 300
    x = (screen_width // 2) - (window_width // 2)
    y = (screen_height // 2) - (window_height // 2)

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


update_display()
root.mainloop()
