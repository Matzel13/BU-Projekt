#include <Arduino.h>
#include <iostream>
#include <list>
//recieve
void sync() ;

bool readAdress() ;

char readCOF();

void readData(char dataSize) ;

void readMessage();

//transmit

void sendAdress(char adress) ;

void sendCOF(char dataSize);

void sendData(char data) ;

void sendMessage();




void setup() ;

void printAdr(std::list<char>& Function, int stelle);

int whichFunction(char adresse) ;

int whichFunction(char adresse, char data);

void functionKeypad(volatile char[]);

void functionAudiopad(volatile char[]);

void switchFunction(char adresse);
