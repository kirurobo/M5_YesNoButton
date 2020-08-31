// Google Spreadsheetと連携して最大４択のデータを表示・登録する
//
// References
//  https://medium.com/tichise/m5stack%E3%81%A7%E7%B0%A1%E5%8D%98%E3%81%AA%E3%82%B9%E3%83%9E%E3%83%BC%E3%83%88%E3%83%9F%E3%83%A9%E3%83%BC%E3%82%92%E4%BD%9C%E3%81%A3%E3%81%A6%E3%81%BF%E3%81%9F-b6b229009637
//  http://www.telomere0101.site/archives/post-1008.html
//
// License
//MIT License
//
//Copyright (c) 2020 Kirurobo
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.


#include <M5Stack.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// 設定を扱うクラスはこちら
#include "Settings.h"

// シリアルポートではなくLCDにデバッグ表示するなら 1 にする（ただし現状スクロールしない）
#define DEBUG_PRINT_TO_LCD 0

#define MAX_REDIRECT 3
#define MAX_RETRY   3
#define RETRY_WAIT  10000

#define LOCATION "location"
#define JST (9 * 3600L)

#if DEBUG_PRINT_TO_LCD
#define debugPrint M5.Lcd.print
#define debugPrintln M5.Lcd.println
#else
#define debugPrint Serial.print
#define debugPrintln Serial.println
#endif

//--------------------------------------------
unsigned long lastRequestedTime;
bool status_loading = false;  // 読込中ならtrueとなる
bool status_failed = false;   // 失敗していたらtrueとなる
uint16_t status_colorcode;    // M5専用のカラーコード
String status_date;
int status_number;
String status_label;
String status_description;
String status_color;

time_t targetTimeOffset;    // 現在を基準とする、取得対象日時のオフセット [s]
String targetDate;          // 取得対象となる日の日付表現
int newStatusValue = 0;     // 送信モード時、登録する値
bool isSetRequest = false;  // 送信モード時、これがtrueなら次のリクエストはsetとなる

const String dummyString = "";
const String question = "?";

const char* headerKeys[] = {LOCATION};
const int headerKeysCount = 1;

const String circleChars = "○〇●Oo";
const String triangleChars = "△▲";
const String crossChars = "×Xx";
const String hyphenChars = "―－ー-";
const String questionChars = "？?";

WiFiMulti wifimulti;
HTTPClient http;
Settings settings;


// 設定を読み込み
void setupSettings() {
  M5.Lcd.print("Loading ");
  M5.Lcd.print(settings.baseDir + settings.jsonFile);
  M5.Lcd.println(" ...");
  if (!settings.load()) {
    M5.Lcd.println("Settings can't be loaded.");

    //設定読込失敗なら、Wi-Fi接続に行かないようウェイト
    delay(300000);
  }
  if (settings.isExists) {
    M5.Lcd.println("Loaded settings from SD.");
  } else {
    M5.Lcd.println("Loaded default settings.");
  }

//// 設定情報の表示
//  debugPrintln(settings.endPoint);
//  debugPrintln(settings.intervalMinutes);
//  debugPrintln(settings.caFile);
//  debugPrintln(settings.isTransmitter ? "Set mode" : "Get mode");
//
//  debugPrint("Small font:");
//  debugPrintln(settings.isSmallFontAvailable ? settings.smallFontName : "None");
//  debugPrint("Medium font:");
//  debugPrintln(settings.isMediumFontAvailable ? settings.mediumFontName : "None");
//  debugPrint("Large font:");
//  debugPrintln(settings.isLargeFontAvailable ? settings.largeFontName : "None");
}

// 日時をNTPから調整
void setupClock() {
  M5.Lcd.println("Adjusting the clock...");
  configTime(JST, 0, "ntp.nict.jp", "pool.ntp.org", "time.nist.gov");
}

// Wifiをセットアップする
void setupWifi()
{
  M5.Lcd.println("Waiting Wi-Fi...");

  WiFi.mode(WIFI_STA);

  // 設定にあった数だけSSID,パスワードを追加
  debugPrintln("Wi-Fi");
  for (int i = 0; i < settings.wifiCount; i++) {
    wifimulti.addAP(settings.wifiSSID[i], settings.wifiPassword[i]);

    debugPrint(settings.wifiSSID[i]);
    debugPrint(" ");
    debugPrintln(settings.wifiPassword[i]);
  }
  
  // Wifi接続待ち
  while (wifimulti.run() != WL_CONNECTED) {
    M5.Lcd.print(WiFi.SSID().c_str());
    M5.Lcd.print(".");
  }
  
  // 接続完了
  debugPrint("\nConnected.SSID:");
  debugPrint(WiFi.SSID().c_str());
  debugPrint(" IP:");
  debugPrintln(WiFi.localIP().toString().c_str());
}

