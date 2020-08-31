# M5_YesNoButton
Read and write Google Spreadsheet from M5Stack and web interface

## What's this
- Google Spreadsheetで日付ごとに「Yes」「No」「Not sure」といった、最大4つの情報を保持します。
- M5StackまたはWeb画面から読み書きができます。

晩御飯について「間に合う」「遅れる」「いらない」を遠隔で伝える目的に作成したものですが、一応汎用的に作ったつもりです。

## Author
Kirurobo

## License
MIT License

# Folder structure
- M5Stack
  - Sketch
    - YesNoButton : M5Stackスケッチ
       - YesNoButton.ino : 本体
       - Settings.h : 各種設定情報クラスの宣言
       - Setthings.cpp : SDからの設定情報読込やデフォルト値の記述
  - SD
    - yesnobutton : 設定情報を置くフォルダ。このフォルダごとmicroSDにコピーしておく
      - settings.json : 設定情報
      - rootca.crt : Google Apps ScriptにHTTPS接続する際のルート証明書

- GoogleAppsScript
   - 準備中

# Settings
setteings.json の中身は下記の項目があります。
ご利用の環境に合わせて書き換えて下さい。

- url
  - Google Apps ScriptでWebアプリケーションとして公開したURL
- interval_minutes
  - データ取得と画面書き換えを行う間隔 [min]
- is_transmitter
  - 1 (true) にすると、今日の内容をボタン3種で登録するモード
  - 0 (false) だと、表示モード。その場合ボタンは日付移動になる
- enable_description
  - 1 (true) にすると、選択されたもの以外に説明文も表示する
    - 日本語があるとM5Stackが落ちる？
  - 0 (false) だと表示しない
- ca_rile
  - GoogleへのHTTPSで使うルート証明書ファイル指定
    - なければ Settings.cpp 内の文字列が使われる
- wifi
  - ssid と password を複数組指定可能
- button
  - 登録モードの時にボタンに表示する文字列（それぞれ最大5桁）
    - 順番に状態番号0～3に対応している
    - 0については現状ボタンには対応させておらず、左から1～3がボタンに割り当てられる
- font_small, font_medium, font_large
  - 日本語を使う場合のフォント指定として用意したが、現状上手く動作しない
    - loadFontが使えたらJSONの取得に失敗したり、そうでなければM5Stackが落ちたりした

