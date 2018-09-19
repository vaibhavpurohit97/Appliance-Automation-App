#include <SoftwareSerial.h>

#define DEBUG true

SoftwareSerial esp8266(2,3); // make RX Arduino line is pin 2, make TX Arduino line is pin 3.
                             // This means that you need to connect the TX line from the esp to the Arduino's pin 2
                             // and the RX line from the esp to the Arduino's pin 3
#define led 13

int loadPin0 =   9;
int loadPin1 =   10;
int loadPin2 =   11;
int loadPin3 =   12;

int a=0,b=0,c=0,d=0;

 // NOTE in command like 1On O is not zero this is capital letter O 
    const char cmd_1On[6]={"1_1On"}; 
    const char cmd_1Of[6]={"1_1Of"};    //recieve data format 31 4F 66 0A
    const char cmd_2On[6]={"1_2On"};    //recieve data format 32 4F 6E 0A 
    const char cmd_2Of[6]={"1_2Of"};
    const char cmd_3On[6]={"1_3On"};
    const char cmd_3Of[6]={"1_3Of"};
    const char cmd_4On[6]={"1_4On"};
    const char cmd_4Of[6]={"1_4Of"};

    const char get_all_status[8]={"get_all"};
    
    const char cmd_sta1[9]=  {"station1"};
    const char cmd_sta2[9]=  {"station2"};
    const char cmd_sta3[9]=  {"station3"};
    const char cmd_server[7]={"server"};
 

    /////***********************************////

void beep(){
  digitalWrite(led,HIGH);  
 delay(25);
   digitalWrite(led,LOW);  
 delay(25);
}

//<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>//

void rx_empty(void) 
{
    while(Serial.available() > 0) {
        char inchar=Serial.read();
    }
}


//<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//
//><<<<<<<<<<<<<<<>>>>>>>>>????????????????????//

//uint32_t esp_recvPkg(uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len, uint32_t timeout, uint8_t *coming_mux_id)
uint32_t esp_recvPkg(uint8_t *buffer, uint32_t buffer_size, uint32_t *data_len, uint32_t timeout, uint8_t *coming_mux_id)
{
    String data;
    char a;
    int32_t index_PIPDcomma = -1;
    int32_t index_colon = -1; /* : */
    int32_t index_comma = -1; /* , */
    int32_t len = -1;
    int8_t id = -1;
    bool has_data = false;
    uint32_t ret;
    unsigned long start;
    uint32_t i;
    
    if (buffer == NULL) {
        return 0;
    }
    
    start = millis();
    a = Serial.read();
    data += a;
    while (millis() - start < timeout) {
        if(Serial.available() > 0) {
            a = Serial.read();
         //   Serial.write(a);
            data += a;
        }
       //Serial.println();
        index_PIPDcomma = data.indexOf("+IPD,");
        if (index_PIPDcomma != -1) {
            index_colon = data.indexOf(':', index_PIPDcomma + 5);
            if (index_colon != -1) {
                index_comma = data.indexOf(',', index_PIPDcomma + 5);
                /* +IPD,id,len:data */
                if (index_comma != -1 && index_comma < index_colon) { 
                    id = data.substring(index_PIPDcomma + 5, index_comma).toInt();
                    if (id < 0 || id > 4) {
                        return 0;
                    }
                    len = data.substring(index_comma + 1, index_colon).toInt();
                    if (len <= 0) {
                        return 0;
                    }
                } else { /* +IPD,len:data */
                    len = data.substring(index_PIPDcomma + 5, index_colon).toInt();
                    if (len <= 0) {
                        return 0;
                    }
                }
                has_data = true;
                break;
            }
        }
    }
    
    if (has_data) {
        i = 0;
        ret = len > buffer_size ? buffer_size : len;
        start = millis();
        while (millis() - start < 3000) {
            while(Serial.available() > 0 && i < ret) {
                a = Serial.read();
                buffer[i++] = a;
            }
            if (i == ret) {
                rx_empty();
                if (data_len) {
                    *data_len = len;    
                }
                if (index_comma != -1 && coming_mux_id) {
                    *coming_mux_id = id;
                }
                return ret;
            }
        }
    }
    return 0;
}
//<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>//

uint32_t esp_recv(uint8_t mux_id, uint8_t *buffer, uint32_t buffer_size, uint32_t timeout)
{
    uint8_t id;
    uint32_t ret;
    ret = esp_recvPkg(buffer, buffer_size, NULL, timeout, &id);
    if (ret > 0 && id == mux_id) {
        return ret;
    }
    return 0;
}


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>//
//<<<<<<<<<<<<<<<<<<<<<<<<<<<//
String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    
    int dataSize = command.length();
    char data[dataSize];
    command.toCharArray(data,dataSize);
           
    Serial.write(data,dataSize); // send the read character to the esp8266
    if(debug)
    {
     beep();
    }
    
    long int time = millis();
    
    while( (time+timeout) > millis())
    {
      while(Serial.available())
      {
        
        // The esp has data so display its output to the serial window 
        char c = Serial.read(); // read the next character.
        response+=c;
      }  
    }
    
    if(debug)
    {
    //  Serial.print(response);
    //    beep();
        beep();
    }
    
    return response;
}

