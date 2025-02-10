#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>

// WiFi credentials
char ssid[] = "TajColloseum";        // your network SSID (name)
char pass[] = "MelanzanaBharta";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key index number (needed only for WEP)

// Server and sensor setup
int status = WL_IDLE_STATUS;
WiFiServer server(80);
const int SENSOR_PIN = A0;
const int BUFFER_SIZE = 100;
int sensorBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// Function prototypes
void printWiFiStatus();
void handleSensorData(WiFiClient& client);
void serveWebApp(WiFiClient& client);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect
  }

  Serial.println("WiFi Web Server");

  // Initialize LED pins
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  // Check WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Connect to WiFi
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }

  Serial.println("Connected to WiFi");
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  server.begin();
  printWiFiStatus();

  // Initialize sensor buffer
  for(int i = 0; i < BUFFER_SIZE; i++) {
    sensorBuffer[i] = 0;
  }
}

void loop() {
  // Read sensor data
  sensorBuffer[bufferIndex] = analogRead(SENSOR_PIN);
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;

  // Handle client connections
  WiFiClient client = server.available();
  
  if (client) {
    Serial.println("New client");
    String currentLine = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Check if request is for sensor data
            if (currentLine.indexOf("GET /sensor") >= 0) {
              handleSensorData(client);
            } else {
              serveWebApp(client);
            }
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        // Handle LED control requests
        if (currentLine.endsWith("GET /RH")) digitalWrite(LEDR, HIGH);
        if (currentLine.endsWith("GET /RL")) digitalWrite(LEDR, LOW);
        if (currentLine.endsWith("GET /GH")) digitalWrite(LEDG, HIGH);
        if (currentLine.endsWith("GET /GL")) digitalWrite(LEDG, LOW);
        if (currentLine.endsWith("GET /BH")) digitalWrite(LEDB, HIGH);
        if (currentLine.endsWith("GET /BL")) digitalWrite(LEDB, LOW);
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }
}

void handleSensorData(WiFiClient& client) {
  StaticJsonDocument<1024> doc;
  JsonArray data = doc.createNestedArray("readings");
  
  for(int i = 0; i < BUFFER_SIZE; i++) {
    data.add(sensorBuffer[i]);
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  
  String jsonString;
  serializeJson(doc, jsonString);
  client.println(jsonString);
}

void serveWebApp(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  
  // Serve the React app using CDN
  client.println("<!DOCTYPE html>");
  client.println("<html><head><title>Sensor Dashboard</title>");
  client.println("<script src='https://unpkg.com/react@17/umd/react.production.min.js'></script>");
  client.println("<script src='https://unpkg.com/react-dom@17/umd/react-dom.production.min.js'></script>");
  client.println("<script src='https://unpkg.com/babel-standalone@6/babel.min.js'></script>");
  client.println("<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>");
  client.println("<style>");
  client.println(".container {max-width: 800px; margin: 0 auto; padding: 20px;}");
  client.println(".chart-container {margin-bottom: 20px;}");
  client.println(".controls {display: flex; justify-content: center; gap: 10px;}");
  client.println(".btn {padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer;}");
  client.println("</style></head><body>");
  client.println("<div id='root'></div>");
  
  // Embedded React application
  client.println("<script type='text/babel'>");
  client.println("const App = () => {");
  client.println("  const [data, setData] = React.useState([]);");
  client.println("  const [zoomLevel, setZoomLevel] = React.useState(1);");
  client.println("  const chartRef = React.useRef(null);");
  
  client.println("  React.useEffect(() => {");
  client.println("    const fetchData = async () => {");
  client.println("      const response = await fetch('/sensor');");
  client.println("      const json = await response.json();");
  client.println("      const storedData = JSON.parse(localStorage.getItem('sensorData') || '[]');");
  client.println("      const newData = [...storedData, ...json.readings];");
  client.println("      setData(newData);");
  client.println("      localStorage.setItem('sensorData', JSON.stringify(newData));");
  client.println("    };");
  client.println("    fetchData();");
  client.println("    const interval = setInterval(fetchData, 5000);");
  client.println("    return () => clearInterval(interval);");
  client.println("  }, []);");
  
  // Chart rendering and controls
  client.println("  return (");
  client.println("    <div className='container'>");
  client.println("      <div className='chart-container'>");
  client.println("        <canvas ref={chartRef}></canvas>");
  client.println("      </div>");
  client.println("      <div className='controls'>");
  client.println("        <button className='btn' onClick={() => setZoomLevel(z => z * 1.2)}>Zoom In</button>");
  client.println("        <button className='btn' onClick={() => setZoomLevel(z => z / 1.2)}>Zoom Out</button>");
  client.println("      </div>");
  client.println("      <div className='controls'>");
  client.println("        <button className='btn' onClick={() => fetch('/RH')}>Red LED</button>");
  client.println("        <button className='btn' onClick={() => fetch('/GH')}>Green LED</button>");
  client.println("        <button className='btn' onClick={() => fetch('/BH')}>Blue LED</button>");
  client.println("      </div>");
  client.println("    </div>");
  client.println("  );");
  client.println("};");
  
  client.println("ReactDOM.render(<App />, document.getElementById('root'));");
  client.println("</script></body></html>");
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}