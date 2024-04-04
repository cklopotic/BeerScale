// Get the root element
var root = document.querySelector(':root');


var loadLogo = function(event){
  var reader = new FileReader()
  reader.addEventListener("load", function(e) {
    processLogo(event.target.files[0], e)}, false)  
  reader.readAsArrayBuffer(event.target.files[0])
}

var loadThumb = function(event){
  var reader = new FileReader()
  reader.addEventListener("load", function(e) {
    processThumb(event.target.files[0], e)}, false)  
  reader.readAsArrayBuffer(event.target.files[0])
}

function processThumb(file,e) {
  var buffer = e.target.result
  var bitmap = getBMP(buffer)
  if (bitmap.infoheader.biHeight == 92 && bitmap.infoheader.biWidth == 66 && bitmap.infoheader.biBitCount == 24) {
    console.log("Thumb meets criteria")
    var image = document.getElementById('thumbimg')
    image.src = URL.createObjectURL(file)
  } else {
    console.log("Thumb bmp not the correct size or format")
  }
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
function changeBorderColor(element) {
  // Reset border color for all elements
  var elements = document.getElementsByClassName('thumb-div');
  for (var i = 0; i < elements.length; i++) {
      elements[i].style.borderColor = 'black';
  }

  // Set border color for the clicked element
  element.style.borderColor = 'red';
}