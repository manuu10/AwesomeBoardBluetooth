#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <FastLED.h>
#include <Arduino_JSON.h>

enum Mode
{
  MoonBoardMode = 1,
  LedTestingMode = 2,
};
enum TestMode
{
  Nothing = 0,
  RainbowMode = 1,
  TrailingMode = 2,
};

TestMode currentTestMode = TestMode::Nothing;
Mode currentMode = Mode::MoonBoardMode;
JSONVar myArray;
//FASTLED SETUP
#define DATA_PIN 12
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 198
CRGB leds[NUM_LEDS];

#define BRIGHTNESS 255
#define FRAMES_PER_SECOND 500
//FASTLED SETUP

//BLUETOOTH SETUP
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
uint8_t hue = 0;

#define SERVICE_UUID "4fb8a9be-c293-4459-a39f-ccf92a532eb5"
#define CHARACTERISTIC_UUID "fe1fce17-fa71-407c-b831-a3b6da3e1deb"

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
    String val = "";
    if (value.length() > 0)
    {

      Serial.println("**********");
      Serial.print("New value: ");

      for (int i = 0; i < value.length(); i++)
      {
        val += value[i];
      }
      Serial.print(val);

      Serial.println();
      Serial.println("**********");

      if (val == "moonMode")
        currentMode = Mode::MoonBoardMode;
      else if (val == "testMode")
        currentMode = Mode::LedTestingMode;
      else if (val == "rainbow")
        currentTestMode = TestMode::RainbowMode;
      else if (val == "trailing")
        currentTestMode = TestMode::TrailingMode;
      else if (val == "nothing")
        currentTestMode = TestMode::Nothing;
      else
        myArray = JSON.parse(val);
    }
  }
};
//BLUETOOTH SETUP

void setup()
{
  delay(3000);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  myArray = JSON.parse("[]");

  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("moonboard :)");
  BLEDevice::setMTU(500);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void handleBTRequest();
void Moonboard();

void loop()
{
  if (currentMode == Mode::MoonBoardMode)
  {
    Moonboard();
  }
  else if (currentMode == Mode::LedTestingMode)
  {
    if (currentTestMode == TestMode::RainbowMode)
    {
      fill_rainbow(leds, NUM_LEDS, hue, 7);
      EVERY_N_MILLISECONDS(20) { hue++; }
    }
    else if (currentTestMode == TestMode::TrailingMode)
    {
      // a colored dot sweeping back and forth, with fading trails
      fadeToBlackBy(leds, NUM_LEDS, 20);
      int pos = beatsin16(13, 0, NUM_LEDS - 1);
      leds[pos] += CHSV(hue, 255, 192);
    }
    else if (currentTestMode == TestMode::Nothing)
    {
      fadeToBlackBy(leds, NUM_LEDS, 30);
    }
  }

  FastLED.show();                          //show current led array setup
  FastLED.delay(1000 / FRAMES_PER_SECOND); //update on set frames per second

  handleBTRequest();
}

void handleBTRequest()
{
  if (deviceConnected)
  {
    EVERY_N_MILLISECONDS(20)
    {

      pCharacteristic->setValue(String(currentMode).c_str());
      pCharacteristic->notify();
    }
    EVERY_N_SECONDS(2)
    {
      Serial.println(currentMode);
    }
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");

    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected)
  {

    oldDeviceConnected = deviceConnected;
  }
}

void Moonboard()
{
  fadeToBlackBy(leds, NUM_LEDS, 50);
  for (int i = 0; i < myArray.length() - 1; i += 2)
  {
    CRGB holdCol = CRGB::Blue;
    if (int(myArray[i + 1]) == 1)
      holdCol = CRGB::Green;
    if (int(myArray[i + 1]) == 2)
      holdCol = CRGB::Red;
    int index = int(myArray[i]);
    int row = int(index / 11);
    int col = int(index % 11);
    if (row % 2 != 0)
    {
      index = (row + 1) * 11 - (col + 1);
    }

    leds[index] = holdCol;
  }
}