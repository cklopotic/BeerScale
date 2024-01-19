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

var loadLogo = function(event){
  var reader = new FileReader()
  reader.addEventListener("load", function(e) {
    processLogo(event.target.files[0], e)}, false)  
  reader.readAsArrayBuffer(event.target.files[0])
}

var loadThumb = function(event){
  var image = document.getElementById('newThumb')
  image.src = URL.createObjectURL(event.target.files[0])
}

function processLogo(file, e) {
  var buffer = e.target.result
  var bitmap = getBMP(buffer)
  if (bitmap.infoheader.biHeight == 280 && bitmap.infoheader.biWidth == 200 && bitmap.infoheader.biBitCount == 24) {
    console.log("Logo meets criteria")
    var image = document.getElementById('logoimg')
    image.src = URL.createObjectURL(file)
  } else {
    console.log("Logo bmp not the correct size or format")
  }
 }

 function getBMP(buffer) {
  var datav = new DataView(buffer)
  var bitmap = {}
  bitmap.fileheader = {}
  bitmap.fileheader.bfType = datav.getUint16(0, true)
  bitmap.fileheader.bfSize = datav.getUint32(2, true)
  bitmap.fileheader.bfReserved1 =  datav.getUint16(6, true)
  bitmap.fileheader.bfReserved2 =  datav.getUint16(8, true)
  bitmap.fileheader.bfOffBits = datav.getUint32(10, true)
  bitmap.infoheader = {}
  bitmap.infoheader.biSize = datav.getUint32(14, true)
  bitmap.infoheader.biWidth = datav.getUint32(18, true)
  bitmap.infoheader.biHeight = datav.getUint32(22, true)
  bitmap.infoheader.biPlanes = datav.getUint16(26, true)
  bitmap.infoheader.biBitCount = datav.getUint16(28, true)
  bitmap.infoheader.biCompression = datav.getUint32(30, true)
  bitmap.infoheader.biSizeImage = datav.getUint32(34, true)
  bitmap.infoheader.biXPelsPerMeter = datav.getUint32(38, true)
  bitmap.infoheader.biYPelsPerMeter = datav.getUint32(42, true)
  bitmap.infoheader.biClrUsed = datav.getUint32(46, true)
  bitmap.infoheader.biClrImportant = datav.getUint32(50, true)
  var start = bitmap.fileheader.bfOffBits
  bitmap.stride = Math.floor((bitmap.infoheader.biBitCount*bitmap.infoheader.biWidth + 31) / 32) * 4
  bitmap.pixels = new Uint8Array(buffer, start)
  return bitmap
}