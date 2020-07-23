#include <string.h>

String oldIDm;

// pass keyアップデート
void update_passKey() {
  String key;
  int i;

  // デバイス接続まち
  while( ! ble_device_is_connected()) {
    delay( 500);
  }
  
  // 受付待ち状態の通知
  Serial.println( "Waiting for a new pass key...");
  while ( true) {
    notify_data( "Give new pass key...");

    // 新pass key受け取り
    if ( ble_data_is_received()) {
      while ( ( key = receive_data()) == "") {
        delay( 100);
      }
  
      // 新pass key妥当性チェック(6桁数字)
      for ( i = 0; i < 6; i++) {
        if ( !isDigit( key[ i])) break;  
      }

      if ( i < 6) {
        notify_data( "Failed.");  
      } else {
        write_passKey( key);
        notify_data( "Succeeded.");
        break;
      }
    }
    delay( 1000);
  }
}

void setup() {
  uint32_t passkey;
  int ret;

  delay( 1000);
  
  // デバッグ用シリアル回線の初期化
  Serial.begin( 115200);
  while ( !Serial) ;
  Serial.println( "Serial initialized.");

  // SPIFFS初期化
  setup_spiffs();
  Serial.println( "SPIFFS initialized.");
  passkey = read_passKey().toInt();
  Serial.println( passkey);
  
  // BLE初期化
  setup_ble( passkey);
  Serial.println( "BLE initialized.");

  // passkeyが0の場合、passkeyを更新する。
  if ( passkey == 0) {
    Serial.println( "Entering to update pass key...");
    update_passKey();
    passkey = read_passKey().toInt();
    Serial.println( passkey);
    Serial.println( "Restart...");
    notify_data( "Restart...");
    delay( 1000);
    ESP.restart();
  }

  // RC-S620S初期化
  setup_rcs620s();
  Serial.println( "RCS620S initialized.");
}

void loop() {
  int ret;
  char buf[ 30];
  String IDm = "";
  
  // FeliCaのタッチ状態を得る
  ret = get_status_of_rcs620s();

  // FeliCaがタッチされた場合
  if(ret) {
    // IDmを取得する
    IDm = get_idm();
  }

  if ( oldIDm != IDm) {
    oldIDm = IDm;
    // BLEで送信
    if ( IDm != "") {
      notify_data( IDm);    
      Serial.println( "Sent IDm: " + IDm);
    }
  }
  
  // 待機
  waiting_for_rcs620s();
}
