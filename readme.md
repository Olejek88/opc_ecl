Инструкция по конфигурированию OPC ECL Danfoss версия 0.1

Конфигурация сервера храниться в формате xml, поэтому ее можно править двумя методами, либо через текстовый редактор, либо через специально разработанный
визуальный конфигуратор.
Запустить конфигуратор OPC ECL. Отобразится чистая конфигурация. Дальше можно либо загрузить готовую конфигурацию, либо создать новую. 
Боевая конфигурация сервера лежит в директории %SYSTEMROOT %/SYSTEM32/ecl.xml.

Конфигурационный файл представляет из себя стандартный xml файл с следующей иерархией тегов.

- <eclOpcCfg version="1.0" description="ECL OPC configuration">
- <comports>
- <comport baudrate="38400" addr="2" description="отсутствует" com="1">
- <comfort addr="2" description="отсутствует" name="TP1">
  <tag description="Режим контура - отопление контроллера" type="0" name="Controller mode.Cir 1 mode" /> 
  <tag description="Режим контура - ГВС контроллера" type="0" name="Controller mode.Cir 2 mode" /> 
  <tag description="Состояние контура - отопление контроллера" type="0" name="Controller mode.Cir 1 status" /> 
  <tag description="Состояние контура - ГВС контроллера" type="0" name="Controller mode.Cir 2 status" /> 
  <tag description="Температура воздуха на улице" type="0" name="Circuit 1 Sensor and ref.S1 sensor" /> 
  <tag description="Температура отопления подача от Данфосс" type="0" name="Circuit 1 Sensor and ref.S3 sensor" /> 
  <tag description="Температура отопления обратка от Данфосс" type="0" name="Circuit 1 Sensor and ref.S4 sensor" /> 
  <tag description="Температура ГВС подача от Данфосс" type="0" name="Circuit 2 Sensor and ref.S5 sensor" /> 
  <tag description="Задание комнатной температуры - комфортная" type="0" name="Circuit 1 primary par._180 Day setpt" /> 
  <tag description="Задание комнатной температуры - пониженная" type="0" name="Circuit 1 primary par._181 Night setpt" /> 
  </comfort>
  </comport>
- <comport baudrate="38400" addr="2" description="отсутствует" com="2">
- <comfort addr="2" description="отсутствует" name="TP2">
  <tag description="Режим контура - отопление контроллера" type="0" name="Controller mode.Cir 1 mode" /> 
  <tag description="Режим контура - ГВС контроллера" type="0" name="Controller mode.Cir 2 mode" /> 
  <tag description="Состояние контура - отопление контроллера" type="0" name="Controller mode.Cir 1 status" /> 
  <tag description="Состояние контура - ГВС контроллера" type="0" name="Controller mode.Cir 2 status" /> 
  <tag description="Температура воздуха на улице" type="0" name="Circuit 1 Sensor and ref.S1 sensor" /> 
  <tag description="Температура отопления подача от Данфосс" type="0" name="Circuit 1 Sensor and ref.S3 sensor" /> 
  <tag description="Температура отопления обратка от Данфосс" type="0" name="Circuit 1 Sensor and ref.S4 sensor" /> 
  <tag description="Температура ГВС подача от Данфосс" type="0" name="Circuit 2 Sensor and ref.S5 sensor" /> 
  <tag description="Задание комнатной температуры - комфортная" type="0" name="Circuit 1 primary par._180 Day setpt" /> 
  <tag description="Задание комнатной температуры - пониженная" type="0" name="Circuit 1 primary par._181 Night setpt" /> 
  </comfort>
  </comport>
  </comports>
  </eclOpcCfg>

Для физуального конфигурирования сервера используется графический интерфейс. 
Для добавления в систему нового контроллера выбираете соответствующий пункт меню, либо нажимаете кнопку <Добавить устройство>. Иерархически связанный com-порт добавиться автоматически. 
Выбираете в дереве объектов коммуникационный порт и изменяете его название, номер, скорость и режим работы.

Дерево тегов для контроллера сформируется автоматически из следующих тегов:

Controller mode.Cir  1 mode - Режим контура - отопление контроллера
Controller mode.Cir  2 mode - Режим контура - ГВС контроллера
Controller mode.Cir  1 status - Состояние контура - отопление контроллера
Controller mode.Cir  2 status - Состояние контура - ГВС контроллера
    
Circuit 1 Sensor and ref.S1 sensor - Температура воздуха на улице
Circuit 1 Sensor and ref.S3 sensor - Температура отопления подача от Данфосс
Circuit 1 Sensor and ref.S4 sensor - Температура отопления обратка от Данфосс
Circuit 2 Sensor and ref.S5 sensor - Температура ГВС подача от Данфосс
Circuit 1 primary par._180 Day setpt - Задание комнатной температуры - комфортная
Circuit 1 primary par._181 Night setpt - Задание комнатной температуры - пониженная
Circuit 1 primary par._175 Heat curve - Установка графика отопления (наклона линии)
Circuit 1 primary par._177 Flow tmp min - Задание минимальной температуры подачи отопления
Circuit 1 primary par._178 Flow tmp max - Задание максимальной температуры подачи отопления
Time and date.Hour - Текущее время в приборе - часы
Time and date.Minutes - Текущее время в приборе - минуты
Time and date.DayMonth - Текущее время в приборе - день
Time and date.Month - Текущее время в приборе - месяц
Time and date.Year - Текущее время в приборе - год
Connect_break_Danfoss - Индикатор обрыва связи с Данфоссом

Выбираете в дереве тегов объект контроллера и редактируете параметры названия, описания и адреса контроллера. 
После этого сохраняете конфигурацию в файл.