// 画面をクリア
void clearScreen()
{
  // LCDをクリア
  M5.Lcd.fillScreen(TFT_BLACK);
}

// 現在の情報を表示
void showStatus()
{
  uint16_t uiColor = TFT_DARKGREY;    // 更新日やボタン欄の色
  
  // 現在日時の文字列を準備
  struct tm timeInfo;
  getLocalTime(&timeInfo);
  char dtime[20];
  sprintf(dtime, "%04d-%02d-%02d %02d:%02d",
    timeInfo.tm_year + 1900,
    timeInfo.tm_mon + 1,
    timeInfo.tm_mday,
    timeInfo.tm_hour,
    timeInfo.tm_min);

  // 大きな文字サイズに変更
  setFontSizeLarge();
  
  // 対象日付を表示
  if (status_loading) {
    // 読込中ならば灰色にしておく
    M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  } else {
    // 通常は白色
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  M5.Lcd.drawString(status_date.substring(5, 7) + "/" + status_date.substring(8,10), 100, 15);   // 月日のみ

  // ラベル、またはそれに対応した図形を表示
  if (status_loading) {
      // Draw loading message
      M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      M5.Lcd.drawString("Loading...", 10, 110);
  } else if (circleChars.indexOf(status_label) >= 0) {
      // Draw circle
      M5.Lcd.fillCircle(160, 120, 60, status_colorcode);
      M5.Lcd.fillCircle(160, 120, 50, TFT_BLACK);
  } else if (triangleChars.indexOf(status_label) >= 0) {
      // Draw triangle
      M5.Lcd.fillTriangle(160, 60, 99, 165, 221, 165, status_colorcode);
      M5.Lcd.fillTriangle(160, 80, 117, 155, 203, 155, TFT_BLACK);
  } else if (crossChars.indexOf(status_label) >= 0) {
      // Draw cross
      M5.Lcd.fillTriangle(105, 65, 95, 75, 215, 195, status_colorcode);
      M5.Lcd.fillTriangle(105, 65, 225, 185, 215, 195, status_colorcode);
      M5.Lcd.fillTriangle(215, 65, 225, 75, 105, 195, status_colorcode);
      M5.Lcd.fillTriangle(215, 65, 95, 185, 105, 195, status_colorcode);
  } else if (hyphenChars.indexOf(status_label) >= 0) {
      // Draw hyphen
      M5.Lcd.fillRect(70, 115, 180, 20, status_colorcode);
  } else if (questionChars.indexOf(status_label) >= 0) {
      // Draw question mark
      M5.Lcd.fillCircle(160, 100, 40, status_colorcode);
      M5.Lcd.fillCircle(160, 100, 30, TFT_BLACK);
      M5.Lcd.fillRect(120, 100, 40, 40, TFT_BLACK);
      M5.Lcd.fillRect(155, 130, 10, 30, status_colorcode);
      M5.Lcd.fillRect(155, 170, 10, 10, status_colorcode);
  } else {
      // ラベルを表示
      M5.Lcd.setTextColor(status_colorcode, TFT_BLACK);
      M5.Lcd.drawString(String(status_label), 100, 110, 1);
  }

  // 中くらいの文字サイズに変更
  setFontSizeMedium();

  // 説明表示
  if (settings.enableDescription) {
    //M5.Lcd.setTextColor(status_colorcode, TFT_BLACK);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setCursor(0, 200);
    M5.Lcd.drawString(status_description, 0, 200, 1);
  }

  // ボタン表示
  M5.Lcd.fillRect(32, 219, 64, 20, uiColor);
  M5.Lcd.fillRect(130, 219, 64, 20, uiColor);
  M5.Lcd.fillRect(224, 219, 64, 20, uiColor);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_BLACK);
  if (settings.isTransmitter) {
    M5.Lcd.drawString(settings.buttonLabel[1], 37, 221);
    M5.Lcd.drawString(settings.buttonLabel[2], 133, 221);
    M5.Lcd.drawString(settings.buttonLabel[3], 229, 221);
  } else {
    M5.Lcd.drawString("Prev", 38, 220);
    M5.Lcd.drawString("Today", 133, 220);
    M5.Lcd.drawString("Next", 230, 220);
  }
  
  // デフォルトの文字に戻す
  if (M5.Lcd.fontsLoaded()) {
    M5.Lcd.unloadFont();
  }
  M5.Lcd.setTextSize(1);

  // 更新日時
  M5.Lcd.setTextColor(uiColor, TFT_BLACK);
  M5.Lcd.drawString(dtime, 0, 0);
  M5.Lcd.drawRightString("Updated every " + String(settings.intervalMinutes) + " Min.", 320, 0, 1);

  // 文字列描画設定を戻す
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
}

