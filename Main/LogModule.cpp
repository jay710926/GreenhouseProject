#include "LogModule.h"
#include "ModuleController.h"

#ifdef LOGGING_DEBUG_MODE
  #define LOG_DEBUG_WRITE(s) Serial.println((s))
#endif 

#define WRITE_TO_FILE(f,str) f.write((const uint8_t*) str.c_str(),str.length())
#define WRITE_TO_LOG(str) WRITE_TO_FILE(logFile,str)
#define WRITE_TO_ACTION_LOG(str) WRITE_TO_FILE(actionFile,str)

String LogModule::_COMMA;
String LogModule::_NEWLINE;

void LogModule::Setup()
{
    LogModule::_COMMA = COMMA_DELIMITER;
    LogModule::_NEWLINE = NEWLINE;

    currentLogFileName.reserve(20); // резервируем память, чтобы избежать фрагментации

   lastUpdateCall = 0;
   #ifdef USE_DS3231_REALTIME_CLOCK
   rtc = mainController->GetClock();
   #endif

   lastDOW = -1;
   
#ifdef LOG_ACTIONS_ENABLED   
   lastActionsDOW = -1;
#endif   
#ifdef USE_LOG_MODULE
   hasSD = mainController->HasSDCard();
#else
   hasSD = false;  
#endif
   loggingInterval = LOGGING_INTERVAL; // по умолчанию, берём из Globals.h. Позже - будет из настроек.
  // настройка модуля тут
 }
