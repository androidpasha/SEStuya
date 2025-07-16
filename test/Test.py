import logging
import tinytuya
import time
#скрипт выводит в консоль зашифрованные пакеты для вкл и выкл
# Данные устройства
# DEVICE_ID = "08051804bcddc2542c4e"
# DEVICE_IP = "192.168.1.253"
# LOCAL_KEY = "~)HQx>7?MnN:=6?f"

DEVICE_ID = "bf2e94117ce277198fmkm4"
DEVICE_IP = "192.168.1.3"
LOCAL_KEY = "]TGA`FGmj=/Zd7aO"


# Настройка логирования
logging.basicConfig(level=logging.DEBUG)
# Инициализация устройства
device = tinytuya.OutletDevice(DEVICE_ID, DEVICE_IP, LOCAL_KEY)

device.set_version(3.4)  # важно для новых устройств

# Проверка статуса
status = device.status()

print("Текущий статус:", status)

# # Включение розетки
# print("Включаем розетку...")
# #device.set_status(True, 1)  # True = ВКЛ, False = ВЫКЛ, 1 = DP ID (обычно для розеток)
# #time.sleep(3)
# device.turn_on()
# # Выключение розетки
# print("Выключаем розетку...")
# device.set_status(False, 1)

#Результат
#payload=b'{"gwId":"08051804bcddc2542c4e","devId":"08051804bcddc2542c4e","uid":"08051804bcddc2542c4e","t":"1751798811"}''
#payload encrypted=b'000055aa000000010000000a000000781071e6e0364a089b03c24b9a87c01d79a71191f857984661161dd1a7b0de394e3061ab63ff270b8a8d16d7b4ae6f5c2078d40ad499b49cee6d31fb356460fabecac833e886c2846f3095ab494fd78a4fe0331c6a2f453bc5f119fd5d375e635ec20e2096f6f147f098839013579e521e98753f4e0000aa55'
#                    000055AA000000010000000A780000111071E6E0364A089B03C24B9A87C01D79A71191F857984661161DD1A7B0DE394E3061AB63FF270B8A8D16D7B4AE6F5C2078D40AD499B49CEE6D31FB356460FABECAC833E886C2846F3095AB494FD78A4FE0331C6A2F453BC5F119FD5D375E635EC20E2096F6F147F098839013579E521EEDB445EC0000AA55