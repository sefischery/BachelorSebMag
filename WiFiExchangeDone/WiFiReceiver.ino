/////RECEIVER/SERVER UDP WIFI CODE/////
#include "RSA.h"
#include "AES.h"
#include "WiFi.h"
#include "WiFiUdp.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
//The udp library class
WiFiUDP udp;

//Buffer setup for, receiving. 
#define MAX_BUFFER_SIZE 350
#define MAX_AES_BUFFER_SIZE 16
char standard_packet_buffer[MAX_BUFFER_SIZE];   //Where we get the UDP data
char AES_packet_buffer[MAX_AES_BUFFER_SIZE];

//WiFi information for the setup. 
const char *SoftAP_SSID = "ESP32SOFTAP";
const char *SoftAP_PASS = "testpassword";
const int SoftAP_Channel = 5;
const int SoftAP_Cloak = 0;
const int SoftAP_Max_Conn = 5;

// local port to listen for UDP packets
const unsigned int UDPPort = 2000;
char ReplyBuffer[] = "ACK";

//IP-addresses declarations. 
IPAddress ServerIP(192,168,4,1);
IPAddress ClientIP(192,168,4,2);
//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
//AES KEY Receive
int AES_KEY_RECEIVED[16] = {0};


String encAesMsg = "";
int AES_TEMP_HOLD[16] = {0};
boolean test = true;
boolean firsttime = true;

//Prototype declarations
void softAPConfigESP(void);
void APSetup(void);
String readFromClient(void);
String readFromClientAES(void);
void sendBignumberPacket(BigNumber);
void Clear_Buffers(char*, int);
BigNumber castToBignumber(String);
void fromStringToIntarray(String,int *);
void CompleteKeySetup(void);
String intArrayToString(int*);

////////// SETUP //////////
void setup(){
  Serial.begin(115200);
  BigNumber::begin();

  delay(1000);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Booting System");
  
  
  // Waiting until a connection is established
  APSetup();
  
  // RSA and AES key exchange
  CompleteKeySetup();
}

////////// MAIN LOOP //////////

void loop(){
  if (WiFi.softAPgetStationNum() != 0){
    encAesMsg = readFromClientAES();
    fromStringToIntarray(encAesMsg,AES_TEMP_HOLD);
    String encMSG = intArrayToString(AES_TEMP_HOLD);
    Serial.print("Encrypted message:  ");
    Serial.println(encMSG);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Receiving:");
    
    Serial.print("Decrypted message:  ");
    AES_decryption(AES_TEMP_HOLD, AES_KEY_RECEIVED);
    for(int i = 0; i<16;i++){
      Serial.print(AES_TEMP_HOLD[i]);
      Serial.print(" ");
    }
    Serial.println();
    
    String temperature = intArrayToString(AES_TEMP_HOLD);
    Serial.print("Received: Temp = ");
    Serial.println(temperature);
    
    if (firsttime){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Receiving...");
      lcd.setCursor(0, 1);
      lcd.print("Temp: "+temperature);
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Temp: "+temperature);
    }
    firsttime = false;
    
     // This is print the decrypted input as string
    Serial.println();

    Clear_Buffers(AES_packet_buffer, MAX_AES_BUFFER_SIZE);
    
  }
  else{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No WiFi Con");
  }
}


void softAPConfigESP(){
  //Create WiFi with WPA2 PSK
  //WiFi.softAP(SoftAP_SSID,SoftAP_PASS,SoftAP_Channel,SoftAP_Cloak,SoftAP_Max_Conn);
  //Create open WiFi (NO password)
  WiFi.softAP(SoftAP_SSID,NULL,SoftAP_Channel,SoftAP_Cloak,SoftAP_Max_Conn);
  Serial.println();
  Serial.print("IP address of ESPWiFi: ");
  Serial.println(WiFi.softAPIP()); 
  //should be 192.168.4.1 (this is the SoftAP IP, aka. server IP)
  Serial.println();
  Serial.print("The MAC Address of ESPWiFi: ");
  Serial.println(WiFi.macAddress());
  udp.begin(UDPPort);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi booted");
}

void APSetup(){
  softAPConfigESP();
  while(WiFi.softAPgetStationNum() == 0){
    if(WiFi.softAPgetStationNum() != 0){ //this should be done in the while, but let's just make sure.
      break;
    }
  }
  Serial.println();
  Serial.print("WiFi Clients Connected : ");
  Serial.println(WiFi.softAPgetStationNum()); //Client count. 
  Serial.println();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Client connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.softAPgetStationNum());
}

