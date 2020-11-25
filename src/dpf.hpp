#define VERSION "0.1.0"

#include <Arduino.h>
#include <Wire.h>

#include <WiFi.h>
#include <WiFiGeneric.h>
#include <WiFiServer.h>
#include <WiFiMulti.h>
#include <WiFiScan.h>
#include <WiFiClient.h>
#include <WiFiSTA.h>
#include <WiFiAP.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include <WiFiType.h>

#include <SPIFFS.h>

#include <AsyncEventSource.h>
#include <StringArray.h>
#include <AsyncWebSocket.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <WebResponseImpl.h>
#include <AsyncJson.h>
#include <WebAuthentication.h>
#include <AsyncWebSynchronization.h>
#include <WebHandlerImpl.h>

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#include <stdint.h>


const int TFT_CS = 15;
const int TFT_DC = 4;
const int TFT_MOSI = 23;
const int TFT_SLK = 18;
const int TFT_RST = 2;
const int TFT_LED = -1;   
const int TFT_MISO = 19;   


const int READ_BUFFER_SIZE = 128;

const char *ssid = "DPFSCREEN";
const char *password = "CHANGE_ME_PLEASE";

static const char* PARAM_INPUT_1 = "fname";
static const char* PARAM_INPUT_2 = "lname";
static const char* PARAM_INPUT_3 = "local";


static const char FRM_PASS[] PROGMEM = R"rawliteral(<html>
<meta name='viewport' content='width=device-width, initial-scale=1'>


<body style='background-color:#161317; color: white'>
<br/><br/>
    <form action='/save'><label for='local'>Nombre Bluetooth</label><br><input type='text' id='local' name='local'
            value='' maxlength='32'><br><label for='fname'>PIN Bluetooth</label><br><input type='text' id='fname'
            name='fname' value=''><br><br><br><input type='submit' value='Guardar'>
            
    <p>Bluetooth serial dump:</p>
    <p>
        <textarea name="serial" id="serial" rows="10" cols="40"></textarea>
    </p>
    
</body>

<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; 
  }
  function onOpen(event) {
    console.log('Connection opened');
  }

  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  
  function onMessage(event) {
    var txtArea = document.getElementById('serial');
    txtArea.innerHTML = txtArea.innerHTML + event.data;
    txtArea.scrollTop = txtArea.scrollHeight;
  }
  
  
  
  function onLoad(event) {
    initWebSocket();
  }
  function toggle(){
    websocket.send('toggle');
  }
  
  window.addEventListener('load', onLoad);
</script>

</html>
)rawliteral";



#define SEND_BLE(X) {\
  SerialBT.print(X);\
  Serial.print("S:");\
  Serial.println(X);\
  ws.textAll(String(X));\
}


void writeFile(fs::FS &fs, const char * path, const char * message);
String readFile(fs::FS &fs, const char * path);


void getStoredParams();
void handleRoot(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

void initODB();
void readOBD();

void drawScreen(bool half);
void getRpm();
void getEngineLoad();
void getCoolant();
void getOilTemp();
void getIntakeTemperature();
void getCACT();
void getBattery(); 
void getSMC();
void getEGT();
int getSpeed();
int getIntakePressure();
int getBarometricPressure();
void getTurboPressure();
