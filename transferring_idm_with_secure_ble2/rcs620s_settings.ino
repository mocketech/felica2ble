#include <RCS620S.h>

// RCS620S設定
#define COMMAND_TIMEOUT  400
#define POLLING_INTERVAL 200
RCS620S rcs620s;

void setup_rcs620s() {
  int ret;

  // RCS620S初期化
  Serial2.begin( 115200);
  while ( !Serial2) ;

  do {
    ret = rcs620s.initDevice();
  } while (!ret);
  rcs620s.timeout = COMMAND_TIMEOUT;
}

int get_status_of_rcs620s() {
  return rcs620s.polling();
}

String get_idm() {
  String idm = "";
  byte b;
  int i;
  
  for(int i = 0; i < 8; i++){
    b = rcs620s.idm[ i];
    if ( b < 0x10) {
      idm += "0";
    }
    idm += String( b, HEX);
  }

  return idm;
}

void waiting_for_rcs620s() {
  rcs620s.rfOff();
  delay(POLLING_INTERVAL);
}
