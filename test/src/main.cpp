#include "header.h"

#define COMM_IN 12                   //Kommunikations Pin (MOSI) ATMEGA 15 | ATTINY 5
#define COMM_OUT 14                  //Kommunikations Pin (MISO) ATMEGA 16 | ATTINY 6
#define DELAY(x) delayMicroseconds(x)
#define DEBUG false                   //debug konsolen output
#define DEBUG1 true
#define SENDDELAY 1000

volatile unsigned long timeStamp;   //Zeit Merker
volatile unsigned long delayTime;   //berechnete Taktzeit
volatile char global_adress = 0x00;        //eingelesene Adresse
volatile char global_COF = 0x00;           //Größe der Nachricht
volatile char global_message[8];           //eingelesene Daten
char mask = 0x01;                   //0000 0001 Binärmaske
int global_adressSize = 3;                 //Adressengröße in Bits (maximal 2^adressSize Teilnehmer)

//Speichern der Adressen in die Liste mit der zugehörigen Funktion
std::list<char> InputKeypad;
std::list<char> InputAudiopad;
std::list<char> InputModule3;
std::list<char> InputModule4;
std::list<char> unusedAdresses;

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
  
}
//___________________________________________________________________________________________

void sync() {
  if (DEBUG) Serial.println("sync");
  timeStamp = micros();
  while (digitalRead(COMM_IN) == HIGH)
    ;
  delayTime = micros() - timeStamp;
}

bool readAdress() {
  if (DEBUG) Serial.println("readAdress");
  for (int i = 0; i < (global_adressSize + 1); i++) {
    if (digitalRead(COMM_IN) == HIGH) {
      global_adress = global_adress | (mask << 1);
    }
  }
  return true;
}

char readCOF(){
  if (DEBUG) Serial.println("readCOF");
  char byteCount = 0x00;
  for (int i = 0; i < 3; i++) {
    if (digitalRead(COMM_IN) == HIGH) {
      byteCount = byteCount | (mask << 1);
    }
  }
  return byteCount;  
}

void readData(char dataSize) {
  if (DEBUG) Serial.println("readData");
  for (int n = 0; n < dataSize; n++) {
    for (int i = 0; i < 3;i++){
      if (digitalRead(COMM_IN) == HIGH) {
        global_message[n] = global_message[n] | (mask << 1);
      }
    }
  }
}

void readMessage(){
  if (DEBUG) Serial.println("readMessage");
  sync();
  readAdress();
  readData(readCOF());
}
//___________________________________________________________________________________________
void sendSOF(){
  if (DEBUG) Serial.println("sendSOF");
  digitalWrite(COMM_OUT, HIGH);
  DELAY(SENDDELAY);
  digitalWrite(COMM_OUT, LOW);
  DELAY(SENDDELAY);
}

void sendAdress(char adress) {
  if (DEBUG) Serial.println("sendAdress");
  for (int i = 0; i < (global_adressSize + 1); i++) {
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
  char byteCount = (dataSize);
  for (int i = 0; i < 4; i++) {
    if ((byteCount & 0x01) == 0x01) {
      digitalWrite(COMM_OUT, HIGH);
      if (DEBUG) Serial.print("1");
    } else {
      digitalWrite(COMM_OUT, LOW);
      if (DEBUG) Serial.print("0");
    }
    DELAY(SENDDELAY);
    byteCount = byteCount >> 1;
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
  if (unusedAdresses.empty() == false)
  {
    char freeAdress = unusedAdresses.back();
    if (DEBUG) Serial.println(freeAdress,HEX);
    unusedAdresses.pop_back();

    return freeAdress;
  }

  else return 0xff;
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
  else if (adresse == 0xff) //Vergeben einer neuen Adresse
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
  switch (whichFunction(adresse))
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

void polling(){
  /* 
  -request data from **adress**
  -wait for response (tbd)
    -if response
      -request data from **adress+1** (no response counter = 0)
      -do stuff
    -else
      -no response counter + 1
  -request data from next **adress**
  ...
  ...
  -if no response counter > max(tbd)
    -delete **adress**
    

  */
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if (DEBUG) Serial.println("Loop");
  if (DEBUG) Serial.println();
  if (DEBUG) Serial.println();
  if (DEBUG) Serial.println();
  //sendMessage(0x04,2,0xAAAAAAAA);
  whichFunction(0xff,0xf2);
  if (DEBUG1)Serial.println("unusedAdresses:");
  if (DEBUG1) printList(unusedAdresses);
  if (DEBUG1)Serial.println("unusedAdresses:");
  if (DEBUG1) printList(InputModule3);


  delay(1000);
}