/*

*/
#define COMM_IN 0   //Kommunikations Pin (MOSI) ATMEGA 15 | ATTINY 5
#define COMM_OUT 1  //Kommunikations Pin (MISO) ATMEGA 16 | ATTINY 6
#define DELAY(x) delayMicroseconds(x)


#define LED_1 2  //LED Output Pin
#define LED_2 3  //LED Output Pin
#define LED_3 4  //LED Output Pin
#define LED_4 5  //LED Output Pin
#define LED_5 6  //LED Output Pin
//...
//__DON'T CHANGE!!!__________________________________________________________________________
volatile unsigned long timeStamp;  //Zeit Merker
volatile unsigned long delayTime;  //berechnete Taktzeit
volatile char adress = 0x00;       //eingelesene Adresse
volatile char data[8];             //eingelesene Daten
char myAdress = 0x00;              //Speicherplatz für Slaveadresse
char mask = 0x01;                  //0000 0001 Binärmaske
int adressSize = 3;                //Adressengröße in Bits (maximal 2^adressSize Teilnehmer)
int dataSize = 1;                  //Größe der Daten in Byte
//___________________________________________________________________________________________

char deviceID = 0x00;  //Funktionsbeschreibung


void setup() {
  //__DON'T CHANGE!!!__________________________________________________________________________
  pinMode(COMM_IN, INPUT);
  pinMode(COMM_OUT, OUTPUT);
  //___________________________________________________________________________________________
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
  pinMode(LED_5, OUTPUT);
  //....
};
/*
  calculate delay 
*/
void sync() {
  timeStamp = micros();
  while (digitalRead(COMM_IN) == HIGH)
    ;
  delayTime = micros() - timeStamp;
}
/*
@returns if the read Adress == myAdress
*/
bool readAdress() {
  for (int i = 0; i < (adressSize + 1); i++) {
    if (digitalRead(COMM_IN) == HIGH) {
      adress = adress | (mask << 1);
    }
  }
  return (adress == myAdress);
}
/*

*/
void readData(int dataSize) {
  for (int n = 0; n < dataSize; n++) {
    for (int i = 0; i < 3;i++){
      if (digitalRead(COMM_IN) == HIGH) {
        data[n] = data[n] | (mask << 1);
      }
    }
  }
}
/*
not done yet
*/
bool CRC(){
  return true;
}
/*
listen to SOF
*/
void readMessage() {
  //SOF
  if (digitalRead(COMM_IN == HIGH)) {
    //sync via SOF
    sync();
  
  // wait 2 Bits
  DELAY(delayTime * 2);
  //Adresse Lesen
  if (readAdress() == false) {
    return;
  }
  readData(dataSize);
  CRC();
  }
}
/*
Not done yet
*/
void sendMessage(){
}
/*
custom code for each module
*/
void custom() {
  if ((data[0] | 0x01) == 0x01) {
    digitalWrite(LED_1, HIGH);
  } else digitalWrite(LED_1, LOW);
  if ((data[0] | 0x02) == 0x02) {
    digitalWrite(LED_2, HIGH);
  } else digitalWrite(LED_2, LOW);
  if ((data[0] | 0x04) == 0x04) {
    digitalWrite(LED_3, HIGH);
  } else digitalWrite(LED_3, LOW);
  if ((data[0] | 0x08) == 0x08) {
    digitalWrite(LED_4, HIGH);
  } else digitalWrite(LED_4, LOW);
  if ((data[0] | 0x10) == 0x10) {
    digitalWrite(LED_5, HIGH);
  } else digitalWrite(LED_5, LOW);
}


//___________________________________________________________________________________________



void loop(){
  readMessage();
  custom();
}