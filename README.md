# Все классы
    Display_Class    Display;
    
    Fs_Class         Fs;
    
    Prefs_Class      Prefs;
    
    Apps_Class       Apps;
    
    QuickPanel_Class QuickPanel;
    
    Font_Class       Fonts;
    
    Usb_Class        Usb;
    
    Bt_Class         Bt;        
    
    Touch_Class      Touch;
    
    Rtc_Class        Rtc;
    
    Imu_Class        Imu;
# Данные по классам и функциям
Все классы находятся в репе по пути: lib/PDA2Core/src/utility

Там реализованы классы для всех аспектов устройства(не учитывая реализацию tgx).

Из самых базовых функций прошивки можно попробывать поиграться с setBrightness, она находится в Display_Class, и как следует из названия изменяет яркость дисплея. Пороговое значение:255(максимум), минимальное: 10. 

Реализация класса прилагается:
```
void Display_Class::setBrightness(uint8_t v) {
    _brightness = v;
    _lcd.setBrightness(v);
}
```

Все функции Display_Class можно изучит по пути lib/PDA2Core/src/utility/Display_Class_esp32.cpp.

Также для изучения структуры можно изучить Bluetooth класс. Например можно вручную включать и выключать Bluetooth с помощью функций в классе. Реализация прилагается:
```
void Bt_Class::enable() {
    if (_state != BT_OFF) return;
    _state = BT_IDLE;
    PDA_LOGI(TAG, "enabled → IDLE");
}

void Bt_Class::disable() {
    if (_state == BT_OFF) return;
    if (_state == BT_SCANNING)  stopScan();
    if (_state == BT_CONNECTED) disconnect();
    _state = BT_OFF;
    PDA_LOGI(TAG, "disabled → OFF");
}
```
# Как писать приложения

По пути src/main.cpp хранится главный файл который подключает приложения, туда с помощью Include мы подключаем .h файл нашего приложения, по пути src/apps вы создаете папку приложения, там же создаете cpp файл и .h(.h обязательное условие, он подключается в main.cpp). После чего в коде приложения создаются 4 обязательных функции: void GLTestApp::onInit(), void GLTestApp::onOpen(), void GLTestApp::onTick(uint32_t delta_ms), void GLTestApp::onClose().

void GLTestApp::onInit(): эта функция инициализирует приложение во время первого запуска, и грузит все нужные данные в RAM для будующей загрузки ради быстрого запуска, также в неё можно засунуть команду на логи, и любые другие действия.
Вот пример кода:
```
void GLTestApp::onInit() {
    screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
}
```

void GLTestApp::onOpen(): эта функция уже просто подгружает данные из RAM после например второго открытия приложения, ну либо же может выполнять любую другую функцию при каждом открытие.

void GLTestApp::onTick(uint32_t delta_ms): как следует из названия сюда мы пишем код который будет выполнятся каждый тик(итерацию процессора), сюда вы пишете все что нужно выполнять в цикле. Можете его так и воспринимать как цикл.

void GLTestApp::onClose(): также как следует из названия тут мы пишем код который будет выполнятся при закрытие приложения. Если вам не нужно ничего выгрузить, закрыть, и тп то просто оставляете функцию пустой.