/*
* Name: sendHTTPResponse
* Description: Function that sends HTTP 200, HTML UTF-8 response
*/
void sendHTTPResponse(int connectionId, String content)
{
     
     // build HTTP response
     String httpResponse;
     String httpHeader;
     // HTTP Header
     httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n"; 
     httpHeader += "Content-Length: ";
     httpHeader += content.length();
     httpHeader += "\r\n";
     httpHeader +="Connection: close\r\n\r\n";
     httpResponse = httpHeader + content+ " "; // There is a bug in this code: the last character of "content" is not sent, I cheated by adding this extra space
     sendCIPData(connectionId,httpResponse);
   /*  
     bool espdata=0;
     while(!espdata){
     esp_CIPSENDMultiple(connectionId,httpResponse);
     if(espdata) {
      beep();
      }else {
        delay(2000);
      }
    }
    */
}

/*
* Name: sendCIPDATA
* Description: sends a CIPSEND=<connectionId>,<data> command
*
*/
void sendCIPData(int connectionId, String data)
{
   String cipSend = "AT+CIPSEND=";
   cipSend += connectionId;
   cipSend += ",";
   cipSend +=data.length();
   cipSend +="\r\n";   
   sendCommand(cipSend,100,DEBUG);
   sendData(data,100,DEBUG);
}

/*
* Name: sendCommand
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/
String sendCommand(String command, const int timeout, boolean debug)
{
    String response = "";
           
    Serial.print(command); // send the read character to the esp8266
    
    long int time1 = millis();
    
    while( (time1+timeout) > millis())
    {
      while(Serial.available())
      {
        
        // The esp has data so display its output to the serial window 
        char c = Serial.read(); // read the next character.
        response+=c;
      }  
    }
    
    if(debug)
    {
    //  Serial.print(response);
      beep();
   //   beep();
    }
    
    return response;
}


//<<<<<<<<<<<<<<<<<<<<Recieve data from esp with timeout<<>>>>>>>>>>>>>>>>>>>//


                      
void setup()
{
  Serial.begin(115200);
  esp8266.begin(9600); // your esp's baud rate might be different

  
  pinMode(loadPin0,OUTPUT);
  pinMode(loadPin1,OUTPUT);
  pinMode(loadPin2,OUTPUT);
  pinMode(loadPin3,OUTPUT);
  pinMode(led,OUTPUT);
  digitalWrite(led,LOW);  
  digitalWrite(loadPin0,LOW);    
  digitalWrite(loadPin1,LOW); 
  digitalWrite(loadPin2,LOW);  
  digitalWrite(loadPin3,LOW);    
     
  pinMode(11,OUTPUT);
  digitalWrite(11,LOW);
  
  pinMode(12,OUTPUT);
  digitalWrite(12,LOW);
  
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  
  pinMode(10,OUTPUT);
  digitalWrite(10,LOW);
   
  sendCommand("AT+RST\r\n",1000,DEBUG); // reset module
  delay(2000);
  beep();
  sendCommand("AT+CWMODE=3\r\n",1000,DEBUG); // configure as access point
  beep();
  //sendCommand("AT+CWJAP=\"JioFi3_091879\",\"24b2thsc8e\"\r\n",3000,DEBUG);
 // sendCommand("AT+CWJAP=\"ACCESS_POINT1\",\"11221234\"\r\n",3000,DEBUG);
  beep();
  //delay(3000);
  sendCommand("AT+CIFSR\r\n",1000,DEBUG); // get ip address
  beep();
  sendCommand("AT+CIPMUX=1\r\n",1000,DEBUG); // configure for multiple connections
  beep();
  sendCommand("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // turn on server on port 80
  beep();
  sendCommand("AT+CIPSTO=360\r\n",1000,DEBUG); // turn on server on port 80
  beep();
  digitalWrite(led,HIGH); 
//  Serial.println("Server Ready");
}

//**********************************************//

