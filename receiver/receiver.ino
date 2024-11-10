
#define COMM_DP 16

#define DELAY(x) delayMicroseconds(x)

char mask = 0x01;
int adressSize = 3;

volatile char adress;
volatile unsigned long timeStamp;
volatile unsigned long delayTime;

void setup() {
  // put your setup code here, to run once:
  pinMode(COMM_DP, INPUT);

  pinMode(LED, OUTPUT);

Serial.begin(115200);
 // attachInterrupt(digitalPinToInterrupt(COMM), receiving, RISING);
}

void readMessage(){
  // SOF
if(digitalRead(COMM) == HIGH){
  
  // synchronisation via SOF: 


  timeStamp = micros();               // set tiemStamp to calculate transmittionSpeed
  while(digitalRead(COMM) == HIGH);   // wait till starting Bit has passed

  delayTime = micros() - timeStamp;   // calculate delay time
  Serial.print("Delaytime: ");
  Serial.println(delayTime);


  DELAY(delayTime*2);
  
  // Adress 
  Serial.print("Adresse: ");
  for(int i = 0; i < (adressSize+1); i++){
    if(digitalRead(COMM) == HIGH){
      adress = adress | (mask << i);
      digitalWrite(LED, HIGH);
      Serial.print("1");
    }
    else{
      digitalWrite(LED, LOW);
      Serial.print("0");
    }
    DELAY(delayTime);
  }
  Serial.println();
  
  // COF
  char byteCount = 0x00;
  for(int i = 0; i < 3; i++){
    if(digitalRead(COMM) == HIGH){
      byteCount = byteCount | (mask << i);
      digitalWrite(LED, HIGH);
      Serial.print("1");
    }
    else{
      digitalWrite(LED, LOW);
      Serial.print("0");
    }
    DELAY(delayTime);
  }
  Serial.print("Received: ");
  Serial.println(byteCount, HEX);  


  // when adress = myAdress then listen to the data
  
  // store data:
  char message[] = {0,0,0,0,0,0,0,0};
  for(int j = 0; j < byteCount; j++){
    for(int i = 0; i < 8; i++){
      if(digitalRead(COMM) == HIGH){
        message[j] = message[j] | (mask << i);
        digitalWrite(LED, HIGH);
        Serial.print("1");
      }
      else{
        digitalWrite(LED, LOW);
        Serial.print("0");
      }
      DELAY(delayTime);
    }
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Received: ");
  for(int i = 0; i < 8; i++){
    Serial.print(message[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // EOF
  for(int i = 0; i < 5; i++){
    DELAY(delayTime);
  }
}
}

void loop() {
  readMessage();
}


void blink(){
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
}
