#pragma once

#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_MAX_WIFI 10
#define SETTINGS_MAX_SSID 80
#define SETTINGS_MAX_PASSWORD 80
#define SETTINGS_MAX_CA 2000
#define SETTINGS_MAX_BUTTONNAME 5
#define SETTINGS_BUTTON_COUNT 4

#include <Arduino.h>

class Settings {
public:
    static const String baseDir;
    static const String jsonFile;
    static const char* defaultSettings;
    static const char* defaultRootCA;

    bool wasLoaded = false;  // 読み込めたらtrueとなる
    bool isExists = false;   // SDに設定ファイルがあればtrueとなる
    String endPoint;      // 接続先URL
    String keyword;       // 接続先のキーワード
    String caFile;        // ROOT CA のファイル名
    int wifiCount = 0;    // Wi-Fi設定の数
    char wifiSSID[SETTINGS_MAX_WIFI][SETTINGS_MAX_SSID];
    char wifiPassword[SETTINGS_MAX_WIFI][SETTINGS_MAX_PASSWORD];
    char rootCA[SETTINGS_MAX_CA];
    bool isTransmitter = false;     // 送信モードならtrueとなる
    bool enableDescription = false; // メッセージを表示させるならtrueとする（日本語だと落ちたりするので）
    char buttonLabel[SETTINGS_BUTTON_COUNT][SETTINGS_MAX_BUTTONNAME];

    // データ取得を繰り返す間隔
    float intervalMinutes = 2.0;

    // フォントが見つかればtrue
    bool isSmallFontAvailable = false;
    bool isMediumFontAvailable = false;
    bool isLargeFontAvailable = false;

    String smallFontName;
    String mediumFontName;
    String largeFontName;

    // コンストラクタ
    Settings();

    // 設定をSDカードから読込。失敗したらデフォルトを読込
    bool load();

    // 設定のJSONを解釈
    bool parse(String json);

    // [ms]単位でのインターバルを返す
    unsigned long getIntervalMilliseconds();

private:
    // ルート証明書読込
    bool loadCA();
};

#endif