// 色の名前（小文字）からM5の色を取得
uint16_t getColorCode(String color)
{
  if (color == "black") {
    return TFT_WHITE;     // ダークモード相当で、反転しています
  } else if (color == "white") {
    return TFT_DARKGREY;  // ダークモード相当で、反転しています
  } else if (color == "gray" || color == "grey") {
    return TFT_LIGHTGREY;
  } else if (color == "red") {
    return TFT_RED;
  } else if (color == "green") {
    return TFT_GREEN;
  } else if (color == "blue") {
    return TFT_BLUE;
  } else if (color == "yellow") {
    return TFT_YELLOW;
  } else if (color == "cyan") {
    return TFT_CYAN;
  } else if (color == "magenta" || color == "purple") {
    return TFT_MAGENTA;
  }
  // デフォルト
  return TFT_WHITE;
}

// 小さなフォントにする
void setFontSizeSmall() {
  if (M5.Lcd.fontsLoaded()) {
    M5.Lcd.unloadFont();
  }
  if (settings.isSmallFontAvailable) {
    M5.Lcd.loadFont(settings.smallFontName, SD);
  } else {
    M5.Lcd.setTextSize(1);
  }
}

// 中くらいのフォントにする
void setFontSizeMedium() {
  if (M5.Lcd.fontsLoaded()) {
    M5.Lcd.unloadFont();
  }
  if (settings.isMediumFontAvailable) {
    M5.Lcd.loadFont(settings.mediumFontName, SD);
  } else {
    M5.Lcd.setTextSize(2);
  }
}

// 大きなフォントにする
void setFontSizeLarge() {
  if (M5.Lcd.fontsLoaded()) {
    M5.Lcd.unloadFont();
  }
  if (settings.isLargeFontAvailable) {
    M5.Lcd.loadFont(settings.largeFontName, SD);
  } else {
    M5.Lcd.setTextSize(4);
  }
}

// 送信モード時、次回リクエストを登録用として新しい値をセット
void setNewValue(int value) {
  if (settings.isTransmitter) {
    newStatusValue = value;
    isSetRequest = true;
  }
}

// メインとなるHTTPSリクエスト
bool request(){
  debugPrint("\nTarget date: ");
  debugPrintln(targetDate);
  
  String query = getQueryString();
  bool result = false;
  for (int i = 0; i < MAX_RETRY; i++) {
    result = requestStatus(query, 0);
    
    // 情報を要求した時刻を保存
    lastRequestedTime = millis();
    
    if (result) break;
    delay(RETRY_WAIT);   // 失敗していれば待った後でリトライ
  }
  status_failed = (!result);  
  isSetRequest = false;       // 登録用のクエリは終了
  return result;
}

// エンドポイントとGETパラメータを結合した文字列を返す
String getQueryString() {
  String query = "";
  query.concat(settings.endPoint);
  query.concat("?o=JSON&d=");
  query.concat(targetDate);
  query.concat("&k=");
  query.concat(settings.keyword);

  // 登録用リクエストにする場合
  if (isSetRequest) {
    query.concat("&m=set&v=");
    query.concat(newStatusValue);
  }
  return query;
}

// 再帰呼び出し対応のHTTPSリクエスト
bool requestStatus(String url, int redirect_count)
{
  bool result = false;

  // 接続
  debugPrint("Redirect#: ");
  debugPrint(redirect_count);
  debugPrint(" Heap: ");
  debugPrintln(esp_get_free_heap_size());
  debugPrint("Url: ");
  debugPrintln(url);
  debugPrintln("");
  
  http.begin(url, settings.rootCA);

  // 拾うヘッダーを指定
  http.collectHeaders(headerKeys, headerKeysCount);
  
  // 新規リクエスト開始
  debugPrintln("GET...");

  // GETリクエストを送信
  int http_code = http.GET();

  //// デバッグ用にヘッダーを表示
  //debugPrintHttpHeaders();

  //// 応答結果を出力
  debugPrint(String(http_code));
  debugPrint(" ");
  debugPrintln(http.errorToString(http_code));
  
  if (http_code == 200) {
    // リクエスト成功ならJSONを取得して解釈
    result = parseJSON(http.getString());
    debugPrintln("JSON OK");
  } else if (http_code == 302) {
    if (redirect_count >= MAX_REDIRECT) {
      // リダイレクト回数オーバー
      debugPrintln("Error : Too many redirects");
      result = false;
    } else if (!http.hasHeader(LOCATION)) {
      // リダイレクト先が無ければ失敗
      debugPrintln("Error : Redirect location not found");
      result = false;
    } else {
      // 再帰的にリダイレクト先呼び出し
      String nextUrl = http.header(LOCATION);
      http.end();
      return requestStatus(nextUrl, redirect_count + 1);
    }
  } else {
    // JSON取得失敗
    debugPrintln("Error on http request");
    debugPrint(String(http_code));
    debugPrint(" ");
    debugPrintln(http.errorToString(http_code));
    debugPrintln("");
    debugPrintln(http.getString());
    debugPrintln("");
    result = false;
  }
  
  // 切断
  http.end();
  
  return result;
}

