#include "header.h"

#include <sstream>
#include <iomanip>

// true -> Upload fuer Master | false -> Upload fur Slave
#define SENDER true
//Spezifische Konfiguration fuer Slaves 
#if SENDER == false
  //Kommunikations Pin (MOSI) ATMEGA 15 | ATTINY 5
  #define COMM_IN 12    
  //Kommunikations Pin (MISO) ATMEGA 16 | ATTINY 6                      
  #define COMM_OUT 14                         
  //0x01 := Tastatur, 0x02 := Audiopad, ...
  #define FUNCTION  0x01 
  //Standardadresse eines neuen Busteilnehmers
  #define STARTADRESS 0x00
  #define REQUESTADRESS 0xFF
  //PIN-Belegung spezifisch fuer Slave
  #define ROW1 2
  #define ROW2 0
  #define ROW3 4
  #define ROW4 16
  
  #define COL1 17
  #define COL2 5
  #define COL3 18
  #define COL4 19
//Spezifische Konfiguration fuer Master
#elif SENDER == true
  //Kommunikations Pin
  #define COMM_IN 12
  //Kommunikations Pin                          
  #define COMM_OUT 14                         
  //Adresse des Hauptmoduls
  #define STARTADRESS 0x01
  #define REQUESTADRESS 0xFF
  //bei n = INIT Polling Durchlaeufen wird die INIT 
  //Nachricht an den PC gesendet
  #define INIT 10
  #define LIST 10
  //Timeout-Schwellwert
  #define TIMEOUT_THRESHOLD 3
#endif
//nach 'BITSTUFFING' gleichwertigen Bits wird ein 
// anderswertiges Bit eingeschoben
#define BITSTUFFING 3
#define DELAY(x) delayMicroseconds(x)
//laenge eines Bits in us
#define SENDDELAY 500                       
#define NOADRESS 0x00
#define WAITLOOP 100000
// Zum testen!
#define DEBUG false                    
#define DEBUG1 false
#define DEBUG2 false
#define DEBUG3 false
#define DEBUG4 false
#define DEBUG5 false
#define DEBUG6 false
#if SENDER == false
  //Matrix-Tastatur--------------------------
  //           row col              
  int tasten2x2[2] [2] = {
    {0xA1,0xB1},
    {0xA2,0xB2}
  };
  int tasten4x4[4] [4] = {
    {0xA1,0xB1,0xC1,0xD1},
    {0xA2,0xB2,0xC2,0xD2},
    {0xA3,0xB3,0xC3,0xD3},
    {0xA4,0xB4,0xC4,0xD4}
  };

    int ROWS[2] {
      ROW1,
      ROW2
    };
    int COLS[2] {
      COL1,
      COL2
    };

    int ROWS4[4]{
      ROW1,ROW2,ROW3,ROW4
    };
    int COLS4[4]{
      COL1,COL2,COL3,COL4
    };
#endif
//Variablen--------------------------------
bool FLAG_NEWDEVICE = false;
bool FLAG_DELETEDEVICE = false;
volatile unsigned long timeStamp;      //Zeit Merker
volatile unsigned long delayTime;      //berechnete Taktzeit
volatile char global_adress = 0x00;    //eingelesene Adresse
volatile char global_COF = 0x00;       //Groesse der Nachricht
volatile char global_message[8];       //eingelesene Daten
volatile int global_CRC;
volatile char mask = 0x01;             //0000 0001 Binaermaske
//Adressengroesse in Bits (maximal (2^adressSize)-2 Teilnehmer)
volatile int global_adressSize = 3;    
volatile int global_COFSize = 3;       //Groesse des COF Pakets
//Standardadresse eines neuen Busteilnehmers
volatile char myAdress = STARTADRESS;  
//Feste Adresse des Hauptcontrollers
volatile char controllerAdress = 0x01; 
//Zaehler fuer die Initialisierung
volatile int init_cnt = 100;
volatile int list_cnt = 100;
char* global_decoded_message; //Bitstuffed message
char* global_BITSTUFFED_message; //Bitstuffed message
char* global_BITSTUFFED_message_recv; //Bitstuffed message empfangen
char* global_decoded_message_recv; //decodierte message empfangen
//Listen des Hauptcontollers------------------------------
//Speichern der Adressen in die Liste mit der zugehoerigen Funktion

