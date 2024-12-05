#define COMM_RX 15
#define COMM_TX 16
#define LED 17

#define ROW1 2
#define ROW2 3
#define COL1 4
#define COL2 5

int ROWS[2] {
  ROW1,
  ROW2
};
int COLS[2] {
  COL1,
  COL2
};

#define DELAY delayMicroseconds(1000) // time between bits
#define DELAYX(x) delayMicroseconds(x) // time between bits

//#define message 'P' // 0x50

volatile char adress = 0x00;
volatile unsigned long timeStamp;
volatile unsigned long delayTime;

int adressSize = 3;
char mask = 0x01;
char messageRead[] = {0,0,0,0,0,0,0,0};
int messageToCheck = 0x100;
char myAdress = 0x02;

//volatile char message[] = {0, 0, 0, 0, 0, 0, 0, 0};
volatile int sizeOfMessage = 2;
volatile char pressedButton = NULL;
volatile unsigned input = 0x0000;

void sendMessage(unsigned message, char adress) {
    Serial.println("sendMessage");

  //char message[] = {'H', 'k', 'K', 0, 0, 0, 0};
  //adress = 0x05;
  Serial.print("Sending: ");
  Serial.print(input, HEX);
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

// read the input keys:
void keypad() {
  input = 0x0000;
  for (int row = 0; row < 2; row++) {
    digitalWrite(ROWS[row], HIGH);
    for (int col = 0; col < 2; col++) {
      if (digitalRead(COLS[col]) == HIGH) {
        Serial.println("COL HIGH!");
        Serial.println(COLS[col]);
        input = input | (0x0001 << col);
      }
      input = input << 2;
    }
    delay(5);
    digitalWrite(ROWS[row], LOW);
  }
  input = input >> 2;
  // Serial.println(input, HEX);
}

void readMessage() {
      Serial.println("readMessage");

  // SOF
  if (digitalRead(COMM_RX) == HIGH) {

    // synchronisation via SOF: 

    timeStamp = micros(); // set tiemStamp to calculate transmittionSpeed
    while (digitalRead(COMM_RX) == HIGH); // wait till starting Bit has passed

    delayTime = micros() - timeStamp; // calculate delay time
    Serial.print("Delaytime: ");
    Serial.println(delayTime);

    DELAYX(delayTime * 2);

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
    Serial.print("Received: ");
    Serial.println(byteCount, HEX);

    // when adress = myAdress then listen to the data
    Serial.print("Adress in HEX: ");
    Serial.println(adress, HEX);
    Serial.println();

    if (adress == 0x02) {
      for (int i = 0; i < sizeOfMessage; ++i) {
        messageRead[i] = 0;
      }
      Serial.println("adress: %d", adress);


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
    } else {
      DELAYX((byteCount + 5) * delayTime);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(COMM_RX, INPUT_PULLDOWN);
  pinMode(COMM_TX, OUTPUT);

  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);

  pinMode(COL1, INPUT_PULLDOWN);
  pinMode(COL2, INPUT_PULLDOWN);

  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  readMessage();

  for (int i = 0; i < sizeOfMessage; ++i) {
    if (messageRead[i] == messageToCheck) {
      keypad();
    }
  }

  if (input != 0x0000 && (adress == myAdress)) {
    if (((input & 0x0100) == 0x0100)) {
      sendMessage(input, 0x01);
    }
  }
  delay(100); // delay 1 ms
}