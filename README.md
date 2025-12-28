\# Очередной обход блокировок DPI на Windows



Простой инструмент для обхода блокировок DPI на Windows с использованием WinDivert. Программа перехватывает исходящие TLS-пакеты и модифицирует их для обхода систем глубокой проверки пакетов. Выполнен в виде С++ класса, для простой и быстрой интеграции в любое С++ приложение



\## Методы обхода



\### 1. Simple SNI Fake (SIMPLE\_SNI\_FAKE)

Отправляет один фейковый Client Hello пакет с SNI `www.google.com` перед настоящим соединением. Подходит для простых систем DPI, которые блокируют только по первому пакету.



\### 2. Multi SNI Fake (MULTI\_SNI\_FAKE)

Отправляет несколько (по умолчанию 6) фейковых Client Hello пакетов. Эффективен против систем, которые анализируют несколько первых пакетов сессии.



\### 3. SSF Faked Split (SSF\_FAKED\_SPLIT)

Комбинированный метод: отправка нескольких фейковых пакетов + разделение настоящего пакета на части с добавлением мусорных данных. Наиболее эффективен против продвинутых систем DPI.



\### 4. Легко добавлять новые.



\## Конфигурация



В коде можно настроить правила обхода для конкретных доменов:



```cpp

bypasser.AddBypassRequiredHostname("osu.direct", BypassMethod::SIMPLE\_SNI\_FAKE);

bypasser.AddBypassRequiredHostname("www.youtube.com", BypassMethod::SSF\_FAKED\_SPLIT);

bypasser.AddBypassRequiredHostname("googlevideo.com", BypassMethod::SSF\_FAKED\_SPLIT);

```



Проверяется не только сам домен, но и производные от него.



\## Добавление новых методов



\- Добавьте новый enum в BypassMethod

```cpp

enum class BypassMethod {

&nbsp;   NON = 0,

&nbsp;   SIMPLE\_SNI\_FAKE = 1,

&nbsp;   MULTI\_SNI\_FAKE = 2,

&nbsp;   SSF\_FAKED\_SPLIT = 3

&nbsp;   //...

};

```

\- Реализуйте метод в классе DPIBypasser

```cpp

class DPIBypasser {

...

void YourNewMethod();

...

};

```

\- Добавьте обработку в метод Bypass()

```cpp

case BypassMethod::YOUR\_NEW\_METHOD:

&nbsp;   YourNewMethod();

&nbsp;   return;

```



\## Сборка



Убедитесь что файлы WinDivert.sys, WinDivert.dll, WinDivert.lib и windivert.h находятся в папке проекта

Соберите с помощью CMake:



```bash

mkdir build \&\& cd build

cmake ..

cmake --build . --config Release

```