//Spezifische Konfiguration fuer Master
#if SENDER == true
  //Freie Adressen
  std::list<char> unusedAdresses;
  auto iterator_unusedAdresses = unusedAdresses.begin();
  //Adressen aller Teilnehmer
  std::list<char> Adresses;
  auto iterator_Adresses = Adresses.begin();
  //polling counter iteriert durch die liste der adressen 
  int polling_counter = 0; 
  //Anzahl der vergebenen Adressen
  int num_adresses = 0;              
  std::list<device> devices;
  auto iterator_devices = devices.begin(); 
#endif


//allg. Funktionen -------------------------------
// CRC-16-Implementierung mit Polynom 0x1021
unsigned int crc16(char* data, size_t length) {
    unsigned int crc = 0xFFFF; // Initialwert
    unsigned int polynomial = 0x1021; // CRC-16-Polynom

    for (size_t i = 0; i < length; i++) {
        crc ^= (data[i] << 8); // Datenbyte in CRC-Register laden

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) { // Pruefen, ob das MSB gesetzt ist
                crc = (crc << 1) ^ polynomial; // Polynom anwenden
            } else {
                crc <<= 1; // Bitweise nach links verschieben
            }
        }
    }
    return crc & 0xFFFF; // Nur die unteren 16 Bits zurueckgeben
}

bool verifyCrc16(const unsigned char* data, size_t length) {
    unsigned int crc = 0xFFFF; // Initialwert
    unsigned int polynomial = 0x1021; // CRC-16-Polynom

    for (size_t i = 0; i < length; i++) {
        crc ^= (data[i] << 8); // Datenbyte in CRC-Register laden

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) { // Pruefen, ob das MSB gesetzt ist
                crc = (crc << 1) ^ polynomial; // Polynom anwenden
            } else {
                crc <<= 1; // Bitweise nach links verschieben
            }
        }
    }

    return (crc & 0xFFFF) == 0; // Pruefen, ob der Rest 0 ist
}


#if SENDER == true
  bool isArrayZero(volatile char* array, int size) {
    for (int i = 0; i < size; i++) {
        if (array[i] != 0) {
            return false; // Ein Element ist nicht 0
        }
    }
    return true; // Alle Elemente sind 0
  }
#endif

void appendChar(char* &str, char c) {
    if (str == NULL) {
        // Wenn str null ist, initialisieren Sie es mit dem neuen Zeichen
        str = new char[2]; // 1 Zeichen + Nullterminator
        str[0] = c;
        str[1] = '\0';
    } else {
        size_t len = strlen(str);
        // +1 fuer das neue Zeichen, +1 fuer den Nullterminator
        char* newStr = new char[len + 2]; 
        strcpy(newStr, str);
        newStr[len] = c;
        newStr[len + 1] = '\0';
        delete[] str;
        str = newStr;
    }
}

//receive
bool syncronisation(){
  //if (DEBUG) Serial.println("syncronisation");
  unsigned long lok_delayTime;
  timeStamp = micros();
  while (digitalRead(COMM_IN) == HIGH);
  lok_delayTime = micros() - timeStamp;
  if (lok_delayTime > SENDDELAY*1.25 &&
      lok_delayTime < SENDDELAY*1.75){
    delayTime = lok_delayTime/1.5;
    DELAY(delayTime*1.2); 
    return true;
  }
  else return false;
}


