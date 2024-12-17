#include "header.h"

// true -> Upload fuer Master | false -> Upload fur Slave
#define SENDER true

//Spezifische Konfiguration fuer Slaves 
#if SENDER == false
  //Kommunikations Pin (MOSI) ATMEGA 15 | ATTINY 5
  #define COMM_IN 15    
  //Kommunikations Pin (MISO) ATMEGA 16 | ATTINY 6                      
  #define COMM_OUT 16                         
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

#define DELAY(x) delayMicroseconds(x)
#define SENDDELAY 1000                      
#define NOADRESS 0x00


// Zum testen!
#define DEBUG false                    
#define DEBUG1 false
#define DEBUG2 true

volatile unsigned long timeStamp;      //Zeit Merker
volatile unsigned long delayTime;      //berechnete Taktzeit
volatile char global_adress = 0x00;    //eingelesene Adresse
volatile char global_COF = 0x00;       //Groesse der Nachricht
volatile char global_message[8];       //eingelesene Daten
volatile char mask = 0x01;             //0000 0001 Binaermaske
volatile int global_adressSize = 3;    //Adressengroesse in Bits (maximal (2^adressSize)-2 Teilnehmer)
volatile int global_COFSize = 3;       //Groesse des COF Pakets
volatile char myAdress = STARTADRESS;  //Standardadresse eines neuen Busteilnehmers
volatile char controllerAdress = 0x01; //Feste Adresse des Hauptcontrollers

//Listen des Hauptcontollers-----------------------------------------------------------------------------
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

  volatile int polling_counter = 0;           //polling counter iteriert durch die liste der adressen  
  volatile int num_adresses = 0;              //anzahl der vergebenen Adressen (Adresse 0x00 fuer die Zuweisung neuer Geraete)

  std::list<device> devices;
  auto iterator_devices = devices.begin(); 
#endif


//Funktionen aller Teilnehmer-------------------------------------------------------------------------------

//receive

bool sync(){
  if (DEBUG) Serial.println("sync");
  unsigned long lok_delayTime;
  timeStamp = micros();
  while (digitalRead(COMM_IN) == HIGH);
  lok_delayTime = micros() - timeStamp;
  if (lok_delayTime > SENDDELAY*1.25 && lok_delayTime < SENDDELAY*1.75){
    delayTime = lok_delayTime/1.5;
    Serial.println(delayTime);                                            // Ich kann diese Zeile nicht loeschen?? Warum?!
    DELAY(delayTime);
    return true;
  }
  else return false;
}

char readAdress() {
  
  if (DEBUG) Serial.println("readAdress");
  char lok_adress = 0x00;
  for (int i = 0; i < global_adressSize; i++){
    if (digitalRead(COMM_IN) == HIGH)
    {
      lok_adress |= (1 << i);
    }
    DELAY(delayTime);
  }
  global_adress = lok_adress;
  return lok_adress;
}

char readCOF(){
  if (DEBUG) Serial.println("readCOF");
  char lok_COF =0x00;
  for (int i = 0; i < global_COFSize; i++){
    if (digitalRead(COMM_IN) == HIGH)
    {
      lok_COF |= (1 << i);
    }
    DELAY(delayTime);
  }
  global_COF = lok_COF;
  return lok_COF;   
}

void readData(char dataSize) {
  if (DEBUG) Serial.println("readData");
  for (int n = dataSize; n >= 0; n--){
    char message = 0x00;
    for (int i = 0; i < 8; i++){
      if (digitalRead(COMM_IN) == HIGH)
      {
        message |= (1 << i);
      }
      DELAY(delayTime);
    }
    global_message[n] = message;

  }  
}

bool readMessage(){
  //neue Nachricht auf dem Bus 
  if (sync()) {                   
      readAdress();
      readCOF();
      readData(global_COF);
      return true; 
  }
  else return false;
}
//___________________________________________________________________________________________

//send

bool busFree(){
  if (DEBUG) Serial.println("busFree");
  
  for (int i = 0; i < 4; i++)
  {
    if ((digitalRead(COMM_IN) == LOW))    //Keine Kommunikation
    {
      DELAY(delayTime);
    }
    else return false;                    //Kommunikation!
  }
  return true;
}

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
      digitalWrite(COMM_OUT, HIGH);
      if (DEBUG) Serial.print("1");
    } else {
      digitalWrite(COMM_OUT, LOW);
      if (DEBUG) Serial.print("0");
    }
    DELAY(SENDDELAY);
    adress = adress >> 1;
  }
  if (DEBUG) Serial.println();
}

