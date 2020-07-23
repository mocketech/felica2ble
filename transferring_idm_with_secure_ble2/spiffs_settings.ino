#include "FS.h"
#include "SPIFFS.h"

// PassKey保存場所指定
const char* passKey_path = "/passkey.txt";

String read_passKey(){
  String ret = "";
  Serial.println("Reading PassKey...");

  File file = SPIFFS.open( passKey_path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return "";
  }

  Serial.print("Read from file: ");
  while (file.available()){
    char c;
    if ( ( c = file.read()) == '\n') break;
    ret = ret + char( c);
  }
  file.close();
  return ret;
}

void write_passKey( String passkey){
    Serial.println("Writing PassKey");

    File file = SPIFFS.open( passKey_path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print( passkey)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void setup_spiffs(){
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
}