// Funktion zum Dekodieren von Bitstuffing
void decodeBitstuffedMessage(const char* stuffedMessage, 
                char* &decodedMessage, int x) {
    int count;
    char lastBit = '\0';
    size_t len = strlen(stuffedMessage);
    decodedMessage = new char[len + 1]; // +1 fuer den Nullterminator
    size_t decodedIndex = 0;
    count = 1; // Zaehler auf 1 setzen
    
    // Durchlaufen der bitgestopften Nachricht
    for (size_t i = 0; i < len; ++i) {
        char currentBit = stuffedMessage[i];
        if (currentBit == lastBit) {
            count++;
            if (count < x) {
                decodedMessage[decodedIndex++] = currentBit;
            } else {// Wenn x gleiche Bits gefunden wurden
                // Fuege das Bit hinzu
                decodedMessage[decodedIndex++] = currentBit; 
                i++; // ueberspringe das naechste Bit
                count = 1; // Zaehler zuruecksetzen
                if (i >= len) break; // Vermeiden von ueberlaeufen
                currentBit = stuffedMessage[i];
            }
        } else {
            decodedMessage[decodedIndex++] = currentBit;
            count = 1; // Zaehler zuruecksetzen und auf 1 setzen
        }

        lastBit = currentBit;
        
    }

    decodedMessage[decodedIndex] = '\0'; // Nullterminator hinzufuegen
    if (DEBUG3) Serial.println("Empfangene decodierte Nachricht:");
    if (DEBUG3) Serial.println(decodedMessage);
}



void parseDecodedMessage(const char* decodedMessage) {
    // Adresse extrahieren
    global_adress = 0;
    for (int i = 0; i < global_adressSize; i++) {
        if (decodedMessage[i] == '1') {
            global_adress |= (1 << i);
        }
    }

    // COF extrahieren
    global_COF = 0;
    for (int i = 0; i < global_COFSize; i++) {
        if (decodedMessage[global_adressSize + i] == '1') {
            global_COF |= (1 << i);
        }
    }
    // Daten extrahieren basierend auf der Groesse des COF
    // COF gibt die Groesse der Nachricht in Bytes an (0 bedeutet 1 Byte)
    int dataSize = global_COF + 1;
    for (int i = 0; i < dataSize; i++) {
        char message[8] = {0};
        for (int j = 0; j < 8; j++) {
            if (decodedMessage[global_adressSize + 
                  global_COFSize + i * 8 + j] == '1') {
                message[i] |= (1 << j);
            }
            
          }
        global_message[dataSize-1-i] = message[i];
        //global_message[i] = message[i];
    }
    // Pruefsumme extrahieren
    for (int i = 0; i < 16; i++) {
        if (decodedMessage[global_adressSize +global_COF 
              + (global_COF*8) + i] == '1') {
            global_CRC |= (1 << i);
        }
    }
    // Ausgabe der extrahierten Bestandteile
    if (DEBUG3) Serial.print("Empfangene Adresse: ");
    if (DEBUG3) Serial.println(global_adress, HEX);
    if (DEBUG3) Serial.print("Empfangener COF: ");
    if (DEBUG3) Serial.println(global_COF, HEX);
    if (DEBUG3) Serial.print("Empfangene Daten: ");
    if (DEBUG3) Serial.println();
}



