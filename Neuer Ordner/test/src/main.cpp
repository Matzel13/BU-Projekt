#include "header.h"

#define COMM_IN 12                          //Kommunikations Pin (MOSI) ATMEGA 15 | ATTINY 5
#define COMM_OUT 14                         //Kommunikations Pin (MISO) ATMEGA 16 | ATTINY 6
#define DELAY(x) delayMicroseconds(x)
#define DEBUG false                         //debug konsolen output
#define DEBUG1 false
#define SENDDELAY 1000                       //bis runter auf 200!
#define NOADRESS 0x00

// Zum testen!
#define SENDER false

volatile int counter = 0;
volatile unsigned long timeStamp;           //Zeit Merker
volatile unsigned long delayTime;           //berechnete Taktzeit
volatile char global_adress;                //eingelesene Adresse
volatile char global_COF = 0x00;            //Größe der Nachricht
volatile char global_message[8];            //eingelesene Daten
volatile char mask = 0x01;                           //0000 0001 Binärmaske
volatile int global_adressSize = 4;                  //Adressengröße in Bits (maximal 2^adressSize Teilnehmer)
volatile int global_COFSize = 4;
volatile char myAdress = 0x00;
volatile char controllerAdress = 0x01;
//volatile char free_adress;

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
//genutze Adressen
std::list<char> Adresses;
auto iterator_Adresses = Adresses.begin();

volatile int num_adresses = 0;




bool sync() {
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
  char adress =0x00;
  for (int i = 0; i < global_adressSize; i++){
    if (digitalRead(COMM_IN) == HIGH)
    {
      adress = adress + pow(2,i);
    }
    DELAY(delayTime);
  }
  global_adress = adress;
  return adress;
}

char readCOF(){
  if (DEBUG) Serial.println("readCOF");
  char COF =0x00;
  for (int i = 0; i < global_COFSize; i++){
    if (digitalRead(COMM_IN) == HIGH)
    {
      COF = COF + pow(2,i);
    }
    DELAY(delayTime);
  }
  return COF;   
}

void readData(char dataSize) {
  if (DEBUG) Serial.println("readData");
  for (int n = 0; n < dataSize; n++){
    char message = 0x00;
    for (int i = 0; i < 8; i++){
      if (digitalRead(COMM_IN) == HIGH)
      {
        message = message + pow(2,i);
      }
      DELAY(delayTime);
    }
    global_message[dataSize-(n+1)] = message;
  }  
}

bool readMessage(){
  if (sync()) {
      readAdress();
      
      char size = readCOF();
      
      readData(size);
      //for (int i = 0; i < size; i++)
      //{
      //  Serial.print(global_message[i],HEX);
      //}
      Serial.println(); 
      return true; 
  }
  else return false;
}
//___________________________________________________________________________________________
void sendSOF(){
  if (DEBUG) Serial.println("sendSOF");
  digitalWrite(COMM_OUT, HIGH); //111
  DELAY(SENDDELAY*1.5);
  digitalWrite(COMM_OUT, LOW);  //0
  DELAY(SENDDELAY);
}

void sendAdress(char adress) {//0100
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

void sendCOF(char dataSize){//0001
  if (DEBUG) Serial.println("sendCOF");
  for (int i = 0; i < 4; i++) {
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
  for (int j = 0; j < dataSize; j++) {
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

void sendEOF(){// 2,5 *  delay --> sichere erkennung!!!
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
  sendData(data,dataSize);
  sendEOF();
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
    if (DEBUG) Serial.println(freeAdress,HEX);  //
    unusedAdresses.pop_back();                  //entferne diese aus 'unused Adresses'
    Adresses.push_back(freeAdress);             //füge Sie zur pollingliste hinzu
    num_adresses++;
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
    sendMessage(0xff,0x01,temp_adresse);
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
      if (adress == global_adress) return true;
      else return false;
    }
  }
  return false;
}

void timeout(char adress,bool no_response){
  if (adress == NOADRESS && !no_response) {             // neuer Teilnehmer!
    Serial.println("Neuer Teilnehmer");
    DELAY(15 * SENDDELAY);                               
    sendMessage(NOADRESS,1,newAdress());                // freie Adresse senden
    
  }
  else if (adress == NOADRESS && no_response){
    Serial.println("kein Neuer Teilnehmer");
    return;                                             //kein neuer Teilnehmer
  } 

  if (no_response){
    // wie oft? 
    // wenn zu oft, dann aus den Listen kicken! 
    Serial.println("keine Antwort von:");
    Serial.println(adress,HEX);
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
  std::advance(iterator_Adresses,counter);            //setzen des iterators auf die abzufragende Adresse
  Adress = *iterator_Adresses;
  sendMessage(Adress,1,0xFF);                         //request für abzufragende Adresse                     
  if (await_response(Adress)){                        //warten auf Rückmeldung
    timeout(Adress,false);                            //Rücksetzen des Timeouts (zudem Zuweisung von neuen Adressen)
    whichFunction(Adress,global_message[0]);          //bearbeiten der Daten    
  }
  else{                                               //keine Antwort erhalten
    timeout(Adress,true);                             //Zählen der nicht vorhandnen Antworten (bei NOADRESS nicht!)
  }
  if (counter < num_adresses) counter++;              //nächstes element der adressliste
  else counter = 0;                                   //Zurücksetzen des counters
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
  for (int i = 1; i <= 0x0F; i++)
  {
    unusedAdresses.push_back(i);
    if (DEBUG) Serial.println(unusedAdresses.back(),HEX);
  }
  Adresses.push_back(NOADRESS);  
}
//___________________________________________________________________________________________

void loop() {
  
  if(SENDER){
  polling();
  delay(1000);
  }
  else{
    if (readMessage()){                       
      //Serial.println("Meine Adresse:");
      //Serial.println(myAdress,HEX); 
      //Serial.println("zugewiesene Adresse:");
      //Serial.println(global_message[0],HEX);
      //Serial.println("eingelesense Adresse");
      //Serial.println(global_adress,HEX);
      if (myAdress == NOADRESS && global_adress == NOADRESS){       //noch keine Adresse zugewiesen!
        Serial.println("anfrage nach Adresse");
        Serial.println(myAdress,HEX);
        DELAY(delayTime*10);
        sendMessage(NOADRESS,1,NOADRESS);                           //Bitte um Adresse
        if (await_response(NOADRESS)){
          myAdress = global_message[0];                             //Speichern der neuen Adresse
          Serial.println("Meine Adresse:");
          Serial.println(myAdress,HEX);
        } 
      }
      else if(myAdress != NOADRESS && global_adress == myAdress)
      {
        DELAY(delayTime*10);
        sendMessage(controllerAdress,1,0xaa);
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