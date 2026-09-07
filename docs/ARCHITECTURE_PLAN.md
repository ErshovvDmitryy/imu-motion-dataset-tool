# QControl — План архитектуры для мультимодальных датасетов

> Статус: **план / черновик**. Текущий переезд с монолитной IMU-архитектуры
> на универсальную модель данных + систему плагинов/схем.
> Решения помечены как **выбранные** (зафиксированы с пользователем) или **открытые**
> (нужно уточнить).

---

## 1. Проблема: что мешает сейчас

Ядро жёстко заточено под один сценарий (IMU-жесты). Закрытые зависимости:

| Файл | Жёсткая часть |
|------|---------------|
| `models/motionsample.h` | фиксированная `MotionSample {6×float + time}`, `MotionPacket`, `SegmentEndPacket` |
| `models/MotionType.h`   | жёсткий `enum MotionType`, `motionTypeToString`, `sliceWindows` |
| `core/serialport.cpp`   | парсинг зашит в `parseMotionPacket` по `sizeof(MotionSample)`, длина кадра по типу |
| `core/databasemanager.cpp` | фиксированные таблицы `samples` / `motion_data` в `createDatasetDatabase`/`insertGesture`/`getGestureSamples` |
| `models/packettype.h`   | фиксированный набор `PacketType` |

Чтобы поддерживать «произвольные» данные (камеры, микрофоны, другие датчики, текст),
этот слой нужно генерализовать. IMU-жесты становятся **одной из схем** поверх новой модели.

**Выбранная стратегия:** полная переархитектура ядра (не мягкая обёртка).
Миграция старых IMU-БД допустима и не обязана быть бесшовной.

---

## 2. Целевая многослойная архитектура

```
┌─ UI (pages) ────────────────────────────────────────┐
│  ProtocolEditor · DatasetSchemaEditor ·              │
│  ConnectionPage · DataBasePage · ExportCSV           │
└──────────────┬───────────────────────────────────────┘
               │
┌─ Data Model (универсальная) ────────────────────────┐
│  DataValue (тип + значение)                         │
│  DataField / DataFrame / DataSample                 │
│  DataSchema (произвольные поля)                     │
│  DataTypeRegistry (int8/float/string/bool/blob…)    │
└──────────────┬───────────────────────────────────────┘
               │
┌─ Core ──────────────────────────────────────────────┐
│  IProtocolSource (интерфейс, транспорт-агностик)    │
│  BinaryProtocol · TextProtocol                      │
│  FrameParser / FrameDeserializer                    │
│  DatabaseManager (schema-агностик)                  │
│  SchemaParser (DSL) · SchemaSerializer              │
└──────────────┬───────────────────────────────────────┘
               │      (реализации / плагины)
        ┌──────┴───────┬───────────┬──────────────┐
        │              │           │              │
   IMUSource     CameraSource  MicSource     TextSource
   (текущий,     (перспектива) (перспектива)  (перспектива)
    как плагин)
```

Ключевая идея — **разделить «данные» (универсальные) от «типа источника» (плагин)**.
IMU-жест — просто одна конкретная схема данных.

---

## 3. Компоненты плана

### 3.1. Модель данных (универсальная)

Новые файлы в `models/`:

- `datavalue.h` — вариантное значение поля:
  - `enum DataValueType { Int8, Int16, Int32, UInt8..UInt32, Float32, Float64, Bool, String, Blob, Array }`
  - `DataValue` хранит `QVariant` + `DataValueType`, валидирует диапазон/приведение типа.
- `dataschema.h` — описание структуры записи:
  - `DataField { QString name; DataValueType type; int maxLen/arrayLen; flags: isCrc16, isTimestamp, isArray }`
  - `DataSchema { QString name; QVector<DataField> fields; }`
  - Позволяет собрать любой «пакет-строку»: `name:String; x:int8; y:int8; active:bool; crc:crc16`.
- `datapacket.h` — экземпляр данных по схеме:
  - `DataFrame { QVector<DataValue> values; }` + метаданные (схема, время приёма, источник).

Заменяет `MotionSample` / `MotionPacket` / `SegmentEndPacket` как универсальный контейнер.
IMU-структура создаётся через `DataSchema` (7 полей: ax, ay, az, gx, gy, gz + timestamp).

### 3.2. Система протоколов

- `core/protocol/IProtocolSource.h` — интерфейс источника данных (транспорт-агностик):
  - `connect(config)`, `disconnect()`, сигнал `frameReceived(DataFrame)`, `segmentEnd()`, `logMessage()`.
  - Реализации на разные транспорты: `BinaryProtocol`, `TextProtocol`
    (построены на `QIODevice` / `QSerialPort` / файле / UDP-TCP — **открытый вопрос №4**).
- `core/protocol/FrameParser.h` — разбор потока по схеме:
  - накопление буфера, поиск начала кадра, framing (по длине / по разделителю / по заголовку),
  - десериализация полей по `DataSchema`, вычисление CRC.
- `core/protocol/SchemaParser` (DSL) — разбирает декларацию
  `"name:String; x:int8; y:int8; bool:active; crc:crc16"` в `DataSchema`.
  (**открытый вопрос №1** о точном синтаксисе).