bool readMessage() {
    //if (DEBUG) Serial.println("readMessage");
    // neue Nachricht auf dem Bus
    if (syncronisation()) {
        // Initialisieren Sie die globale bitgestopfte Nachricht
        global_BITSTUFFED_message_recv = new char[1];
        global_BITSTUFFED_message_recv[0] = '\0';
        // Lesen Sie die Nachricht bitweise ein
        int eofCounter = 0;
        while (true) {
            char bit = (digitalRead(COMM_IN) == HIGH) ? '1' : '0';
            appendChar(global_BITSTUFFED_message_recv, bit);
            DELAY(delayTime);
            // ueberpruefen Sie auf EOF (Ende der Nachricht)
            if (bit == '1') {
                eofCounter = 1; // Start des EOF-Musters erkannt
            } else if (eofCounter > 0) {
                eofCounter++;
                if (eofCounter == 6) {
                    // EOF-Muster erkannt: 1 gefolgt von 5 Nullen
                    // Entfernen Sie das EOF-Muster aus der Nachricht
                    global_BITSTUFFED_message_recv[strlen(
                          global_BITSTUFFED_message_recv) - 6] = '\0';
                    break;
                }
            } else {
              // Zaehler zuruecksetzen, wenn das Muster unterbrochen wird
                eofCounter = 0; 
            }
        }
        if (DEBUG3) Serial.println("Empfangene Nachricht:");
        if (DEBUG3) Serial.println((global_BITSTUFFED_message_recv));
        // Dekodieren Sie die bitgestopfte Nachricht
        char* decodedMessage = NULL;
        decodeBitstuffedMessage(global_BITSTUFFED_message_recv,
              decodedMessage, BITSTUFFING);
        global_decoded_message_recv = decodedMessage;
        if (DEBUG3) Serial.println("Empfangene decodierte Nachricht:");
        if (DEBUG3) Serial.println(global_decoded_message_recv);

        parseDecodedMessage(global_decoded_message_recv);

        if (DEBUG3) Serial.println("Adresse:");
        if (DEBUG3) Serial.println(global_adress,HEX);
        if (DEBUG3) Serial.println("COF:");
        if (DEBUG3) Serial.println(global_COF,HEX);
        if (DEBUG3) Serial.println("Daten:");
        if (DEBUG3) for (int i = 0; i <= global_COF; i++) {
            Serial.println(global_message[i],HEX);
        }
        delete[] decodedMessage; // Speicher freigeben
        return true;
    } else {
        return false;
    }
}


//------------------------------

//send
bool awaitBusFree(){
  if (DEBUG2) Serial.println("awaitBusFree");
  for (int i = 0; i < 10000; i++){
    bool busFree = true;
    for (int j = 0; j < 4; j++) {
      if (digitalRead(COMM_IN) == HIGH) {
        busFree = false;
        break;
      }
      DELAY(delayTime);
    }
    if (busFree) return true;
  }
  return false;
}
//Bleibt gleich
void sendSOF(){
  if (DEBUG) Serial.println("sendSOF");
  digitalWrite(COMM_OUT, HIGH);
  DELAY(SENDDELAY*1.5);
  digitalWrite(COMM_OUT, LOW);
  DELAY(SENDDELAY);
}

void writeAdress(char adress) {
  if (DEBUG) Serial.println("writeAdress");
  for (int i = 0; i < (global_adressSize); i++) {
    if ((adress & 0x01) == 0x01) {
      appendChar(global_decoded_message, '1');
      //digitalWrite(COMM_OUT, HIGH);
      if (DEBUG) Serial.print("1");
    } else {
      //digitalWrite(COMM_OUT, LOW);
      appendChar(global_decoded_message, '0');
      if (DEBUG) Serial.print("0");
    }
    adress = adress >> 1;
  }
  if (DEBUG) Serial.println();
}


void writeCOF(char dataSize){
  if (DEBUG) Serial.println("writeCOF");
  for (int i = 0; i < (global_COFSize); i++) {
    if ((dataSize & 0x01) == 0x01) {
      appendChar(global_decoded_message, '1');
      if (DEBUG) Serial.print("1");
    } else {
      appendChar(global_decoded_message, '0');
      if (DEBUG) Serial.print("0");
    }
    dataSize = dataSize >> 1;
  }
  if (DEBUG) Serial.println();
}

void writeData(char dataSize,char* data) {
    if (DEBUG) Serial.println("writeData"); 
    for (int i = dataSize; i >= 0; i--)
    {
      for (int j = 0; j < 8; j++)
      {
        if ((data[i] & 0x01) == 0x01) {
          appendChar(global_decoded_message, '1');
          if (DEBUG) Serial.print("1");
        } else {
          appendChar(global_decoded_message, '0');
          if (DEBUG) Serial.print("0");
        }
        data[i] = data[i] >> 1;
      }
    }
}

