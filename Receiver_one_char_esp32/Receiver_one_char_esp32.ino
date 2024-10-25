
#define COMM 4
#define LED 16

#define DELAY(x) delayMicroseconds(x)

char mask = 0x01;
int adressSize = 3;

volatile char message, adress;
volatile unsigned long timeStamp;
volatile unsigned long delayTime;

void setup() {
  // put your setup code here, to run once:
  pinMode(COMM, INPUT);

  pinMode(LED, OUTPUT);

Serial.begin(115200);
 // attachInterrupt(digitalPinToInterrupt(COMM), receiving, RISING);
}

void loop() {


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
  Serial.print(", ");
  Serial.println(adress);
  

  // when adress = myAdress then listen to the data
  
  // store data:
  message = 0x00;
  int byteCount = 1;
  for(int i = 0; i < ((8 * byteCount)-1); i++){
    if(digitalRead(COMM) == HIGH){
      message = message | (mask << i);
      digitalWrite(LED, HIGH);
      Serial.println("1");
    }
    else{
      digitalWrite(LED, LOW);
      Serial.println("0");
    }
    DELAY(delayTime);
  }
  Serial.print("Received: ");
  Serial.println(message, HEX);

  // EOF

}
}


void blink(){
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
}
