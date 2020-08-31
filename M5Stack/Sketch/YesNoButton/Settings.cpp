#include "Settings.h"
#include <ArduinoJson.h>
#include <M5Stack.h>

// SDカード上の設定ファイルパスを変える場合はここを書き換えてください
const String Settings::baseDir = "/yesnobutton/";
const String Settings::jsonFile = "settings.json";

// SDカード無しで動かす場合はこの内容（特にwifi）を書き換えてください
const char* Settings::defaultSettings = "{" \
  "\"url\":\"https://script.google.com/macros/s/AKfycbyu1TKccjd8SSgMQ2PRgT_kYqjLvatSN-2fhoHdOoFRxdwCnrGV/exec\"," \
  "\"keyword\":\"\"," \
  "\"is_transmittrer\":0," \
  "\"enable_description\":1," \
  "\"interval_minutes\":2.0," \
  "\"ca_file\":\"rootca.crt\"," \
  "\"font_small\":\"\"," \
  "\"font_medium\":\"\"," \
  "\"font_large\":\"\"," \
  "\"wifi\":[" \
  " {\"ssid\":\"foo\",\"password\":\"hogehoge\"}," \
  " {\"ssid\":\"bar\",\"password\":\"fugafuga\"}" \
  "]," \
  "\"button\":[" \
  " {\"label\":\"Set 0\"}," \
  " {\"label\":\"Set 1\"}," \
  " {\"label\":\"Set 2\"}," \
  " {\"label\":\"Set 3\"}" \
  "]}";

// 2020/08現在、script.google.com で利用されていたルート証明書
const char* Settings::defaultRootCA = \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n" \
  "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n" \
  "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n" \
  "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n" \
  "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n" \
  "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n" \
  "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n" \
  "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n" \
  "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n" \
  "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n" \
  "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n" \
  "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n" \
  "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n" \
  "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n" \
  "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n" \
  "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n" \
  "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n" \
  "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n" \
  "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n" \
  "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n" \
  "-----END CERTIFICATE-----\n";

Settings::Settings()
{
}

bool Settings::load()
{
  String json = "";
  wasLoaded = false;

  File file = SD.open(baseDir + jsonFile, FILE_READ);

  if (file) {
    while (file.available()) {
      json += file.readString();
    }
    file.close();

    wasLoaded = parse(json);
    isExists = true;
  }

  if (!wasLoaded) {
    // SDカードから読めなければデフォルト値
    json = String(defaultSettings);
    wasLoaded = parse(json);
    isExists = false;
  }

  return wasLoaded;
}

// ルート証明書読込
bool Settings::loadCA() {
  if (caFile != "") {
    String ca = "";
    File file = SD.open(baseDir + caFile, FILE_READ);

    if (file) {
      while (file.available()) {
        ca += file.readString();
      }
      file.close();
    }
    ca.toCharArray(rootCA, SETTINGS_MAX_CA);
  }
  if (rootCA == "") {
    strcpy(rootCA, defaultRootCA);
  }
}

// 設定のJSONを解釈
bool Settings::parse(String json) {
  StaticJsonDocument<2000> buf;
  auto error = deserializeJson(buf, json);

  if (error)
  {
    return false;
  }
  else
  {
    // JSONから情報を取り出しメンバー変数に保存
    JsonObject obj = buf.as<JsonObject>();

    intervalMinutes = obj["interval_minutes"].as<float>();
    endPoint = obj["url"].as<String>();
    caFile = obj["ca_file"].as<String>();
    keyword = obj["keyword"].as<String>();
    isTransmitter = obj["is_transmitter"].as<bool>();
    enableDescription = obj["enable_description"].as<bool>();
    smallFontName = baseDir + obj["font_small"].as<String>();
    mediumFontName = baseDir + obj["font_medium"].as<String>();
    largeFontName = baseDir + obj["font_large"].as<String>();
    isSmallFontAvailable = SD.exists(smallFontName + ".vlw");
    isMediumFontAvailable = SD.exists(mediumFontName + ".vlw");
    isLargeFontAvailable = SD.exists(largeFontName + ".vlw");

    // Wi-Fi設定一覧
    wifiCount = min(obj["wifi"].size(), SETTINGS_MAX_WIFI);
    for (int i = 0; i < wifiCount; i++) {
      obj["wifi"][i]["ssid"].as<String>().toCharArray(wifiSSID[i], SETTINGS_MAX_SSID);
      obj["wifi"][i]["password"].as<String>().toCharArray(wifiPassword[i], SETTINGS_MAX_PASSWORD);
    }

    // 送信モード時のボタン名
    for (int i = 0; i < SETTINGS_BUTTON_COUNT; i++) {
      if (obj["button"].size() <= i) break;
      obj["button"][i]["label"].as<String>().toCharArray(buttonLabel[i], SETTINGS_MAX_BUTTONNAME);
    }

    // ルート証明書読込
    loadCA();
  }
  return true;
}

// [ms]単位のインターバルを返す
unsigned long Settings::getIntervalMilliseconds() {
  return (unsigned long)(intervalMinutes * 60 * 1000);
}
