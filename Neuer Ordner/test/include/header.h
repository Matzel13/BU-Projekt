#include <Arduino.h>
#include <iostream>
#include <list>
//recieve

bool sync();

char readAdress() ;

char readCOF();

void readData(char dataSize) ;

bool readMessage();


//transmit

void sendAdress(char adress) ;

void sendCOF(char dataSize);

void sendData(char data) ;

void sendMessage();




void setup() ;

void printAdr(std::list<char>& Function, int stelle);

int whichFunction(char adresse, volatile char* data);

void functionKeypad(volatile char[]);

void functionAudiopad(volatile char[]);

void switchFunction(char adresse);