void writeCRC(unsigned int CRC){
  if (DEBUG) Serial.println("writeAdress");
  for (int i = 0; i < 16; i++) {
    if ((CRC & 0x01) == 0x01) {
      appendChar(global_decoded_message, '1');
      //digitalWrite(COMM_OUT, HIGH);
      if (DEBUG) Serial.print("1");
    } else {
      //digitalWrite(COMM_OUT, LOW);
      appendChar(global_decoded_message, '0');
      if (DEBUG) Serial.print("0");
    }
    CRC = CRC >> 1;
  }
  if (DEBUG) Serial.println();
}

//Bleibt gleich
void sendEOF(){
  if (DEBUG) Serial.println("sendEOF");
  digitalWrite(COMM_OUT, HIGH);
  if (DEBUG) Serial.print("1");
  DELAY(SENDDELAY);
  digitalWrite(COMM_OUT, LOW);
  for (int i = 0; i < 5; i++) {   //5 mal?                
    if (DEBUG) Serial.print("0");                 
    DELAY(SENDDELAY);                             
  }
  if (DEBUG) Serial.println();
}

void sendDataOnBus(char* stuffedMessage) {
  if (DEBUG) Serial.println("sendDataOnBus");
      for (int i = 0; i < sizeof(stuffedMessage); i++) {
        if (stuffedMessage[i] == '1') {
            digitalWrite(COMM_OUT, HIGH);
        } else {
            digitalWrite(COMM_OUT, LOW);
        }
        DELAY(SENDDELAY);
    }
  
}
void writeBitstuffedMessage(int x) {
    if (DEBUG) Serial.println("writeBitstuffedMessage");
    std::string stuffedMessage;
    int count = 0;
    char lastBit = '\0';
    for (int i = 0; i < strlen(global_decoded_message); i++) {
        char currentBit = global_decoded_message[i];
        stuffedMessage += currentBit;
        if (currentBit == lastBit) {
            count++;
            // Wenn x gleiche Bits gefunden wurden
            if (count == x) { 
                // Bitwechsel einfuegen
                char stuffedBit = (currentBit == '1') ? '0' : '1';
                stuffedMessage += stuffedBit;
                count = 1; // Zaehler zuruecksetzen
                switch (currentBit)
                {
                case '0': lastBit = '1';
                  /* code */
                  break;
                case '1': lastBit = '0';
                  /* code */
                  break;
                default:
                  break;
                }
            }
        } else {
          //Zaehler zuruecksetzen und auf 1 setzen
            count = 1; 
            lastBit = currentBit;
        }
    }

    // Update the global_BITSTUFFED_message with the stuffed message
    delete[] global_BITSTUFFED_message;
    global_BITSTUFFED_message = new char[stuffedMessage.length() + 1];
    strcpy(global_BITSTUFFED_message, stuffedMessage.c_str());
    if (DEBUG3) Serial.println("Decoded Message:");
    if (DEBUG3) Serial.println(global_decoded_message);
    if (DEBUG3) Serial.println("Bitstuffed Message:");
    if (DEBUG3) Serial.println(global_BITSTUFFED_message);
}

void sendMessage(char adress,char dataSize,char* data){
  if (DEBUG) Serial.println("sendMessage");
    writeAdress(adress);
    writeCOF(dataSize);
    writeData(dataSize,data);
    writeCRC(crc16(global_decoded_message,
              global_adressSize+global_COFSize+(global_COFSize*8)));
    writeBitstuffedMessage(BITSTUFFING);
  //Abfrage Bus frei
  if (awaitBusFree())  { 
    if (DEBUG3) Serial.println("Sending!");                       
    sendSOF();
    sendDataOnBus(global_BITSTUFFED_message);
    sendEOF();
    global_decoded_message = NULL;
  }
}
//------------------------------

void global_reset(){
  for (int i = 0; i < 8; i++)
  {
    global_message[i] = 0;
  }
  global_COF = 0;
  global_adress = 0;
}


bool await_response(char adress) {
    for (int i = 0; i < WAITLOOP; i++) {
        if (readMessage()) {
          if (DEBUG3) Serial.println("eingelesene Adresse:");
          if (DEBUG3) Serial.println(global_adress,HEX);
            if (global_adress == adress) {
                Serial.println("Antwort erhalten!");
                return true;
            }
        }
    }
    return false;
}


