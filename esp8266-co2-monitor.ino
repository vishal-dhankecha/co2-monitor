// Load Wi-Fi library
#include <ESP8266WiFi.h>

#include <Adafruit_BMP280.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>




// Replace with your network credentials
const char* ssid = "AirTech";
const char* password = "12345678";


unsigned long ms_from_start =0;
unsigned long ms_previous  = 0;
unsigned long interval=1000;



Adafruit_BMP280 bmp;
// HardwareSerial SerialPort(2);

int packate[9];
int pi = 0;

#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Sensor Values
double temperature;
float pressure;
float altitude;
int co2;

// Set web server port number to 80
AsyncWebServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "off";
String output4State = "off";

// Assign output variables to GPIO pins
const int output5 = 5;
const int output4 = 4;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;



void BMP280() {
  temperature = bmp.readTemperature();
  pressure = bmp.readPressure() / 100;
  altitude = bmp.readAltitude();
}

void requestCO2() {
  // Serial.println("request");
  Serial.write(0x01);
}

void readCO2() {
  if (Serial.available())
  {
    int data = Serial.read();
    if (pi < 9) {
        packate[pi] = data;
    } else {
      pi = 0;
      co2 = packate[4]*256+packate[5];
    }
    pi++;

  }
}


void updpateDisplay() {
  // Serial.println("updating Display");
  display.clearDisplay();
  display.setTextSize(2);               // Normal 1:1 pixel scale
  display.setTextColor(1);  // Draw white text
  display.setCursor(0, 0);              // Start at top-left corner           // Start at top-left corner
  display.print(temperature);
  display.print((char)247);
  display.println("C");
  display.print(co2);
  display.println("ppm");
  display.print(pressure);
  display.println("bar");
  display.print(altitude);
  display.println("m");
  display.display();
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .value { font-size: 3.0rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>AirTech</h2>
  <p>
    <span class="dht-labels">Temperature</span> 
    <div><span id="temperature" class="value">%TEMPERATURE%</span><sup class="units">&deg;C</sup></div>
  </p>
  <p>
    <span class="dht-labels">Co2</span>
    <div><span id="co2" class="value">%CO2%</span><sub class="units">ppm</sub></div>
  </p>
  <p>
    <span class="dht-labels">Atmospheric Pressure</span>
    <div><span id="ap" class="value">%AP%</span><sub class="units">bar</sub></div>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("co2").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/co2", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("ap").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/ap", true);
  xhttp.send();
}, 1000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(temperature);
  }
  else if(var == "CO2"){
    return String(co2);
  }
  else if(var == "AP"){
    return String(pressure);
  }
  return String();
}

void setup() {
  Serial.begin(1200);
  
  // Initialize the output variables as outputs
  pinMode(output5, OUTPUT);
  pinMode(output4, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output5, LOW);
  digitalWrite(output4, LOW);

  WiFi.softAP(ssid, password);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperature).c_str());
  });
  server.on("/co2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(co2).c_str());
  });
  server.on("/ap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(pressure).c_str());
  });

  // Start server
  server.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // Serial.println(F("SSD1306 allocation failed"));
  }
  display.display();

  if (!bmp.begin(0x76)) {
    // Serial.println(F("BMP communication failed"));
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

}

void loop() {
  ms_from_start  = millis();

  readCO2();


  if (ms_from_start-ms_previous> interval){
    ms_previous=ms_from_start;
    BMP280();
    requestCO2();
    updpateDisplay();
  }
}
