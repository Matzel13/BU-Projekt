#include <Arduino.h>
#include <string.h>
  //Kommunikations Pin (MOSI) ATMEGA 15 | ATTINY 5
  #define COMM_IN 9    
  //Kommunikations Pin (MISO) ATMEGA 16 | ATTINY 6                      
  #define COMM_OUT 10                         
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

#define BITSTUFFING 3
#define DELAY(x) delayMicroseconds(x)
#define SENDDELAY 1000                       
#define NOADRESS 0x00
#define WAITLOOP 20000

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
//Variablen--------------------------------
volatile unsigned long timeStamp;      //Zeit Merker
volatile unsigned long delayTime;      //berechnete Taktzeit
volatile unsigned char global_adress = 0x00;    //eingelesene Adresse
volatile unsigned char global_COF = 0x00;       //Groesse der Nachricht
volatile unsigned char global_message[8];       //eingelesene Daten
volatile unsigned char mask = 0x01;             //0000 0001 Binaermaske
//Adressengroesse in Bits (maximal (2^adressSize)-2 Teilnehmer)
volatile int global_adressSize = 3;    
volatile int global_COFSize = 3;       //Groesse des COF Pakets
//Standardadresse eines neuen Busteilnehmers
volatile unsigned char myAdress = STARTADRESS;  
//Feste Adresse des Hauptcontrollers
volatile unsigned char controllerAdress = 0x01; 
//Zaehler fuer die Initialisierung
volatile int init_cnt = 100;
volatile int list_cnt = 100;
unsigned char* global_decoded_message; //Bitstuffed message
unsigned char* global_BITSTUFFED_message; //Bitstuffed message
unsigned char* global_BITSTUFFED_message_recv; //Bitstuffed message empfangen
unsigned char* global_decoded_message_recv; //decodierte message empfangen

int length_BITSTUFFED_message;
int eofCounter = 0;
//Listen des Hauptcontollers------------------------------
//Speichern der Adressen in die Liste mit der zugehoerigen Funktion


//allg. Funktionen -------------------------------
void appendChar(unsigned char* &str, unsigned char c) {
    if (str == NULL) {
        // Wenn str null ist, initialisieren Sie es mit dem neuen Zeichen
        str = new unsigned char[2]; // 1 Zeichen + Nullterminator
        str[0] = c;
        str[1] = '\0';
    } else {
        size_t len = strlen(str);
        unsigned char* newStr = new unsigned char[len + 2]; // +1 für das neue Zeichen, +1 für den Nullterminator
        strcpy(newStr, str);
        newStr[len] = c;
        newStr[len + 1] = '\0';
        delete[] str;
        str = newStr;
    }
}

