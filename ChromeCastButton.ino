/*
 ChromeCast Arduino Button
 
 This sketch connects to a ChromeCast device and controls media playback using a button.
 This sketch requires an Arduino Ethernet shield and a button. The sketch has been tested with these buttons:
 1. Massive Arcade Button with LED: https://www.adafruit.com/products/1185
 2. Tactile Button switch: https://www.adafruit.com/products/367
 
 To control playback, you need to make a websocket connection to the current app running on the ChromeCast device.
 These steps are required in order:
 1. Check if there is an app running on the ChromeCast device and get its app id.
 2. Get the app status to get the connection service URL.
 3. Use the connection service URL to get the websocket address.
 4. Create a websocket connection.
 5. Send RAMP PLAY/STOP messages to control media playback.
 
 Author: Leon Nicholls
 https://github.com/entertailion/ChromeCast-Arduino-Button
 */

#include <SPI.h>
// http://arduino.cc/en/Reference/Ethernet
#include <Ethernet.h>
// https://github.com/entertailion/Arduino-Websocket
#include <WebSocketClient.h>


// Enter a MAC address for your ethernet shield (or keep the default value).
// Newer Ethernet shields have a MAC address printed on a sticker on the shield.
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// TODO Enter your ChromeCast device IP address:
IPAddress chromecast(192,168,0,22);

////////////////////////////////////////////////////////
// You shouldn't have to change anything below this line.

int chromeCastPort = 8008;

// Track the steps for determining the websocket address
int connecting = 0;
int doneAppRunning = 0;
int doneAppStatus = 0;
int doneWebSocketUrl = 0;

int ledPin = 7;      // Choose the pin for the LED for feedback
int buttonInput = 2; // Digital button input
int buttonState = 0; // Digital button state
int buttonAnalogInput = A0;
int debounce = 0;   // Analog input debounce counter

char responseString[100]; // string for each line of HTTP response data
int stringPos = 0; // string index counter
char value[100]; // variable for extracted values from HTTP responses

// Track media playing status
int playing = 1;

// Start of the sketch
void setup() {
  pinMode(ledPin, OUTPUT);      // declare LED as output for feedback
  pinMode(buttonInput, INPUT);
  digitalWrite(buttonAnalogInput, HIGH);     // Enable pullup on analog input

  // Open serial communications
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for the serial port to open
  }

  // Start the ethernet connection
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to get an IP address using DHCP"));
    // Do nothing forever
    for(;;)
      ;
  }

  Serial.print(F("Arduino IP address: "));
  Serial.println(Ethernet.localIP());

  // Wait for the ethernet shield to initialize
  delay(1000);

  // Flash the LED to show ethernet is ready
  digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);
  digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
}

// Add common HTTP headers for requests
void createRequestHeaders(EthernetClient client) {
  client.print(F("Host: "));
  client.println(chromecast);
  client.println(F("User-Agent: arduino-ethernet"));
  client.println(F("Connection: close"));
}

// Determine which app is running on the ChromeCast device
void determineAppRunning() {
  Serial.println(F("determineAppRunning..."));
  // Connect to the ChromeCast web server on port 8008
  EthernetClient client;
  if (client.connect(chromecast, chromeCastPort)) {
    // Make a HTTP GET request
    client.println(F("GET /apps HTTP/1.1"));
    createRequestHeaders(client);
    client.println();

    int statusCode = readResponse(client);
    if (statusCode==302) {
      // Success; expected HTTP 302 redirect
      doneAppRunning = 1;
    } 
    else {
      Serial.print(F("determineAppRunning: Invalid response: "));
      Serial.println(statusCode);
    }
  }
  else {
    Serial.println(F("determineAppRunning: Connection to ChromeCast device failed"));
  }
}

// Get the application information request for the app in XML
void getAppStatus() {
  Serial.print(F("getAppStatus..."));
  Serial.println(value);
  // Connect to the ChromeCast web server on port 8008
  EthernetClient client;
  if (client.connect(chromecast, chromeCastPort)) {
    // Make a HTTP GET request
    client.print(F("GET /apps/")); 
    client.print(value);
    client.println(F(" HTTP/1.1"));
    createRequestHeaders(client);
    client.println();

    int statusCode = readResponse(client);
    if (statusCode==200) {
      // Success; expected HTTP 200 OK
      doneAppStatus = 1;
    } 
    else {
      Serial.print(F("getAppStatus: Invalid response: "));
      Serial.println(statusCode);
    }
  }
  else {
    Serial.println(F("getAppStatus: Connection to ChromeCast device failed"));
  }
}


