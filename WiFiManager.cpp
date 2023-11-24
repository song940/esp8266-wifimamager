#include "WiFiManager.h"

DNSServer dns;
WiFiServer server(80);

WiFiManager::WiFiManager(const char *apName)
{
  _apName = apName;
  _customSSID = WiFiManagerParameter("s", "SSID", "", 32);
  _customPassword = WiFiManagerParameter("p", "Password", "", 64);

  addParameter(&_customSSID);
  addParameter(&_customPassword);
}

void WiFiManager::begin(int eepromStart)
{
  _eepromStart = eepromStart;
  EEPROM.begin(512);
}

void WiFiManager::begin()
{
  begin(0);
}

boolean WiFiManager::autoConnect()
{
  return autoConnect(0);
}

boolean WiFiManager::autoConnect(int eepromStart)
{
  begin(eepromStart);

  delay(10);

  Serial.println();
  Serial.println("AutoConnect");
  // read eeprom for ssid and pass
  loadParameters();
  String ssid = _customSSID.getValue();
  String pass = _customPassword.getValue();

  if (ssid.length() > 1)
  {
    Serial.println("Waiting for Wifi to connect");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    if (waitForConnection())
    {
      Serial.println("WiFi Connected, IP: " + WiFi.localIP().toString());
      return true;
    }
  }
  startConfigMode();
  return false;
}

boolean WiFiManager::waitForConnection(void)
{
  int c = 0;
  while (c < 20)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("");
      return true;
    }
    delay(500);
    Serial.print(".");
    c++;
  }
  Serial.println("");
  Serial.println("Could not connect to WiFi");
  return false;
}

void WiFiManager::startWebConfig()
{
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());

  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());
  // Start the server
  server.begin();
  Serial.println("Server started");

  while (serverLoop() == WM_WAIT)
  {
    // looping
  }
}

void WiFiManager::startConfigMode(void)
{

  WiFi.softAP(_apName);
  Serial.println("Started Soft Access Point");

  startWebConfig();
  Serial.println("Setup done");
  delay(10000);
  ESP.reset();
}

int WiFiManager::serverLoop()
{
  dns.processNextRequest();
  WiFiClient client = server.accept();
  if (!client)
  {
    return (WM_WAIT);
  }
  // Wait for data from client to become available
  while (client.connected() && !client.available())
  {
    delay(1);
  }

  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');

  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1)
  {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return (WM_WAIT);
  }
  String path = req.substring(addr_start + 1, addr_end);
  Serial.println("request: " + req);
  client.flush();

  String s;
  if (path == "/")
  {

    s = HTTP_200;
    s += HTTP_HEAD;
    s += HTTP_SCRIPT;
    s += HTTP_STYLE;
    s += HTTP_HEAD_END;
    s.replace("{v}", _apName);

    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
      Serial.println("no networks found");
      s += "<div>No networks found. Refresh to scan again.</div>";
    }
    else
    {
      for (int i = 0; i < n; ++i)
      {
        s += "<li>";
        s += "<a href='#' onclick='c(this)'>" + WiFi.SSID(i) + "</a>";
        s += "<i>" + String(WiFi.RSSI(i)) + "db</i>";
        s += "</li>";
      }
    }

    s += "<form method='get' action='s'>";
    s += "<h2>Wi-Fi Config</h2>";

    for (int i = 0; i < _paramsCount; i++)
    {
      String id = String(_params[i]->getId());
      String name = String(_params[i]->getName());
      String len = String(_params[i]->getLength());
      String value = String(_params[i]->getDefaultValue());
      s += "<label for='" + id + "'>" + name + "</label>";
      s += "<input type='text' id='" + id + "' name='" + id + "' length='" + len + "' placeholder='" + name + "' value='" + value + "'>";
    }

    s += "<input type='submit'>";
    s += "</form>";
    s += HTTP_END;

    Serial.println("Sending config page");
  }
  else if (path.startsWith("/s"))
  {
    parseQueryString(path);
    delay(100);
    saveParameters();
    s = HTTP_200;
    s += HTTP_HEAD;
    s += HTTP_STYLE;
    s += HTTP_HEAD_END;
    s += "Saved to EEPROM...<br/>Restart in 10 seconds";
    s += HTTP_END;
    s.replace("{v}", "Saved config");
    client.print(s);
    client.flush();

    Serial.println("Saved WiFiConfig...restarting.");
    return WM_DONE;
  }
  else
  {
    s = "HTTP/1.1 302 Found\r\n";
    s += "Location: http://" + WiFi.softAPIP().toString() + "\r\n";
    s += "Content-Type: text/html\r\n";
    s += "Connection: close\r\n";
    s += "\r\n";
    s += "Redirecting...";
  }

  client.print(s);
  client.flush();
  return (WM_WAIT);
}

