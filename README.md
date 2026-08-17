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

Из самых базовых функций прошивки можно попробывать поиграться с setBrightness, она находится в Display_Class, и как следует из названия изменяет яркость дисплея. Пороговое значение:200(максимум), минимальное: 10. 

Реализация класса прилагается:
```
void Display_Class::setBrightness(uint8_t v) {
    _brightness = v;
    _lcd.setBrightness(v);
}
```

Все функции Display_Class можно изучит по пути lib/PDA2Core/src/utility/Display_Class_esp32.cpp.

Также для изучения структуры можно изучить Bluetooth класс. Например можно вручную включать и выключать Bluetooth с помощью функций к классе. Реализация прилагается:
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