// Get the dynamic URL for the websocket connection
void getWebSocketUrl() {
  Serial.print(F("getWebSocketUrl..."));
  Serial.println(value);
  // Connect to the ChromeCast web server on port 8008
  EthernetClient client;
  if (client.connect(chromecast, chromeCastPort)) {
    // Make a HTTP POST request
    client.print(F("POST ")); 
    client.print(value);
    client.println(F(" HTTP/1.1"));
    createRequestHeaders(client);
    client.println(F("Content-Type: application/json"));
    client.println(F("Content-Length: 13"));
    client.println();
    client.println(F("{\"channel\":0}"));
    client.println();

    int statusCode = readResponse(client);
    if (statusCode==200) {
      // Success; expected HTTP 200 OK
      doneWebSocketUrl = 1;
    } 
    else {
      Serial.print(F("getWebSocketUrl: Invalid response: "));
      Serial.println(statusCode);
    }
  }
  else {
    Serial.println(F("getWebSocketUrl: Connection to ChromeCast device failed"));
  }
}

// Disconnect the client connection
void disconnectClient(EthernetClient client) {
  client.stop();
  client.flush();
}

// Make a websocket connection to the ChromeCast app
void createWebSocket() {
  Serial.print(F("createWebSocket..."));
  Serial.println(value);
  EthernetClient client;
  if (client.connect(chromecast, chromeCastPort)) {
    WebSocketClient webSocketClient;
    webSocketClient.path = value;
    // Set the ChromeCast device IP address as a string for the websocket host
    sprintf(responseString,"%d.%d.%d.%d\0", chromecast[0], chromecast[1], chromecast[2], chromecast[3]);
    webSocketClient.host = responseString;

    if (webSocketClient.handshake(client)) {
      Serial.println(F("Handshake succeeded"));
      if (playing==1) {
        playing = 0;
        webSocketClient.sendDataMasked("[\"ramp\",{\"type\":\"STOP\",\"cmd_id\":0}]");
      } 
      else {
        playing = 1;
        webSocketClient.sendDataMasked("[\"ramp\",{\"type\":\"PLAY\",\"cmd_id\":0}]");
      }

      // Disconnect after sending the command
      disconnectClient(client);
      return;

      /*
      // Else if we wanted to keep the connection live and send more commands
       String data;
       while (client.connected()) {
         data = webSocketClient.getData();
       
         if (data.length() > 0) {
           Serial.print(F("Received data: "));
           Serial.println(data);
       
           // Need to respond to pings with pongs otherwise device will drop connection
           if (data.equals("[\"cm\",{\"type\":\"ping\"}]")){
             webSocketClient.sendDataMasked("[\"cm\",{\"type\":\"pong\"}]");
           }
         }
       } 
       Serial.println(F("createWebSocket: Not connected."));
       */
    } 
    else {
      Serial.println(F("Handshake failed"));
    }
  }
  disconnectClient(client);
}

