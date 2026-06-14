

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

//  Access Point Configuration
const char* ap_ssid     = "S3_Seismic_Gateway";
const char* ap_password = "engineering123";

WebServer server(80);

//  SX1278 Gateway Pin Map (ESP32-S3 GPIOs) 
#define LORA_SS    10
#define LORA_RST   17
#define LORA_DIO0  16
#define SPI_SCK    12
#define SPI_MISO   13
#define SPI_MOSI   11

//  Hardware Audio Alert Pin 
#define BUZZER_PIN  4  // Long leg to GPIO 4, Short leg to GND

const int MAX_NODES = 10;
const unsigned long NODE_TIMEOUT = 15000; 

struct TelemetryNode {
  int id = -1;
  int seismic = 0;
  int sound = 0;
  String aiVerdict = "MONITORING_NOMINAL"; 
  unsigned long lastCheckIn = 0;
  bool active = false;
};

TelemetryNode networkMesh[MAX_NODES];
volatile bool processingNetworkData = false;

// Variables to handle non-blocking buzzer alert timing
unsigned long buzzerEndTime = 0;
bool buzzerActive = false;

// Helper functions for non-blocking active buzzer behavior
void triggerAudioAlert() {
  digitalWrite(BUZZER_PIN, HIGH);
  buzzerEndTime = millis() + 300; // Buzz sharply for 300 milliseconds
  buzzerActive = true;
}

void checkBuzzerTimeout() {
  if (buzzerActive && (millis() > buzzerEndTime)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }
}

void processIncomingData(int incomingID, int seismicVal, int soundVal, String incomingVerdict) {
  if (incomingID <= 0 || incomingID > MAX_NODES) return;

  int idx = incomingID - 1; 
  networkMesh[idx].id = incomingID;
  networkMesh[idx].seismic = seismicVal;
  networkMesh[idx].sound = soundVal;
  
  if (incomingVerdict.length() > 0) {
    networkMesh[idx].aiVerdict = incomingVerdict;
  }
  
  networkMesh[idx].lastCheckIn = millis();
  networkMesh[idx].active = true;
}

void verifyNodeHeartbeats() {
  unsigned long currentMillis = millis();
  for (int i = 0; i < MAX_NODES; i++) {
    if (networkMesh[i].active && (currentMillis - networkMesh[i].lastCheckIn > NODE_TIMEOUT)) {
      networkMesh[i].active = false;
    }
  }
}