void loop()
{
  uint8_t inchar='\0';
  uint8_t buffer[255] = {0};
  char rcv_cmd[20]={0};
  unsigned int mux_id=0;
  
  if(Serial.available()) // check if the esp is sending a message 
  {
   /* uint32_t len = esp_recv(mux_id, buffer, sizeof(buffer), 10000);
    for(uint32_t i = 0; i < len;) {    
          inchar=(char)buffer[i];       
          if(inchar!=0x0a) 
          rcv_cmd[i]=inchar;   
          Serial.print(rcv_cmd[i]);
          i++;
          rcv_cmd[i]='\0';
        }
      // Serial.print("\r\n");   
   */
  if(Serial.find("+IPD,"))
    {
     delay(100); // wait for the serial buffer to fill up (read all the serial data)
     // get the connection id so that we can then disconnect
     int connectionId = Serial.read()-48; // subtract 48 because the read() function returns   
             
      if(Serial.find("GET"))
       {  
         char null_chr=Serial.read(); //read space
              null_chr=Serial.read(); //read / char
         for(uint8_t i = 0; inchar!=' ';) {    
          inchar= Serial.read();  
          if(inchar!=' '){    
          rcv_cmd[i]=inchar;   
          Serial.print(rcv_cmd[i]);
          }
          i++;
          rcv_cmd[i]='\0';
        }

      
    //*************LOAD0 TO SWITCH ON******************.//   
    if(strcmp(cmd_1On,rcv_cmd)==0){
      //  Serial.println("load0 switch to on"); 
        beep();
        a=1;
        digitalWrite(loadPin0,HIGH); 
       
       }
    //*************LOAD1 TO SWITCH ON****************.//    
    else if(strcmp(cmd_2On,rcv_cmd)==0){
      //  Serial.println("load1 switch to on");
        b=1;
        beep(); 
        digitalWrite(loadPin1,HIGH); 
       
       }
    //......LOAD2 TO SWITCH ON..........>>///   
    else if(strcmp(cmd_3On,rcv_cmd)==0){
       // Serial.println("load2 switch to on"); 
        c=1;
        beep();
        digitalWrite(loadPin2,HIGH); 
        
       }
    //*************LOAD3 TO SWITCH ON******************.//    
    else if(strcmp(cmd_4On,rcv_cmd)==0){
       // Serial.println("load3 switch to on"); 
        d=1;
        beep();
        digitalWrite(loadPin3,HIGH); 
       
       }    
    //*************LOAD0 TO SWITCH OFF******************.//    
    else if(strcmp(cmd_1Of,rcv_cmd)==0){
       // Serial.println("load0 switch to off"); 
        a=0;
        beep();
        digitalWrite(loadPin0,LOW); 
      
     }
   //*************LOAD1 TO SWITCH OFF******************.//
    else if(strcmp(cmd_2Of,rcv_cmd)==0){
        //Serial.println("load1 switch to off"); 
         b=0;
         beep();
        digitalWrite(loadPin1,LOW); 
        
     }
   //*************LOAD2 TO SWITCH OFF******************.//
    else if(strcmp(cmd_3Of,rcv_cmd)==0){
        //Serial.println("load2 switch to off"); 
         c=0;
         beep();
        digitalWrite(loadPin2,LOW); 
     
     }
   //*************LOAD3 TO SWITCH OFF******************./
    else if(strcmp(cmd_4Of,rcv_cmd)==0){
        //Serial.println("load3 switch to off"); 
        d=0;
        beep();
        digitalWrite(loadPin3,LOW);        
      }
    else if(strcmp(get_all_status,rcv_cmd)==0){
      String sss = String(a)+String(b)+String(c)+String(d);
      // beep();
     sendHTTPResponse(mux_id,sss); 
       
     }
    if(strcmp(get_all_status,rcv_cmd)!=0){  
     //  beep();
    sendHTTPResponse(mux_id,rcv_cmd);  
    }

    
// delay(100);
 String closeCommand = "AT+CIPCLOSE="; 
 closeCommand+=mux_id; // append connection id
 closeCommand+="\r\n";     
 sendCommand(closeCommand,100,DEBUG); // close connection    
 digitalWrite(led,HIGH);  
    }

   }  
  }
 }
/*
if(Serial.available()) // check if the esp is sending a message 
  {
    if(Serial.find("+IPD,"))
    {
     delay(1000); // wait for the serial buffer to fill up (read all the serial data)
     // get the connection id so that we can then disconnect
     int connectionId = Serial.read()-48; // subtract 48 because the read() function returns 
                                           // the ASCII decimal value and 0 (the first decimal number) starts at 48
          
     Serial.find("pin="); // advance cursor to "pin="
          
     int pinNumber = (Serial.read()-48); // get first number i.e. if the pin 13 then the 1st number is 1
     int secondNumber = (Serial.read()-48);
     if(secondNumber>=0 && secondNumber<=9)
     {
      pinNumber*=10;
      pinNumber +=secondNumber; // get second number, i.e. if the pin number is 13 then the 2nd number is 3, then add to the first number
     }
     
     digitalWrite(pinNumber, !digitalRead(pinNumber)); // toggle pin    
     
     // build string that is send back to device that is requesting pin toggle
     String content;
     content = "Pin ";
     content += pinNumber;
     content += " is ";
     
     if(digitalRead(pinNumber))
     {
       content += "ON";
     }
     else
     {
       content += "OFF";
     }
     
    }
/*
* Name: sendData
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/

