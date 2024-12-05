#define COMM_TX 5
#define COMM_RX 18
#define LED 17

#define DELAY delayMicroseconds(10000)
#define DELAYX(x) delayMicroseconds(x)

#define callInput 0x64

char mask = 0x01;
int adressSize = 3;

volatile char adress = 0x00;
char myAdress = 0x01;
volatile unsigned long timeStamp;
volatile unsigned long delayTime;

volatile char messageRead[] = {0,0,0};

volatile int sizeOfMessage = 3;


void setup() {
  // put your setup code here, to run once:
  pinMode(COMM_RX, INPUT);
  pinMode(COMM_TX, OUTPUT);

  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
}

void sendMessage(unsigned message, char adress) {
  Serial.print("Sending: ");
  Serial.print(message, HEX);
  Serial.println();

  // SOF:
  digitalWrite(COMM_TX, HIGH);
  DELAY;
  digitalWrite(COMM_TX, LOW);
  DELAY;

  // Adress:
  Serial.print("Adresse: ");
  for (int i = 0; i < (adressSize + 1); i++) {
    if ((adress & 0x01) == 0x01) {
      digitalWrite(COMM_TX, HIGH);
      Serial.print("1");
    } else {
      digitalWrite(COMM_TX, LOW);
      Serial.print("0");
    }
    DELAY;
    adress = adress >> 1;
  }
  Serial.println();

  // COF (Bit count: 3 DataBits,)
  Serial.print("COF: ");
  char byteCount = ((char) sizeOfMessage);
  for (int i = 0; i < 3; i++) {
    if ((byteCount & 0x01) == 0x01) {
      digitalWrite(COMM_TX, HIGH);
      Serial.print("1");
    } else {
      digitalWrite(COMM_TX, LOW);
      Serial.print("0");
    }
    DELAY;
    byteCount = byteCount >> 1;
  }
  Serial.println();

  // DATA:
  for (int j = 0; j < sizeOfMessage; j++) {
    for (int i = 0; i < 8; i++) {
      if ((message & 0x01) == 0x01) {
        digitalWrite(COMM_TX, HIGH);
        Serial.print("1");
      } else {
        digitalWrite(COMM_TX, LOW);
        Serial.print("0");
      }
      DELAY;
      message = message >> 1;
    }
    Serial.print(" ");
  }
  Serial.println();

  // EOF
  digitalWrite(COMM_TX, HIGH);
  Serial.print("1");
  DELAY;
  digitalWrite(COMM_TX, LOW);
  for (int i = 0; i < 5; i++) {
    Serial.print("0");
    DELAY;
  }
  Serial.println();
}

void readMessage() {
  //char message[] = {0,0,0,0,0,0,0,0};
  // SOF
  if (digitalRead(COMM_RX) == HIGH) {

    // synchronisation via SOF: 

    timeStamp = micros(); // set tiemStamp to calculate transmittionSpeed
    while (digitalRead(COMM_RX) == HIGH); // wait till starting Bit has passed

    delayTime = micros() - timeStamp; // calculate delay time
    Serial.print("Delaytime: ");
    Serial.println(delayTime);

    DELAYX(delayTime);

    // Adress 
    Serial.print("Adresse: ");
    adress = 0x00;
    for (int i = 0; i < (adressSize + 1); i++) {
      if (digitalRead(COMM_RX) == HIGH) {
        adress = adress | (mask << i);
        //digitalWrite(LED, HIGH);
        Serial.print("1");
      } else {
        //digitalWrite(LED, LOW);
        Serial.print("0");
      }
      DELAYX(delayTime);
    }
    Serial.println();

    // COF
    char byteCount = 0x00;
    for (int i = 0; i < 3; i++) {
      if (digitalRead(COMM_RX) == HIGH) {
        byteCount = byteCount | (mask << i);
        //digitalWrite(LED, HIGH);
        Serial.print("1");
      } else {
        //digitalWrite(LED, LOW);
        Serial.print("0");
      }
      DELAYX(delayTime);
    }
    Serial.println();
    Serial.print("Received COF: ");
    Serial.println(byteCount, HEX);

    // when adress = myAdress then listen to the data
    Serial.print("Adress in HEX: ");
    Serial.println(adress, HEX);
    Serial.println();

    if (adress == 0x01) {

      // store data:
      for (int j = 0; j < byteCount; j++) {
        for (int i = 0; i < 8; i++) {
          if (digitalRead(COMM_RX) == HIGH) {
            messageRead[j] = messageRead[j] | (mask << i);
            digitalWrite(LED, HIGH);
            Serial.print("1");
          } else {
            digitalWrite(LED, LOW);
            Serial.print("0");
          }
          DELAYX(delayTime);
        }
        Serial.print(" ");
      }
      Serial.println();
      Serial.print("Received: ");
      for (int i = 0; i < 8; i++) {
        Serial.print(messageRead[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // EOF
      for (int i = 0; i < 5; i++) {
        DELAYX(delayTime);
      }
    }
    else{
      for(int i = 0; i < (byteCount + 5); i++){
        DELAYX(delayTime);
      }
    }
    Serial.print("Received Message: ");
    for(int i = 0; i < 3; i++){
      Serial.print(messageRead[i], HEX);
      Serial.print("");
    }
    Serial.println();
  }
}

bool waitForSignalWithTimeout(int pin, int timeoutMs) {
  unsigned long startTime = millis(); // Record the start time

  // Loop until signal is detected or timeout is reached
  while (digitalRead(pin) == LOW) { // Assuming LOW means no signal

    if (millis() - startTime >= timeoutMs) {
      return false; // Timeout occurred
    }
  }
  return true; // Signal received
}

void loop() {
  DELAYX(1000000);
  // get input from user:
  sendMessage(callInput, 0x02);
  // wait for response... timer if no response comes after x ms

  bool signalReceived = waitForSignalWithTimeout(COMM_RX, 100); // (PIN, timeout)
  if (signalReceived) {
    Serial.println("Signal received!");
    readMessage();
  } else {
    Serial.println("Timeout occurred!");
  }

  // Send received input to PC:
  for(int i = 0; i < sizeOfMessage; i++){
    if(messageRead[i] != 0x00) Serial2.println(messageRead[i]);
  }
}

void blink() {
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
}
