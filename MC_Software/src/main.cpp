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
  #define COL1 4
  #define COL2 5

//Spezifische Konfiguration fuer Master
#elif SENDER == true
  //Kommunikations Pin
  #define COMM_IN 12
  //Kommunikations Pin                          
  #define COMM_OUT 14                         
  //Adresse des Hauptmoduls
  #define STARTADRESS 0x01
  #define REQUESTADRESS 0xFF
  //bei n = INIT Polling Durchläufen wird die INIT 
  //Nachricht an den PC gesendet
  #define INIT 10
  #define LIST 10
#endif
#define BITSTUFFING 3

#define DELAY(x) delayMicroseconds(x)
#define SENDDELAY 1000                      
#define NOADRESS 0x00

// Zum testen!
#define DEBUG false                    
#define DEBUG1 false
#define DEBUG2 false
#define DEBUG3 false
#define DEBUG4 false
#define DEBUG5 true
#define DEBUG6 true

//Variablen--------------------------------
bool FLAG_NEWDEVICE = false;

volatile unsigned long timeStamp;      //Zeit Merker
volatile unsigned long delayTime;      //berechnete Taktzeit
volatile char global_adress = 0x00;    //eingelesene Adresse
volatile char global_COF = 0x00;       //Groesse der Nachricht
volatile char global_message[8];       //eingelesene Daten
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
#ifdef SENDER == true
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
        char* newStr = new char[len + 2]; // +1 für das neue Zeichen, +1 für den Nullterminator
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
    Serial.println(delayTime);                                                       // Ich kann diese Zeile nicht loeschen?? Warum?!
    DELAY(delayTime*1.2);
    //digitalWrite(COMM_OUT, HIGH);
    //DELAY(delayTime);
    return true;
  }
  else return false;
}

bool sync2() { //in Arbeit
  if (DEBUG6) Serial.println("sync");
  int lowCount = 0;
  if (digitalRead(COMM_IN) == LOW) {
    lowCount++;
    DELAY(delayTime); // Warten Sie auf das nächste Bit
    for (int i = 0; i < 3; i++)
    {
      if (digitalRead(COMM_IN) == LOW) {
        lowCount++;
        DELAY(delayTime); // Warten Sie auf das nächste Bit
      }
      else {
        if (DEBUG6) Serial.println("nur " + String(lowCount) + " LOW Bits gefunden");
        return false; // Synchronisation fehlgeschlagen
      }
    }
    
    if (lowCount == 4) {
      // Überprüfen Sie, ob das nächste Bit HIGH ist
      DELAY(50); // Warten Sie auf das nächste Bit
      if (digitalRead(COMM_IN) == HIGH) {
        unsigned long lok_delayTime;
        timeStamp = micros();
        while (digitalRead(COMM_IN) == HIGH);
        lok_delayTime = micros() - timeStamp;
        if (DEBUG6) Serial.println(lok_delayTime);
        DELAY(lok_delayTime); // Warten Sie auf das Ende des Startbits
        return true; // Synchronisation erfolgreich
      }
      else {
        if (DEBUG6) Serial.println("fehlgeschlagen");
        if (DEBUG6) Serial.println("nur " + String(lowCount) + " LOW Bits gefunden");
        return false; // Synchronisation fehlgeschlagen
      }
    }
    } 
  else {
    return false; // Synchronisation fehlgeschlagen
  }
}

