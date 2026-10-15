
// Assist setup for new app installations
// original provided by gemi254
// 
// s60sc 2023, 2026

#include "appGlobals.h"

#if INCLUDE_CERTS

 /*

 To set app as HTTPS server, a server private key and public certificate are required
 Create keys and certificates using openssl tool
 On Windows, paste commands below into a Command Prompt (cmd) window

 Define app to have static IP address, and use this as variable substitution for openssl:
   set APP_IP="192.168.1.135"

 Create app server private key and public certificate:
   openssl req -nodes -x509 -sha256 -newkey rsa:2048 -subj "/CN=%APP_IP%" -addext "subjectAltName=IP:%APP_IP%" -extensions v3_ca -keyout prvtkey.pem -out servercert.pem -days 3660

 View server cert content:
   openssl x509 -in servercert.pem -noout -text

 Use app web page OTA Upload tab to copy servercert.pem and prvtkey.pem into ESP storage.

 Use of HTTPS is controlled on web page by option 'Use HTTPS' under Access Settings / Authentication settings or Edit Config / Network settings
 If the private key or public certificate is not loaded, the Use HTTPS setting is ignored.
 
 Enter `https://static_ip` to access the app from the browser. A security warning will be displayed as the certificate is self signed so untrusted. 
 To trust the certificate it needs to be installed on the device: 
 - open the Chrome settings page.
 - in the Privacy and security panel, expand the Security section, click on Manage certificates.
 - in the Certificate Manager panel, press Manage imported certificates from Windows
 - in the Certificates popup, select the Trusted Root Certification Authorities tab, click the Import... button to launch the Import Wizard.
 - click Next, on the next page, select Browse... All Files and locate the servercert.pem file.
 - click Next, then Finish, then in the Security Warning popup, click on Yes and another popup indicates that the import was successful.

 */

#ifndef CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC
#define CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC 1
#endif

/************************************************************************************************************/

#define PRVTKEY DATA_DIR "/prvtkey" ".pem"
#define SERVERCERT DATA_DIR "/servercert" ".pem"

char* serverCerts[2]; // private key, server key
#define NUM_CERTS 2

void loadCerts() {
  if (useHttps) {
    const char* certFiles[NUM_CERTS] = {PRVTKEY, SERVERCERT};
    for (int i = 0; i < NUM_CERTS; i++) {
      File file;
      if (STORAGE.exists(certFiles[i])) {
        file = STORAGE.open(certFiles[i], FILE_READ);
        if (!file || !file.size()) {
          LOG_WRN("Failed to load file %s", certFiles[i]);
          useHttps = false;
        } else {
          // load contents
          serverCerts[i] = psramFound() ? (char*)ps_malloc(file.size() + 1) : (char*)malloc(file.size() + 1); 
          size_t inBytes = file.read((uint8_t*)serverCerts[i], file.size());
          if (inBytes != file.size()) {
            LOG_WRN("File %s not correctly loaded", certFiles[i]);
            useHttps = false;
          }
        }
        file.close();
      } else {
        LOG_WRN("File %s not found", certFiles[i]);
        useHttps = false;
      }
    }
    if (!useHttps) LOG_WRN("HTTPS not available as server keys not loaded, using HTTP");
  }
}

#endif

/*****************************************************************/

TaskHandle_t checkDataHandle = NULL;

static bool wgetFile(const char* filePath) {
  // download required data file from github repository and store
  bool res = false;
  if (!STORAGE.exists(filePath)) {
    NetworkClientSecure wclient;
    if (remoteServerConnect(wclient, GITHUB_HOST, HTTPS_PORT, SETASSIST)) {
      char downloadURL[150];
      snprintf(downloadURL, 150, "%s%s", GITHUB_PATH, filePath);
      File f = STORAGE.open(filePath, FILE_WRITE);
      if (f) {
        HTTPClient https;
        if (https.begin(wclient, GITHUB_HOST, HTTPS_PORT, downloadURL)) {
          LOG_INF("Downloading %s from %s", filePath, downloadURL);
          int httpCode = https.GET();
          int fileSize = 0;
          if (httpCode == HTTP_CODE_OK) {
            fileSize = https.writeToStream(&f);
            if (fileSize <= 0) {
              LOG_WRN("Download failed: writeToStream - %s", https.errorToString(fileSize).c_str());
              httpCode = 0;
            } else LOG_INF("Downloaded %s, size %s", filePath, fmtSize(fileSize));       
          } else LOG_WRN("Download failed, error: %s", https.errorToString(httpCode).c_str());    
          https.end();
          f.close();
          if (httpCode == HTTP_CODE_OK) {
            if (!strcmp(filePath, CONFIG_FILE_PATH)) doRestart("Config file downloaded");
            res = true;
          } else {
            LOG_WRN("HTTP Get failed with code: %d", httpCode);
            STORAGE.remove(filePath);
          }
        }
      } else LOG_WRN("Open failed: %s", filePath);
    }
    remoteServerClose(wclient);
  } else res = true;
  return res;
}