- `core/protocol/SchemaSerializer` — обратная операция: схема → текст (для сохранения/обмена).
- UI-конструктор протокола — графический редактор полей (имя/тип/размер),
  сохранение в `protocols/*.qproto` (JSON/XML); поддерживает и строку-DSL, и список полей
  (**выбрано: оба варианта**).
- Схемы сохраняются в файлах и/или регистрируются в «StartDB».

### 3.3. Генерализация DatabaseManager

- `createDatabase(schema)` — создаёт БД по схеме, динамически генерируя таблицы:
  - `samples` (id, label, record_time, metadata)
  - `data` (sample_id, index, динамические колонки из схемы)
- `insertRecord(db, schema, DataFrame)` — вставка произвольного кадра.
- `queryRecords(db, schema, filters)` — выборка.
- Миграция: существующие `samples`/`motion_data` IMU-БД конвертируются автоматически
  (встроенная IMU-схема) либо читаются напрямую как устаревший формат (**выбрано: допустимо**).
- Метаданные: в «StartDB» добавить таблицу `schemas` + `databases.schema_name`.
- Класс/метка записи: вместо жёсткого `enum MotionType` — **произвольная строка-метка (label)**
  (`SwipeLeft`, `cat`, `dog`, `voice_cmd` …). Может быть отдельной таблицей классов
  (**открытый вопрос №2**).

### 3.4. Классы для разных типов данных

- `models/DataTypeRegistry` — реестр известных типов полей
  (int, float, string, bool, blob/большие бинарные для картинок/аудио, array).
- Хранение «тяжёлых» данных (картинки/аудио/текст):
  - `QVariant` держит `QImage` / `QByteArray` / `QString`.
  - Для больших объектов — SQLite **BLOB** (или файлы на диске + путь в БД —
    **открытый вопрос №3**).
- Провайдеры доступа с общим интерфейсом:
  - `ImageDataProvider` (загрузка/сохранение `QImage` → blob, конвертация, превью)
  - `AudioDataProvider`, `TextDataProvider`
  - реализуют общий интерфейс, чтобы DataBasePage/ExportCSV работали с любым типом через схему.
- Экспорт: текущий `ExportCSV` заточен под числовые окна (`sliceWindows`).
  Генерализовать: экспорт по схеме (CSV/JSON); для картинок — папка файлов по id.

---

## 4. Поэтапность (рабочая сборка на каждом шаге)

### Этап A — Модель данных (безопасно, `.h`)
- `DataValue`, `DataField`, `DataSchema`, `DataFrame`, `DataTypeRegistry`.
- Проверка сериализации схемы (DSL ↔ объект).
- `MotionSample` держим рядом; существующий IMU-поток не трогаем.

### Этап B — Протокольный слой
- `IProtocolSource` + `BinaryProtocol`/`TextProtocol` + `FrameParser`.
- Реализовать текущий IMU как первую схему поверх `FrameParser`
  (заменить байтовый парсинг `serialport.cpp`) — проверка гипотезы на реальном потоке.
- UI-конструктор схем + парсер строки-декларации.

### Этап C — БД
- Генерализовать `DatabaseManager`: создание БД по схеме, вставка/чтение произвольного кадра.
- Миграция старых IMU-БД / прямое чтение старого формата.
- Замена `enum MotionType` на строковый label (классы).

### Этап D — Типы данных
- `ImageDataProvider`, `AudioDataProvider`, `TextDataProvider` + BLOB-хранение.
- Генерализация `DataBasePage` (графика/превью/экспорт) под схему.

### Этап E — UI-страницы
- Перевод ConnectionPage / DataBasePage / ExportCSV на `DataSchema` вместо MotionSample-специфики.
- Панель «Источники» для выбора протокола.

---

## 5. Открытые вопросы (для решения)

1. **Синтаксис декларации схемы.** Только позиционный (`String, int8, int8, bool, crc16`)
   или именованные поля (`name:String; x:int8; y:int8; active:bool; crc:crc16`)?
2. **Метка класса (label).** Голый строковый label или отдельная настраиваемая таблица классов?
3. **Хранение картинок/звука.** BLOB в SQLite (проще в датасете, но раздувает файл)
   или файлы на диске + путь в БД (надёжнее по размеру)?
4. **Транспорт источника.** Везде ли QSerialPort, или для камер/микрофонов нужны
   другие транспорты (файл, UDP/TCP, камера)? — интерфейс делать транспорт-агностичным.
5. **Framing.** IMU-протокол использует первый байт-тип + длину по типу. В новых схемах:
   (а) длина в заголовке, (б) фиксированная длина, (в) разделитель `\n` для текста,
   (г) стартовый маркер + тип. Какие поддержать в первой версии конструктора?

---

## 6. Совместимость и риски

- **Существующий UI/логика** (`ConnectionPage`, `DataBasePage`, `ExportCSV`, `MotionRecorder`)
  завязаны на `MotionSample`/`MotionPacket`/`MotionType` — этап E это устранит.
- **QCustomPlot** остаётся для числовых потоков; для картинок/аудио понадобятся
  свои виджеты превью.
- Миграция старых `.db` — автоматическая конвертация с внутренней IMU-схемой.
- Делаем поэтапно с сохранением рабочей сборки; полная генерализация —
  долгосрочная цель, выпуски фич идут отдельными этапами.
