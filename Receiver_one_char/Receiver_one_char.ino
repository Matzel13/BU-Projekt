
#define COMM 2
#define LED 3

#define DELAY 100

volatile bool transmit = false;

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
  char message = 0x00;
  delay(DELAY*2);
  for(int i = 0; i < 8; i++){
    if(digitalRead(COMM) == HIGH){
      message = message & 0x01;
      Serial.println("1");
    }
    else{
      Serial.println("0");
    }
    delay(DELAY);
    message = message << 1;
  }
  Serial.print("Received:");
  Serial.println(message);
  transmit = false;
}
}


void blink(){
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
}