//------------------------------
//Spezifische Funktionen fuer Slaves 
#if SENDER == false
void getAdress(){
  //Bitte um Adresse + 2. byte = Funktion
  char message[] = {REQUESTADRESS,FUNCTION};
  sendMessage(controllerAdress,0x01,message); 
  Serial.println("Adresse angefragt!");
  //Antwort erhalten
  if (await_response(NOADRESS)){   
    //Speichern der neuen Adresse
    myAdress = global_message[0];
    if (DEBUG3) Serial.print("meine Adresse: ");
    if (DEBUG3) Serial.print(myAdress,HEX);
    if (DEBUG3) Serial.println();
  } 
}

void myFunction(){
  Serial.println("myFunction!");
  char myDatasize = 0;
  char myData[8] = {0};
  //---------------------------

  //----------------------------

  //----------------------------
    for (int row = 0; row < 2; row++) {
      digitalWrite(ROWS[row], HIGH);
      for (int col = 0; col < 2; col++) {
        if (digitalRead(COLS[col]) == HIGH) {
          //bei OUTPUT ROWS[row] ist COLS[col] HIGH 
          myData[myDatasize] = tasten2x2[row][col];
          myDatasize++;
          }
        }
        digitalWrite(ROWS[row], LOW);
        DELAY(5);
      }

    //----------------------------
 // Uebermitteln der Daten
  if (myDatasize == 0){
    sendMessage(controllerAdress,myDatasize,myData);
  }else{
    sendMessage(controllerAdress,myDatasize-1,myData);
  }
}


//Spezifische Funktionen fuer Master
#elif SENDER == true

void printDeviceList(const std::list<device>& deviceList) {
    // ueberpruefen, ob die Liste leer ist
    if (deviceList.empty()) {
        Serial.println("Die Liste ist leer");
        return;
    }
    // Elemente der Liste ausgeben
    auto it = deviceList.begin();
    if (it != deviceList.end()) {
        ++it; // Erster Teilnehmer wird uebersprungen
    }
    for (; it != deviceList.end(); ++it) {    
        //Serial.print("Adresse: ");
        Serial2.println(it->adress, HEX);
        //Serial.print(", Funktion: ");
        Serial2.println(it->funktion, HEX);
}
    }
    

void USB(char adress,char function,volatile char* data){
  // uebermitteln der INIT Nachricht, zum feststellen des COM-Ports
  if (adress == 0x00){
    init_cnt++;
    //INIT legt die fes, wie oft diese Nachricht uebermittelt wird
    if (init_cnt >= INIT){
      Serial2.println("INIT");
      if (DEBUG5) Serial.println("INIT");
      init_cnt = 0;
    }
  }
  // uebermitteln der Geraeteliste mit den entsprechenden Funktionen
  if (adress == 0x00){
    list_cnt++;
    //INIT legt die fes, wie oft diese Liste uebermittelt wird
    //Die Flag wird immer dann gesetzt, wenn ein neues Geraet 
    //angeschlossen wird
    if (list_cnt >= LIST | FLAG_NEWDEVICE){
      FLAG_NEWDEVICE = false;
      list_cnt = 0;
      Serial2.println("LIST");
      if (DEBUG5) Serial.println("LIST");
      printDeviceList(devices);
      Serial2.println("END");
    }
  return;
  }
  // Nullpointer abfangen
  if (data == NULL){
    if (DEBUG4) Serial.println("data == NULL");
    return;
  }
  //keine Daten zu uebertragen
  if (isArrayZero(data,8)){
    if (DEBUG4) Serial.println("keine Daten zu uebertragen");
    return;
  }
  // uebermitteln der Daten des jeweiligen Slaves
  Serial2.println("START");
  if (DEBUG5) Serial.println("START");
  Serial2.println(adress,HEX);
  if (DEBUG5) Serial.println(adress,HEX);
  for (int i = 0; i <= global_COF; i++)
  {
    Serial2.println(data[i],HEX);
    if (DEBUG5) Serial.println(data[i],HEX);
  }
  Serial2.println("END");
  if (DEBUG5) Serial.println("END");
}