//receive
bool syncronisation(){
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
void decodeBitstuffedMessage(const unsigned char* stuffedMessage, unsigned char* &decodedMessage, int x) {
    int count;
    unsigned char lastBit = '\0';
    size_t len = strlen(stuffedMessage);
    decodedMessage = new unsigned char[len + 1]; // +1 für den Nullterminator
    size_t decodedIndex = 0;
    count = 1; // Zähler auf 1 setzen
    
    // Durchlaufen der bitgestopften Nachricht
    for (size_t i = 0; i < len; ++i) {
        unsigned char currentBit = stuffedMessage[i];
        
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
}



void parseDecodedMessage(const unsigned char* decodedMessage) {
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
        unsigned char message[8] = {0};
        for (int j = 0; j < 8; j++) {
            if (decodedMessage[global_adressSize + global_COFSize + i * 8 + j] == '1') {
                message[i] |= (1 << j);
            }
            
          }
        global_message[dataSize-1-i] = message[i];
    }
}



bool readMessage() {
    // neue Nachricht auf dem Bus
    if (syncronisation()) {
        // Initialisieren Sie die globale bitgestopfte Nachricht
        global_BITSTUFFED_message_recv = new unsigned char[1];
        global_BITSTUFFED_message_recv[0] = '\0';
        // Lesen Sie die Nachricht bitweise ein
        
        while (true) {
            unsigned char bit = (digitalRead(COMM_IN) == HIGH) ? '1' : '0';
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
        // Dekodieren Sie die bitgestopfte Nachricht
        unsigned char* decodedMessage = NULL;
        decodeBitstuffedMessage(global_BITSTUFFED_message_recv, decodedMessage, BITSTUFFING);
        global_decoded_message_recv = decodedMessage;
        // Verarbeiten Sie die decodierte Nachricht
        parseDecodedMessage(global_decoded_message_recv);
        delete[] decodedMessage; // Speicher freigeben
        return true;
    } else {
        return false;
    }
}




//------------------------------

//send

bool awaitBusFree(){
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
  digitalWrite(COMM_OUT, HIGH);
  DELAY(SENDDELAY*1.5);
  digitalWrite(COMM_OUT, LOW);
  DELAY(SENDDELAY);
}


void sendAdress(unsigned char adress) {
  for (int i = 0; i < (global_adressSize); i++) {
    if ((adress & 0x01) == 0x01) {
      appendChar(global_decoded_message, '1');
      //digitalWrite(COMM_OUT, HIGH);
    } else {
      //digitalWrite(COMM_OUT, LOW);
      appendChar(global_decoded_message, '0');
    }
    adress = adress >> 1;
  }
}
void sendAdress2(unsigned char adress) {
  for (int i = global_adressSize -1; i >=0; i--) {
    if ((adress & 0x01) == 0x01) {
      appendChar(global_decoded_message, '1');
      //digitalWrite(COMM_OUT, HIGH);
    } else {
      //digitalWrite(COMM_OUT, LOW);
      appendChar(global_decoded_message, '0');
    }
    adress = adress >> 1;
  }
}


void sendCOF(unsigned char dataSize){
  for (int i = 0; i < (global_COFSize); i++) {
    if ((dataSize & 0x01) == 0x01) {
      appendChar(global_decoded_message, '1');
    } else {
      appendChar(global_decoded_message, '0');
    }
    dataSize = dataSize >> 1;
  }
}

void sendData(unsigned char dataSize,unsigned char* data) {
    for (int i = dataSize; i >= 0; i--)
    {
      for (int j = 0; j < 8; j++)
      {
        if ((data[i] & 0x01) == 0x01) {
          appendChar(global_decoded_message, '1');
        } else {
          appendChar(global_decoded_message, '0');
        }
        data[i] = data[i] >> 1;
      }
      
    }
}
//Bleibt gleich
void sendEOF(){
  digitalWrite(COMM_OUT, HIGH);
  DELAY(SENDDELAY);
  digitalWrite(COMM_OUT, LOW);
  for (int i = 0; i < 5; i++) {   //5 mal?                                
    DELAY(SENDDELAY);                             
  }
}
int writeBitstuffedMessage(int x) {
    String stuffedMessage;
    int count = 0;

    unsigned char lastBit = '\0';

    for (int i = 0; i < strlen(global_decoded_message); i++) {

        unsigned char currentBit = global_decoded_message[i];
        stuffedMessage += currentBit;

        if (currentBit == lastBit) {
            count++;
            if (count == x) { // Wenn x gleiche Bits gefunden wurden
                // Bitwechsel einfügen
                unsigned char stuffedBit = (currentBit == '1') ? '0' : '1';
                stuffedMessage += stuffedBit;

                count = 1; // Zähler zurücksetzen
                switch (currentBit)
                {
                case '0': lastBit = '1';
                  break;
                case '1': lastBit = '0';
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
    length_BITSTUFFED_message = stuffedMessage.length() + 1;
    global_BITSTUFFED_message = new unsigned char[length_BITSTUFFED_message];
    strcpy(global_BITSTUFFED_message, stuffedMessage.c_str());
    return length_BITSTUFFED_message;
 

}

void sendMessage(unsigned char adress,unsigned char dataSize,unsigned char* data){
    sendAdress(adress);
    sendCOF(dataSize);
    sendData(dataSize,data);
    int len = writeBitstuffedMessage(BITSTUFFING);
  //Abfrage Bus frei
  if (awaitBusFree())  { 
    //Sende Nachricht              
    sendSOF();

    // Send the bitstuffed message
    for (int i = 0; i < len; i++) {
        if (global_BITSTUFFED_message[i] == '1') {
            digitalWrite(COMM_OUT, HIGH);
        } else {
            digitalWrite(COMM_OUT, LOW);
        }
        DELAY(SENDDELAY);
    }
    
    
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
  eofCounter = 0;
}


bool await_response(unsigned char adress) {
    for (int i = 0; i < WAITLOOP; i++) {
        if (readMessage()) {
            if (global_adress == adress) {
                return true;
            }
        }
    }
    return false;
}


//------------------------------
//Spezifische Funktionen fuer Slaves 
void getAdress(){
  //Bitte um Adresse + 2. byte = Funktion
  unsigned char message[] = {REQUESTADRESS,FUNCTION};
  sendMessage(controllerAdress,0x01,message); 

  //Antwort erhalten
  if (await_response(NOADRESS)){   
    //Speichern der neuen Adresse
    myAdress = global_message[0];
  } 
}

void myFunction(){
  unsigned char myDatasize = 0;
  unsigned char myData[8] = {0};
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
          //bei OUTPUT ROWS[row] ist COLS[col] HIGH 
          myData[myDatasize] = tasten2x2[row][col];
          myDatasize++;
          }
        }
      delay(5);
      digitalWrite(ROWS[row], LOW);
      }

    //----------------------------
    unsigned char message[] = {0xA1};
 // Uebermitteln der Daten
  DELAY(1000);
  if (myDatasize == 0){
    sendMessage(controllerAdress,myDatasize,message);
  }else{
    sendMessage(controllerAdress,myDatasize-1,myData);
  }
}
//SETUP-----------------------------
void setup() {
//__DON'T CHANGE!!!------------------
  pinMode(COMM_IN, INPUT);
  pinMode(COMM_OUT, OUTPUT);
//------------------------------
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);

  pinMode(COL1, INPUT);
  pinMode(COL2, INPUT);

}
//LOOP------------------------------
void loop() {
    if (global_adress == 1) digitalWrite(COMM_OUT,HIGH);
    if (readMessage())
    { 
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