//  HTTP SERVER HANDLER: MAIN DASHBOARD V3.1 
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>"; 
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>VARCHASVA SYSTEM ENGINE</title>";
  html += "<style>";
  html += "@import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Rajdhani:wght@500;600;700&display=swap');";
  html += "body{font-family:'Rajdhani',sans-serif; background:#04060b; color:#f8fafc; margin:0; padding:0; height:100vh; overflow:hidden;}";
  html += "#bg-grid{position:absolute; top:0; left:0; width:100%; height:100%; background-image:linear-gradient(rgba(56,189,248,0.015) 1px, transparent 1px), linear-gradient(90deg, rgba(56,189,248,0.015) 1px, transparent 1px); background-size:30px 30px; z-index:-1;}";
  
  html += ".menu-container{padding:40px; box-sizing:border-box; height:100vh; width:100%; position:relative; display:flex; flex-direction:column;}";
  html += "h1{font-family:'Orbitron',sans-serif; font-weight:900; font-size:24px; letter-spacing:2px; color:#f8fafc; text-transform:uppercase; margin:0;}";
  html += ".sub{color:#d4af37; font-family:'Orbitron',sans-serif; font-size:10px; letter-spacing:3px; text-transform:uppercase; margin-bottom:20px; margin-top:5px;}";
  
  html += ".view-screen{display:none; width:100%; height:calc(100vh - 130px); overflow-y:auto;}";
  
  html += "#screen-main.active-view{display:grid; grid-template-columns:1fr 440px; gap:40px;}";
  html += "#screen-active-list.active-view{display:flex; flex-direction:column; gap:20px;}";
  html += "#screen-node-focus.active-view{display:flex; flex-direction:column; gap:20px;}";
  
  html += ".left-panel{display:flex; flex-direction:column; gap:20px;}";
  html += ".btn-row{display:flex; gap:15px; margin-bottom:10px;}";
  
  html += ".nav-btn{font-family:'Orbitron',sans-serif; font-weight:700; font-size:12px; letter-spacing:1px; background:rgba(20,25,35,0.8); border:1px solid rgba(212,175,55,0.3); color:#d4af37; padding:14px 28px; cursor:pointer; text-transform:uppercase; transition:all 0.2s; clip-path:polygon(0 0, calc(100% - 12px) 0, 100% 12px, 100% 100%, 12px 100%, 0 calc(100% - 12px));}";
  html += ".nav-btn:hover{background:rgba(212,175,55,0.1); border-color:#d4af37; box-shadow:0 0 15px rgba(212,175,55,0.15); transform:translateY(-2px);}";
  html += ".back-btn{color:#94a3b8; border-color:rgba(148,163,184,0.3);} .back-btn:hover{background:rgba(255,255,255,0.05); border-color:#94a3b8; box-shadow:none;}";
  
  html += ".matrix-slab{background:rgba(12,17,28,0.7); border:1px solid rgba(239,68,68,0.25); border-radius:4px; padding:20px; box-shadow:inset 0 0 20px rgba(239,68,68,0.02); height:fit-content;}";
  html += ".matrix-title{font-family:'Orbitron',sans-serif; font-size:13px; font-weight:700; color:#ef4444; letter-spacing:2px; margin-bottom:15px; text-transform:uppercase; display:flex; align-items:center; gap:8px;}";
  html += ".matrix-title::before{content:''; display:inline-block; width:6px; height:6px; background:#ef4444; box-shadow:0 0 8px #ef4444;}";
  html += "table{width:100%; border-collapse:collapse; font-size:14px;}";
  html += "th{font-family:'Orbitron',sans-serif; font-size:11px; color:#4a5568; text-transform:uppercase; border-bottom:1px solid rgba(255,255,255,0.05); padding:10px; text-align:left;}";
  html += "td{padding:12px 10px; border-bottom:1px solid rgba(255,255,255,0.02); color:#cbd5e1;}";
  html += ".row-alert{animation:rowPulse 2s infinite; background:rgba(239,68,68,0.03);} @keyframes rowPulse{0%{background:rgba(239,68,68,0.01);} 50%{background:rgba(239,68,68,0.04);} 100%{background:rgba(239,68,68,0.01);}}";
  
  html += ".section-title{font-family:'Orbitron',sans-serif; font-size:14px; font-weight:700; color:#d4af37; letter-spacing:2px; text-transform:uppercase; border-bottom:1px solid rgba(212,175,55,0.15); padding-bottom:8px; margin-bottom:15px;}";
  html += ".node-list-item{background:rgba(12,17,28,0.6); border:1px solid rgba(212,175,55,0.2); padding:20px 25px; cursor:pointer; display:flex; justify-content:space-between; align-items:center; font-family:'Orbitron',sans-serif; font-weight:700; font-size:14px; color:#94a3b8; transition:all 0.2s; margin-bottom:10px; clip-path:polygon(0 0, calc(100% - 12px) 0, 100% 12px, 100% 100%, 12px 100%, 0 calc(100% - 12px));}";
  html += ".node-list-item:hover{border-color:#d4af37; background:rgba(212,175,55,0.05); transform:translateX(6px); color:#d4af37;}";
  
  html += ".charts-grid{display:grid; grid-template-columns:1fr 1fr; gap:25px;}";
  html += ".graph-box{display:flex; flex-direction:column; gap:8px;}";
  html += ".graph-title{font-size:12px; font-family:'Orbitron',sans-serif; text-transform:uppercase; letter-spacing:1px; color:#4a5568; display:flex; justify-content:space-between;}";
  html += "canvas{background:#020408; border:1px solid rgba(255,255,255,0.03); width:100%; height:240px;}";
  html += ".ai-slab{background:rgba(20,25,35,0.4); border-left:3px solid #d4af37; padding:18px 22px; margin-top:10px;}";
  html += ".ai-label{font-family:'Orbitron',sans-serif; font-size:10px; text-transform:uppercase; letter-spacing:1px; color:#d4af37; margin-bottom:6px; font-weight:700;}";
  html += ".ai-content{font-size:15px; color:#f8fafc; font-style:italic; font-weight:600; letter-spacing:0.5px;}";
  
  html += ".loading-overlay{position:absolute; top:0; left:0; width:100%; height:100%; background:#04060b; z-index:999; display:none; flex-direction:column; justify-content:center; align-items:center; gap:15px;}";
  html += ".loading-text{font-family:'Orbitron',sans-serif; font-weight:900; font-size:20px; letter-spacing:4px; color:#f8fafc; animation:blinkText 0.8s infinite alternate;}";
  html += ".bar-container{width:280px; height:18px; border:2px solid #f8fafc; padding:2px; box-sizing:border-box;}";
  html += ".bar-fill{height:100%; width:80%; background:#f8fafc; display:flex;}"; 
  html += "@keyframes blinkText{0%{opacity:0.3;} 100%{opacity:1;}}";
  
  html += ".varchasva-sidebar{background:rgba(9,14,26,0.3); border-radius:6px; border:1px solid rgba(212,175,55,0.1); display:flex; flex-direction:column; align-items:center; padding:40px 20px; box-sizing:border-box; height:fit-content; position:relative;}";
  html += ".varchasva-sidebar::before{content:''; position:absolute; top:0; left:0; width:100%; height:3px; background:#d4af37; border-radius:6px 6px 0 0;}";
  html += ".emblem-box{position:relative; width:180px; height:110px; margin-bottom:30px; display:flex; justify-content:center;}";
  html += ".wing-left{position:absolute; width:0; height:0; border-top:35px solid #d4af37; border-left:75px solid transparent; transform:rotate(-22deg); left:10px; top:15px;}";
  html += ".wing-right{position:absolute; width:0; height:0; border-top:35px solid #d4af37; border-right:75px solid transparent; transform:rotate(22deg); right:10px; top:15px;}";
  html += ".center-spike{position:absolute; width:0; height:0; border-left:12px solid transparent; border-right:12px solid transparent; border-bottom:40px solid #d4af37; top:5px; animation:radarPulse 2s infinite alternate;}";
  html += ".crosshair-line{position:absolute; width:2px; height:90px; background:rgba(212,175,55,0.3); top:15px;}";
  html += ".radar-rings{position:absolute; bottom:5px; width:140px; height:50px; display:flex; justify-content:center; overflow:hidden;}";
  html += ".ring{position:absolute; border:1px solid rgba(212,175,55,0.25); border-radius:50%; animation:ripple 2.5s infinite linear;}";
  html += ".ring1{width:40px; height:40px; animation-delay:0s;} .ring2{width:80px; height:80px; animation-delay:0.8s;} .ring3{width:120px; height:120px; animation-delay:1.6s;}";
  html += "@keyframes ripple{0%{transform:scale(0.4); opacity:1;} 100%{transform:scale(1.5); opacity:0;}}";
  html += "@keyframes radarPulse{0%{border-bottom-color:#b89020;} 100%{border-bottom-color:#f6df7c;}}";
  html += ".sidebar-title{font-family:'Orbitron',sans-serif; font-weight:900; font-size:30px; letter-spacing:5px; color:#d4af37; margin:0; text-align:center;}";
  html += ".sidebar-tagline{font-family:'Orbitron',sans-serif; font-size:9px; font-weight:700; color:#5a6578; letter-spacing:1.2px; text-transform:uppercase; text-align:center; margin-top:8px; line-height:14px;}";
  html += ".sidebar-hindi{font-family:'Rajdhani',sans-serif; font-size:16px; font-weight:700; color:#d4af37; letter-spacing:4px; margin-top:25px; word-spacing:10px; opacity:0.85;}";
  html += "</style></head><body>";
  
  html += "<div id='bg-grid'></div>";
  html += "<div class='menu-container'>";
  
  html += "  <div id='load-block' class='loading-overlay'>";
  html += "    <div class='loading-text'>LOADING...</div>";
  html += "    <div class='bar-container'><div class='bar-fill'></div></div>";
  html += "  </div>";
  
  html += "  <h1>System monitoring panel</h1><p class='sub'>Seismic Monitoring Network &bull; Engine V3.1</p>";
  
  html += "  <div id='screen-main' class='view-screen active-view'>";
  html += "    <div class='left-panel'>";
  html += "      <div class='btn-row'>";
  html += "        <button class='nav-btn' onclick='triggerLoadTransition(\"screen-active-list\")'>Active Nodes Menu</button>";
  html += "        <button class='nav-btn' style='color:#10b981; border-color:rgba(16,185,129,0.35);' onclick='forceRefresh()'>Force Refresh</button>";
  html += "      </div>";
  html += "      <div class='matrix-slab'>";
  html += "        <div class='matrix-title'>High-Disturbance Event Matrix</div>";
  html += "        <table>";
  html += "          <thead><tr><th>Asset Location</th><th>Seismic Load</th><th>Acoustic Amp</th><th>AI Local Classification</th></tr></thead>";
  html += "          <tbody id='disturbance-table-body'></tbody>";
  html += "        </table>";
  html += "      </div>";
  html += "    </div>";
  
  html += "    <div class='varchasva-sidebar'>";
  html += "      <div class='emblem-box'>";
  html += "        <div class='crosshair-line'></div>";
  html += "        <div class='wing-left'></div>";
  html += "        <div class='wing-right'></div>";
  html += "        <div class='center-spike'></div>";
  html += "        <div class='radar-rings'><div class='ring ring1'></div><div class='ring ring2'></div><div class='ring ring3'></div></div>";
  html += "      </div>";
  html += "      <div class='sidebar-title'>VARCHASVA</div>";
  html += "      <div class='sidebar-tagline'>Underground Infiltration Tunnel<br>Detection System</div>";
  html += "      <div class='sidebar-hindi'>खोज &bull; निगरानी &bull; रक्षा</div>";
  html += "    </div>";
  html += "  </div>"; 
  
  html += "  <div id='screen-active-list' class='view-screen'>";
  html += "    <div class='btn-row'><button class='nav-btn back-btn' onclick='triggerLoadTransition(\"screen-main\")'>&laquo; Back to Main Menu</button></div>";
  html += "    <div class='section-title'>Currently Active Sensor Nodes</div>";
  html += "    <div id='active-nodes-list-container'></div>";
  html += "  </div>";
  
  html += "  <div id='screen-node-focus' class='view-screen'>";
  html += "    <div class='btn-row'><button class='nav-btn back-btn' onclick='triggerLoadTransition(\"screen-active-list\")'>&laquo; Back to Active Nodes</button></div>";
  html += "    <h2 id='focused-node-header' style='margin:0; font-family:Orbitron; font-size:16px; letter-spacing:1px; color:#d4af37; text-transform:uppercase;'>Node Focus</h2>";
  html += "    <div style='display:flex; flex-direction:column; gap:20px; margin-top:15px;'>";
  html += "      <div class='charts-grid'>";
  html += "        <div class='graph-box'><div class='graph-title'><span>Seismic graph</span><span id='focus-val-m' style='color:#38bdf8;'>0 LSB</span></div><canvas id='f-canvas-m'></canvas></div>";
  html += "        <div class='graph-box'><div class='graph-title'><span>Acoustic amp graph</span><span id='focus-val-s' style='color:#a855f7;'>0 AMP</span></div><canvas id='f-canvas-s'></canvas></div>";
  html += "      </div>";
  html += "      <div class='ai-slab'>";
  html += "        <div class='ai-label'>Node Edge-AI Classification Verdict</div>";
  html += "        <div class='ai-content' id='focus-ai-verdict'>Nominal Baseline Activity...</div>";
  html += "      </div>";
  html += "    </div>";
  html += "  </div>";
  
  html += "</div>"; 
  
  html += "<script>";
  html += "let nodeHistory = {}; let currentFocusedNodeId = null; const MAX_POINTS = 40;";
  
  html += "function triggerLoadTransition(targetScreenId){";
  html += "  let loader = document.getElementById('load-block');";
  html += "  loader.style.display = 'flex';";
  html += "  setTimeout(() => {";
  html += "    loader.style.display = 'none';";
  html += "    document.querySelectorAll('.view-screen').forEach(s => s.classList.remove('active-view'));";
  html += "    document.getElementById(targetScreenId).classList.add('active-view');";
  html += "    if(targetScreenId === 'screen-node-focus' && currentFocusedNodeId) { setTimeout(() => initFocusCanvasResolution(), 40); }";
  html += "  }, 750);";
  html += "}";
  
  html += "function openNodeFocus(id){ currentFocusedNodeId = id; document.getElementById('focused-node-header').innerText = 'Telemetry Core Analysis: Node ' + id; triggerLoadTransition('screen-node-focus'); }";
  
  html += "async function forceRefresh(){";
  html += "  let loader = document.getElementById('load-block');";
  html += "  loader.style.display = 'flex';";
  html += "  try {";
  html += "    await fetch('/data');";
  html += "    await pollEngine();";
  html += "  } catch(e) { console.error(e); }";
  html += "  setTimeout(() => { loader.style.display = 'none'; }, 600);";
  html += "}";
  
  html += "async function pollEngine(){";
  html += "  try{";
  html += "    let res = await fetch('/data'); if(res.status !== 200) return;";
  html += "    let data = await res.json();";
  
  html += "    let tableBody = document.getElementById('disturbance-table-body'); if(tableBody) tableBody.innerHTML = '';";
  html += "    let listContainer = document.getElementById('active-nodes-list-container'); if(listContainer) listContainer.innerHTML = '';";
  
  html += "    data.sort((x, y) => x.id - y.id);";
  
  html += "    data.forEach(node => {";
  html += "      if(!nodeHistory[node.id]){ nodeHistory[node.id] = { seismic: Array(MAX_POINTS).fill(0), sound: Array(MAX_POINTS).fill(0) }; }";
  html += "      nodeHistory[node.id].seismic.push(node.seismic); nodeHistory[node.id].seismic.shift();";
  html += "      nodeHistory[node.id].sound.push(node.sound); nodeHistory[node.id].sound.shift();";
  
  html += "      if(listContainer) {";
  html += "        let item = document.createElement('div'); item.className = 'node-list-item'; item.innerHTML = `<span>Node ${node.id}</span><span style='color:#64748b; font-size:11px;'>ONLINE &raquo;</span>`; item.onclick = () => openNodeFocus(node.id);";
  html += "        if(node.ai !== 'BACKGROUND_NOISE' && node.ai !== 'MONITORING_NOMINAL') { item.innerHTML = `<span>Node ${node.id}</span><span style='color:#ef4444; font-size:11px; font-weight:700;'>[ALERT: ${node.ai}] &raquo;</span>`; }";
  html += "        listContainer.appendChild(item);";
  html += "      }";
  
  html += "      if((node.seismic > 150 || node.sound > 200 || (node.ai !== 'BACKGROUND_NOISE' && node.ai !== 'MONITORING_NOMINAL')) && tableBody){";
  html += "        let row = document.createElement('tr'); row.className = 'row-alert';";
  html += "        row.innerHTML = `<td><b style='color:#f8fafc;'>Node ${node.id}</b></td><td style='color:#ef4444; font-weight:700;'>${node.seismic} LSB</td><td style='color:#f97316; font-weight:700;'>${node.sound} AMP</td><td style='color:#d4af37; font-weight:700; font-style:italic;'>${node.ai}</td>`;";
  html += "        tableBody.appendChild(row);";
  html += "      }";
  
  html += "      if(currentFocusedNodeId === node.id){";
  html += "        let fSeismic = document.getElementById('focus-val-m'); if(fSeismic) fSeismic.innerText = node.seismic + ' LSB';";
  html += "        let fSound = document.getElementById('focus-val-s'); if(fSound) fSound.innerText = node.sound + ' AMP';";
  html += "        let fVerdict = document.getElementById('focus-ai-verdict'); if(fVerdict) fVerdict.innerText = node.ai ? node.ai : 'No active anomaly detections or structural anomalies recorded.';";
  html += "        drawGraph('f-canvas-m', nodeHistory[node.id].seismic, '#38bdf8');";
  html += "        drawGraph('f-canvas-s', nodeHistory[node.id].sound, '#a855f7');";
  html += "      }";
  html += "    });";
  html += "  }catch(e){console.error(e);}";
  html += "}";
  
  html += "function initFocusCanvasResolution(){";
  html += "  ['f-canvas-m', 'f-canvas-s'].forEach(cId => {";
  html += "    let canvas = document.getElementById(cId); if(!canvas) return; let rect = canvas.getBoundingClientRect();";
  html += "    canvas.width = rect.width * window.devicePixelRatio; canvas.height = rect.height * window.devicePixelRatio;";
  html += "  });";
  html += "}";
  
  html += "function drawGraph(canvasId, dataPoints, strokeColor){";
  html += "  let canvas = document.getElementById(canvasId); if(!canvas || canvas.width===0) return; let ctx = canvas.getContext('2d');";
  html += "  let w = canvas.width; let h = canvas.height; ctx.clearRect(0, 0, w, h);";
  html += "  ctx.strokeStyle = 'rgba(255,255,255,0.02)'; ctx.lineWidth = 1; for(let i=1; i<5; i++){ ctx.beginPath(); ctx.moveTo(0, h*(i/5)); ctx.lineTo(w, h*(i/5)); ctx.stroke(); }";
  html += "  ctx.beginPath(); ctx.strokeStyle = strokeColor; ctx.lineWidth = 2 * window.devicePixelRatio; ctx.lineJoin = 'round';";
  html += "  for(let i=0; i<dataPoints.length; i++){";
  html += "    let x = (i / (dataPoints.length - 1)) * w; let y = h - ((dataPoints[i] / 1000) * h);";
  html += "    if(i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);";
  html += "  } ctx.stroke();";
  html += "  ctx.lineTo(w, h); ctx.lineTo(0, h); ctx.closePath();";
  html += "  let grad = ctx.createLinearGradient(0,0,0,h); grad.addColorStop(0, strokeColor+'0c'); grad.addColorStop(1, strokeColor+'00');";
  html += "  ctx.fillStyle = grad; ctx.fill();";
  html += "}";
  
  html += "setInterval(pollEngine, 1000); window.onload = () => { pollEngine(); };";
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

