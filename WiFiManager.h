#ifndef WiFiManager_h
#define WiFiManager_h

#include <EEPROM.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

class WiFiManagerParameter
{
public:
  WiFiManagerParameter()
  {
    WiFiManagerParameter("", "", "", 20);
  }
  WiFiManagerParameter(const char *id, const char *name, const char *defaultValue, int length)
  {
    _id = id;
    _name = name;
    _length = length;
    _value = new char[_length];
    _defaultValue = defaultValue;
  }

  void setValue(String value)
  {
    strlcpy(_value, value.c_str(), _length);
  }

  const char *getValue()
  {
    return _value;
  }

  const char *getDefaultValue()
  {
    return _defaultValue;
  }

  const char *getId()
  {
    return _id;
  }

  const char *getName()
  {
    return _name;
  }

  int getLength()
  {
    return _length;
  }

private:
  int _length;
  const char *_id;
  const char *_name;
  char *_value;
  const char *_defaultValue;
};

class WiFiManager
{
public:
  WiFiManager(void);
  WiFiManager(const char *apName);
  void begin();
  void begin(int eepromStart);
  boolean waitForConnection();
  boolean autoConnect();
  boolean autoConnect(int eepromStart);
  void addParameter(WiFiManagerParameter *p);
  WiFiManagerParameter **getParameters();
  void loadParameters();
  void saveParameters();

  void startConfigMode(void);
  void startWebConfig();

private:
  const int WM_DONE = 0;
  const int WM_WAIT = 10;

  const String HTTP_200 = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  const String HTTP_HEAD = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no\"/><title>{v}</title>";
  const String HTTP_STYLE = "<style>body{width:80%;margin:auto;}input,button{display: block;width:100%;box-sizing:border-box;margin:5px 0;font-size:14px;}li{display:flex;justify-content:space-between;}</style>";
  const String HTTP_SCRIPT = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
  const String HTTP_HEAD_END = "</head><body><h1>{v}</h1>";
  const String HTTP_END = "</body></html>";

  int serverLoop();
  int _eepromStart;
  int _paramsCount = 0;
  const char *_apName = "ESP-AP";
  static const int MAX_PARAMS = 50;
  WiFiManagerParameter *_params[MAX_PARAMS];
  WiFiManagerParameter _customSSID;
  WiFiManagerParameter _customPassword;

  String getEEPROMString(int start, int len);
  void setEEPROMString(int start, int len, String string);

  String urlDecode(String input);
  void parseQueryString(String queryString);
  void updateParameters(String key, String value);
};

#endif