void sendCOF(char dataSize){
  if (DEBUG) Serial.println("sendCOF");
  for (int i = 0; i < (global_COFSize); i++) {
    if ((dataSize & 0x01) == 0x01) {
      digitalWrite(COMM_OUT, HIGH);
      if (DEBUG) Serial.print("1");
    } else {
      digitalWrite(COMM_OUT, LOW);
      if (DEBUG) Serial.print("0");
    }
    DELAY(SENDDELAY);
    dataSize = dataSize >> 1;
  }
  if (DEBUG) Serial.println();
}

void sendData(unsigned data,char dataSize){
  if (DEBUG) Serial.println("sendData");
  for (int j = 0; j <= dataSize; j++) {
    for (int i = 0; i < 8; i++) {
      if ((data & 0x01) == 0x01) {
        digitalWrite(COMM_OUT, HIGH);
        if (DEBUG) Serial.print("1");
      } else {
        digitalWrite(COMM_OUT, LOW);
        if (DEBUG) Serial.print("0");
      }
      DELAY(SENDDELAY);
      data = data >> 1;
    }
  }
  if (DEBUG) Serial.println();
}

void sendEOF(){
  if (DEBUG) Serial.println("sendEOF");
  digitalWrite(COMM_OUT, HIGH);
  if (DEBUG) Serial.print("1");
  DELAY(SENDDELAY);
  digitalWrite(COMM_OUT, LOW);
  for (int i = 0; i < 5; i++) {                   
    if (DEBUG) Serial.print("0");                 
    DELAY(SENDDELAY);                             
  }
  if (DEBUG) Serial.println();
}

void sendMessage(char adress,char dataSize,unsigned data){
  if (DEBUG) Serial.println("sendMessage");
  //Abfrage Bus frei
  if (busFree())  {                        
    sendSOF();
    sendAdress(adress);
    sendCOF(dataSize);
    sendData(data,dataSize);
    sendEOF();
  }
}
//___________________________________________________________________________________________

//
void global_reset(){
  for (int i = 0; i < 8; i++)
  {
    global_message[i] = 0;
    global_COF = 0;
    global_adress = 0;
  }
  
}

bool await_response(char adress){
  for (int i = 0; i < 10000; i++)
  {
    if (readMessage()){
      Serial.println("Meine Adresse:");
      Serial.println(myAdress,HEX);
      if (global_adress == myAdress) return true;
      else return false;
    }
  }
  return false;
}


//___________________________________________________________________________________________
//Spezifische Funktionen fuer Slaves 
#if SENDER == false
void getAdress(){
  DELAY(delayTime*10);
  //Bitte um Adresse + 2. byte = Funktion
  sendMessage(controllerAdress,1,0x00AB); 
  //Antwort erhalten
  if (await_response(NOADRESS)){   
    //Speichern der neuen Adresse
    myAdress = global_message[0];                                     
  } 
}

void myFunction(){
  char myDatasize = 0;
  unsigned int myData = 0;
  /*
  do sth.
  */
 // Uebermitteln der Daten
  sendMessage(myAdress,myDatasize,myData);
}

//Spezifische Funktionen fuer Master
#elif SENDER == true
void USB(char adress,char function,char data){
  Serial2.println("START");
  Serial2.println(adress);
  Serial2.println(function);
  Serial2.println(data);
  Serial2.println("ENDE");
}

void printAdr(std::list<char>& Function, int stelle){
  if (DEBUG) Serial.println("printAdr");
  auto it = Function.begin(); 
  std::advance(it,stelle);
  Serial.println(*it,HEX);
}

char newAdress(){
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
  return freeAdress;                          
}
// keine Adressen mehr frei!
else return 0x00;                             
}

int whichFunction(char adresse,char data) {
  if (DEBUG) Serial.println("whichFunction");
  if (std::find(InputKeypad.begin(), InputKeypad.end(), adresse) != InputKeypad.end())
  {
    return 1;
  }
  else if (std::find(InputAudiopad.begin(), InputAudiopad.end(), adresse) != InputAudiopad.end())
  {
    return 2;
  }
  else if (adresse == 0xff)
}

