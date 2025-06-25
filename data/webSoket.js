let gateway = `ws://${window.location.hostname}/ws`;
//let gateway = `ws://192.168.4.1/ws`;
let websocket;
window.addEventListener('load', onLoadWindow);

function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
    websocket.onopen = send;
}

function onClose(event) {
    setTimeout(initWebSocket, 100);
}

function onMessage(event) {
    if (event === 'saved')
        alert('Saved ok')
}

function onLoadWindow(event) {
    initWebSocket();
}

function sendData(sendString) {
    websocket.send(sendString);
}

function onMessage(event) {
    getSettings(event);
}

function send() {
    websocket.send('{"getSettings":true}');
}