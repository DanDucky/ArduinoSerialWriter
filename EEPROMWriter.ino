#define START_DATA_PIN 0
#define BUFFER_SIZE 2300
#define NEXT_PACKET 255

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
        uint16_t num;
        num = buffer[0];
        num |= (buffer[1] << 8);
        return num;
    }
private:
    const int bufferSize;
    uint8_t* buffer;
};

byte *fileBuffer = new byte[BUFFER_SIZE];
uint32_t currentAddress = 0;

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

void writeByte(byte byteToWrite, uint16_t address) {

  for (short i = 0; i < 8; i++) { // set the data pins
    digitalWrite(i + START_DATA_PIN,
    (byteToWrite >> i) & 1 == 1 ? HIGH : LOW);
  }
}

void writeNextByte(byte byteToWrite) {
  writeByte(byteToWrite, currentAddress);
  currentAddress++;
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  for (short i = 0; i < 8; i++) {
    pinMode(i + START_DATA_PIN, OUTPUT);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() != 0) {
    String input = Serial.readString();
    input.trim();
    if (input == "write") {
      establishConnection();

      bool eof = false;

      while (!eof) {

        uint16_t packetSize = readToBuffer();
        
        eof = packetSize < BUFFER_SIZE;
        
//        for (uint32_t index = 0; index < packetSize; index++) {
//          writeNextByte(fileBuffer[index]);
//        }
      }
    }
  }
}
