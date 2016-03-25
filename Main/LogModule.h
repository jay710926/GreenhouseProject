#ifndef _LOG_MODULE_H
#define _LOG_MODULE_H
#include "AbstractModule.h"
#include "Globals.h"
#include "DS3231Support.h"
#include <SD.h>

typedef struct
{
  AbstractModule* RaisedModule; // модуль, который инициировал событие
  String Message; // действие, которое было произведено
  
} LogAction; // структура с описанием действий, которые произошли 

class LogModule : public AbstractModule // модуль логгирования данных с датчиков
{
  private:

  static String _COMMA;
  static String _NEWLINE;

  unsigned long lastUpdateCall;
  #ifdef USE_DS3231_REALTIME_CLOCK
  DS3231Clock rtc;
  #endif
  int8_t lastDOW;

  bool hasSD;
  File logFile; // текущий файл для логгирования
  File actionFile; // файл с записями о произошедших действиях
  String currentLogFileName; // текущее имя файла, с которым мы работаем сейчас
  unsigned long loggingInterval; // интервал между логгированиями

#ifdef LOG_ACTIONS_ENABLED
  int8_t lastActionsDOW;
  void EnsureActionsFileCreated(); // убеждаемся, что файл с записями текущих действий создан
  void CreateActionsFile(const DS3231Time& tm); // создаёт новый файл лога с записью действий
#endif

  void CreateNewLogFile(const DS3231Time& tm);
  void GatherLogInfo(const DS3231Time& tm); 
#ifdef ADD_LOG_HEADER  
  void TryAddFileHeader();
#endif  

  String csv(const String& input);

  // HH:MM,MODULE_NAME,SENSOR_TYPE,SENSOR_IDX,SENSOR_DATA\r\n
  void WriteLogLine(const String& hhmm, const String& moduleName, const String& sensorType, const String& sensorIdx, const String& sensorData);
  
  public:
    LogModule() : AbstractModule(F("LOG")) {}

    bool ExecCommand(const Command& command, bool wantAnswer);
    void Setup();
    void Update(uint16_t dt);

    void WriteAction(const LogAction& action); // записывает действие в файл событий

};


#endif