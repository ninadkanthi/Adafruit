/*
  This example creates a client object that connects and transfers
  data using always SSL.

  It is compatible with the methods normally related to plain
  connections, like client.connect(host, port).

  Written by Arturo Guadalupi
  last revision November 2015

*/

#include <SPI.h>
#include <Adafruit_WINC1500.h>

// Define the WINC1500 board connections below.
// If you're following the Adafruit WINC1500 board
// guide you don't need to modify these:
#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2     // or, tie EN to VCC and comment this out
// The SPI pins of the WINC1500 (SCK, MOSI, MISO) should be
// connected to the hardware SPI port of the Arduino.
// On an Uno or compatible these are SCK = #13, MISO = #12, MOSI = #11.
// On an Arduino Zero use the 6-pin ICSP header, see:
//   https://www.arduino.cc/en/Reference/SPI

// Setup the WINC1500 connection with the pins above and the default hardware SPI.
Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);

// Or just use hardware SPI (SCK/MOSI/MISO) and defaults, SS -> #10, INT -> #7, RST -> #5, EN -> 3-5V
//Adafruit_WINC1500 WiFi;


static const char ssid[] = "KANTHIHOME-2G";
static const char pass[] = "[Insert your Key]";
//char ssid[] = "yourNetwork"; //  your network SSID (name)
//char pass[] = "secretPassword";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

char hostname[] = "nkarduino.azure-devices.net";    // host name address for your Azure IoT Hub
char azureReceive[] = "/devices/nk_arduino_device_1/messages/devicebound?api-version=2016-02-03"; //feed URI
char azurePost[] = "/devices/nk_arduino_device_1/messages/events?api-version=2016-02-03"; //Post Request URI
char authSAS[] = "[Insert your Key]";


// message Complete/Reject/Abandon URIs.  "etag" will be replaced with the message id E-Tag recieved from recieve call.
String azureComplete = "/devices/nk_arduino_device_1/messages/devicebound/etag?api-version=2016-02-03";
String azureReject   = "/devices/nk_arduino_device_1/messages/devicebound/etag?reject&api-version=2016-02-03";
String azureAbandon  = "/devices/nk_arduino_device_1/messages/devicebound/etag/abandon?&api-version=2016-02-03";

String JsonPayload = "[{\"DeviceId\":\"nk_arduino_device_1\",\"Temperature\":35.91369848265952,\"Humidity\":26.573888225282349,\"ExternalTemperature\":null}]Properties:'EventId': '954c33b0-6ce2-4596-ae7e-ad3d63251fde'";


unsigned long lastConnectionTime = 0;
const unsigned long pollingInterval = 5L * 1000L; // 5 sec polling delay, in milliseconds
int status = WL_IDLE_STATUS;

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
// char server[] = "www.google.com";    // name address for Google (using DNS)

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
Adafruit_WINC1500SSLClient client;

void setup() {
#ifdef WINC_EN
  //pinMode(WINC_EN, OUTPUT);
  //digitalWrite(WINC_EN, HIGH);
  pinMode(WINC_EN, INPUT_PULLUP);
#endif

  //Initialize serial and wait for port to open:
  Serial.begin(152000);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    uint8_t timeout = 10;
    while (timeout && (WiFi.status() != WL_CONNECTED)) {
      timeout--;
      delay(1000);
    }
  }

  Serial.println("Connected to wifi");
  printWifiStatus();

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:

   azureIoTPostTelemetryMessage();
}