#ifdef LOG_ACTIONS_ENABLED 
void LogModule::CreateActionsFile(const DS3231Time& tm)
{  
  // формат YYYYMMDD.LOG
   String logFileName;
   logFileName += String(tm.year);

   if(tm.month < 10)
    logFileName += F("0");
   logFileName += String(tm.month);

   if(tm.dayOfMonth < 10)
    logFileName += F("0");
   logFileName += String(tm.dayOfMonth);

   logFileName += F(".LOG");

   String logDirectory = ACTIONS_DIRECTORY; // папка с логами действий

   if(!SD.exists(logDirectory)) // нет папки ACTIONS_DIRECTORY
   {
    #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("Creating the actions directory..."));
    #endif
      
      SD.mkdir(logDirectory); // создаём папку
   }

 if(!SD.exists(logDirectory)) // проверяем её существование, на всякий
  {
    // не удалось создать папку actions
    #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("Unable to access to actions directory!"));
    #endif

    return;
  }

  logFileName = logDirectory + String(F("/")) + logFileName; // формируем полный путь

  if(actionFile)
  {
    // уже есть открытый файл, проверяем, не пытаемся ли мы открыть файл с таким же именем
    if(logFileName.endsWith(actionFile.name())) // такой же файл
      return; 
    else
      actionFile.close(); // закрываем старый
  } // if
  
  actionFile = SD.open(logFileName,FILE_WRITE); // открываем файл
   
}
#endif
void LogModule::WriteAction(const LogAction& action)
{
#ifdef LOG_ACTIONS_ENABLED  
  EnsureActionsFileCreated(); // убеждаемся, что файл создан
  if(!actionFile)
  {
    // что-то пошло не так
    #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("NO ACTIONS FILE AVAILABLE!"));
    #endif

    return;
  }

  #ifdef LOGGING_DEBUG_MODE
  LOG_DEBUG_WRITE( String(F("Write the \"")) + action.Message + String(F("\" action...")));
  #endif


#ifdef USE_DS3231_REALTIME_CLOCK

  DS3231Time tm = rtc.getTime();
  
  String hhmm;
  if(tm.hour < 10)
     hhmm += F("0");
  hhmm += String(tm.hour);

  hhmm += F(":");
  
  if(tm.minute < 10)
    hhmm += F("0");
  hhmm += String(tm.minute);

  WRITE_TO_ACTION_LOG(hhmm); 
  WRITE_TO_ACTION_LOG(LogModule::_COMMA);
  WRITE_TO_ACTION_LOG(action.RaisedModule->GetID());
  WRITE_TO_ACTION_LOG(LogModule::_COMMA);
  WRITE_TO_ACTION_LOG(csv(action.Message));
  WRITE_TO_ACTION_LOG(LogModule::_NEWLINE);

  actionFile.flush(); // сливаем данные на диск
#else
  UNUSED(action);  
#endif
  
  yield(); // даём поработать другим модулям
#else
UNUSED(action);
#endif
  
}
#ifdef LOG_ACTIONS_ENABLED
void LogModule::EnsureActionsFileCreated()
{
#ifdef USE_DS3231_REALTIME_CLOCK
  
  DS3231Time tm = rtc.getTime();

  if(tm.dayOfWeek != lastActionsDOW)
  {
    // перешли на другой день недели, создаём новый файл
    lastActionsDOW = tm.dayOfWeek;
    
    if(actionFile)
      actionFile.close();
      
    CreateActionsFile(tm); // создаём новый файл
  }
#endif 
}
#endif
void LogModule::CreateNewLogFile(const DS3231Time& tm)
{
    if(logFile) // есть открытый файл
      logFile.close(); // закрываем его

   // формируем имя нашего нового лог-файла:
   // формат YYYYMMDD.LOG

   currentLogFileName = String(tm.year);

   if(tm.month < 10)
    currentLogFileName += F("0");
    
   currentLogFileName += String(tm.month);
   
   if(tm.dayOfMonth < 10)
    currentLogFileName += F("0");
   currentLogFileName += String(tm.dayOfMonth);

   currentLogFileName += F(".LOG");

   String logDirectory = LOGS_DIRECTORY; // папка с логами
   if(!SD.exists(logDirectory)) // нет папки LOGS_DIRECTORY
   {
    #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("Creating the logs directory..."));
    #endif
      
      SD.mkdir(logDirectory); // создаём папку
   }
   
  if(!SD.exists(logDirectory)) // проверяем её существование, на всякий
  {
    // не удалось создать папку logs
    #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("Unable to access to logs directory!"));
    #endif

    return;
  } 

   #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(String(F("Creating the ")) + currentLogFileName + String(F(" log file...")));
   #endif

   // теперь можем создать файл - даже если он существует, он откроется на запись
   currentLogFileName = logDirectory + String(F("/")) + currentLogFileName; // формируем полный путь

   logFile = SD.open(currentLogFileName,FILE_WRITE);

   if(logFile)
   {
   #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(String(F("File ")) + currentLogFileName + String(F(" successfully created!")));
   #endif
    
   }
   else
   {
   #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(String(F("Unable to create the ")) + currentLogFileName + String(F(" log file!")));
   #endif
    return;
   }

   // файл создали, можем с ним работать.
#ifdef ADD_LOG_HEADER
   TryAddFileHeader(); // пытаемся добавить заголовок в файл
#endif   
      
}
#ifdef ADD_LOG_HEADER
void LogModule::TryAddFileHeader()
{
  uint32_t sz = logFile.size();
  if(!sz) // файл пуст
  {
   #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(String(F("Adding the header to the ")) + String(logFile.name()) + String(F(" file...")));
   #endif

    // тут можем добавлять свой заголовок в файл

    // сначала опрашиваем все модули в системе, записывая их имена в файл, и попутно сохраняя те типы датчиков, которые есть у модулей
    int statesFound = 0; // какие состояния нашли

    size_t cnt = mainController->GetModulesCount();
    bool anyModuleNameWritten = false;

    
    for(size_t i=0;i<cnt;i++)
    {
      AbstractModule* m = mainController->GetModule(i);
      if(m == this) // пропускаем себя
        continue;

      // смотрим, есть ли хоть одно интересующее нас состояние, попутно сохраняя найденные нами состояния в список найденных состояний
        bool anyInterestedStatesFound = false;
        if(m->State.HasState(StateTemperature))
        {
          statesFound |= StateTemperature;
          anyInterestedStatesFound = true;
        }
        if(m->State.HasState(StateLuminosity))
        {
          statesFound |= StateLuminosity;
          anyInterestedStatesFound = true;
        }
        if(m->State.HasState(StateHumidity))
        {
          statesFound |= StateHumidity;
          anyInterestedStatesFound = true;
        }
        
        if(anyInterestedStatesFound)
        {
          // есть интересующие нас состояния, можно писать в файл имя модуля и его индекс.
          if(anyModuleNameWritten) // уже было записано имя модуля, надо добавить запятую
          {
            WRITE_TO_LOG(LogModule::_COMMA);
          }
          
          String mName = m->GetID(); // получаем имя модуля
          mName += COMMAND_DELIMITER;
          mName += String(i);

          // строка вида MODULE_NAME=IDX сформирована, можно писать её в файл
          WRITE_TO_LOG(mName);
          anyModuleNameWritten = true;
        }
    } // for

    if(anyModuleNameWritten) // записали по крайней мере имя одного модуля, надо добавить перевод строки
          WRITE_TO_LOG(LogModule::_NEWLINE);

    // теперь можем писать в файл вторую строку, с привязками типов датчиков к индексам
    String secondLine;
 
    if(statesFound & StateTemperature) // есть температура
    {
      if(secondLine.length())
        secondLine += LogModule::_COMMA;

      secondLine += LOG_TEMP_TYPE;
      secondLine += COMMAND_DELIMITER;
      secondLine += String(StateTemperature);
    }
  
    if(statesFound & StateLuminosity) // есть освещенность
    {
      if(secondLine.length())
        secondLine += LogModule::_COMMA;
        
      secondLine += LOG_LUMINOSITY_TYPE;
      secondLine += COMMAND_DELIMITER;
      secondLine += String(StateLuminosity);
    }

    if(statesFound & StateHumidity) // есть влажность
    {
      if(secondLine.length())
        secondLine += LogModule::_COMMA;
        
      secondLine += LOG_HUMIDITY_TYPE;
      secondLine += COMMAND_DELIMITER;
      secondLine += String(StateHumidity);
    }
    
   if(secondLine.length()) // можем записывать в файл вторую строку с привязкой датчиков к типам
   {
    secondLine += LogModule::_NEWLINE;
    WRITE_TO_LOG(secondLine);
   }

  // записали, выдыхаем, курим, пьём пиво :)
   #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("File header written successfully!"));
   #endif

   logFile.flush(); // сливаем данные на карту
   
   yield(); // т.к. запись на SD-карту у нас может занимать какое-то время - дёргаем кооперативный режим
    
  } // if(!sz) - файл пуст
}
#endif
void LogModule::GatherLogInfo(const DS3231Time& tm)
{
  // собираем информацию в лог
  
  if(!logFile) // что-то пошло не так
  {
    #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("Current log file not open!"));
    #endif
    return;
  }
  
  logFile.flush(); // сливаем информацию на карту
  
  yield(); // т.к. запись на SD-карту у нас может занимать какое-то время - дёргаем кооперативный режим
 
    #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("Gathering sensors data..."));
    #endif

  // строка с данными у нас имеет вид:
  // HH:MM,MODULE_NAME,SENSOR_TYPE,SENSOR_IDX,SENSOR_DATA
  // и соответствует формату CSV, т.е. если в данных есть "," и другие запрещенные символы, то данные обрамляются двойными кавычками
  String hhmm;
  if(tm.hour < 10)
     hhmm += F("0");
  hhmm += String(tm.hour);

  hhmm += F(":");
  
  if(tm.minute < 10)
    hhmm += F("0");
  hhmm += String(tm.minute);

// формируем типы данных, чтобы не дёргать их каждый раз в цикле
  String temperatureType = 
  #ifdef LOG_CHANGE_TYPE_TO_IDX
  String(StateTemperature);
  #else
  LOG_TEMP_TYPE;
  #endif

  String humidityType = 
  #ifdef LOG_CHANGE_TYPE_TO_IDX
  String(StateHumidity);
  #else
  LOG_HUMIDITY_TYPE;
  #endif

  String luminosityType = 
  #ifdef LOG_CHANGE_TYPE_TO_IDX
  String(StateLuminosity);
  #else
  LOG_LUMINOSITY_TYPE;
  #endif
  
  // он сказал - поехали
  size_t cnt = mainController->GetModulesCount();
  // он махнул рукой
    for(size_t i=0;i<cnt;i++)
    {
      AbstractModule* m = mainController->GetModule(i);
      if(m == this) // пропускаем себя
        continue;

        // смотрим, чего там есть у модуля
        String moduleName = 
        #ifdef LOG_CNANGE_NAME_TO_IDX
        String(i);
        #else
        m->GetID();
        #endif

        // обходим температуру
        uint8_t stateCnt = m->State.GetStateCount(StateTemperature);
        if(stateCnt > 0)
        {
          // о, температуру нашли, да? Так и запишем.
          for(uint8_t stateIdx = 0; stateIdx < stateCnt;stateIdx++)
          {
            OneState* os = m->State.GetStateByOrder(StateTemperature,stateIdx); // обходим все датчики последовательно, вне зависимости, какой у них индекс
            if(os)
            {
              String sensorIdx = String(os->GetIndex());
              TemperaturePair tp = *os;
              String sensorData = tp.Current;
              
              if(
                #ifdef WRITE_ABSENT_SENSORS_DATA
                true
                #else
                tp.Current.Value != NO_TEMPERATURE_DATA // только если датчик есть на линии
                #endif
                ) 
              {
                  // пишем строку с данными
                  WriteLogLine(hhmm,moduleName,temperatureType,sensorIdx,sensorData);
              } // if

            } // if(os)
            #ifdef _DEBUG
            else
            {
             Serial.println(F("[ERR] LOG - No GetState(StateTemperature)!"));
            }
            #endif
            
          } // for
          
        } // if(stateCnt > 0)

         // температуру обошли, обходим влажность
        stateCnt = m->State.GetStateCount(StateHumidity);
        if(stateCnt > 0)
        {
          // нашли влажность
          for(uint8_t stateIdx = 0; stateIdx < stateCnt;stateIdx++)
          {
            OneState* os = m->State.GetStateByOrder(StateHumidity,stateIdx);// обходим все датчики последовательно, вне зависимости, какой у них индекс
            if(os)
            {
              String sensorIdx = String(os->GetIndex());
              HumidityPair hp = *os;
              String sensorData = hp.Current;
              if(
                #ifdef WRITE_ABSENT_SENSORS_DATA
                true
                #else
                hp.Current.Value != NO_TEMPERATURE_DATA // только если датчик есть на линии
                #endif
                ) 
              {
                  // пишем строку с данными
                  WriteLogLine(hhmm,moduleName,humidityType,sensorIdx,sensorData);
              } // if

            } // if(os)
            #ifdef _DEBUG
            else
            {
             Serial.println(F("[ERR] LOG - No GetState(StateHumidity)!"));
            }
            #endif
            
          } // for
          
        } // if(stateCnt > 0)

        // влажность обошли, обходим освещенность
        stateCnt = m->State.GetStateCount(StateLuminosity);
        if(stateCnt > 0)
        {
          // нашли освещенность
          for(uint8_t stateIdx = 0; stateIdx < stateCnt;stateIdx++)
          {
            OneState* os = m->State.GetStateByOrder(StateLuminosity,stateIdx);// обходим все датчики последовательно, вне зависимости, какой у них индекс
            if(os)
            {
              String sensorIdx = String(os->GetIndex());
              LuminosityPair lp = *os;
              long dt = lp.Current;
              
              String sensorData = String(dt);
              if(
                #ifdef WRITE_ABSENT_SENSORS_DATA
                true
                #else
                dt != NO_LUMINOSITY_DATA // только если датчик есть на линии
                #endif
                ) 
              {
                  // пишем строку с данными
                  WriteLogLine(hhmm,moduleName,luminosityType,sensorIdx,sensorData);
              } // if

            } // if(os)
            #ifdef _DEBUG
            else
            {
             Serial.println(F("[ERR] LOG - No GetState(StateLuminosity)!"));
            }
            #endif
            
          } // for
          
        } // if(stateCnt > 0)

        
    } // for

  
    // записали, выдохнули, расслабились.
    #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("Sensors data gathered."));
    #endif
  
}
void LogModule::WriteLogLine(const String& hhmm, const String& moduleName, const String& sensorType, const String& sensorIdx, const String& sensorData)
{
  // пишем строку с данными в лог
  // HH:MM,MODULE_NAME,SENSOR_TYPE,SENSOR_IDX,SENSOR_DATA\r\n
  WRITE_TO_LOG(hhmm);             WRITE_TO_LOG(LogModule::_COMMA);
  WRITE_TO_LOG(moduleName);       WRITE_TO_LOG(LogModule::_COMMA);
  WRITE_TO_LOG(sensorType);       WRITE_TO_LOG(LogModule::_COMMA);
  WRITE_TO_LOG(sensorIdx);        WRITE_TO_LOG(LogModule::_COMMA);
  WRITE_TO_LOG(csv(sensorData));  WRITE_TO_LOG(LogModule::_NEWLINE);

  logFile.flush(); // сливаем данные на карту

  yield(); // т.к. запись на SD-карту у нас может занимать какое-то время - дёргаем кооперативный режим

}
String LogModule::csv(const String& src)
{
  String fnd = F("\"");
  String rpl = fnd + fnd;
  String input = src;
  input.replace(fnd,rpl); // заменяем кавычки на двойные
  
 if(input.indexOf(LogModule::_COMMA) != -1 ||
    input.indexOf(F("\"")) != -1 ||
    input.indexOf(F(";")) != -1 ||
    input.indexOf(F(",")) != -1 || // прописываем запятую принудительно, т.к. пользователь может переопределить COMMA_DELIMITER
    input.indexOf(LogModule::_NEWLINE) != -1
 )
 { // нашли запрещённые символы - надо обрамить в двойные кавычки строку
  
  String s; s.reserve(input.length() + 2);
  s += fnd;
  s += input;
  s += fnd;
  
  return s;
 }

  return input;
}
void LogModule::Update(uint16_t dt)
{ 
  lastUpdateCall += dt;
  if(lastUpdateCall < loggingInterval) // не надо обновлять ничего - не пришло время
    return;
  else
    lastUpdateCall = 0;

  if(!hasSD) // нет карты или карту не удалось инициализировать
  {
    #ifdef LOGGING_DEBUG_MODE
    LOG_DEBUG_WRITE(F("NO SD CARD PRESENT!"));
    #endif
    return;
  }
#ifdef USE_DS3231_REALTIME_CLOCK
  DS3231Time tm = rtc.getTime();
  if(lastDOW != tm.dayOfWeek) // наступил следующий день недели, надо создать новый лог-файл
  {
   lastDOW = tm.dayOfWeek;
   CreateNewLogFile(tm); // создаём новый файл
#ifdef LOG_ACTIONS_ENABLED
   EnsureActionsFileCreated(); // создаём новый файл действий, если он ещё не был создан
#endif
  }

  GatherLogInfo(tm); // собираем информацию в лог
#endif    
  // обновление модуля тут

}

