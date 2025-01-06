#include "header.h"

#include <sstream>
#include <iomanip>


// true -> Upload fuer Master | false -> Upload fur Slave
#define SENDER false
//Spezifische Konfiguration fuer Slaves 
#if SENDER == false
  //Kommunikations Pin (MOSI) ATMEGA 15 | ATTINY 5
  #define COMM_IN 12    
  //Kommunikations Pin (MISO) ATMEGA 16 | ATTINY 6                      
  #define COMM_OUT 14                         
  //0xAB := Tastatur, 0xAC := ...
  #define FUNCTION  0xAB
  #define STARTADRESS 0x00
//Spezifische Konfiguration fuer Master
#elif SENDER == true
  //Kommunikations Pin
  #define COMM_IN 12
  //Kommunikations Pin                          
  #define COMM_OUT 14                         
  //Adresse des Hauptmoduls
  #define STARTADRESS 0x01
#endif
#define BITSTUFFING 3

#define DELAY(x) delayMicroseconds(x)
#define SENDDELAY 1000                      
#define NOADRESS 0x00

// Zum testen!
#define DEBUG false                    
#define DEBUG1 false
#define DEBUG2 false
#define DEBUG3 true
#define DEBUG4 true

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

char* global_decoded_message; //Bitstuffed message
char* global_BITSTUFFED_message; //Bitstuffed message
char* global_BITSTUFFED_message_recv; //Bitstuffed message empfangen
char* global_decoded_message_recv; //decodierte message empfangen
//Listen des Hauptcontollers------------------------------
//Speichern der Adressen in die Liste mit der zugehoerigen Funktion

//Spezifische Konfiguration fuer Master
#if SENDER == true
  std::list<char> InputKeypad;
  auto iterator_InputKeypad = InputKeypad.begin();
  std::list<char> InputAudiopad;
  auto iterator_InputAudiopad = InputAudiopad.begin();
  std::list<char> InputModule3;
  auto iterator_InputModule3 = InputModule3.begin();
  std::list<char> InputModule4;
  auto iterator_InputModule4 = InputModule4.begin();
  //Freie Adressen
  std::list<char> unusedAdresses;
  auto iterator_unusedAdresses = unusedAdresses.begin();

  //Adressen aller Teilnehmer
  std::list<char> Adresses;
  auto iterator_Adresses = Adresses.begin();
  //polling counter iteriert durch die liste der adressen 
  volatile int polling_counter = 0; 
  //Anzahl der vergebenen Adressen
  volatile int num_adresses = 0;              
  std::list<device> devices;
  auto iterator_devices = devices.begin(); 
#endif


