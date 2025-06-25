//Функция возвращает объект с данными всех input, select, checbox со страницы
function readfromHtmlUserValue() {
    const elements = document.querySelectorAll("input, select");
    const settings = {};
    elements.forEach(element => {
        const key = element.name || element.id || 'unnamed';
        if (element.type === "checkbox") {
            settings[key] = element.checked;
        } else if (element.type === "number") {
            settings[key] = element.valueAsNumber;
        } else if (element.tagName === "SELECT") {
            settings[key] = parseInt(element.value.trim());
        } else {
            settings[key] = element.value.trim();
        }
    });
    return settings;
}

//Функция заполняет данными из принятого объека все input, select, checbox на странице
function writeToHtmlUserValue(settings) {
    const e = document.querySelectorAll("input, select");
    e.forEach(e => {
        const key = e.name || e.id;
        if (!key || !(key in settings)) return;

        const value = settings[key];

        if (e.type === "checkbox") {
            e.checked = Boolean(value);
        } else if (e.type === "number") {
            e.valueAsNumber = Number(value);
        } else if (e.tagName === "SELECT") {
            e.value = String(value);
        } else {
            e.value = value;
        }
    });
}

function getSettings(event) {
    console.log(event);
    let jsonParseEvent = JSON.parse(event.data);
    writeToHtmlUserValue(jsonParseEvent);
}

function saveSettings() {
    // let ESPVal = {"tuyaSwith":true,"wifiSsid":"ssid","wifiPass":"pass","tuyaDevId":"id","tuyaLocalKey":"key","tuyaIp1":192,"tuyaIp2":168,"tuyaIp3":1,"tuyaIp4":2,"timer":30,"bataryVoltageMin":15,"bataryVoltageMax":16,"telegramBotToken":"token"};
    // writeToHtmlUserValue(ESPVal);         
    let Settings = readfromHtmlUserValue();
    console.log(JSON.stringify(Settings, null, 2));
    if (confirm("Зберегти наступні налаштування? " + JSON.stringify(Settings, null, 2))) {
        if (sendData(JSON.stringify(Settings))) {
            alert('Налаштування відправлені!');
        }  //     location.href = "../";
    }
}

// function checed1(element) {
//     console.log("checed");
//     const isChecked = element.checked;
//     websocket.send(JSON.stringify({ tuyaSwithUserPush: isChecked }));

// }

function checed(element) {
    const state = element.checked;

    fetch(`/tuyaSwithUserPush?state=${state}`)
        .then(response => {

            if (!response.ok) {
                return response.text().then(text => {
                  console.error("Помилка мережі:", response.status, text);
                  throw new Error(`Помилка мережі: ${response.status} ${text}`);
                });
              }
              return response.text(); // если все ок, читаем тело
            })
        .then(text => {
            console.log(text);
            const result = text.trim();
            console.log(result);
            if (result === "1") {
                element.checked = true;
            } else if (result === "0") {
                element.checked = false;
            } else {
                alert("Невизначена відповідь сервера");
                element.checked = !element.checked; // откат
            }
        })
        .catch(error => {
            alert("Помилка: " + error.message);
            element.checked = !element.checked; // откат
        });
}

async function checed3(element) {
    const state = element.checked ? 1 : 0;

    try {
        const response = await fetch(`/tuyaSwithUserPush?state=${state}`, {
            method: "GET",
            cache: "no-cache"
        });

        if (!response.ok) {
            const errorText = await response.text();
            console.error("Сетевая ошибка:", response.status, errorText);
            element.checked = !element.checked; // откат состояния
            return;
        }

        const result = (await response.text()).trim();

        if (result === "1") {
            element.checked = true;
        } else if (result === "0") {
            element.checked = false;
        } else {
            console.warn("Неизвестный ответ сервера:", result);
            element.checked = !element.checked; // откат
        }
    } catch (err) {
        console.error("Ошибка подключения к серверу:", err);
        element.checked = !element.checked; // откат
    }
}


