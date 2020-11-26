#include "dpf.hpp"

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SLK, TFT_RST, TFT_MISO);

char rxData[READ_BUFFER_SIZE];
uint8_t rxIndex = 0;

int rpm, LDR, LDRMAX, LDRMIN, EGT, turboRAW, reading, COOLANT, x, engineLoad, oilTemp;
uint32_t speed;
byte INTEMP, CACT, SPEED;
float BATTERY, turboPRESS, turboMAX;

unsigned long time_now2 = 0;
unsigned long period2 = 10000; 
unsigned long time_now5 = 0;
unsigned long period5 = 30000; 
unsigned long delayTime = 0;   

uint16_t SMC;
uint32_t km;
uint8_t percentFap;

char iBuf[16] = {};
char eBuf[16] = {};
char cBuf[16] = {};
char oBuf[16] = {};


BluetoothSerial SerialBT;

String name = "obd-2";
String pin = "1234";

String WIFI_CLIENT;
String WIFI_CLIENT_PASS;
String LOCAL_NAME;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  //Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file)
  {
    //Serial.println("- failed to open file for writing");
    return;
  }

  file.print(message);
}

String readFile(fs::FS &fs, const char *path)
{
  //Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file)
  {
    Serial.println("- empty file or failed to open file");

    return String();
  }
  //Serial.println("- read from file:");
  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
  //Serial.println(fileContent);
  return fileContent;
}

void handleRoot(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", FRM_PASS);
}

void handleSave(AsyncWebServerRequest *request)
{
  if (request->hasParam(PARAM_INPUT_1))
  {
    WIFI_CLIENT = request->getParam(PARAM_INPUT_1)->value();
    writeFile(SPIFFS, "/wifi1", WIFI_CLIENT.c_str());
  }

  if (request->hasParam(PARAM_INPUT_2))
  {
    WIFI_CLIENT_PASS = request->getParam(PARAM_INPUT_2)->value();
    writeFile(SPIFFS, "/wifi2", WIFI_CLIENT_PASS.c_str());
  }

  if (request->hasParam(PARAM_INPUT_3))
  {
    LOCAL_NAME = request->getParam(PARAM_INPUT_3)->value();
    writeFile(SPIFFS, "/wifi3", LOCAL_NAME.c_str());
  }

  char buffer[256];
  snprintf(buffer, 256, "%s %s %s", WIFI_CLIENT.c_str(), WIFI_CLIENT_PASS.c_str(), LOCAL_NAME.c_str());

  request->send_P(200, "text/html", buffer);
}

void getStoredParams()
{
  WIFI_CLIENT = readFile(SPIFFS, "/wifi1");
  WIFI_CLIENT_PASS = readFile(SPIFFS, "/wifi2");
  LOCAL_NAME = readFile(SPIFFS, "/wifi3");

  Serial.print("WIFI_CLIENT: ");
  Serial.println(WIFI_CLIENT);
  Serial.print("WIFI_CLIENT_PASS: ");
  Serial.println(WIFI_CLIENT_PASS);
  Serial.print("LOCAL_NAME: ");
  Serial.println(LOCAL_NAME);
  if (LOCAL_NAME == "")
  {
    LOCAL_NAME = "obd-2";
  }

  if (WIFI_CLIENT == "")
  {
    WIFI_CLIENT = "1234";
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    //handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }
  Serial.println("Get params()");
  getStoredParams();
  Serial.println("OK!");
  name = LOCAL_NAME;
  pin = WIFI_CLIENT;
  Serial.println("ACCESS POINT MODE");
  Serial.print("Configuring access point...");
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_GET, handleSave);
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();

  SerialBT.begin("dpfread", true);
  SerialBT.setPin(pin.c_str());

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_WHITE);

  tft.setCursor(250, 230);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  tft.print(VERSION);

  tft.setCursor(0, 20);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);

  tft.print("WIFI: ");
  tft.print(ssid);
  tft.print("\n\nPASS: ");
  tft.print(password);
  tft.print("\n\n");

  tft.print("Bluetooth: ");
  tft.print(name);
  tft.print("\n\nPIN: ");
  tft.print(pin);
  tft.print("\n\n");

  delay(5000);

  bool connected = SerialBT.connect(name);
  while (connected == false)
  {
    Serial.println("Connecting...");
    delay(500);
  }
  tft.fillScreen(ILI9341_WHITE);
  tft.setCursor(0, 10);
  tft.setTextSize(1);
  //SerialBT.flush();
  Serial.println("Init ODB");
  tft.print("Init ELM327\n\n");
  initODB();
  tft.fillScreen(ILI9341_BLACK);

  tft.drawLine(0, 70, 320, 70, ILI9341_RED);
  tft.drawLine(0, 125, 320, 125, ILI9341_RED);
  tft.drawLine(0, 185, 320, 185, ILI9341_RED);
  tft.drawLine(140, 0, 140, 185, ILI9341_RED);

  drawScreen(false);
}