//  TELEMETRY JSON DATA REPLICATOR ENDPOINT (THREAD SAFE) 
void handleDataJson() {
  if (processingNetworkData) {
    server.send(503, "text/plain", "Service Busy");
    return;
  }

  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  for (int i = 0; i < MAX_NODES; i++) {
    if (networkMesh[i].active) {
      JsonObject obj = array.add<JsonObject>();
      obj["id"] = networkMesh[i].id;
      obj["seismic"] = networkMesh[i].seismic;
      obj["sound"] = networkMesh[i].sound;
      obj["ai"] = networkMesh[i].aiVerdict;
    }
  }
  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}

//  CORES INITIALIZATION 
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize hardware pin as output and set it low out of the gate
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.softAP(ap_ssid, ap_password);
  Serial.printf("[WIFI] Gateway Online. Dashboard Address: http://%s\n", WiFi.softAPIP().toString().c_str());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleDataJson);
  server.begin();

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("[CRITICAL] LoRa Baseband Hardware Configuration Failure.");
    while (1);
  }
  Serial.println("[SUCCESS] LoRa Mesh Core Active & Awaiting Nodes...");
}

//  CENTRAL CONCURRENT PROCESSING LOOP 
void loop() {
  server.handleClient();
  verifyNodeHeartbeats();
  checkBuzzerTimeout(); // Continuously monitor non-blocking active buzzer duration

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    processingNetworkData = true; 

    String packetStr = "";
    while (LoRa.available()) { packetStr += (char)LoRa.read(); }
    packetStr.trim();

    Serial.print("[LORA RX] Raw Packet Received: ");
    Serial.println(packetStr);

    //  STRIP COMMAS AND PARSE SUBSTRINGS SAFELY 
    // Target Format: NODE:1,SEISMIC:450,SOUND:120,AI_CLASS:DRILLING_TUNNELING,CLASS_ID:2
    int nodeIdx    = packetStr.indexOf("NODE:");
    int seismicIdx = packetStr.indexOf(",SEISMIC:");
    int soundIdx   = packetStr.indexOf(",SOUND:");
    int aiClassIdx = packetStr.indexOf(",AI_CLASS:");
    int classIdIdx = packetStr.indexOf(",CLASS_ID:");

    if (nodeIdx != -1 && seismicIdx != -1 && soundIdx != -1) {
      
      // Extract numeric values out of the bounded tokens cleanly
      int parsedID      = packetStr.substring(nodeIdx + 5, seismicIdx).toInt();
      int parsedSeismic = packetStr.substring(seismicIdx + 9, soundIdx).toInt();
      
      int parsedSound = 0;
      String parsedAI = "MONITORING_NOMINAL";

      if (aiClassIdx != -1) {
        // Handle Advanced Multi-Classification Payload Strings
        parsedSound = packetStr.substring(soundIdx + 7, aiClassIdx).toInt();
        
        if (classIdIdx != -1) {
          parsedAI = packetStr.substring(aiClassIdx + 10, classIdIdx);
        } else {
          parsedAI = packetStr.substring(aiClassIdx + 10);
        }
      } else {
        // Fallback execution block if processing non-AI legacy diagnostic transmissions
        parsedSound = packetStr.substring(soundIdx + 7).toInt();
        if (parsedSeismic > 150 || parsedSound > 200) {
          parsedAI = "ALERT: HIGH DISTURBANCE";
        }
      }

      parsedAI.trim();

      // Fire audio alarm indicator on critical exceptions or dynamic non-noise classifications
      if (parsedSeismic > 150 || parsedSound > 200 || (aiClassIdx != -1 && parsedAI != "BACKGROUND_NOISE" && parsedAI != "MONITORING_NOMINAL")) {
        triggerAudioAlert();
      }

      // Commit processed attributes directly into local mesh memory structure array
      processIncomingData(parsedID, parsedSeismic, parsedSound, parsedAI);
    } else {
      Serial.println("[PARSER ERROR] Non-conforming payload architecture dropped.");
    }

    processingNetworkData = false; 
  }
  delay(1); 
}