String WiFiManager::getEEPROMString(int start, int len)
{
  String out = "";
  char firstChar = char(EEPROM.read(_eepromStart + start));
  // 如果第一个字符不是 '\0'，则返回空字符串
  if (firstChar != '\0')
  {
    return out;
  }

  for (int i = _eepromStart + start + 1; i < _eepromStart + start + len; i++)
  {
    char readChar = char(EEPROM.read(i));
    if (readChar == '\0')
    { // 遇到零字符，表示字符串结束
      break;
    }
    out += readChar;
  }
  return out;
}
void WiFiManager::setEEPROMString(int start, int len, String value)
{
  // 首先写入一个零字符作为开始标记
  EEPROM.write(_eepromStart + start, '\0');
  unsigned int si = 0;
  for (int i = _eepromStart + start + 1; i < _eepromStart + start + len; i++)
  {
    char c = 0; // 默认为零字符
    if (si < value.length())
    {
      c = value[si++];
    }
    EEPROM.write(i, c);
    Serial.println("Wrote: " + String(c));
    if (si >= value.length())
    {
      // 写入结束的零字符
      EEPROM.write(i + 1, '\0');
      break;
    }
  }
  EEPROM.commit();
}

void WiFiManager::addParameter(WiFiManagerParameter *p)
{
  if (_paramsCount < MAX_PARAMS)
  {
    _params[_paramsCount] = p;
    _paramsCount++;
  }
  else
  {
    Serial.println("Error: Too many parameters added to WiFiManager");
  }
}

WiFiManagerParameter **WiFiManager::getParameters()
{
  return _params;
}

void WiFiManager::loadParameters()
{
  int pos = 0;
  for (int i = 0; i < _paramsCount; i++)
  {
    String value = getEEPROMString(pos, _params[i]->getLength());
    Serial.println("Loading key: " + String(_params[i]->getId()) + ", Pos:" + String(pos) + ", Length:" + String(_params[i]->getLength()) + ", Value:" + value);
    pos += _params[i]->getLength();
    _params[i]->setValue(value);
  }
}

void WiFiManager::saveParameters()
{
  int pos = 0;
  for (int i = 0; i < _paramsCount; i++)
  {
    Serial.println("Saving: " + String(_params[i]->getId()) + ", Value: " + _params[i]->getValue());
    setEEPROMString(pos, _params[i]->getLength(), _params[i]->getValue());
    pos += _params[i]->getLength();
  }
}

void WiFiManager::parseQueryString(String queryString)
{
  // 移除路径部分，只保留查询字符串
  int pathEnd = queryString.indexOf('?');
  if (pathEnd != -1)
  {
    queryString = queryString.substring(pathEnd + 1);
  }

  // 分割查询字符串
  int pos = 0;
  while (pos != -1)
  {
    int next = queryString.indexOf('&', pos);
    String token = next == -1 ? queryString.substring(pos) : queryString.substring(pos, next);
    pos = next != -1 ? next + 1 : -1;

    // 分离键值对
    int separator = token.indexOf('=');
    if (separator != -1)
    {
      String key = token.substring(0, separator);
      String value = token.substring(separator + 1);
      value = urlDecode(value); // URL解码
      Serial.println("Key: " + key + ", Value: " + value);
      updateParameters(key, value);
    }
  }
}

String WiFiManager::urlDecode(String input)
{
  String decoded = "";
  char temp[] = "0x00";
  int length = input.length();

  for (int i = 0; i < length; i++)
  {
    if (input[i] == '%')
    {
      if (i + 2 < length)
      {
        temp[2] = input[i + 1];
        temp[3] = input[i + 2];
        decoded += char(strtol(temp, NULL, 16));
        i += 2;
      }
    }
    else if (input[i] == '+')
    {
      decoded += ' ';
    }
    else
    {
      decoded += input[i];
    }
  }

  return decoded;
}

void WiFiManager::updateParameters(String key, String value)
{
  for (int i = 0; i < _paramsCount; i++)
  {
    if (key == _params[i]->getId())
    {
      _params[i]->setValue(value.c_str());
      Serial.println("Updated: " + String(_params[i]->getId()) + " = " + String(_params[i]->getValue()));
      break;
    }
  }
}