static void checkDataFilesTask(void* parameter) {
  // try and get get each remote file in turn
  wgetFile(COMMON_JS_PATH); 
  delay(100);
  wgetFile(INDEX_PAGE_PATH);
  delay(100);
  appDataFiles(); 
  doRestart("Restart after web file download");
  vTaskDelete(NULL);
}

static bool checkFilePresent(const char* filePath) {
  if (STORAGE.exists(filePath)) {
    // if file exists but is empty, delete it to allow re-download
    File f = STORAGE.open(filePath, FILE_READ);
    size_t fSize = f.size();
    f.close();
    if (!fSize) STORAGE.remove(filePath);
  }
  return STORAGE.exists(filePath);
}

void checkDataFiles() {
  // check if required files have been downloaded
  bool res = true;
  if (strlen(GITHUB_PATH)) { 
     res = checkFilePresent(COMMON_JS_PATH);
    if (res) res = checkFilePresent(INDEX_PAGE_PATH);
    if (!res) {
      if (WiFi.status() == WL_NO_SSID_AVAIL || WiFi.status() == WL_DISCONNECTED) LOG_WRN("Need to connect to AP to setup router");
      else if (WiFi.status() != WL_CONNECTED) snprintf(startupFailure, SF_LEN, STARTUP_FAIL "No internet connection to download files");
      else if (checkDataHandle == NULL) {
        // separate transient download task due to TLS memory usage 
        LOG_INF("Download web files then restart");
        xTaskCreate(&checkDataFilesTask, "checkDataFilesTask", CHECK_STACK_SIZE, NULL, CHECK_PRI, &checkDataHandle);
      }
      if (strlen(startupFailure)) LOG_WRN("%s", startupFailure);
    } // else no download needed if GITHUB_PATH not defined
  }
  if (res) dataFilesChecked = true;
}