// デバッグ用。受け取ったヘッダー一覧を表示
void debugPrintHttpHeaders()
{
  int count = http.headers();
  debugPrint("Headers:");
  debugPrintln(String(count));
  for (int i = 0; i < count; i++) {
    debugPrint(http.headerName(i));
    debugPrint(":");
    debugPrintln(http.header(i));
  }
}

// 受信されたJSONから必要なデータを記憶
bool parseJSON(String json)
{
  StaticJsonDocument<2000> json_buffer;
  auto error = deserializeJson(json_buffer, json);
  
  if (error)
  {
    debugPrint("JSON deserializing failed.");
    debugPrintln(error.c_str());
    return false;
  }
  else
  {
    // JSONから情報を取り出しグローバル変数に保存
    JsonObject json_object = json_buffer.as<JsonObject>();

    // 受け取ったそのままを使う情報
    status_date = json_object["date"].as<String>();
    status_number = json_object["status"].as<int>();
    status_label = json_object["label"].as<String>();
    status_description = json_object["description"].as<String>();
    status_color = json_object["color"].as<String>();

    // このプログラム独自の情報
    status_color.toLowerCase();
    status_colorcode = getColorCode(status_color);
    status_loading = false;
    
    //// 情報を取得した時間を保存
    //lastRequestedTime = millis();
  }
  return true;
}

// 対象日付を進めるまたは戻す
void incrementTargetDate(int days)
{
  targetTimeOffset += days * 24 * 60 * 60;   // 秒に変換
}

// 対象日付を今日の日付とする
void resetTargetDate()
{
  targetTimeOffset = 0;
}

// targetTimeOffset だけ現在からずらした targetDate 文字列を生成
void updateTargetDateString()
{
  time_t now;
  time(&now);
  now += targetTimeOffset;
  struct tm *targettm = localtime(&now);
  char datestr[11];
  sprintf(datestr, "%04d-%02d-%02d",
    targettm->tm_year + 1900,
    targettm->tm_mon + 1,
    targettm->tm_mday
  );
  targetDate = String(datestr);
}

// 空の状態で画面更新
void resetStatusAndShow()
{
  // 対象日付文字列生成
  updateTargetDateString();

  // 読込情報
  status_loading = true;
  status_failed = false;
  
  // 表示情報初期値
  status_date = targetDate;
  status_number = -1;
  status_label = question;
  status_description = dummyString;
  status_color = dummyString;
  status_colorcode = TFT_WHITE;

  // 画面消去と描画
  clearScreen();
  showStatus();
}

// データの取得と描画
void getStatusAndShow() {
  updateTargetDateString();
  request();
  clearScreen();
  showStatus();
}

// 初期化
void setup()
{
  // M5Stackオブジェクトを初期化する
  M5.begin();
  
  // LCDの色設定
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);

  // 設定の読込
  setupSettings();
  
  // Wifiに接続する
  setupWifi();

  // NTPで時刻合わせ
  setupClock();

  // 対象日付を初期化
  resetTargetDate();

  // 初期値表示
  resetStatusAndShow();
  
  // 初回の情報取得
  getStatusAndShow();
}

// メインループ
void loop()
{
  unsigned long currentTime = millis();
  M5.update();

  if (M5.BtnA.wasPressed()) {
    if (settings.isTransmitter) {
      setNewValue(1);
    } else {
      incrementTargetDate(-1);
    }
    resetStatusAndShow();
    getStatusAndShow();
  } else if (M5.BtnC.wasPressed()) {
    if (settings.isTransmitter) {
      setNewValue(3);
    } else {
      incrementTargetDate(1);
    }
    resetStatusAndShow();
    getStatusAndShow();
  } else if (M5.BtnB.wasPressed()) {
    if (settings.isTransmitter) {
      setNewValue(2);
    }
    resetTargetDate();    // これは送信モード時でも行っても良い
    resetStatusAndShow();
    getStatusAndShow();
  } else if (settings.getIntervalMilliseconds() < (currentTime - lastRequestedTime)) {
    // 指定時間経過で再取得
    getStatusAndShow();
  }
}