String readFromClient(){
  String temp = "";
  //Needs to be a while check, since we need to read untill something else than "". 
  while (temp == ""){
    udp.parsePacket();
    while(udp.read(standard_packet_buffer,MAX_BUFFER_SIZE)>0){
      udp.read(standard_packet_buffer, MAX_BUFFER_SIZE); 
      // read the packet into the buffer, we are reading only one byte
      delay(20);
    }
    temp = standard_packet_buffer;
  }
  return temp;
}


String readFromClientAES(){
  String temp = "";
  while (temp == ""){ 
    udp.parsePacket();
    while(udp.read(AES_packet_buffer,MAX_AES_BUFFER_SIZE)>0){
      udp.read(AES_packet_buffer, MAX_AES_BUFFER_SIZE);
      delay(20);
    }
    temp = AES_packet_buffer;
  }
  return temp;
}

void sendBignumberPacket(BigNumber msg){
  udp.beginPacket(ClientIP,UDPPort);
  udp.print(msg);
  udp.endPacket();
}

void Clear_Buffers(char* buffername, int sizeofbuffer){
  for(int i = 0; i < sizeofbuffer; i++){
    buffername[i] = 0;
  }
}


BigNumber castToBignumber(String msg){
  char temp[(msg.length()+1)];
  msg.toCharArray(temp, (msg.length()+1));
  return temp;
}



void fromStringToIntarray(String src, int *dst){
  char HoldRes;
  for (int i = 0; i < 16; i++){
    HoldRes = src[i];

    dst[i] = (int) HoldRes;
  }
}

void CompleteKeySetup(){
  // Generate RSA keys
  BigNumber publickey = PublicKeyGen("34169090529181804975509056946439162865195316898547061713223274034873259174398131810568013649207194042897653590213438750899278631021362723640271358164566011","6480748263904619031680260331936564669623869952492469641161930262856628003360317687993928982065215349138700631936521201127616864444987232349342262924810817");
  BigNumber privatekey = PrivateKeyGen("34169090529181804975509056946439162865195316898547061713223274034873259174398131810568013649207194042897653590213438750899278631021362723640271358164566011","6480748263904619031680260331936564669623869952492469641161930262856628003360317687993928982065215349138700631936521201127616864444987232349342262924810817");
  //could be done at the very beginning, before serial.begin, so the calculations are done very early.
  
  //STEP 1. Send Public Key to Sender //
  Serial.println();
  Serial.println("Sending: RSA Public Key");
  udp.beginPacket(ClientIP,UDPPort);
  udp.print(publickey);
  udp.endPacket();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sending:");
  lcd.setCursor(0, 1);
  lcd.print("RSA Public Key");

  // Receiving RSA key ack
  String ReceivedAck = readFromClient();
  Serial.println();
  Serial.print("Received: ");
  Serial.println(ReceivedAck);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Received:");
  lcd.setCursor(0, 1);
  lcd.print("RSA ACK");

  // Clear buffer
  Clear_Buffers(standard_packet_buffer, MAX_BUFFER_SIZE);

  // Receiving encrypted AES key.
  String encKey = readFromClient();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Received:");
  lcd.setCursor(0, 1);
  lcd.print("AES Key");
  delay(5000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Decrypting");
  lcd.setCursor(0, 1);
  lcd.print("AES KEY...");
  
  delay(1000);
  BigNumber Encrypted_AES_key = castToBignumber(encKey);
  delay(1000);
    
  delay(1000);
  BigNumber Decrypted_AES_key = RSA_decryption(Encrypted_AES_key, publickey, privatekey);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Decryption:");
  lcd.setCursor(0, 1);
  lcd.print("Successed");
  
  fromBignumberToIntarray(Decrypted_AES_key, AES_KEY_RECEIVED);
  Serial.println();
  Serial.print("Received AES Key: ");
  for(int i = 0; i < 16; i++){
    Serial.print(AES_KEY_RECEIVED[i]);
    Serial.print(" ");
  }
  Serial.println();


  // Send AES ACK
  Serial.println();
  Serial.println("Sending: AES ACK");
  
  udp.beginPacket(ClientIP,UDPPort);
  udp.printf("AES ACK");
  udp.endPacket();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sending:");
  lcd.setCursor(0, 1);
  lcd.print("AES ACK");

  Serial.println();
}

String intArrayToString(int *src){
  String printer;
  for (int i = 0; i < 16; i++){
    if (!(src[i] == 0)){
        printer += src[i];
    }
  }
  return printer;
}