const char* setupPage_html = R"~(
<!doctype html>
<html>
<head>
<meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Application setup</title>
<style>
  body{background-color:#e0f7fa;font-family:Arial,sans-serif}
  .dropdown{position:relative;display:inline-block;width:calc(100%)}
  .dropdown-content{display:none;position:absolute;background-color:#fff;width:100%;box-shadow:0 8px 16px 0 rgba(0,0,0,0.2);z-index:1;overflow:hidden;white-space:nowrap}
  .dropdown-content div,.dropdown-content button{color:black;padding:12px 16px;text-decoration:none;display:flex;justify-content:space-between;align-items:center;width:100%;border:0;background:0;cursor:pointer}
  .dropdown-content div:hover,.dropdown-content button:hover{background-color:#f1f1f1}
  .network-details{display:flex;align-items:center;gap:8px}
  .signal-icon{width:16px;height:16px;background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAAAQCAMAAADeZIrLAAAAJFBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHJj5lAAAAC3RSTlMAIjN3iJmqu8zd7vF8pzcAAABsSURBVHja7Y1BCsAwCASNSVo3/v+/BUEiXnIoXkoX5jAQMxTHzK9cVSnvDxwD8bFx8PhZ9q8FmghXBhqA1faxk92PsxvRc2CCCFdhQCbRkLoAQ3q/wWUBqG35ZxtVzW4Ed6LngPyBU2CobdIDQ5oPWI5nCUwAAAAASUVORK5CYII=');background-size:96px 16px;position:absolute;right:16px}
  .wpa-text{position:absolute;right:64px;font-size:.3em}
  .encryption-icon{position:absolute;right:32px;font-size:1em}
  .input{width:calc(100% - 32px);padding:12px 16px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;display:inline-block}
  .center-button{display:flex;justify-content:center}
  .styled-button{background-color:#007bff;color:white;border:0;padding:10px 20px;text-align:center;text-decoration:none;display:inline-block;font-size:16px;margin:4px 2px;cursor:pointer;border-radius:4px}
  .styled-button:hover{background-color:#0056b3}
</style>
</head>
<script>
let ssidList=[];

function fetchSSIDs(){
  const ssidSelect=document.getElementById('ST_SSID');
  ssidSelect.placeholder='Loading...';
  const scanButton=document.getElementById('scan-button');
  if(scanButton){scanButton.textContent='Scanning...';scanButton.disabled=true;}

  fetch('/wifi').then(response=>response.json()).then(data=>{
    ssidList=data.networks;
    updateDropdown();
    ssidSelect.placeholder='Select SSID from dropdown';
    if(scanButton){scanButton.textContent='Scan';scanButton.disabled=false;}
  }).catch(error=>{console.error('Error fetching SSIDs:',error);
  ssidSelect.placeholder='Select SSID from dropdown';
  if(scanButton){scanButton.textContent='Scan';scanButton.disabled=false;}
  });
}

function updateDropdown(){
  const dropdownContent=document.getElementById('dropdown-content');
  dropdownContent.innerHTML='';
  let longestSSID='';
  ssidList.forEach(network=>{if(network.ssid.length>longestSSID.length){longestSSID=network.ssid;}});
  const tempElement=document.createElement('span');
  tempElement.style.visibility='hidden';
  tempElement.style.whiteSpace='nowrap';
  tempElement.textContent=longestSSID;
  document.body.appendChild(tempElement);
  const dropdownWidth=tempElement.offsetWidth+100;
  document.body.removeChild(tempElement);
  dropdownContent.style.width=`${dropdownWidth}px`;
  ssidList.forEach(network=>{
    let signalStrength;
    if(network.strength>=-65){signalStrength=4;}
    else if(network.strength>=-75){signalStrength=3;}
    else if(network.strength>=-85){signalStrength=2;}
    else if(network.strength>=-95){signalStrength=1;}
    else{signalStrength=0;}
    const div=document.createElement('div');
    const encryptionStatus=network.encryption==='Open'?'🔓':'🔒';
    div.innerHTML=`<span>${network.ssid}</span><div class="network-details"><span class="encryption-icon">${encryptionStatus}</span><span class="signal-icon"style="background-position: -${signalStrength * 16}px 0;"alt="Signal Strength"></span></div>`;div.onclick=()=>{document.getElementById('ST_SSID').value=network.ssid;dropdownContent.style.display='none';};
    dropdownContent.appendChild(div);
  });
  const scanButton=document.createElement('button');
  scanButton.id='scan-button';
  scanButton.textContent='Scan';
  canButton.classList.add('center-button');
  scanButton.onclick=fetchSSIDs;
  dropdownContent.appendChild(scanButton);
}

function toggleDropdown(){
  const dropdownContent=document.getElementById('dropdown-content');
  dropdownContent.style.display=dropdownContent.style.display==='block'?'none':'block';
}

function hideDropdown(event){
  const dropdownContent=document.getElementById('dropdown-content');
  if(!event.target.closest('.dropdown')){dropdownContent.style.display='none';}

}

function Config(){
  if(!window.confirm('This will reboot the device to activate new settings.'))return false;
  fetch('/control?ST_SSID='+encodeURI(document.getElementById('ST_SSID').value)).then(r=>{
    console.log(r);
    return fetch('/control?ST_Pass='+encodeURI(document.getElementById('ST_Pass').value))}).then(r=>{console.log(r);
    return fetch('/control?save=1')}).then(r=>{console.log(r);
    return fetch('/control?reset=1')}).then(r=>{console.log(r);
  });
  return false;
}

window.onload=fetchSSIDs;
document.addEventListener('click',hideDropdown);
</script>
<body>
<br>
<center>
<table border=0>
<tr>
<th colspan=3>Wifi setup..</th>
</tr>
<tr>
<td colspan=3></td>
</tr>
<tr>
<td colspan=3>
<label for=ST_SSID>SSID</label>
<div class=dropdown>
<input id=ST_SSID name=ST_SSID placeholder=Loading... class=input onclick=toggleDropdown() autocomplete=off>
<div id=dropdown-content class=dropdown-content></div>
</div>
</td>
</tr>
<tr>
<td colspan=3>
<label for=ST_Pass>Password</label>
<input id=ST_Pass name=ST_Pass length=64 placeholder="Router password" class=input autocomplete=off>
</td>
</tr>
<tr>
<td colspan=3></td>
</tr>
<tr>
<td colspan=3 align=center>
<button type=button class=styled-button onClick="return Config()">Connect</button>
<button type=button class=styled-button onclick=window.location.reload()>Cancel</button>
</td>
</tr>
</table>
<br /><br /><a href=/web?OTA.htm><button class=styled-button>OTA
Update</button></a>
</center>
</body>
</html>
)~";

const char* otaPage_html = R"~(
<!DOCTYPE html>
<html>
  <head>
    <title>OTA</title>
    <style>
      html body {height: 100%;}
      body {
        font-family: Helvetica  !important;
        background: #181818;
        color: WhiteSmoke;
        font-size: 1rem;; 
      }
    </style>
  </head>
  <body>
    <br>
    <h3>Upload data file or bin file to ESP32</h3>
    <br>
    <a href="javascript:history.back()" style="color: WhiteSmoke;">Go Back</a>
    <br><br><br>
    <form id="upload_form" enctype="multipart/form-data" method="post">
      <input type="file" name="otafile" id="otafile" onchange="otaUploadFile()"><br>
      <br>
      <progress id="progressOta" value="0" max="100" style="width:200px;"></progress>%
      <h3 id="status"></h3>
      <p id="loaded_n_total"></p>
    </form>
    
    <script>
      const defaultPort = window.location.protocol == 'http:' ? 80 : 443; 
      const webPort = !window.location.port ? defaultPort : window.location.port; // in case alternative ports specified
      const webServer = window.location.protocol + '//' + document.location.hostname + ':' + webPort;
      const $ = document.querySelector.bind(document);
   
      async function otaUploadFile() {
        // notify server to start ota 
        let file = $("#otafile").files[0];
        const response = await fetch('/control?startOTA=' + file.name);
        if (response.ok) {
          // submit file for uploading
          let xhr = new XMLHttpRequest();
          xhr.upload.addEventListener("progress", progressHandler, false);
          xhr.addEventListener("load", completeHandler, false);
          xhr.addEventListener("error", errorHandler, false);
          xhr.addEventListener("abort", abortHandler, false);
          xhr.open("POST", webServer +  '/upload');
          xhr.send(file);
        } else alert(response.status + ": " + response.statusText); 
      }

      function progressHandler(event) {
        $("#loaded_n_total").innerHTML = "Uploaded " + event.loaded + " of " + event.total + " bytes";
        let percent = (event.loaded / event.total) * 100;
        $("#progressOta").value = Math.round(percent);
        $("#status").innerHTML = Math.round(percent) + "% transferred";
        if (event.loaded  == event.total) $("#status").innerHTML = 'Uploaded, wait for completion result';
      }

      function completeHandler(event) {
        $("#status").innerHTML = event.target.responseText;
        $("#progressOta").value = 0;
      }

      function errorHandler(event) {
        $("#status").innerHTML = "Upload Failed";
        $("#progressOta").value = 0;
      }

      function abortHandler(event) {
        $("#status").innerHTML = "Upload Aborted";
        $("#progressOta").value = 0;
      }
    </script>
  </body>
</html>
)~";

const char* failPageS_html = R"~(
<!DOCTYPE html>
<html>
  <head>
    <title>Startup Failure</title>
    <script>
      function getLog() {
        fetch('/control?displayLog=1')
        .then(response => response.text())
        .then(logdata => { document.getElementById('appLog').innerText = logdata;})
        .catch(error => alert('Error fetching log:', error));
      }
    </script>
  </head>
  <body>
    <h2>
)~";

const char* failPageE_html = R"~(
    </h2>
    <h3><a href="#" onclick="getLog(); return false;"><button class=styled-button>Check log</button></a></h3>
    <h3><a href=/control?reset=1><button class=styled-button>Reboot ESP after fix</button></a></h3>
    <h3><a href=/web?OTA.htm><button class=styled-button>OTA Update</button></a></h3>
    <div id="appLog"></div>
  </body>
</html>
)~";

const char* heroDashboard_html = R"~(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Hero's Walls - DNS Filtering Appliance</title>
  <style>
    :root {
      --bg: #09090b;
      --card-bg: #121215;
      --card-border: #27272a;
      --card-border-hover: #3f3f46;
      --text: #f4f4f5;
      --text-muted: #71717a;
      --text-dim: #3f3f46;
      --accent-red: #dc2626;
      --accent-red-hover: #b91c1c;
      --input-bg: #18181b;
      --radius: 6px;
      --mono: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif; }
    body {
      background: var(--bg);
      color: var(--text);
      min-height: 100vh;
      padding: 32px 16px;
      display: flex;
      flex-direction: column;
      align-items: center;
      -webkit-font-smoothing: antialiased;
    }
    .container {
      width: 100%;
      max-width: 820px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }
    .header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding-bottom: 16px;
      border-bottom: 1px solid var(--card-border);
    }
    .brand {
      display: flex;
      flex-direction: column;
      gap: 2px;
    }
    .brand-title {
      font-size: 1.15rem;
      font-weight: 700;
      letter-spacing: 0.12em;
      color: #ffffff;
      text-transform: uppercase;
      font-family: var(--mono);
    }
    .brand-tag {
      font-size: 0.7rem;
      color: var(--text-muted);
      letter-spacing: 0.08em;
      text-transform: uppercase;
      font-family: var(--mono);
    }
    .badge {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      font-size: 0.72rem;
      font-family: var(--mono);
      letter-spacing: 0.06em;
      text-transform: uppercase;
      padding: 6px 12px;
      border-radius: var(--radius);
      background: #18181b;
      color: #fafafa;
      border: 1px solid var(--card-border);
      font-weight: 600;
    }
    .badge .indicator {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: var(--accent-red);
    }
    .badge.off {
      color: var(--text-muted);
    }
    .badge.off .indicator {
      background: #52525b;
    }

    /* Cards */
    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      padding: 20px 24px;
      transition: border-color 0.15s ease;
    }
    .card:hover {
      border-color: var(--card-border-hover);
    }
    .card-title {
      font-size: 0.75rem;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 0.08em;
      font-family: var(--mono);
      margin-bottom: 8px;
      font-weight: 600;
    }
    .stat-val {
      font-size: 2.2rem;
      font-weight: 600;
      color: #ffffff;
      font-family: var(--mono);
      letter-spacing: -0.02em;
      font-variant-numeric: tabular-nums;
    }
    .stat-sub {
      font-size: 0.78rem;
      color: var(--text-muted);
      margin-top: 6px;
      line-height: 1.4;
    }

    /* Grid */
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
      gap: 16px;
    }

    /* Minimalist Toggle */
    .toggle-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 16px;
    }
    .switch-title {
      font-size: 0.95rem;
      font-weight: 600;
      color: #ffffff;
      letter-spacing: 0.02em;
    }
    .toggle {
      position: relative;
      display: inline-block;
      width: 48px;
      height: 26px;
      flex-shrink: 0;
    }
    .toggle input { opacity: 0; width: 0; height: 0; }
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background-color: #27272a;
      transition: .2s ease;
      border-radius: var(--radius);
      border: 1px solid #3f3f46;
    }
    .slider:before {
      position: absolute;
      content: "";
      height: 18px;
      width: 18px;
      left: 3px;
      bottom: 3px;
      background-color: #71717a;
      transition: .2s ease;
      border-radius: 4px;
    }
    input:checked + .slider {
      background-color: var(--accent-red);
      border-color: var(--accent-red);
    }
    input:checked + .slider:before {
      transform: translateX(22px);
      background-color: #ffffff;
    }

    /* Inputs & Buttons */
    .input-box {
      width: 100%;
      background: var(--input-bg);
      border: 1px solid var(--card-border);
      padding: 10px 14px;
      border-radius: var(--radius);
      color: #ffffff;
      font-size: 0.88rem;
      font-family: var(--mono);
      margin-top: 10px;
      outline: none;
      transition: border-color .15s ease;
    }
    .input-box:focus {
      border-color: var(--accent-red);
    }
    .btn {
      background: var(--accent-red);
      color: #ffffff;
      border: 1px solid transparent;
      padding: 10px 18px;
      border-radius: var(--radius);
      font-size: 0.82rem;
      font-weight: 600;
      letter-spacing: 0.04em;
      text-transform: uppercase;
      font-family: var(--mono);
      cursor: pointer;
      transition: all .15s ease;
      margin-top: 12px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
    }
    .btn:hover {
      background: var(--accent-red-hover);
    }
    .btn.secondary {
      background: #18181b;
      color: #d4d4d8;
      border: 1px solid var(--card-border);
    }
    .btn.secondary:hover {
      background: #27272a;
      border-color: #3f3f46;
      color: #ffffff;
    }
    .btn.danger {
      background: transparent;
      border: 1px solid var(--accent-red);
      color: var(--accent-red);
    }
    .btn.danger:hover {
      background: var(--accent-red);
      color: #ffffff;
    }
    .btn-group {
      display: flex;
      gap: 10px;
    }

    .status-msg {
      margin-top: 8px;
      font-size: 0.8rem;
      min-height: 1.2rem;
      font-family: var(--mono);
    }

    /* Minimalist Footer */
    .nav-links {
      display: flex;
      gap: 16px;
      justify-content: center;
      margin-top: 16px;
      padding-top: 16px;
      border-top: 1px solid var(--card-border);
    }
    .nav-links a {
      color: var(--text-muted);
      text-decoration: none;
      font-size: 0.75rem;
      font-family: var(--mono);
      letter-spacing: 0.06em;
      text-transform: uppercase;
      transition: color .15s;
    }
    .nav-links a:hover {
      color: #ffffff;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <div class="brand">
        <span class="brand-title">Hero's Walls</span>
        <span class="brand-tag">DNS Filtering Appliance</span>
      </div>
      <div id="statusBadge" class="badge">
        <span class="indicator"></span>
        <span id="statusText">Active & Protecting</span>
      </div>
    </div>

    <!-- Main Sinkhole Toggle -->
    <div class="card">
      <div class="toggle-row">
        <div>
          <div class="switch-title">Ad & Tracker Sinkhole</div>
          <div class="stat-sub">Intercepts ad networks, telemetry, and tracking beacons network-wide</div>
        </div>
        <label class="toggle">
          <input type="checkbox" id="adblockToggle" checked onchange="toggleAdblock(this.checked)">
          <span class="slider"></span>
        </label>
      </div>
    </div>

    <!-- Real-Time Metrics -->
    <div class="grid">
      <div class="card">
        <div class="card-title">Ads Sinkholed</div>
        <div id="statBlocked" class="stat-val">0</div>
        <div class="stat-sub">Resolved to 0.0.0.0 / ::</div>
      </div>
      <div class="card">
        <div class="card-title">Queries Allowed</div>
        <div id="statAllowed" class="stat-val">0</div>
        <div class="stat-sub">Forwarded to upstream resolver</div>
      </div>
      <div class="card">
        <div class="card-title">Filter Engine</div>
        <div id="statDomains" class="stat-val" style="font-size: 1.5rem; line-height: 2.2rem;">Core + YouTube</div>
        <div class="stat-sub">Active ad patterns & telemetry blocks</div>
      </div>
    </div>

    <!-- Wi-Fi Network Setup -->
    <div class="card">
      <div class="card-title">Wi-Fi Network Configuration</div>
      <p class="stat-sub">The device reconnects to your network on boot. To configure a new network, enter credentials below:</p>
      <input type="text" id="wifiSsid" class="input-box" placeholder="Network SSID (e.g. Home_WiFi)">
      <input type="password" id="wifiPass" class="input-box" placeholder="Network Password">
      <button class="btn" onclick="saveWifiCredentials()">Save & Connect</button>
      <div id="wifiMsg" class="status-msg"></div>
    </div>

    <!-- Custom Domain Rules -->
    <div class="card">
      <div class="card-title">Custom Domain Rules</div>
      <p class="stat-sub">Add or delete custom rules with immediate precedence over blocklists:</p>
      <input type="text" id="customDomain" class="input-box" placeholder="e.g. ads.example.com">
      <div class="btn-group">
        <button class="btn" onclick="addCustomDomain()">Block Domain</button>
        <button class="btn secondary" onclick="delCustomDomain()">Unblock Domain</button>
      </div>
      <div id="customMsg" class="status-msg"></div>
    </div>

    <!-- System Controls -->
    <div class="card" style="display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 12px;">
      <div>
        <div class="switch-title">Appliance Controls</div>
        <div class="stat-sub">Restart ESP32 or access advanced engineering settings</div>
      </div>
      <div class="btn-group" style="margin-top: 0;">
        <button class="btn secondary" style="margin-top: 0;" onclick="location.href='/web?AdBlocker.htm'">Classic Console</button>
        <button class="btn danger" style="margin-top: 0;" onclick="rebootEsp()">Reboot</button>
      </div>
    </div>

    <div class="nav-links">
      <a href="/web?AdBlocker.htm">Full Configuration</a>
      <span style="color: var(--text-dim);">&bull;</span>
      <a href="/control?displayLog=1" target="_blank">Live Logs</a>
      <span style="color: var(--text-dim);">&bull;</span>
      <a href="/web?OTA.htm">Firmware OTA</a>
    </div>
  </div>

  <script>
    function updateStats() {
      fetch('/status')
        .then(r => r.json())
        .then(d => {
          if (d.blockCnt !== undefined) document.getElementById('statBlocked').innerText = d.blockCnt;
          if (d.allowCnt !== undefined) document.getElementById('statAllowed').innerText = d.allowCnt;
          if (d.zzzAdblockOn !== undefined) {
            const on = d.zzzAdblockOn == "1" || d.zzzAdblockOn == true;
            document.getElementById('adblockToggle').checked = on;
            const badge = document.getElementById('statusBadge');
            const txt = document.getElementById('statusText');
            if (on) {
              badge.className = 'badge';
              txt.innerText = 'Active & Protecting';
            } else {
              badge.className = 'badge off';
              txt.innerText = 'Protection Paused';
            }
          }
        })
        .catch(e => console.error(e));
    }

    function toggleAdblock(enabled) {
      const val = enabled ? "1" : "0";
      fetch('/control?zzzAdblockOn=' + val)
        .then(() => {
          updateStats();
        });
    }

    function saveWifiCredentials() {
      const ssid = document.getElementById('wifiSsid').value.trim();
      const pass = document.getElementById('wifiPass').value.trim();
      const msg = document.getElementById('wifiMsg');
      if (!ssid) {
        msg.style.color = 'var(--accent-red)';
        msg.innerText = 'Please enter an SSID';
        return;
      }
      msg.style.color = '#ffffff';
      msg.innerText = 'Saving credentials and restarting ESP32...';
      fetch('/control?ST_SSID=' + encodeURIComponent(ssid))
        .then(() => fetch('/control?ST_Pass=' + encodeURIComponent(pass)))
        .then(() => fetch('/control?save=1'))
        .then(() => fetch('/control?reset=1'))
        .then(() => {
          msg.innerText = 'Rebooting! Reconnect to ' + ssid + ' once online.';
        })
        .catch(() => {
          msg.style.color = 'var(--accent-red)';
          msg.innerText = 'Request sent. Device is rebooting.';
        });
    }

    function addCustomDomain() {
      const d = document.getElementById('customDomain').value.trim();
      const msg = document.getElementById('customMsg');
      if (!d) return;
      msg.style.color = '#a1a1aa';
      msg.innerText = 'Adding ' + d + '...';
      fetch('/control?uLoad=' + encodeURIComponent(d))
        .then(() => {
          msg.style.color = '#ffffff';
          msg.innerText = d + ' is now blocked.';
          document.getElementById('customDomain').value = '';
          updateStats();
        })
        .catch(e => {
          msg.style.color = 'var(--accent-red)';
          msg.innerText = 'Failed to add domain.';
        });
    }

    function delCustomDomain() {
      const d = document.getElementById('customDomain').value.trim();
      const msg = document.getElementById('customMsg');
      if (!d) return;
      msg.style.color = '#a1a1aa';
      msg.innerText = 'Removing ' + d + '...';
      fetch('/control?vLoad=' + encodeURIComponent(d))
        .then(() => {
          msg.style.color = '#ffffff';
          msg.innerText = d + ' was removed from blocklist.';
          document.getElementById('customDomain').value = '';
          updateStats();
        });
    }

    function rebootEsp() {
      if (confirm('Reboot Hero\'s Walls ESP32 device?')) {
        fetch('/control?reset=1');
        alert('Device rebooting. Please wait 10 seconds.');
      }
    }

    setInterval(updateStats, 3000);
    window.onload = updateStats;
  </script>
</body>
</html>
)~";

