#include <Arduino.h>
#include <iostream>
#include <list>
struct device
{
    char adress;
    char funktion;
    volatile char* latest_data;
    unsigned int timeout;

    device() : adress(0x00),funktion(0x00),latest_data(NULL),timeout(0){}

    public:
        device(char Adresse,char Funktion,char* Daten,unsigned int Zaehler)
        : adress(Adresse),funktion(funktion),latest_data(Daten),timeout(Zaehler){}
};


//recieve funktionen---------------------------------------------------------------------

/*
synchronisiert die Taktgeschwindigkeit
@return SOF wurde empfangen
*/
bool sync();

/*
liest die Adresse ein (global gespeichert)
@return eingelesene Adresse
*/
char readAdress();

/*
liest COF ein (global gespeichert)
@return eingelesenes COF
*/
char readCOF();

/*
liest die Daten ein (global gespeichert)

@dataSize anzahl der einzulesenden datenbytes
@return 
*/
void readData(char dataSize);

/*
Kombiniert die vorangegangenen Funktionen um eine gesamte Nachricht einzulesen
@return fertig eingelesen
*/
bool readMessage();


//transmit funktionen--------------------------------------------------------------------


/*

@return Bus ist frei
*/
bool busFree();
/*

@return 
*/
void sendAdress(char adress) ;
/*

@return 
*/
void sendCOF(char dataSize);
/*

@return 
*/
void sendData(char data) ;
/*

@return 
*/
void sendMessage();



//----------------------------------------------------------------------------------------------------------------------------------------

/*

@return 
*/
void setup() ;

/*

@return 
*/
void printAdr(std::list<char>& Function, int stelle);
/*

@return 
*/
int whichFunction(char adresse, volatile char* data);

/*

@return 
*/
void functionKeypad(volatile char[]);

/*

@return 
*/
void functionAudiopad(volatile char[]);

/*

@return 
*/
void switchFunction(char adresse);

/*
Vergibt neue Adressen
@return neue Adresse (0x00, wenn keine Adressen mehr frei sind)
*/
char newAdress();

/*
wartet auf die Antwort eines Busteilnehmers
@adress Adresse von der die Antwort kommen soll
@return Antwort erhalten
*/
bool await_response(char adress);

/*
Handling des timeouts von teilnehmern
@return 
*/
void timeout(char adress, bool no_response);

void global_reset();