char newAdress(char function){
  if (DEBUG4) Serial.println("newAdress");
//es sind noch Adressen frei
if (unusedAdresses.empty() == false)          
{
  //nehme eine freie Adresse
  char freeAdress = unusedAdresses.back();    
  if (DEBUG) Serial.println(freeAdress,HEX);
  //entferne diese aus 'unused Adresses' 
  unusedAdresses.pop_back(); 
  //fuege Sie zur Pollingliste hinzu                 
  Adresses.push_back(freeAdress);             
  //neuen Teilnehmer der device Liste hinzufuegen
  if (DEBUG5) Serial.print("device function: ");
  if (DEBUG5) Serial.println(function,HEX);
  device teilnehmer(freeAdress,function,nullptr,0);
  if (DEBUG5) Serial.print("device adress: ");
  if (DEBUG5) Serial.println(teilnehmer.adress,HEX);  
  if (DEBUG5) Serial.print("device funktion: ");
  if (DEBUG5) Serial.println(teilnehmer.funktion,HEX);
  devices.push_back(teilnehmer);
  //Pollingschleife vergroessern
  num_adresses++;   
  //Uebermittlung der Geraeteliste, wenn ein Teilnehmer dazukommt
  FLAG_NEWDEVICE = true;
  if (DEBUG4) Serial.println("Neue Adresse:");
  if (DEBUG4) Serial.println(freeAdress,HEX);                          
  return freeAdress;                          
}
// keine Adressen mehr frei!
else return 0x00;                             
}
void printList(const std::list<char>& lst) {
    // Ueberpruefen, ob die Liste leer ist
    if (lst.empty()) {
        Serial.println("Die Liste ist leer");
        return;
    }

    // Elemente der Liste ausgeben
    for (const char &element : lst) {
        Serial.print(element,HEX);
        Serial.print(",");
    }
    std::cout << std::endl;
}

void timeout(char adress, bool no_response){
  if (DEBUG) Serial.println("timeout");
  //neuer Teilnehmer!
  if (global_message[0] == REQUESTADRESS && global_adress ==
      controllerAdress && !no_response ) {                                                       
    if (DEBUG2) Serial.println("Neuer Teilnehmer");  
    // neuen Teilnehmer hinzufuegen mit seiner Funktion
    char freeAdress[] = {newAdress(global_message[1])}; 
    // freie Adresse senden an noadress                       
    sendMessage(NOADRESS,0,freeAdress);                                                           
  }
  //kein neuer Teilnehmer
  else if (no_response && adress == NOADRESS){
    if (DEBUG2) Serial.println("kein Neuer Teilnehmer");
    return;                                                                                       
  } 
  //Keine Anwort des angefragten Teilnehmers
  else if (no_response && adress != NOADRESS){
    // inkrementieren des Timeout-counters
    iterator_devices->timeout++;                                                                             
    if (DEBUG2) Serial.println("keine Antwort von:");
    if (DEBUG2) Serial.println(iterator_devices->adress,HEX);
    if (iterator_devices->timeout >= TIMEOUT_THRESHOLD) {
      //Adresse wieder freigeben
      unusedAdresses.push_back(iterator_devices->adress); 
      //Adresse aus der Pollingliste entfernen
      Adresses.remove(iterator_devices->adress);        
      //Adresse aus der device Liste entfernen
      devices.erase(iterator_devices);                  
      //Pollingschleife verkleinern
      num_adresses--;
      //Uebermittlung der Geraeteliste, wenn ein Teilnehmer wegfaellt                            
      FLAG_NEWDEVICE = true;
      //keine Iteration in der Pollingschleife
      FLAG_DELETEDEVICE = true;
    }
  }
  //Antwort erhalten
  else if (!no_response && adress != NOADRESS){
    //Timeout zuruecksetzen
    iterator_devices->timeout = 0;                                                                             
    if (DEBUG2) Serial.println("Antwort von:");
    if (DEBUG2) Serial.println(iterator_devices->adress,HEX);
    //Daten speichern
    iterator_devices->latest_data = global_message; 
  }
}

