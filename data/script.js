// Get the root element
var root = document.querySelector(':root');

var BeerStatus = {
    weight_lbs: 50.00,
    level_percent: 50.00,
    units_remain: 50.00
  };

  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    console.log(event.data)
    var payload = event.data.split(",");
    BeerStatus.weight_lbs = parseFloat((payload[0].split(":")[1])).toFixed(2);
    BeerStatus.units_remain = parseFloat(payload[1].split(":")[1]).toFixed(2);
    BeerStatus.level_percent = parseFloat(payload[2].split(":")[1]).toFixed(2);
    updateUIValues();
  }
  function onLoad(event) {
    initWebSocket();
  }
 
  
// Create a function for setting a variable value
function updateUIValues() {
    root.style.setProperty('--percentRemain', BeerStatus.level_percent);
    root.style.setProperty('--redSlide', 255 - Math.round(BeerStatus.level_percent*2.55));
    root.style.setProperty('--greenSlide', Math.round(BeerStatus.level_percent*2.55));

    document.querySelector(".barIndicator").innerHTML = BeerStatus.level_percent + '%';
    document.querySelector(".divTopTxt").innerHTML = 'Approx Beer Remaining: ' + BeerStatus.weight_lbs + ' lbs<br>Approx Drinks Remaining: ' + BeerStatus.units_remain;
}
