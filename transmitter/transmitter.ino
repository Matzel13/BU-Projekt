
#define COMM    16
#define TASTER  4

#define DELAY delayMicroseconds(1000)  // time between bits
//#define message 'P' // 0x50

int adressSize = 3;

char adress = 0x06;

// volatile char message[] = {'K', 'h', 'n', 0, 0, 0, 0};
volatile int sizeOfMessage = 3;

void sendMessage(){
  char message[] = {'H', 'k', 'K', 0, 0, 0, 0};
  adress = 0x05;
  Serial.print("Sending:");
  for(int i = 0; i < 8; i++){
    Serial.print(message[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // SOF:
  digitalWrite(COMM, HIGH);
  DELAY;
  digitalWrite(COMM, LOW);
  DELAY;

  // Adress:
  Serial.print("Adresse: ");
  for(int i = 0; i < (adressSize+1); i++){
    if((adress & 0x01) == 0x01){ 
      digitalWrite(COMM, HIGH);
      Serial.print("1");
    }
    else{
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
  for(int i = 0; i < 3; i++){
    if((byteCount & 0x01) == 0x01){ 
      digitalWrite(COMM, HIGH);
      Serial.print("1");
    }
    else{
      digitalWrite(COMM, LOW);
      Serial.print("0");
    }
    DELAY;
    byteCount = byteCount >> 1;
  }
  Serial.println();  

  // DATA:
  for(int j = 0; j < sizeOfMessage; j++){
    for(int i = 0; i < 8; i++){
      if((message[j] & 0x01) == 0x01){ 
        digitalWrite(COMM, HIGH);
        Serial.print("1");
      }
      else{
        digitalWrite(COMM, LOW);
        Serial.print("0");
      }
      DELAY;
      message[j] = message[j] >> 1;
    }
    Serial.print(" ");
  }
  Serial.println();

  // EOF
  digitalWrite(COMM, HIGH);
  Serial.print("1");
  DELAY;
  digitalWrite(COMM, LOW);
  for(int i = 0; i < 5; i++){
      Serial.print("0");
    DELAY;
  } 
  Serial.println();
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

sendMessage();
delay(10000);
}



