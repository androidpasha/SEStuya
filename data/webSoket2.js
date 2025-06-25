let gatewayBatStat = `ws://${window.location.hostname}/ws2`;
//let gateway = `ws://192.168.4.1/ws2`;
let websocketBatStat;
window.addEventListener('load', onLoadWindow2);

function initWebSocket2() {
    websocketBatStat = new WebSocket(gatewayBatStat);
    websocketBatStat.onclose = onClose2;
    websocketBatStat.onmessage = onMessage2;
    websocketBatStat.onopen = send2;
}

function onClose2(event) {
    setTimeout(initWebSocket2, 100);
}

function onMessage2(event) {
    console.log(event);
    let jsonParseEvent = JSON.parse(event.data);
    console.log(jsonParseEvent);
    //  console.log(jsonParseEvent['bat4']);
    // writeToHtmlBatVoltage(jsonParseEvent['bat4']);
    updateBatteryVoltages(jsonParseEvent);


}

function updateBatteryVoltages(data) {
    for (let i = 0; i <= 4; i++) {
        const key = `bat${i}`;
        const element = document.getElementById(key);
        if (element && key in data) {
            const voltage = parseFloat(data[key]).toFixed(3).replace('.', ','); // три знака и замена точки на запятую
            element.textContent = `${voltage} В`;
        }
    }
    const unixSeconds = parseFloat(data['sistemTime']);
    const totalSeconds = Math.floor(unixSeconds);

    const days = Math.floor(totalSeconds / 86400);
    const hours = Math.floor((totalSeconds % 86400) / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;

    const result = `${days}д. ${hours.toString().padStart(2, '0')}:${minutes
        .toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;

    document.getElementById('sistemTime').textContent = result;


    const freeHeap = parseInt(data['freeHeap']);
    document.getElementById('freeHeap').textContent = freeHeap.toString();
    const FSfree = parseInt(data['FSfree']);
    document.getElementById('FSfree').textContent = FSfree.toString();

}

function writeToHtmlBatVoltage(value) {
    const voltageElement = document.getElementById("bat5");
    if (voltageElement) {
        voltageElement.textContent = `${parseFloat(value).toFixed(3)} В`;
    }
}


function onLoadWindow2(event) {
    initWebSocket2();
}

function sendData2(sendString) {
    websocket2.send(sendString);
}

// function onMessage2(event) {
//     console.log(event);
// }

function send2() {
    websocket2.send('{"getSettings":true}');
}