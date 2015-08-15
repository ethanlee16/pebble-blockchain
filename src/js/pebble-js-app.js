// Function to send a message to the Pebble using AppMessage API
var stats;
function sendMessage() {
	var dictionary = {
  		'BLOCKSMINED': stats.n_blocks_mined,
  		'BLOCKSTOTAL': stats.n_blocks_mined,
  		'BUYPRICE': stats.buyprice,
  		'SELLPRICE': stats.sellprice
	};


	Pebble.sendAppMessage(dictionary,
  		function(e) {
    		console.log('sent to Pebble successfully!');
  		},
  		function(e) {
    		console.log('Error sending info to Pebble!');
  		}
	);
}

function getStats() {
  var url;


  var xhrRequest = function (url, type, callback) {
  	var xhr = new XMLHttpRequest();
  	xhr.onload = function () {
    	callback(this.responseText);
  	};
  	xhr.open(type, url);
  	xhr.send();
	};

  url = 'https://blockchain.info/stats?format=json';

  xhrRequest(url, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);
      	stats.n_blocks_mined = json.n_blocks_mined;
      	stats.n_blocks_total = json.n_blocks_total;
      }      
  	);

  url = 'http://blockchain.info/ticker';
  xhrRequest(url, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);
      	stats.buyprice = json.USD.buy;
      	stats.sellprice = json.USD.sell;
      }      
  	);
}


// Called when JS is ready
Pebble.addEventListener("ready",function(e) {
	getStats();
});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",function(e) {
	console.log("Received Status: " + e.payload.status);
	sendMessage();
});