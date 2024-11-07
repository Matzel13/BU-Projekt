
#define COMM    16
#define TASTER  4

#define DELAY delayMicroseconds(1500)  // time between bits
//#define message 'P' // 0x50

volatile char message = 'b';

void sendMessage(char message){
  Serial.print("Sending:");
  Serial.println(message, HEX);
  digitalWrite(COMM, HIGH);
  DELAY;
  digitalWrite(COMM, LOW);
  DELAY;
  for(int i = 0; i < 8; i++){
    if((message & 0x01) == 0x01){ 
      digitalWrite(COMM, HIGH);
      Serial.println("1");
    }
    else{
      digitalWrite(COMM, LOW);
      Serial.println("0");
    }
    DELAY;
    message = message >> 1;
  }
}

// ISR:
void taster(){
  //sendMessage();
}

void setup() {
  // put your setup code here, to run once:
  pinMode(COMM, OUTPUT);

  pinMode(TASTER, INPUT);
  //attachInterrupt(digitalPinToInterrupt(TASTER), taster, RISING);

  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:

sendMessage(message);
delay(10000);
}



