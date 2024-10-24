
#define COMM 4
#define LED 16

#define DELAY delayMicroseconds(1000)

volatile bool transmit = false;
volatile char message, newMessage;
char mask = 0x01;

void setup() {
  // put your setup code here, to run once:
  pinMode(COMM, INPUT);

  pinMode(LED, OUTPUT);

Serial.begin(115200);
 // attachInterrupt(digitalPinToInterrupt(COMM), receiving, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:

if(digitalRead(COMM) == HIGH){
  message = 0x00;
  DELAY;
  DELAY;
  DELAY;
  for(int i = 0; i < 7; i++){
    if(digitalRead(COMM) == HIGH){
      message = message | (mask << i);
      digitalWrite(LED, HIGH);
      Serial.println("1");
    }
    else{
      digitalWrite(LED, LOW);
      Serial.println("0");
    }
    DELAY;
  }
  Serial.print("Received: ");
  Serial.println(message, HEX);
  transmit = false;
}
}


void blink(){
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
}
