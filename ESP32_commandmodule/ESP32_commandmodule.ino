#define COMM 5
#define LED 17

#define DELAY delayMicroseconds(1000)

#define callInput 0x100

char mask = 0x01;
int adressSize = 3;

volatile char adress = 0x00;
char myAdress = 0x01;
volatile unsigned long timeStamp;
volatile unsigned long delayTime;

void setup() {
  // put your setup code here, to run once:
  pinMode(COMM, INPUT);

  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
}

void sendMessage(unsigned message, char adress) {
  Serial.print("Sending: ");
  Serial.print(message, HEX);
  Serial.println();

  // SOF:
  digitalWrite(COMM, HIGH);
  DELAY;
  digitalWrite(COMM, LOW);
  DELAY;

  // Adress:
  Serial.print("Adresse: ");
  for (int i = 0; i < (adressSize + 1); i++) {
    if ((adress & 0x01) == 0x01) {
      digitalWrite(COMM, HIGH);
      Serial.print("1");
    } else {
      digitalWrite(COMM, LOW);
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
      digitalWrite(COMM, HIGH);
      Serial.print("1");
    } else {
      digitalWrite(COMM, LOW);
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
        digitalWrite(COMM, HIGH);
        Serial.print("1");
      } else {
        digitalWrite(COMM, LOW);
        Serial.print("0");
      }
      DELAY;
      message = message >> 1;
    }
    Serial.print(" ");
  }
  Serial.println();

  // EOF
  digitalWrite(COMM, HIGH);
  Serial.print("1");
  DELAY;
  digitalWrite(COMM, LOW);
  for (int i = 0; i < 5; i++) {
    Serial.print("0");
    DELAY;
  }
  Serial.println();
}

char readMessage() {
  char message[] = {0,0,0,0,0,0,0,0};
  // SOF
  if (digitalRead(COMM) == HIGH) {

    // synchronisation via SOF: 

    timeStamp = micros(); // set tiemStamp to calculate transmittionSpeed
    while (digitalRead(COMM) == HIGH); // wait till starting Bit has passed

    delayTime = micros() - timeStamp; // calculate delay time
    Serial.print("Delaytime: ");
    Serial.println(delayTime);

    DELAY;
    DELAY;

    // Adress 
    Serial.print("Adresse: ");
    adress = 0x00;
    for (int i = 0; i < (adressSize + 1); i++) {
      if (digitalRead(COMM) == HIGH) {
        adress = adress | (mask << i);
        //digitalWrite(LED, HIGH);
        Serial.print("1");
      } else {
        //digitalWrite(LED, LOW);
        Serial.print("0");
      }
      DELAY(delayTime);
    }
    Serial.println();

    // COF
    char byteCount = 0x00;
    for (int i = 0; i < 3; i++) {
      if (digitalRead(COMM) == HIGH) {
        byteCount = byteCount | (mask << i);
        //digitalWrite(LED, HIGH);
        Serial.print("1");
      } else {
        //digitalWrite(LED, LOW);
        Serial.print("0");
      }
      DELAY;
    }
    Serial.print("Received: ");
    Serial.println(byteCount, HEX);

    // when adress = myAdress then listen to the data
    Serial.print("Adress in HEX: ");
    Serial.println(adress, HEX);
    Serial.println();

    if (adress == 0x01) {

      // store data:
      for (int j = 0; j < byteCount; j++) {
        for (int i = 0; i < 8; i++) {
          if (digitalRead(COMM) == HIGH) {
            message[j] = message[j] | (mask << i);
            digitalWrite(LED, HIGH);
            Serial.print("1");
          } else {
            digitalWrite(LED, LOW);
            Serial.print("0");
          }
          DELAY(delayTime);
        }
        Serial.print(" ");
      }
      Serial.println();
      Serial.print("Received: ");
      for (int i = 0; i < 8; i++) {
        Serial.print(message[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // EOF
      for (int i = 0; i < 5; i++) {
        DELAY;
      }
    }
    else{
      for(int i = 0; i < (byteCount + 5); i++){
        DELAY;
      }
    }
  }
  return message;
}

void loop() {
  char input[] = {0,0,0,0,0,0,0,0};

  // get input from user:
  sendMessage(callInput, 0x02);
  input = readMessage();

  // Send received input to PC:
  Serial2.println(input);
}

void blink() {
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);
}