void functionKeypad(volatile char[]){
  if (DEBUG) Serial.println("functionKeypad");
  
}

void functionAudiopad(volatile char[]){
  if (DEBUG) Serial.println("functionAudiopad");

}

void switchFunction(char adresse){
  if (DEBUG) Serial.println("switchFunction");
  switch (whichFunction(adresse,global_message[0]))
  {
  case 1:
    functionKeypad(global_message);
    break;
  case 2:
    functionAudiopad(global_message);
    break;
  
  default:
    break;
  }
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
  if (DEBUG2) Serial.println("eingelesene Daten:");
  if (DEBUG2) Serial.print(global_message[0],HEX);if (DEBUG2) Serial.println(global_message[1],HEX);
  if (DEBUG2) Serial.println("eingelesense Adresse");
  if (DEBUG2) Serial.println(global_adress,HEX);
  if (!no_response) Serial.println("Antwort!");
  // neuer Teilnehmer!
  if (global_message[0] == NOADRESS && global_adress == controllerAdress && !no_response ) {      
    device geraet = *iterator_devices;
    // speichern der Funktion
    geraet.funktion = global_message[1];                                                          
    if (DEBUG2) Serial.println("Neuer Teilnehmer");
    DELAY(15 * SENDDELAY);   
    // freie Adresse senden an noadress                            
    sendMessage(NOADRESS,0,newAdress());                                                           
  }
  //kein neuer Teilnehmer
  else if (no_response && adress == NOADRESS){
    if (DEBUG2) Serial.println("kein Neuer Teilnehmer");
    return;                                                                                       
  } 
  //Keine Anwort des angefragten Teilnehmers
  if (no_response){
    device geraet = *iterator_devices;
    // inkrementieren des Timeout-counters
    geraet.timeout++;                                                                             
    if (DEBUG2) Serial.println("keine Antwort von:");
    if (DEBUG2) Serial.println(adress,HEX);
  }
}

void polling(){
  global_reset();
  device geraet;
  iterator_devices = devices.begin();
  //setzen des iterators auf die abzufragende Adresse
  std::advance(iterator_Adresses,polling_counter);            
  geraet = *iterator_devices;

  char Adress;
  iterator_Adresses = Adresses.begin();
  //setzen des iterators auf die abzufragende Adresse
  std::advance(iterator_Adresses,polling_counter);            
  Adress = *iterator_Adresses;

  //request fuer abzufragende Adresse
  sendMessage(geraet.adress,0,0xFF);  
  //warten auf Rueckmeldung                                             
  if (await_response(controllerAdress)){                      
    geraet.latest_data = global_message;
    //Ruecksetzen des Timeouts (zudem Zuweisung von neuen Adressen)
    timeout(Adress,false);      
    //bearbeiten der Daten                                
    whichFunction(Adress,global_message[0]);                    
  } 
  //keine Antwort erhalten      
  else{      
    //Zaehlen der nicht vorhandenen Antworten (bei NOADRESS nicht!)                                                 
    timeout(Adress,true);                                     
  }
  //naechstes element der adressliste
  if (polling_counter < num_adresses) polling_counter++;  
  //Zuruecksetzen des counters     
  else polling_counter = 0;                                   
}


#endif

//SETUP_______________________________________________________________________________________

//Spezifisches Setup fuer Slaves 
#if SENDER == false
void setup() {
//__DON'T CHANGE!!!__________________________________________________________________________
  pinMode(COMM_IN, INPUT);
  pinMode(COMM_OUT, OUTPUT);
  if (DEBUG) Serial.println("COMM Pins are setup!");
//___________________________________________________________________________________________
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
  Adresses.push_front(NOADRESS);

  //__DON'T CHANGE!!!__________________________________________________________________________
  pinMode(COMM_IN, INPUT);
  pinMode(COMM_OUT, OUTPUT);
  if (DEBUG) Serial.println("COMM Pins are setup!");
  //___________________________________________________________________________________________
}
#endif


void loop() {
  if(SENDER){
  polling();
  if (DEBUG | DEBUG1 | DEBUG2) delay(1000);
  }
  else{
    readMessage();
    if (global_adress == myAdress){
    myFunction();
    global_reset();
    }
  }
}