void polling(){
  if (DEBUG) Serial.println("polling");
  //naechstes element der adressliste
  if (!FLAG_DELETEDEVICE)
  {
    polling_counter++;
  }
  // ein Teilnehmer ist weggefallen -> nicht iterieren
  else
  {
    FLAG_DELETEDEVICE = false;
  }
  // ueberpruefen Sie, ob der polling_counter die 
  // Anzahl der Adressen erreicht hat
  if (polling_counter >= num_adresses + 1) {
      polling_counter = 0; // Zuruecksetzen des polling_counter
  }
  //Setzen des Iterators an die entsprechende Stelle
  iterator_devices = devices.begin();
  std::advance(iterator_devices,polling_counter);
  bool response = false;
  char request[] = {0xFF};
  global_reset();
  
  //request fuer abzufragende Adresse
  sendMessage(iterator_devices->adress,0,request);  
  //warten auf Rueckmeldung                                            
  response = await_response(controllerAdress);
  //Ruecksetzen des Timeouts (zudem Zuweisung von neuen Adressen)
  timeout(iterator_devices->adress,!response);
  //Uebermitteln der Daten an den PC
  USB(iterator_devices->adress,iterator_devices->
        funktion,iterator_devices->latest_data);
}


#endif

//SETUP-----------------------------

//Spezifisches Setup fuer Slaves 
#if SENDER == false
void setup() {
  Serial.begin(115200);
//__DON'T CHANGE!!!------------------
  pinMode(COMM_IN, INPUT);
  pinMode(COMM_OUT, OUTPUT);
  if (DEBUG) Serial.println("COMM Pins are setup!");
//------------------------------
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);
  pinMode(ROW3, OUTPUT);
  pinMode(ROW4, OUTPUT);

  pinMode(COL1, INPUT_PULLDOWN);
  pinMode(COL2, INPUT_PULLDOWN);
  pinMode(COL3, INPUT_PULLDOWN);
  pinMode(COL4, INPUT_PULLDOWN);

}
//Spezifisches Setup fuer Master
#elif SENDER == true
void setup(){
  Serial.begin(115200);
  Serial2.begin(9600);
  if (DEBUG) Serial.println("serial is setup!");


  // Elemente hinzufuegen
  //Adresse 0x01 fuer den Controller reserviert!
  for (int i = 2; i < pow(2,global_adressSize); i++)
  {
    unusedAdresses.push_back(i);
    if (DEBUG2) Serial.println(unusedAdresses.back(),HEX);
  }
  device geraet;

  Adresses.push_front(NOADRESS);
  device noadress = device(NOADRESS,0,0,0);
  devices.push_back(noadress);
  iterator_devices = devices.end();
  geraet = *iterator_devices;
  Serial.print(geraet.adress,HEX);

  //__DON'T CHANGE!!!-----------------------
  pinMode(COMM_IN, INPUT);
  pinMode(COMM_OUT, OUTPUT);
  if (DEBUG) Serial.println("COMM Pins are setup!");
  //------------------------------
}
#endif

#if SENDER == true

void loop() {
  if(SENDER){
  polling();

  //Debug Delay
  if (DEBUG | DEBUG1 | DEBUG2 | DEBUG3 | DEBUG4){
    delay(1000);
  } 
  }
}
#elif SENDER == false
void loop() {
    if (readMessage())
    { 
      // Noch keine Adresse
      if (global_adress == myAdress && myAdress == NOADRESS){
      getAdress();
      global_reset();
      }
      //Aufruf meiner Adresse -> fuehre meine Funktion aus
      else if (global_adress == myAdress && myAdress != NOADRESS)
      {
        myFunction();
      } 
  }
}
#endif