// Funktion zum Dekodieren von Bitstuffing
void decodeBitstuffedMessage(const char* stuffedMessage, char* &decodedMessage, int x) {
    int count;
    char lastBit = '\0';
    size_t len = strlen(stuffedMessage);
    decodedMessage = new char[len + 1]; // +1 für den Nullterminator
    size_t decodedIndex = 0;
    count = 1; // Zähler auf 1 setzen
    
    // Durchlaufen der bitgestopften Nachricht
    for (size_t i = 0; i < len; ++i) {
        char currentBit = stuffedMessage[i];
        
        if (currentBit == lastBit) {
            count++;
            if (count < x) {
                decodedMessage[decodedIndex++] = currentBit;
            } else {// Wenn x gleiche Bits gefunden wurden
                decodedMessage[decodedIndex++] = currentBit; // Füge das Bit hinzu
                i++; // überspringe das nächste Bit
                count = 1; // Zähler zurücksetzen
                if (i >= len) break; // Vermeiden von Überläufen
                currentBit = stuffedMessage[i];
            }
        } else {
            decodedMessage[decodedIndex++] = currentBit;
            count = 1; // Zähler zurücksetzen und auf 1 setzen
        }

        lastBit = currentBit;
        
    }

    decodedMessage[decodedIndex] = '\0'; // Nullterminator hinzufügen
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

    // Daten extrahieren basierend auf der Größe des COF
    int dataSize = global_COF + 1; // COF gibt die Größe der Nachricht in Bytes an (0 bedeutet 1 Byte)
    for (int i = 0; i < dataSize; i++) {
        char message[8] = {0};
        for (int j = 0; j < 8; j++) {
            if (decodedMessage[global_adressSize + global_COFSize + i * 8 + j] == '1') {
                message[i] |= (1 << j);
            }
            
          }
        global_message[dataSize-1-i] = message[i];
        //global_message[i] = message[i];
    }
    

    // Ausgabe der extrahierten Bestandteile
    if (DEBUG3) Serial.print("Empfangene Adresse: ");
    if (DEBUG3) Serial.println(global_adress, HEX);
    if (DEBUG3) Serial.print("Empfangener COF: ");
    if (DEBUG3) Serial.println(global_COF, HEX);
    if (DEBUG3) Serial.print("Empfangene Daten: ");
    if (DEBUG3) for (int i = 0; i < dataSize; i++) {
        Serial.print(global_message[i], HEX);
        Serial.print(" ");
    }
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
            //digitalWrite(COMM_OUT, HIGH);
            DELAY(delayTime);
            //digitalWrite(COMM_OUT, LOW);
            // Überprüfen Sie auf EOF (Ende der Nachricht)
            if (bit == '1') {
                eofCounter = 1; // Start des EOF-Musters erkannt
            } else if (eofCounter > 0) {
                eofCounter++;
                if (eofCounter == 6) {
                    // EOF-Muster erkannt: 1 gefolgt von 5 Nullen
                    // Entfernen Sie das EOF-Muster aus der Nachricht
                    global_BITSTUFFED_message_recv[strlen(global_BITSTUFFED_message_recv) - 6] = '\0';
                    break;
                }
            } else {
                eofCounter = 0; // Zähler zurücksetzen, wenn das Muster unterbrochen wird
            }
        }
        if (DEBUG3) Serial.println("Empfangene Nachricht:");
        if (DEBUG3) Serial.println((global_BITSTUFFED_message_recv));
        // Dekodieren Sie die bitgestopfte Nachricht
        char* decodedMessage = NULL;
        decodeBitstuffedMessage(global_BITSTUFFED_message_recv, decodedMessage, BITSTUFFING);
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


