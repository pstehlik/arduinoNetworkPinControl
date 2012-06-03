/*
 Network Pin Control:
 This sketch receives a command via UDP, prints that command to the serial port 
 and sets the pin identified in the command to the respective state.
 After completion of actions and status changes sends back acknowledgement to the sender.

 @author pstehlik
 @since 2012-06-02

 Copyright 2012 Philip Stehlik 
 Use as is - no warranties given
 TODO:
 - Write message text on LCD
 - Allow for sending a message ID to be able to respond to explicit messages and IDs and allow the receiver to check which response was for which message
 - Allow setting multiple pin states within one message
 */

#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

#define SUPPORTED_PIN_MEMORY 10

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 8, 2);

unsigned int localPort = 10042;      // local port to listen on

// details about where the commands are located in the incoming string
const int commandPin[2] = {0,2};
const int commandState[2] = {2,1};
const int commandMessageStart = 3;

//the 'brain' of the machine
int pinMemory[SUPPORTED_PIN_MEMORY];
int pinStateMemory[SUPPORTED_PIN_MEMORY];

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
const char replyBufferOk[] = "OK";       // a string to send back
const char replyBufferError[] = "ER";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);

  Serial.begin(9600);
}


void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize)
  {
    boolean errorOccured = false;
    
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i =0; i < 4; i++)
    {
      Serial.print(remote[i], DEC);
      if (i < 3)
      {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer for processing
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");
    Serial.println(packetBuffer);
    
    //get the pin and state out of the packet and store in memory for later application of state
    char cmdMsg[UDP_TX_PACKET_MAX_SIZE];
    int cmdPin = getIntegerFromCharArray(commandPin[0], commandPin[1], packetBuffer);
    int cmdState = getIntegerFromCharArray(commandState[0], commandState[1], packetBuffer);
    Serial.println("Received pin:");
    Serial.println(cmdPin, DEC);
    Serial.println("Received state to put in:");
    Serial.println(cmdState, DEC);
    errorOccured = !putIntoPinMemory(cmdPin, cmdState);
    
    if(!errorOccured){
      //todo move message parsing into method
      // only try parsing the message out of the command if there are enough chars
      if(packetSize > commandMessageStart){
        //parse out the message part of the packet
        char cmdMessage[packetSize - commandMessageStart]; 
        int i;
        for(i = commandMessageStart; i < packetSize; i++){
          cmdMessage[i - commandMessageStart] = packetBuffer[i];
        }
        Serial.println("Message received:");
        Serial.println(cmdMessage);
        //todo print message on LCD
      }
      
      //apply the overall pin memory to board
      applyPinMemory();
    }
    
    // send a reply to confirm the receipt of the command
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    if(errorOccured){
      Udp.write(replyBufferError);
    } else {
      Udp.write(replyBufferOk);
    }
    Udp.endPacket();
  }
  delay(10);
}

/**
 * Extracts an integer from a char array.  
 * @param startPosition the start position of the integer number in the char array
 * @param integerNumbers the number of single digits of the integer (should be 5 for an integer of '   23' as whitespace is ignored)
 * @param readFromBuffer The char buffer to read from
 */
int getIntegerFromCharArray(int startPosition, int integerNumbers, char readFromBuffer[]){
  char intBuffer[integerNumbers];
  for(int i = 0; i < integerNumbers; i++){
    intBuffer[i] = readFromBuffer[startPosition + i];
  }
  return atoi(intBuffer);
}


/**
 * Sets up the state of a pin.  The 'memory' will leave the pin in this state until changed through another command.
 * @param pin The pin that should be put into state
 * @param state The state of the pin that should be set
 */
boolean putIntoPinMemory(int pin, int state){
  if(state != HIGH && state != LOW){
    Serial.print("Unsupported pin state: ");
    Serial.println(state, DEC);
    return false;
  }
  
  int setStateAtIndex = -1;
  
  int i;
  for(i = 0; i < SUPPORTED_PIN_MEMORY; i++){
    Serial.print("memory loop position: ");
    Serial.println(i, DEC);
    
    if(pinMemory[i] != NULL && pinMemory[i] == pin){
      Serial.println("Found pin state, setting up to update");
      setStateAtIndex = i;
      break;
    } else if (pinMemory[i] == NULL) {
      Serial.println("Found empty pin memory slot to put state into");      
      setStateAtIndex = i;
      break;
    }
  }
  Serial.print("put pin and state at position: ");
  Serial.println(setStateAtIndex, DEC);
  
  if(setStateAtIndex > -1){
    pinMemory[setStateAtIndex] = pin;
    pinStateMemory[setStateAtIndex] = state;
  }
  
  if(setStateAtIndex < 0){
    Serial.println("Reached capacity of pin memory or wrong state sent. Reuse pins and use 1 or 0 as pin state.");
    Serial.print("max pinMemory capacity: ");    
    Serial.println(SUPPORTED_PIN_MEMORY, DEC);
    
    Serial.print("received state:");    
    Serial.println(state, DEC);
  }
  
  Serial.println("Pin and state memory:");

  for(i = 0; i < SUPPORTED_PIN_MEMORY; i++){
    if(pinMemory[i] != NULL){
      Serial.print("[");
      Serial.print(pinMemory[i], DEC);
      Serial.print(" - ");
      Serial.print(pinStateMemory[i], DEC);
      Serial.println("]");
    }
  }
  return (setStateAtIndex > -1);
}



void applyPinMemory(){
  int i;
  Serial.println("Setting pins to respective states:");
  for(i = 0; i < SUPPORTED_PIN_MEMORY; i++){  
    if(pinMemory[i] != NULL){
      pinMode(pinMemory[i], OUTPUT);
      digitalWrite(pinMemory[i], pinStateMemory[i]);
      Serial.print("[");
      Serial.print(pinMemory[i], DEC);
      Serial.print(" - ");
      Serial.print(pinStateMemory[i], DEC);
      Serial.println("]");
    } 
  }  
}


/*
  Groovy code example to use as client for the sketch
 =====================================================
 
 
 setting pin 10 to state 1 and sending message 'Some Message String'
 =====================================================

data = "101Some Message String".getBytes()
addr = InetAddress.getByName("192.168.8.2")
port = 10042
packet = new DatagramPacket(data, data.length, addr, port)
socket = new DatagramSocket()
socket.setSoTimeout(4000) // block for no more than 30 seconds
socket.send(packet)


buffer = (' ' * 4096) as byte[]
response = new DatagramPacket(buffer, buffer.length)
socket.receive(response)
s = new String(response.data, 0, response.length)
println "Server said: '$s'"


 setting multiple pins to states without messages
 =====================================================

addr = InetAddress.getByName("192.168.8.2")
port = 10042
socket = new DatagramSocket()

commands = [
" 30",
" 41",
" 50",
" 61"
]
commands.each{ singleCommand ->
  data = singleCommand.getBytes()
  packet = new DatagramPacket(data, data.length, addr, port)
  socket.send(packet)
}
 */


