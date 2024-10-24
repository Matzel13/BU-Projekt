
#define COMM 2
#define LED 3

#define DELAY delayMicroseconds(1000)

volatile bool transmit = false;
volatile char message, newMessage;

void setup() {
  // put your setup code here, to run once:
  pinMode(COMM, INPUT);

  pinMode(LED, OUTPUT);

Serial.begin(9600);
 // attachInterrupt(digitalPinToInterrupt(COMM), receiving, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:

if(digitalRead(COMM) == HIGH){
  message = 0x00;
  DELAY;
  DELAY;
  for(int i = 0; i < 7; i++){
    if(digitalRead(COMM) == HIGH){
      message = message | 0x01;
      digitalWrite(LED, HIGH);
      Serial.println("1");
    }
    else{
      digitalWrite(LED, LOW);
      Serial.println("0");
    }
    DELAY;
    message = message << 1;
  }
  Serial.print("Received: ");
  Serial.println(message, HEX);
  Serial.print("Mirrored: ");
  mirrorMessage();
  Serial.println(newMessage, HEX);
  transmit = false;
}
}


void mirrorMessage(){
  newMessage = 0x00;
  for(int i = 0; i < 7; i++){
    if((message & 0x01) == 0x01){
      newMessage = newMessage | 0x01;
    }
    message = message >> 1;
    newMessage = newMessage << 1;
  }
}

void blink(){
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
}