void sendAdress(char adress) {
  if (DEBUG) Serial.println("sendAdress");
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


void sendCOF(char dataSize){
  if (DEBUG) Serial.println("sendCOF");
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

void sendData(char dataSize,char* data) {
    if (true) Serial.println("sendData");
    Serial.print("dataSize: ");
    Serial.println(dataSize,HEX);
    Serial.print("data: "); 
    for (int i = 0; i < dataSize; i++) {
      Serial.print(data[i],HEX);
      Serial.print(" ");
    }
    Serial.println();
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

void sendData2(char dataSize,char* data){
  if (DEBUG) Serial.println("sendData");
  for (int j = 0; j <= dataSize; j++) {
    for (int i = 0; i < 8; i++) {
      if ((data[j] & 0x01) == 0x01) {
        appendChar(global_decoded_message, '1');
        if (DEBUG) Serial.print("1");
      } else {
        appendChar(global_decoded_message, '0');
        if (DEBUG) Serial.print("0");
      }
      data[i] = data[i] >> 1;
    }
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
            if (count == x) { // Wenn x gleiche Bits gefunden wurden
                // Bitwechsel einfügen
                char stuffedBit = (currentBit == '1') ? '0' : '1';
                stuffedMessage += stuffedBit;
                count = 1; // Zähler zurücksetzen
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
            count = 1; // Zähler zurücksetzen und auf 1 setzen
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

    // Send the bitstuffed message
    for (int i = 0; i < stuffedMessage.length(); i++) {
        if (stuffedMessage[i] == '1') {
            digitalWrite(COMM_OUT, HIGH);
        } else {
            digitalWrite(COMM_OUT, LOW);
        }
        DELAY(SENDDELAY);
    }
}

void sendMessage(char adress,char dataSize,char* data){
  if (DEBUG) Serial.println("sendMessage");
    sendAdress(adress);
    sendCOF(dataSize);
    sendData(dataSize,data);
    if (true) Serial.println("Message:");
    if (true) Serial.println((global_decoded_message));
  //Abfrage Bus frei
  if (awaitBusFree())  { 

    if (DEBUG3) Serial.println("Sending!");                       
    sendSOF();
    writeBitstuffedMessage(BITSTUFFING);
    sendEOF();
    global_decoded_message = NULL;
  }
}
//------------------------------

//
void global_reset(){
  for (int i = 0; i < 8; i++)
  {
    global_message[i] = 0;
  }
  global_COF = 0;
  global_adress = 0;
}


bool await_response(char adress) {
    for (int i = 0; i < 20000; i++) {
        if (readMessage()) {
          if (DEBUG3) Serial.println("eingelesene Adresse:");
          if (DEBUG3) Serial.println(global_adress,HEX);
            if (global_adress == adress) {
                Serial.println("Antwort erhalten!");
                return true;
            }
        }
    }
    Serial.println("Keine Antwort erhalten!");
    Serial.println("Adresse:");
    Serial.println(global_adress,HEX);
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
  int ROWS[2] {
    ROW1,
    ROW2
  };
  int COLS[2] {
    COL1,
    COL2
  };
  //----------------------------
    for (int row = 0; row < 2; row++) {
      digitalWrite(ROWS[row], HIGH);
      for (int col = 0; col < 2; col++) {
        if (digitalRead(COLS[col]) == HIGH) {
          switch (ROWS[row])
          {
          case ROW1:  
            switch (COLS[col])
            {
            case COL1:
              /* A1 */
              //Serial.println("A1");
              myData[myDatasize] = 0xA1;
              myDatasize ++;  
              break;
            case COL2:
              /* B1 */
              //Serial.println("B1");
              myData[myDatasize] = 0xB1; 
              myDatasize ++;
              break;
            default:
              break;
            }
            break;
          case ROW2:
            switch (COLS[col])
            {
            case COL1:
              /* A2 */
              //Serial.println("A2");
              myData[myDatasize] = 0xA2;
              myDatasize ++;
              break;
            case COL2:
              /* B2 */
              //Serial.println("B2");
              myData[myDatasize] = 0xB2;
              myDatasize ++;
              break;
            break;
          default:
            break;
          }
          Serial.print("COL: ");
          Serial.println(COLS[col]);
          Serial.print("ROW: ");
          Serial.println(ROWS[row]);
          
        }
      }
      delay(5);
      digitalWrite(ROWS[row], LOW);
    }
  }  
    //----------------------------
    //char message[] = myData;
 // Uebermitteln der Daten
  DELAY(1000);
  if (myDatasize == 0){
    sendMessage(controllerAdress,myDatasize,myData);
  }else{
    sendMessage(controllerAdress,myDatasize-1,myData);
  }
  
  
}

void myFunction2(){
  Serial.println("myFunction!");
  char myDatasize = 0;
  char* myData;
  /*
  do sth.
  */
  myData[0] = 0xAB;
 // Uebermitteln der Daten
  DELAY(1000);
  sendMessage(controllerAdress,myDatasize,myData);
}

//Spezifische Funktionen fuer Master
#elif SENDER == true
void USB(char adress,char function,volatile char* data){
  if (adress == 0x00){
    init_cnt++;
    if (init_cnt >= INIT){
      Serial2.println("INIT");
      if (DEBUG5) Serial.println("INIT");
      init_cnt = 0;
    }
  }
  if (adress == 0x00){
    list_cnt++;
    if (list_cnt >= LIST | FLAG_NEWDEVICE){
      FLAG_NEWDEVICE = false;
      Serial2.println("LIST");
      if (DEBUG5) Serial.println("LIST");
      for (int i = 1; i <= num_adresses; i++)
      {
        transmitDeviceInfo(i);
      }
      Serial2.println("END");      
      list_cnt = 0;
    }
  return;
  }
  if (data == NULL){
    Serial.println("data == NULL");
    return;
  }
  //keine Daten zu Übertragen
  if (isArrayZero(data,8)){
    Serial.println("keine Daten zu Übertragen");
    return;
  }
  Serial2.println("START");
  if (DEBUG5) Serial.println("START");
  Serial2.println(adress,HEX);
  if (DEBUG5) Serial.println(adress,HEX);
  //Serial2.println(function);
  for (int i = 0; i <= global_COF; i++)
  {
    Serial2.println(data[i],HEX);
    if (true) Serial.println(data[i],HEX);
  }
  Serial2.println("END");
  if (DEBUG5) Serial.println("END");
}

void transmitDeviceInfo(int stelle){
  if (DEBUG) Serial.println("transmitDeviceInfo");
  device geraet;
  std::advance(iterator_devices,stelle);
  geraet = *iterator_devices;
  Serial2.println(geraet.adress,HEX);
  Serial2.println(geraet.funktion,HEX);
  Serial2.println();
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
  Serial.print("device function: ");
  Serial.println(function,HEX);
  device teilnehmer(freeAdress,function,nullptr,0);
  Serial.print("device adress: ");
  Serial.println(teilnehmer.adress,HEX);  
  Serial.print("device funktion: ");
  Serial.println(teilnehmer.funktion,HEX);
  devices.push_back(teilnehmer);
  //Pollingschleife vergroessern
  num_adresses++;   
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
  device geraet = *iterator_devices;
  if (no_response) Serial.print("no_response");
  Serial.print("global_adress: ");
  Serial.println(global_adress,HEX);
  Serial.print("global_message[0]: ");
  Serial.println(global_message[0],HEX);
  Serial.print("global_message[1]: ");
  Serial.println(global_message[1],HEX);
  //neuer Teilnehmer!
  if (global_message[0] == REQUESTADRESS && global_adress ==
      controllerAdress && !no_response ) {                                                       
    if (true) Serial.println("Neuer Teilnehmer");  
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
    device geraet = *iterator_devices;
    // inkrementieren des Timeout-counters
    geraet.timeout++;                                                                             
    if (true) Serial.println("keine Antwort von:");
    if (true) Serial.println(geraet.adress,HEX);
  }
}

void polling(){
  //naechstes element der adressliste
  polling_counter++;
  // Überprüfen Sie, ob der polling_counter die Anzahl der Adressen erreicht hat
  if (polling_counter >= num_adresses + 1) {
      polling_counter = 0; // Zurücksetzen des polling_counter
  }
  //Setzen des Iterators an die entsprechende Stelle
  iterator_devices = devices.begin();
  std::advance(iterator_devices,polling_counter);
  if (DEBUG) Serial.println("polling");
  bool response = false;
  char request[] = {0xFF};
  global_reset();
  
  device geraet;
  
  geraet = *iterator_devices;
  //request fuer abzufragende Adresse
  sendMessage(geraet.adress,0,request);  
  //warten auf Rueckmeldung                                            
  response = await_response(controllerAdress);
  //Ruecksetzen des Timeouts (zudem Zuweisung von neuen Adressen)
  timeout(geraet.adress,!response);
  //Speichern der Daten
  geraet.latest_data = global_message;
  //Uebermitteln der Daten an den PC
  USB(geraet.adress,geraet.funktion,geraet.latest_data);
  
  Serial.print("num_adresses: ");
  Serial.println(num_adresses); 
  Serial.print("polling_counter: ");
  Serial.println(polling_counter); 
  

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

  pinMode(COL1, INPUT_PULLDOWN);
  pinMode(COL2, INPUT_PULLDOWN);

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

  if (DEBUG | DEBUG1 | DEBUG2 | DEBUG3 | DEBUG4){

    delay(1000);
  } 
  }
}
#elif SENDER == false
void loop() {
    if (readMessage())
    { 
    Serial.println("Nachricht empfangen!");
    Serial.print("Adresse:");
    Serial.print(global_adress,HEX);
    Serial.println();
    Serial.print("COF:");
    Serial.print(global_COF,HEX);
    Serial.println();
    Serial.print("Daten:");
    for (int i = 0; i <= global_COF; i++)
    {
      Serial.print(global_message[i],HEX);
      Serial.print(" ");
    } 
    Serial.println();
    Serial.println("Meine Adresse:");
    Serial.print(myAdress,HEX);   
    Serial.println();
      if (global_adress == myAdress && myAdress == NOADRESS){
      getAdress();
      global_reset();
      }
      else if (global_adress == myAdress && myAdress != NOADRESS)
      {
        myFunction();
      } 
  }
}
//void loop() {
//  char message[] = {0x00,FUNCTION};
//  Serial.println("message");
//  Serial.print(message[0],HEX);Serial.print(" ");Serial.println(message[1],HEX);
//  delay(1000);
//  sendMessage(0x01, 0x01, message);
//}
#endif