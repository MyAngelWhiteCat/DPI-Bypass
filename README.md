# Очередной обход блокировок DPI на Windows
[Скачать](https://github.com/MyAngelWhiteCat/DPI-Bypass/releases/latest) версию с обходом блокировки youtube, osu.direct, catboy.best, api.nerinyan.moe - зеркала для загрузки Osu! карт можно [по ссылке](https://github.com/MyAngelWhiteCat/DPI-Bypass/releases/latest)


Простой инструмент для обхода блокировок DPI на Windows с использованием WinDivert. Программа перехватывает исходящие TLS-пакеты и модифицирует их для обхода систем глубокой проверки пакетов. Выполнен в виде С++ класса для простой интеграции в любое С++ приложение.

# Разблокированные ресурсы:
- youtube
- discord (пока что только текстовые каналы)
- osu.direct И зеркала - api.nerinyan.moe, catboy.best

 

## Методы обхода

### 1. Simple SNI Fake (SIMPLE_SNI_FAKE)
Отправляет один фейковый Client Hello пакет с SNI `www.google.com` перед настоящим соединением. Подходит для простых систем DPI, которые блокируют только по первому пакету.

### 2. Multi SNI Fake (MULTI_SNI_FAKE)
Отправляет несколько (по умолчанию 6) фейковых Client Hello пакетов. Эффективен против систем, которые анализируют несколько первых пакетов сессии.

### 3. Simple SNI Split (SIMPLE_SNI_SPLIT)
Расчленяет полезную нагрузку TLS пакета и отправляет по кусочкам.

### 4. SSF Faked Split (SSF_FAKED_SPLIT)
Комбинированный метод: отправка нескольких фейковых пакетов + разделение настоящего пакета на части с добавлением мусорных данных. Наиболее эффективен против продвинутых систем DPI.

### Легко добавлять новые методы

## Конфигурация

В коде можно настроить правила обхода для конкретных доменов:

```cpp
bypasser.AddBypassRequiredHostname("osu.direct", BypassMethod::SIMPLE_SNI_FAKE);
bypasser.AddBypassRequiredHostname("www.youtube.com", BypassMethod::SSF_FAKED_SPLIT);
bypasser.AddBypassRequiredHostname("googlevideo.com", BypassMethod::SSF_FAKED_SPLIT);
```
Проверяется не только сам домен, но и производные от него.

## Добавление новых методов
Добавьте новый enum в BypassMethod:
```cpp
enum class BypassMethod {
    NON = 0,
    SIMPLE_SNI_FAKE = 1,
    MULTI_SNI_FAKE = 2,
    SSF_FAKED_SPLIT = 3
    //...
};
```
Реализуйте метод в классе DPIBypasser:
```cpp
class DPIBypasser {
    ...
    void YourNewMethod();
    ...
};
```

Добавьте обработку в метод Bypass():
```cpp
case BypassMethod::YOUR_NEW_METHOD:
    YourNewMethod();
    return;
```

## Установка и сборка
Перед сборкой установите драйвер WinDivert:

```bash
windivert_install.bat
```
(Запустите от имени администратора)

Убедитесь что файлы WinDivert64.sys, WinDivert.dll, WinDivert.lib и windivert.h находятся в папке проекта.
Соберите с помощью CMake:

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```
Использование

Убедитесь, что файлы tls_clienthello_www_google_com.bin и WinDivert.dll находятся рядом с DPIbypass.exe
Запустите программу от имени администратора:

```bash
DPIbypass.exe
```