//Funktionen aller Teilnehmer-------------------------------

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
bool sync(){
  if (DEBUG) Serial.println("sync");
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
    if (DEBUG) Serial.println("readMessage");
    // neue Nachricht auf dem Bus
    if (sync()) {
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

void sendData(unsigned data,char dataSize){
  if (DEBUG) Serial.println("sendData");
  for (int j = 0; j <= dataSize; j++) {
    for (int i = 0; i < 8; i++) {
      if ((data & 0x01) == 0x01) {
        appendChar(global_decoded_message, '1');
        if (DEBUG) Serial.print("1");
      } else {
        appendChar(global_decoded_message, '0');
        if (DEBUG) Serial.print("0");
      }
      data = data >> 1;
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

void sendMessage(char adress,char dataSize,unsigned data){
  if (DEBUG) Serial.println("sendMessage");
    sendAdress(adress);
    sendCOF(dataSize);
    sendData(data,dataSize);
    if (DEBUG3) Serial.println("Message:");
    if (DEBUG3) Serial.println((global_decoded_message));
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
  sendMessage(controllerAdress,1,0x00AB); 
  //Antwort erhalten
  if (await_response(NOADRESS)){   
    //Speichern der neuen Adresse
    myAdress = global_message[0];
    if (DEBUG3) Serial.print("Adresse Empfangen!");
    if (DEBUG3) Serial.print(myAdress,HEX);
    if (DEBUG3) Serial.println();
  } 
}

void myFunction(){
  Serial.println("myFunction!");
  char myDatasize = 0;
  unsigned int myData = 0;
  /*
  do sth.
  */
  myData = 0xAB;
 // Uebermitteln der Daten
  DELAY(1000);
  sendMessage(controllerAdress,myDatasize,myData);
}

//Spezifische Funktionen fuer Master
#elif SENDER == true
void USB(char adress,char function,char* data){
  Serial2.println("START");
  Serial2.println(adress);
  //Serial2.println(function);
  Serial2.println(data);
  Serial2.println("ENDE");
}

void printInfo(int stelle){
  if (DEBUG) Serial.println("printInfo");
  // device geraet;
  // std::advance(iterator_devices,stelle);
  // geraet = *iterator_devices;
  // Serial.println("Adresse");
  // Serial.println(geraet.adress,HEX);
  // Serial.println("Funktion");
  // Serial.println(geraet.funktion,HEX);
  // Serial.println("");
  // Serial.println(geraet.);
  // Serial.println("");
  // Serial.println(geraet.adress);
}

char newAdress(){
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
  device teilnehmer = device(freeAdress,0,0,0);
  devices.push_back(teilnehmer);
  //Pollingschleife vergroessern
  num_adresses++;   

  if (DEBUG4) Serial.println("Neue Adresse:");
  if (DEBUG4) Serial.println(freeAdress,HEX);                          
  return freeAdress;                          
}
// keine Adressen mehr frei!
else return 0x00;                             
}
int whichFunction(char adresse,char data) {
  if (DEBUG) Serial.println("whichFunction");
  
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
  // if (DEBUG2) Serial.println("eingelesene Daten:");
  // if (DEBUG2) Serial.print(global_message[0],HEX);
  // if (DEBUG2) Serial.println(global_message[1],HEX);
  //if (!no_response) Serial.println("Antwort!");
  // neuer Teilnehmer!
  if (global_message[0] == NOADRESS && global_adress ==
      controllerAdress && !no_response ) {      
    device geraet = *iterator_devices;
    // Speichern der Funktion
    geraet.funktion = global_message[1];                                                          
    if (DEBUG4) Serial.println("Neuer Teilnehmer");  
    // freie Adresse senden an noadress                            
    sendMessage(NOADRESS,0,newAdress());                                                           
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
    if (DEBUG2) Serial.println("keine Antwort von:");
    if (DEBUG2) Serial.println(geraet.adress,HEX);
  }
}

void polling(){
  bool response = false;
  global_reset();
  
  device geraet;
  
  geraet = *iterator_devices;


  //request fuer abzufragende Adresse
  sendMessage(geraet.adress,0,0xFF);  
  //warten auf Rueckmeldung                                            
  if (await_response(controllerAdress)){ 
    //Serial.println("Antwort!!!");                     
    geraet.latest_data = global_message;
    //Ruecksetzen des Timeouts (zudem Zuweisung von neuen Adressen)
    timeout(geraet.adress,false);      
    //bearbeiten der Daten                                
    whichFunction(geraet.adress,global_message[0]);                    
  } 
  //keine Antwort erhalten      
  else{
    if (DEBUG3) Serial.println("FEHLER!!!");      
    //Zaehlen der nicht vorhandenen Antworten (bei NOADRESS nicht!)                                                 
    timeout(geraet.adress,true);                                     
  }
  //naechstes element der adressliste
  if (polling_counter < num_adresses) polling_counter++;  
  //Zuruecksetzen des counters     
  else polling_counter = 0;
  //Setzen des Iterators an die entsprechende Stelle
  iterator_devices = devices.begin();
  std::advance(iterator_devices,polling_counter);                                   
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
#endif