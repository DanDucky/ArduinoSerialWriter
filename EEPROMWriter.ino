#define BUFFER_SIZE 61
#define NEXT_PACKET 255

byte *fileBuffer = new byte[BUFFER_SIZE];
uint32_t currentAddress = 0;
#define NUMBER_OF_REGISTERS 4
const uint8_t extensionPins[NUMBER_OF_REGISTERS] = {2, 3, A4, A2}; // this is about to get disgusting
const uint8_t addressPins[16 - NUMBER_OF_REGISTERS] = {/*2, 3, A4, A2,*/2, 3, A4, A2, 4, 6, 7, 8, 9, A5, 11, 12};
const uint8_t dataPins[8] = {13, A0, A1, A3, 0, 1, 5, 10};

#define WRITE_ENABLE_PIN 16
#define READ_ENABLE_PIN 14
#define DEBUG_PIN 17

#define REGISTER_ENABLE_PIN 15
// below can be rewritten
#define RESET_PIN A5
#define WRITE_ENABLE_REGISTERS_PIN 4

class InputBuffer {
public:
    explicit InputBuffer(const int sizeInBytes) : bufferSize(sizeInBytes) {
        buffer = new uint8_t[sizeInBytes];
    }
    ~InputBuffer() {
        delete[] buffer;
    }
    int size () const {
        return bufferSize;
    }
    operator uint8_t*() const {
        return buffer;
    }
    operator uint16_t() const {
        uint16_t num = 0;
        num = buffer[0];
        num |= (buffer[1] << 8);
        return num;
    }
private:
    const int bufferSize;
    uint8_t* buffer;
};

void setAddressTo(uint16_t addr) {
  
  setExtension(addr, NUMBER_OF_REGISTERS);
  addr = addr >> NUMBER_OF_REGISTERS;
  for (unsigned short i = 0; i < 16 - NUMBER_OF_REGISTERS; i++) {
    digitalWrite(addressPins[i], ((addr >> i) & 1) == 1 ? HIGH : LOW);
  }
}

void pulse(uint8_t pin) {
  if (pin == WRITE_ENABLE_PIN) goto write;
  digitalWrite(pin, HIGH);
  digitalWrite(pin, LOW);
  return;
  write:
  digitalWrite(pin, LOW);
//  delay(10);
  digitalWrite(pin, HIGH);
}

void setExtension(uint16_t data, unsigned short size) {
  // reset extra registers

  digitalWrite(WRITE_ENABLE_REGISTERS_PIN, LOW);
  digitalWrite(RESET_PIN, LOW);

  delay(5);
  
  digitalWrite(REGISTER_ENABLE_PIN, HIGH);
  
  pulse(RESET_PIN);

  for (unsigned short i = 0; i < size; i++) {
    digitalWrite(extensionPins[i], ((data >> i) & 1) == 1 ? HIGH : LOW);
  }

//  pulse(RESET_PIN);

  pulse(WRITE_ENABLE_REGISTERS_PIN);
  
  digitalWrite(REGISTER_ENABLE_PIN, LOW);
}

void establishConnection() {
  currentAddress = 0;
  const uint16_t out = BUFFER_SIZE;
  Serial.write((byte*)&out, 2);
}

uint16_t readToBuffer() {  
  Serial.write((byte) NEXT_PACKET);
  
  InputBuffer sizeOfPacket(2);

  Serial.readBytes(sizeOfPacket, 2);

  Serial.readBytes(fileBuffer, sizeOfPacket);

  return sizeOfPacket;
}

void writeByte(uint8_t byteToWrite, uint16_t address) {
  setAddressTo(address);
//  delay(10);
  for (short i = 0; i < 8; i++) { // set the data pins
    digitalWrite(dataPins[i], ((byteToWrite >> i) & 1) == 1 ? HIGH : LOW);
  }
//  delay(10);
  
  pulse(WRITE_ENABLE_PIN);
  
  setAddressTo(address);
}

void writeNextByte(byte byteToWrite) {
  writeByte(byteToWrite, currentAddress);
  currentAddress++;
}

uint32_t establishRead() {
  InputBuffer sizeOfBlock(2);
  
  Serial.readBytes(sizeOfBlock, 2);

  return static_cast<uint32_t>((uint16_t)sizeOfBlock);
}

uint8_t readByteFromRom() {
  uint8_t out = 0;
  for (unsigned short i = 0; i < 8; i++) {
    out |= ((digitalRead(dataPins[i]) == HIGH ? 1 : 0) << i);
  }
  return out;
}

void setDataPins(uint8_t direction) {
  for (short i = 0; i < 8; i++) {
    pinMode(dataPins[i], direction);
  }
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  pinMode(RESET_PIN, OUTPUT);
  pinMode(WRITE_ENABLE_REGISTERS_PIN, OUTPUT);
  pinMode(WRITE_ENABLE_PIN, OUTPUT);
  pinMode(READ_ENABLE_PIN, OUTPUT);
  pinMode(REGISTER_ENABLE_PIN, OUTPUT);
  pinMode(DEBUG_PIN, OUTPUT);

  for (short i = 0; i < 16 - NUMBER_OF_REGISTERS; i++) {
    pinMode(addressPins[i], OUTPUT);
  }
    for (short i = 0; i < NUMBER_OF_REGISTERS; i++) {
    pinMode(extensionPins[i], OUTPUT);
  }

  digitalWrite(READ_ENABLE_PIN, HIGH);
  digitalWrite(WRITE_ENABLE_PIN, HIGH);

//  for (int i = 0; i < 600; i++) {
//    setAddressTo(i);
//    delay(1000);
//  }
//  setAddressTo(10);
//  digitalWrite(READ_ENABLE_PIN, LOW);
//  delay(10000);
}

void complete() {
  uint8_t out = 0xFF;
  Serial.write(&out, 1);
}

enum Signal {
  WRITE,
  READ,
  WRITE_BYTE,
  READ_BYTE
};

void loop() {
  // put your main code here, to run repeatedly:
  uint8_t input = 0xFF;
  if (Serial.available() != 0) {
    input = Serial.read();
    switch(input) {

      
      case WRITE: {
              setDataPins(OUTPUT);
      establishConnection();

      bool eof = false;

      while (!eof) {

        uint16_t packetSize = readToBuffer();
        
        eof = packetSize < BUFFER_SIZE;
        
        for (uint32_t index = 0; index < packetSize; index++) {
          writeNextByte(fileBuffer[index]);
        }
      }

      complete();
      }
      break;
      case READ: {
      setDataPins(INPUT);
      digitalWrite(READ_ENABLE_PIN, LOW);
      uint32_t sizeOfBlock = establishRead();
      for (uint16_t i = 0; i < sizeOfBlock; i++) {
        setAddressTo(i);
        uint8_t inputByte = readByteFromRom();
        Serial.write(inputByte);
      }
      digitalWrite(READ_ENABLE_PIN, HIGH);
      complete();
      }
      break;
      case WRITE_BYTE: {
        setDataPins(OUTPUT);
        InputBuffer address(2);
        Serial.readBytes(address, 2);
        while (Serial.available() < 1) {};
        uint8_t data = Serial.read();
        writeByte(data, address);
        complete();
      } break;
      case READ_BYTE: {
        setDataPins(INPUT);
        digitalWrite(READ_ENABLE_PIN, LOW);
        InputBuffer address(2);
        Serial.readBytes(address, 2);
        setAddressTo(address);
        uint8_t inputByte = readByteFromRom();
        Serial.write(inputByte);
        digitalWrite(READ_ENABLE_PIN, HIGH);
        complete();
      } break;
      default: break;
    }
  }
}