bool LogModule::ExecCommand(const Command& command, bool wantAnswer)
{
  UNUSED(wantAnswer);

  PublishSingleton = UNKNOWN_COMMAND;
  size_t argsCnt = command.GetArgsCount();

if(hasSD)
{
  if(command.GetType() == ctSET) 
  {
    PublishSingleton = NOT_SUPPORTED;
  }
  else
  {
    if(argsCnt > 0)
    {
      String cmd = command.GetArg(0);
      if(cmd == FILE_COMMAND)
      {
        // надо отдать файл
        if(argsCnt > 1)
        {
          // получаем полное имя файла
          String fileNameRequested = command.GetArg(1);
          String fullFilePath = LOGS_DIRECTORY;
          fullFilePath += F("/");
          fullFilePath += fileNameRequested;

          if(SD.exists(fullFilePath.c_str()))
          {
            // такой файл существует, можно отдавать
            if(logFile)
              logFile.close(); // сперва закрываем текущий лог-файл

            // теперь можно открывать файл на чтение
            File fRead = SD.open(fullFilePath,FILE_READ);
            if(fRead)
            {
              // файл открыли, можно читать
              // сперва отправим в потом строчку OK=FOLLOW
              Stream* writeStream = command.GetIncomingStream();
              writeStream->print(OK_ANSWER);
              writeStream->print(COMMAND_DELIMITER);
              writeStream->println(FOLLOW);

              //теперь читаем из файла блоками, делая паузы для вызова yield через несколько блоков
              const int DELAY_AFTER = 2;
              int delayCntr = 0;

              uint16_t readed;

               while(fRead.available())
               {
                readed = fRead.read(SD_BUFFER,SD_BUFFER_LENGTH);
                //writeStream->write(fRead.read()); // пишем в поток
                writeStream->write(SD_BUFFER,readed);
                delayCntr++;
                if(delayCntr > DELAY_AFTER)
                {
                  delayCntr = 0;
                  yield(); // даём поработать другим модулям
                }
               } // while
              
              
              fRead.close(); // закрыли файл
              PublishSingleton.Status = true;
              PublishSingleton = END_OF_FILE; // выдаём OK=END_OF_FILE
            } // if(fRead)

            #ifdef USE_DS3231_REALTIME_CLOCK
                DS3231Time tm = rtc.getTime();
                CreateNewLogFile(tm); // создаём новый файл
            #endif
            
          } // SD.exists
          
        }
        else
        {
          PublishSingleton = PARAMS_MISSED;
        }
        
      } // FILE_COMMAND
      else
      {
        PublishSingleton = UNKNOWN_COMMAND;
      }
       
    } // argsCnt > 0
  } // ctGET
  
} // hasSD
  
  // отвечаем на команду
  mainController->Publish(this,command);

  return true;
}
