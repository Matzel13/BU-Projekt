#include "header.h"

#define COMM_IN 12                          //Kommunikations Pin (MOSI) ATMEGA 15 | ATTINY 5
#define COMM_OUT 14                         //Kommunikations Pin (MISO) ATMEGA 16 | ATTINY 6
#define DELAY(x) delayMicroseconds(x)
#define SENDDELAY 1000                       //geht bis runter auf 200!
#define NOADRESS 0x00

// Zum testen!
#define SENDER true
#define DEBUG false                         //debug konsolen output
#define DEBUG1 false
#define DEBUG2 true

volatile unsigned long timeStamp;           //Zeit Merker
volatile unsigned long delayTime;           //berechnete Taktzeit
volatile char global_adress;                //eingelesene Adresse
volatile char global_COF = 0x00;            //Größe der Nachricht
volatile char global_message[8];            //eingelesene Daten
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ hier vielleicht auf uint_64 umsteigen? Überlegung für CRC
// alternativ: encodede message als uint64t speichern:
volatile uint64_t encodedMessage;
volatile int global_adressSize = 3;         //Adressengröße in Bits (maximal (2^adressSize)-2 Teilnehmer)
volatile int global_COFSize = 3;            //größe des COF Pakets
volatile char myAdress = NOADRESS;          //Standardadresse eines neuen Busteilnehmers
volatile char controllerAdress = 0x01;      //Feste Adresse des Hauptcontrollers

char mask = 0x01;                           //0000 0001 Binärmaske
uint16_t generator_polynomial = 0x1021;     // CRC-16-CCITT Generatorpolynom für CRC

//Listen des Hauptcontollers-----------------------------------------------------------------------------
//Speichern der Adressen in die Liste mit der zugehörigen Funktion
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
volatile int num_adresses = 0;              //anzahl der vergebenen Adressen (Adresse 0x00 für die Zuweisung neuer Geräte)

//------------------------------------------------------------------------------------------------------


bool sync(){
  if (DEBUG) Serial.println("sync");
  unsigned long lok_delayTime;
  timeStamp = micros();
  while (digitalRead(COMM_IN) == HIGH);
  lok_delayTime = micros() - timeStamp;
  if (lok_delayTime > SENDDELAY*1.25 && lok_delayTime < SENDDELAY*1.75){
    delayTime = lok_delayTime/1.5;
    Serial.println(delayTime); // Ich kann diese Zeile nicht löschen?? Warum?!
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
      lok_adress = lok_adress + pow(2,i);
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
      lok_COF = lok_COF + pow(2,i);
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
        message = message + pow(2,i);
      }
      DELAY(delayTime);
    }
    //global_message[dataSize-(n+1)] = message;
    global_message[n] = message;

  }  
}

bool readMessage(){
  if (sync()) {                   //neue Nachricht auf dem Bus 
      readAdress();
      readCOF();
      readData(global_COF);       //geht auch global?
      Serial.println("Meine Adresse:DATA");
      Serial.println(myAdress,HEX); 
      //Serial.println(); 
      return true; 
  }
  else return false;
}

// TODO: check if this code actually works as intended
void encodeMessage(){
  uint64_t remainder;

  for(int i = 0; i < 8; i++){
    encodedMessage = (encodedMessage << 8) | (uint8_t)global_message[i]; // merge the message into a single variable
  }

  remainder = encodedMessage << 15;                // append as many zeroes as the degree of the polynomial - 1
  // divide the message with the generator polynomial
  for(int i = 15; i > 0; i--){
    // check for leading 0 -> no XOR while a leading 0
    if((remainder << i+1) != 0){
      remainder = remainder ^ (generator_polynomial << i);
    }
  }
  // append remainder to message
  encodedMessage = (encodedMessage << 15) | remainder   // should only append the remainder after the division
}

//___________________________________________________________________________________________
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


