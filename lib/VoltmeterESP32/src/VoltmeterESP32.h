#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <driver/adc.h>
#include <algorithm>
#include <vector>
#define CORRECTING_VOLTAGE
typedef u16_t voltage_t; // тип переменных хранения напряжений в массиве без запятой умноженные на 1000

class VoltmetrESP32
{
public:
    VoltmetrESP32(u8_t pin, u16_t R1, u16_t R2, float Vref = 3.3) : pin(pin), R1(R1), R2(R2), Vreferense(Vref) {};
    void begin()
    {
        analogReadResolution(12);       // 12-битное разрешение: значения от 0 до 4095
        analogSetAttenuation(ADC_11db); // позволяет измерять до ~3.3В
        // adcRawArr.resize(adcRawArrSize);
        pinMode(pin, INPUT);
        delay(100);
        for (size_t i = 0; i < adcRawArr.size(); i++)
        {
            measure();
            delay(5);
        }
    }
    void setVref(float Vref) { this->Vreferense = Vref; }

    float getVoltage() { return (float)this->voltage; }

    void measure()
    {
        float adcRaw;
        float voltage;
        u32_t sum = 0;
        for (size_t i = 0; i < sampling; i++)
        {
            sum += analogRead(pin);
            adcRaw = sum / (float)sampling;
            voltage = (adcRaw / 4095.0) * Vreferense;
            voltage = voltage * (float)(R1 + R2) / (float)R2;

#ifdef CORRECTING_VOLTAGE
            // Коррекция 4х участков кривой АЦП y=k*x+b
            if (voltage <= 7.588)
                voltage += -0.0419 * voltage + 0.75;
            else if (voltage <= 8.61)
                voltage += -0.0629 * voltage + 1;
            else if (voltage <= 15.52)
                voltage += -0.1201 * voltage + 1.8;
            else
                voltage += -0.18 * voltage + 2.75;
#endif
        }
        pushback((voltage_t)(voltage * 1000));
        this->voltage = medianFilter(adcRawArr);
    }

private:
    void pushback(voltage_t voltage)
    {
        std::rotate(adcRawArr.begin(), adcRawArr.begin() + 1, adcRawArr.end());
        u16_t last = adcRawArr.size() - 1;
        adcRawArr[last] = voltage;
    }

    float medianFilter(std::vector<voltage_t> adcData) // Получаем копию массива для сортировки
    {
        std::sort(adcData.begin(), adcData.end());
        float result = adcData[adcData.size() / 2] / 1000.0;
        return result;
    }

    const u8_t pin;
    const u16_t R1, R2;
    const u16_t sampling = 100;
    const u16_t adcRawArrSize = 100;

    float Vreferense = 3.3;
    float voltage;
    std::vector<voltage_t> adcRawArr = std::vector<voltage_t>(adcRawArrSize);
};