void loop() {
  String response = "";
  char c;
  ///read response if WiFi Client is available
  while (client.available()) {
    c = client.read();
    response.concat(c);
  }

  if (!response.equals(""))
  {
    Serial.println(response);
    //if there are no messages in the IoT Hub Device queue, Azure will return 204 status code.
    if (response.startsWith("HTTP/1.1 204"))
    {
      Serial.println("No Response from Hub");
    }
    else
    {
      //Now that the message is processed.. do either Complete, Reject or Abandon HTTP Call.
      //first get the ETag from the received message response
      String eTag = getHeaderValue(response, "ETag");

      //Uncomment the following line and comment out Reject and Abandon calls to verify Complete
      azureIoTCompleteMessage(eTag);

      //Uncomment the following line and comment out Complete and Abandon calls to verify Reject
      //azureIoTRejectMessage(eTag);

      //Uncomment the following line and comment out Complete and Reject calls to verify Abandon
      //azureIoTAbandonMessage(eTag);

    }
  }

  // polling..if pollingInterval has passed
  if (millis() - lastConnectionTime > pollingInterval) {
    azureIoTReceiveMessage();
    // note the time that the connection was made:
    lastConnectionTime = millis();

  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();

    // do nothing forevermore:
    while (true);
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


void httpRequest(String verb, String uri, String contentType, String content)
{
  if (verb.equals("")) return;
  if (uri.equals("")) return;


  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(hostname, 443))
  {
    client.print(verb); //send POST, GET or DELETE
    client.print(" ");
    client.print(uri);  // any of the URI
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(hostname);  //with hostname header
    client.print("Authorization: ");
    client.println(authSAS);  //Authorization SAS token obtained from Azure IoT device explorer
    client.println("Connection: close");


    if (verb.equals("POST"))
    {
      client.print("Content-Type: ");
      client.println(contentType);
      client.print("Content-Length: ");
      client.println(content.length());
      client.println();
      client.println(content);
    }
    else
    {
      client.println();
    }
  }
  //else
  //{
  //  Serial.println("SSL connection failed");
  //}
}


//Receive Azure IoT Hub "cloud-to-device" message
void azureIoTReceiveMessage()
{
  httpRequest("GET", azureReceive, "", "");
}

//Tells Azure IoT Hub that the message with the msgLockId is handled and it can be removed from the queue.
void azureIoTCompleteMessage(String eTag)
{
  String uri = azureComplete;
  uri.replace("etag", trimETag(eTag));

  httpRequest("DELETE", uri, "", "");
}


//Tells Azure IoT Hub that the message with the msgLockId is rejected and can be moved to the deadletter queue
void azureIoTRejectMessage(String eTag)
{
  String uri = azureReject;
  uri.replace("etag", trimETag(eTag));

  httpRequest("DELETE", uri, "", "");
}

//Tells Azure IoT Hub that the message with the msgLockId is abondoned and can be requeued.
void azureIoTAbandonMessage(String eTag)
{
  String uri = azureAbandon;
  uri.replace("etag", trimETag(eTag));

  httpRequest("POST", uri, "text/plain", "");
}


//Tells Azure IoT Hub that the message with the msgLockId is abondoned and can be requeued.
void azureIoTPostTelemetryMessage()
{
  httpRequest("POST", azurePost, "text/plain", JsonPayload);
}



String getHeaderValue(String response, String headerName)
{
  String headerSection = getHeaderSection(response);
  String headerValue = "";

  int idx = headerSection.indexOf(headerName);

  if (idx >= 0)
  {
    int skip = 0;
    if (headerName.endsWith(":"))
      skip = headerName.length() + 1;
    else
      skip = headerName.length() + 2;

    int idxStart = idx + skip;
    int idxEnd = headerSection.indexOf("\r\n", idxStart);
    headerValue = response.substring(idxStart, idxEnd);
  }

  return headerValue;
}

//For some reason Azure IoT sets ETag string enclosed in double quotes
//and that's not in sync with its other endpoints. So need to remove the double quotes
String trimETag(String value)
{
  String retVal = value;

  if (value.startsWith("\""))
    retVal = value.substring(1);

  if (value.endsWith("\""))
    retVal = retVal.substring(0, retVal.length() - 1);

  return retVal;
}

//To get all the headers from the HTTP Response
String getHeaderSection(String response)
{
  int idxHdrEnd = response.indexOf("\r\n\r\n");

  return response.substring(0, idxHdrEnd);
}

//To get only the message payload from http response.
String getResponsePayload(String response)
{
  int idxHdrEnd = response.indexOf("\r\n\r\n");

  return response.substring(idxHdrEnd + 4);
}