// TODO: check if the code works
void sendData(unsigned data,char dataSize){
  if (DEBUG) Serial.println("sendData");
    for (int i = 0; i < 64; i++) {
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
  if (DEBUG) Serial.println();
}

void sendEOF(){
  if (DEBUG) Serial.println("sendEOF");
  digitalWrite(COMM_OUT, HIGH);
  if (DEBUG) Serial.print("1");
  DELAY(SENDDELAY);
  digitalWrite(COMM_OUT, LOW);
  for (int i = 0; i < 5; i++) {                   /// brauchen wir das???
    if (DEBUG) Serial.print("0");                 /// brauchen wir das???
    DELAY(SENDDELAY);                             /// brauchen wir das???
  }
  if (DEBUG) Serial.println();
}

void sendMessage(char adress,char dataSize,unsigned data){
  if (DEBUG) Serial.println("sendMessage");
  sendSOF();
  sendAdress(adress);
  sendCOF(dataSize);
  encodeMessage();
  sendData(encodedMessage,dataSize);
  sendEOF();
}

// TODO: transform the message back into its seperate parts; check if this code works
void decodeMessage(){
  uint64_t remainder;
  uint64_t message;           // message received

  for(int i = 15; i >= 0; i--){
    // check for leading 0 -> no XOR while a leading 0
    if((remainder << i+1) != 0){
      remainder = remainder ^ (generator_polynomial << i);
    }
  }

  for(int i = 0; i < 17; i++){
    if((remainder & (0x01 << i)) == 0x01){
      // ERROR in the message, ignore this message
      break;
    } 
  }
}

//___________________________________________________________________________________________
void USB(){

}


void printAdr(std::list<char>& Function, int stelle){
  if (DEBUG) Serial.println("printAdr");
  auto it = Function.begin(); 
  std::advance(it,stelle);
  Serial.println(*it,HEX);
}

char newAdress(){
  if (unusedAdresses.empty() == false)          //es sind noch adressen frei
  {
    char freeAdress = unusedAdresses.back();    //nehme eine freie Adresse
    if (DEBUG) Serial.println(freeAdress,HEX);  
    unusedAdresses.pop_back();                  //entferne diese aus 'unused Adresses'
    Adresses.push_back(freeAdress);             //füge Sie zur Pollingliste hinzu
    num_adresses++;                             //Pollingschleife vergrößern
    return freeAdress;                          
  }
  else return 0x00;                             // keine Adressen mehr frei!
}
// put function definitions here:
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
  else if (adresse == 0xff) //Vergeben einer neuen Adresse !! FALSCH !!
  {
    char temp_adresse = newAdress();
    sendMessage(0xff,0x00,temp_adresse);
    if (temp_adresse != 0xff){
      switch (data)
      {
      case 0xf0:
        InputKeypad.push_back(temp_adresse);
        break;
      case 0xf1:
        InputAudiopad.push_back(temp_adresse);
        break;
      case 0xf2:
        InputModule3.push_back(temp_adresse);
        break;
      case 0xf3:
        InputModule4.push_back(temp_adresse);
        break;
      default:
        break;
      }
    }
    return 10;
  }
  else return 0;
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
    // Überprüfen, ob die Liste leer ist
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

bool await_response(char adress){
  for (int i = 0; i < 10000; i++)// vllt noch kleiner!
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

void timeout(char adress, bool no_response){
  if (DEBUG2) Serial.println("eingelesene Daten:");
  if (DEBUG2) Serial.print(global_message[0],HEX);if (DEBUG2) Serial.println(global_message[1],HEX);
  if (DEBUG2) Serial.println("eingelesense Adresse");
  if (DEBUG2) Serial.println(global_adress,HEX);
  if (!no_response) Serial.println("Antwort!");
  if (global_message[0] == NOADRESS && global_adress == controllerAdress && !no_response ) {     // neuer Teilnehmer!
    if (DEBUG2) Serial.println("Neuer Teilnehmer");
    DELAY(15 * SENDDELAY);                               
    sendMessage(NOADRESS,0,newAdress());                                                // freie Adresse senden an noadress 
  }
  else if (no_response && adress == NOADRESS){
    if (DEBUG2) Serial.println("kein Neuer Teilnehmer");
    return;                                             //kein neuer Teilnehmer
  } 

  if (no_response){
    // wie oft? 
    // wenn zu oft, dann aus den Listen kicken! 
    if (DEBUG2) Serial.println("keine Antwort von:");
    if (DEBUG2) Serial.println(adress,HEX);
  }
}

void polling(){
  char Adress;
  /* 
  -request data from **adress**
  -wait for response (tbd)
    -if response
      -do stuff
      -(no response counter (timeout) = 0)
    -else
      -no response counter + 1
  -request data from next **adress**
  ...
  ...
  -if no response counter (timeout) > max(tbd)
    -delete **adress**
  */
  iterator_Adresses = Adresses.begin();
  std::advance(iterator_Adresses,polling_counter);            //setzen des iterators auf die abzufragende Adresse
  Adress = *iterator_Adresses;
  sendMessage(Adress,0,0xFF);                                 //request für abzufragende Adresse                     
  if (await_response(controllerAdress)){                      //warten auf Rückmeldung
    timeout(Adress,false);                                    //Rücksetzen des Timeouts (zudem Zuweisung von neuen Adressen)
    whichFunction(Adress,global_message[0]);                  //bearbeiten der Daten    
  }       
  else{                                                       //keine Antwort erhalten
    timeout(Adress,true);                                     //Zählen der nicht vorhandnen Antworten (bei NOADRESS nicht!)
  }
  if (polling_counter < num_adresses) polling_counter++;       //nächstes element der adressliste
  else polling_counter = 0;                                   //Zurücksetzen des counters
}

//___________________________________________________________________________________________
void setup() {
  Serial.begin(115200);
  if (DEBUG) Serial.println("serial is setup!");
  
  //__DON'T CHANGE!!!__________________________________________________________________________
  pinMode(COMM_IN, INPUT);
  pinMode(COMM_OUT, OUTPUT);
  if (DEBUG) Serial.println("COMM Pins are setup!");
  //___________________________________________________________________________________________
  //freie Adressen
  
  // Elemente hinzufügen
  //Adresse 0x01 für den Controller reserviert!
  for (int i = 2; i < pow(2,global_adressSize); i++)
  {
    unusedAdresses.push_back(i);
    if (DEBUG2) Serial.println(unusedAdresses.back(),HEX);
  }
  Adresses.push_front(NOADRESS);
  //num_adresses++;
  if (SENDER) myAdress = controllerAdress;
}
//___________________________________________________________________________________________

void loop() {
  
  if(SENDER){
  polling();
  //zurücksetzen aller eingelesenen Werte!
  delay(1000);
  }
  else{
    //Serial.println("Meine Adresse:");
    //Serial.println(myAdress,HEX); 
    if (readMessage()){                       
      Serial.println("Meine Adresse:");
      Serial.println(myAdress,HEX); 

      Serial.println("eingelesense Adresse");
      Serial.println(global_adress,HEX);

      Serial.println("eingelesenses COF");
      Serial.println(global_COF,HEX);

      Serial.println("Daten:");
      Serial.println(global_message[0],HEX);
      
      if (myAdress == NOADRESS && global_adress == NOADRESS && global_message[0] == 0xff){       //noch keine Adresse zugewiesen!
        Serial.println("anfrage nach Adresse");
        Serial.println(myAdress,HEX);
        DELAY(delayTime*10);
        sendMessage(controllerAdress,0,NOADRESS);                           //Bitte um Adresse
        if (await_response(NOADRESS)){
          myAdress = global_message[0];                                     //Speichern der neuen Adresse
          Serial.println("Meine Adresse:");
          Serial.println(myAdress,HEX);
        } 
      }
      else if(myAdress != NOADRESS && global_adress == myAdress)
      {
        DELAY(delayTime*10);
        sendMessage(controllerAdress,1,0xAABB);
      //Serial.println(myAdress,HEX); 
      }
    }
  }

  if (DEBUG1) whichFunction(0xff,0xf2);
  if (DEBUG1)Serial.println("unusedAdresses:");
  if (DEBUG1) printList(unusedAdresses);
  if (DEBUG1)Serial.println("Adresses:");
  if (DEBUG1) printList(InputModule3);
}