// Parse the various HTTP responses
int readResponse(EthernetClient client) {
  // Clear strings for variable data
  memset( &responseString, 0, 100 );
  memset( &value, 0, 100 );
  stringPos = 0;

  // Track HTTP headers
  int header = 0;
  // Track HTTP status code
  int statusCode = 0;
  // Track content data
  int hasContent = 0;
  // Track content data bytes count
  int content = 0;

  while(client.connected()){
    // Read HTTP response data if available
    if (client.available()) {
      char c = client.read();
      //Serial.print(c);
      if (c=='\n') {
        responseString[stringPos] = '\0';
        if (header==0) {
          // First HTTP response line is the status line:
          // HTTP/1.1 204 No Content
          stringPos = 9; 
          while (responseString[stringPos]!=' ' && responseString[stringPos]!='\n') {
            if (isdigit(responseString[stringPos]))
            {
              statusCode = statusCode*10 + (responseString[stringPos] - '0');
            }
            stringPos ++;
          }
        } 
        else {
          // Find location HTTP header (see isAppRunning()):
          // Location: http://192.168.0.22/apps/YouTube          
          if ( !strncmp( responseString, "Location:", 9 ) ) {
            // Extract location value
            memcpy(responseString, &responseString[10], strlen(responseString)-9);
            // Extract the app name
            int index = strlen(responseString)-1;
            while (index>0 && responseString[index]!='/') {
              index--;
            }
            memcpy(value, &responseString[index+1], strlen(responseString)-index-2);
          }   

          // Find content length HTTP header:
          // Content-Length: 0
          if ( !strncmp( responseString, "Content-Length:", 15 ) ) {
            memcpy(responseString, &responseString[16], strlen(responseString)-15);
            int length = atoi(responseString);
            if (length==0) {
              disconnectClient(client);
              return statusCode;
            }
            hasContent = 1;
            content = length + 2;
          }

          // Extract the connection service URL (see getAppStatus()):
          // <connectionSvcURL>http://192.168.0.22/connection/63da0e3b-4402-428a-92ca-8216c034efce</connectionSvcURL>
          int index = 0;
          // Trim string beginning whitespace characters
          while (index<strlen(responseString) && isspace(responseString[index])){
            index++;
          }
          memcpy(responseString, &responseString[index], strlen(responseString)-index);
          if ( !strncmp( responseString, "<connectionSvcURL>", 18 ) ) {
            memcpy(responseString, &responseString[18], strlen(responseString)-18);
            int index = 0;
            while (index<strlen(responseString) && responseString[index]!='<') {
              index++;
            }
            responseString[index] = '\0';
            // Extract the URI
            index = 0;
            int slashCount = 0;
            while (index<strlen(responseString)) {
              if (responseString[index]=='/') {
                slashCount++;
                if (slashCount==3) {
                  break;
                }
              }
              index++;
            }
            memcpy(value, &responseString[index], strlen(responseString)-index);
            disconnectClient(client);
            return statusCode;
          }
        }

        // Get ready to parse the next response line
        header++;
        stringPos = 0;
        // Clear the response string for the next line
        memset( &responseString, 0, 100 );
      } 
      else {
        // Collect reponse data
        responseString[stringPos] = c;
        stringPos ++;
      }

      // Parse the HTTP response content data
      if (hasContent==1) {
        content--;
        if (content<0) {
          // Extract JSON response (see getWebSocketUrl()):
          // {"URL":"ws://192.168.0.22:8008/session?8","pingInterval":5}
          if (responseString[0]=='{' && responseString[strlen(responseString)-1]=='}') {
            int index = 0;
            int quoteCount = 0;
            // Extract the websocket URL from the JSON response
            while (index<strlen(responseString)) {
              if (responseString[index]=='\"') {
                quoteCount++;
                // TODO better parsing
                if (quoteCount==3) {
                  break;
                }
              }
              index++;
            }
            memcpy(value, &responseString[index+1], strlen(responseString)-index);
            index = 0;
            while (index<strlen(value) && value[index]!='\"') {
              index++;
            }
            value[index] = '\0';
            // Extract the URI
            index = strlen(value)-1;
            while (index>0 && value[index]!='/') {
              index--;
            }
            memcpy(value, &value[index], strlen(value)-index+1);
          }

          // End of body content
          disconnectClient(client);
          return statusCode;
        }
      }
    }
  }
  disconnectClient(client);
  return statusCode;
}

// Keep running the sketch
void loop()
{
  // Important to do these in order
  if (doneWebSocketUrl==1) {
    doneWebSocketUrl=0;
    createWebSocket();
  } 
  else if (doneAppStatus==1) {
    doneAppStatus=0;
    getWebSocketUrl();
  } 
  else if (doneAppRunning==1) {
    doneAppRunning=0;
    getAppStatus();
  }


  // Digital button
/* TODO Uncomment if using a digital button
  buttonState = digitalRead(buttonInput);
  if (buttonState == HIGH) {
    // Button not pressed
    digitalWrite(ledPin, LOW);
    // TODO handle quick presses
    connecting = 0;
  } 
  else {
    // Button pressed
    digitalWrite(ledPin, HIGH);
    if (connecting==0) {
      connecting = 1;
      determineAppRunning();
    }
  }
*/

  // Button on analog input
  if(digitalRead(buttonAnalogInput) == HIGH) { // Button not pressed
    debounce = 0;               // Reset debounce counter
    digitalWrite(ledPin, LOW);
    // TODO handle quick presses
    connecting = 0;
    return;
  }

  if(++debounce >= 20) { // Debounced button press
    debounce = 0;        // Reset debounce counter
    digitalWrite(ledPin, HIGH);
    if (connecting==0) {
      connecting = 1;
      determineAppRunning();
    }
    while(digitalRead(buttonAnalogInput) == LOW); // Wait for button release
  }
}
