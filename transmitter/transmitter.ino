
#define COMM    16
#define ROW1  2
#define ROW2  3
#define COL1  4
#define COL2  5

int ROWS[2] {ROW1, ROW2};
int COLS[2] {COL1, COL2};

#define DELAY delayMicroseconds(1000)  // time between bits
//#define message 'P' // 0x50

int adressSize = 3;

char adress = 0x06;

//volatile char message[] = {0, 0, 0, 0, 0, 0, 0, 0};
volatile int sizeOfMessage = 2;
volatile char pressedButton = NULL;
volatile unsigned input = 0x0000;

void sendMessage(unsigned message){
  //char message[] = {'H', 'k', 'K', 0, 0, 0, 0};
  adress = 0x05;
  Serial.print("Sending: ");
  Serial.print(input, HEX);
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
      if((message & 0x01) == 0x01){ 
        digitalWrite(COMM, HIGH);
        Serial.print("1");
      }
      else{
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
  for(int i = 0; i < 5; i++){
      Serial.print("0");
    DELAY;
  } 
  Serial.println();
}

// read the input keys:
void keypad(){
  input = 0x0000;
  for(int row = 0; row < 2; row++){
    digitalWrite(ROWS[row], HIGH);
    for(int col = 0; col < 2; col++){
      if(digitalRead(COLS[col]) == HIGH){
        input = input ^ (0x01 << col);
      }
      input = input << 2;
    }
    digitalWrite(ROWS[row], LOW);
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(COMM, OUTPUT);

  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);

  pinMode(COL1, INPUT_PULLDOWN);
  pinMode(COL2, INPUT_PULLDOWN);
  
  //attachInterrupt(digitalPinToInterrupt(TASTER1), taster, RISING);
  //attachInterrupt(digitalPinToInterrupt(TASTER2), taster, RISING);

  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:

  // constantly check for pressed buttons:
  keypad();
  if(input != 0x0000){
    sendMessage(input);
  }
  delay(1000); // delay 1 ms
}