void loop()
{
  ws.cleanupClients();
  getRpm();
  getSpeed();
  getTurboPressure();
  getEngineLoad();

  if (millis() > time_now2 + period2)
  {
    //getIntakeTemperature();
    getCoolant();
    getOilTemp();
    time_now2 = millis();
    //drawScreen(false);
  }
  else if (millis() > time_now5 + period5)
  {
    getSMC();
    time_now5 = millis();
    drawScreen(false);
  }
  else
  {
    drawScreen(true);
  }
}

void initODB()
{
  delay(0); //1000
  SEND_BLE("ATZ\r");
  delay(500); //2000
  readOBD();
  SEND_BLE("ATD\r");
  delay(500); //2000
  readOBD();
  tft.print(rxData);
  tft.print("\n");
  SEND_BLE("ATE0\r");
  delay(500); //1000
  readOBD();
  tft.print(rxData);
  tft.print("\n");
  SEND_BLE("ATSP6\r");
  delay(500); //1000
  readOBD();
  tft.print(rxData);
  tft.print("\n");
  SEND_BLE("ATSH7E0\r");
  delay(700);
  readOBD();
  tft.print(rxData);
  tft.print("\n");
  //SerialBT.flush();
}

void drawScreen(bool half)
{
  static int randomColor = 0;
 

  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(7);
  snprintf(oBuf, sizeof(oBuf), "%03d", speed);
  tft.print(oBuf);

  tft.setCursor(150, 10);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setTextSize(7);
  snprintf(oBuf, sizeof(oBuf), "%04d", rpm);
  tft.print(oBuf);

  tft.setCursor(10, 75);
  tft.setTextSize(2);
  tft.print("P. TURBO");

  tft.setCursor(10, 95);
  tft.setTextSize(3);
  tft.print(turboPRESS, 2);

  tft.setTextSize(2);
  tft.setCursor(160, 75);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.print("CARGA MOTOR");

  tft.setTextSize(3);
  tft.setCursor(160, 95);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.print(engineLoad);
  tft.print("%");

  if (randomColor % 2)
  {
    tft.fillCircle(310, 230, 3, ILI9341_BLACK);
  }
  else
  {
    tft.fillCircle(310, 230, 3, ILI9341_RED);
  }

  randomColor++;

  if (half == true)
  {
    return;
  }

  /* Not supported :(
  tft.setCursor(10, 130);
  tft.setTextSize(2);
  tft.print("ACEITE");
  tft.setTextSize(3);
  tft.setCursor(10, 150);
  snprintf(oBuf, sizeof(oBuf), "%03d", oilTemp);
  tft.print(oBuf);
  tft.println("c");

  tft.setCursor(160, 130);
  tft.setTextSize(2);
  tft.print("REFRIGERANTE");
  tft.setTextSize(3);
  tft.setCursor(160, 150);
  snprintf(oBuf, sizeof(oBuf), "%03dc", COOLANT);
  tft.print(oBuf);

  tft.setCursor(210, 190);
  tft.setTextSize(2);
  tft.print("ESTADO");

  tft.setCursor(210, 210);
  tft.setTextSize(3);
  tft.print(percentFap);

  float SMCF = SMC;
  SMCF = SMCF / 100;
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(100, 190);
  tft.print("SMC g");
  tft.setTextSize(3);
  char buffer[16];
  snprintf(buffer, 16, "%.2f", SMCF);
  tft.setCursor(100, 210);
  tft.print(buffer);

  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 190);
  tft.print("KM FAP");
  tft.setTextSize(3);
  tft.setCursor(10, 210);
  snprintf(buffer, 16, "%03d", km);
  tft.print(buffer);
}

void getRpm()
{
  SerialBT.flush();
  SEND_BLE("010C\r");
  delay(delayTime);
  readOBD();
  rpm = ((strtol(&rxData[6], 0, 16) * 256) + strtol(&rxData[9], 0, 16)) / 4;
}

void getEngineLoad()
{
  SerialBT.flush();
  SEND_BLE("0104\r");
  delay(delayTime);
  readOBD();
  engineLoad = (strtol(&rxData[6], 0, 16) * 100) / 255;
}

void getCoolant()
{
  SerialBT.flush();
  SEND_BLE("0105\r");
  delay(delayTime);
  readOBD();
  COOLANT = strtol(&rxData[6], 0, 16) - 40;
}

//
void getOilTemp()
{
  SerialBT.flush();
  SEND_BLE("015C\r");
  delay(delayTime);
  readOBD();
  oilTemp = strtol(&rxData[6], 0, 16) - 40;
}

void getIntakeTemperature()
{
  SerialBT.flush();
  SEND_BLE("010F\r");
  delay(delayTime);
  readOBD();
  INTEMP = strtol(&rxData[6], 0, 16) - 40;
  tft.setCursor(220, 0);
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.print("T. AIRE");
  tft.setTextSize(3);
  tft.setCursor(225, 25);
  snprintf(iBuf, sizeof(iBuf), "%3d", INTEMP);
  tft.println(iBuf);
  tft.setCursor(290, 25);
  tft.println("c");
  tft.drawCircle(285, 25, 3, ILI9341_GREEN);
}

void getCACT()
{
  SerialBT.flush();
  SEND_BLE("0177\r");
  delay(delayTime);
  readOBD();
  CACT = strtol(&rxData[9], 0, 16) - 40;
  tft.setCursor(120, 60);
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.print("T ICOOL");
  tft.setTextSize(3);
  tft.setCursor(125, 85);
  snprintf(cBuf, sizeof(cBuf), "%3d", CACT);
  tft.println(cBuf);
  //tft.println(CACT)+tft.print((char)247)+tft.print("C");
  tft.setCursor(190, 85);
  tft.println("c");
  tft.drawCircle(185, 85, 3, ILI9341_GREEN);
}

void getBattery()
{
  SerialBT.flush();
  SEND_BLE("0142\r");
  delay(delayTime);
  readOBD();
  BATTERY = ((strtol(&rxData[6], 0, 16) * 256) + strtol(&rxData[9], 0, 16)); //((A*256)+B)/1000
  BATTERY = BATTERY / 1000;
  if (BATTERY < 12)
  {
    tft.setTextColor(ILI9341_BLACK, ILI9341_YELLOW);
  }
  else
  {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  }
  tft.setCursor(0, 60);
  tft.setTextSize(2);
  tft.print(BATTERY, 1);
  tft.println("v");
}

//SOOT MASS CALC
void getSMC()
{
  SerialBT.flush();
  SEND_BLE("ATSH7E0\r");
  delay(200);
  readOBD();
  SEND_BLE("0100\r");
  delay(200);
  readOBD();
  SEND_BLE("22114F\r");
  delay(200);
  readOBD();

   SMC = ((strtol(&rxData[9], 0, 16) * 256) + strtol(&rxData[12], 0, 16)); //((A*256)+B)/100

  //SerialBT.println(SMC);

  //KM
  SEND_BLE("221156\r");
  delay(200);
  readOBD();
  km = (strtol(&rxData[9], 0, 16) << 24) + (strtol(&rxData[12], 0, 16) << 16 ) +  (strtol(&rxData[15], 0, 16) << 8) + strtol(&rxData[18], 0, 16); 
  Serial.print("\nKM: ");
  SerialBT.println(km);
  km = km / 1000;

  //Percent
  SEND_BLE("22115B\r");
  delay(200);
  readOBD();


  percentFap = strtol(&rxData[9], 0, 16);
}

void getEGT()
{
  SerialBT.flush();
  SEND_BLE("0178\r");
  delay(100);
  readOBD();
  EGT = (((strtol(&rxData[30], 0, 16) * 256) + strtol(&rxData[33], 0, 16)) / 10) - 40;
  if (EGT < 350)
  {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.drawCircle(185, 25, 3, ILI9341_GREEN);
    tft.fillCircle(190, 7, 7, ILI9341_BLACK);
  }
  else if ((EGT >= 350) && (EGT < 450))
  {
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft.drawCircle(185, 25, 3, ILI9341_YELLOW);
    tft.fillCircle(190, 7, 7, ILI9341_YELLOW);
  }
  else if (EGT >= 450)
  {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.drawCircle(185, 25, 3, ILI9341_RED);
    tft.fillCircle(190, 7, 7, ILI9341_RED);
  }
  tft.setCursor(140, 0);
  tft.setTextSize(2);
  tft.print("EGT");
  tft.setTextSize(3);
  tft.setCursor(120, 25);
  snprintf(eBuf, sizeof(eBuf), "%3d", EGT);
  tft.println(eBuf);
  tft.setCursor(190, 25);
  tft.println("c");
}

int getSpeed()
{
  SerialBT.flush();
  SEND_BLE("010D\r");
  delay(delayTime);
  readOBD();
  speed = strtol(&rxData[6], 0, 16);
  return speed;
}

int getIntakePressure()
{
  SerialBT.flush();
  SEND_BLE("010B\r");
  delay(delayTime);
  readOBD();
  return strtol(&rxData[6], 0, 16);
}

int getBarometricPressure()
{
  /*Not supported
  SerialBT.flush();
  SEND_BLE("0133\r");
  delay(delayTime);
  readOBD();
  return strtol(&rxData[6], 0, 16);*/
  return 1; //Approx 1atm ~ 1bar
}

void getTurboPressure()
{
  turboRAW = (getIntakePressure() - getBarometricPressure());
  turboPRESS = turboRAW;
  turboPRESS = (turboPRESS * 0.01);
}

void readOBD()
{
  char c;
  do
  {
    if (SerialBT.available() > 0)
    {
      c = SerialBT.read();
      //lcd.print(c);
      if ((c != '>') && (c != '\r') && (c != '\n'))
      {
        rxData[rxIndex++] = c;
      }
      if (rxIndex > READ_BUFFER_SIZE)
      {
        rxIndex = 0;
      }
    }
  } while (c != '>'); //ELM327 response end
  rxData[rxIndex++] = '\0';
  rxIndex = 0;
  Serial.print("R:");
  Serial.println(rxData);
  ws.textAll("R:");
  ws.textAll(rxData);
  ws.textAll("\n");
}
