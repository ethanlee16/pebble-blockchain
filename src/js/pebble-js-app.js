// Function to send a message to the Pebble using AppMessage API
// var stats;
// function sendMessage() {
// 	var dictionary = {
//   		// 'BLOCKSMINED': stats.n_blocks_mined,
//   		// 'BLOCKSTOTAL': stats.n_blocks_mined,
//   		'BUYPRICE': stats.buyprice,
//   		'SELLPRICE': stats.sellprice
// 	};


// 	Pebble.sendAppMessage(dictionary,
//   		function(e) {
//     		console.log('sent to Pebble successfully!');
//   		},
//   		function(e) {
//     		console.log('Error sending info to Pebble!');
//   		}
// 	);
// }
var walletArray = [];
function generatePassword() {
    var length = 10,
        charset = "abcdefghijklnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
        retVal = "";
    for (var i = 0, n = charset.length; i < length; ++i) {
        retVal += charset.charAt(Math.floor(Math.random() * n));
    }
    return retVal;
}

function createWallet() {
	var address;
	var pass = generatePassword();
	var apikey = /** insert API key here */


	var xhrRequest = function (url, type, callback) {
  	var xhr = new XMLHttpRequest();
  	xhr.onload = function () {
    	callback(this.responseText);
  	};
  	xhr.open(type, url);
  	xhr.send();
	};

	url = 'https://blockchain.info/api/v2/create_wallet&password='+ pass+'&api_code='+apikey;


  	xhrRequest(url, 'GET', 
    function(responseText) {

      var json = JSON.parse(responseText);
      	address=json.address;

      	Parse.sendAppMessage({
      		'ADDRESS':address
      	})

      }      
  	);

  	var newWallet;
  	newWallet.address = address;
  	newWallet.pass = pass
  	walletArray.push(newWallet);
  	console.log(walletArray.length + 'hi im the wallets current length');
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

  // url = 'https://blockchain.info/stats?format=json';

  // xhrRequest(url, 'GET', 
  //   function(responseText) {
  //     var json = JSON.parse(responseText);
  //     	stats.n_blocks_mined = json.n_blocks_mined;
  //     	stats.n_blocks_total = json.n_blocks_total;
  //     }      
  // 	);

  url = 'http://blockchain.info/ticker';
  xhrRequest(url, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);
      	stats.buyprice = json.USD.buy;
      	stats.sellprice = json.USD.sell;
      	Pebble.sendAppMessage({
      	  'BUYPRICE': stats.buyprice,
          'SELLPRICE': stats.sellprice
      	});
      }      
  	);
}

function viewWallets() {
	if(walletArray) {
		Pebble.sendAppMessage({
			'walletArray': walletArray
		})
	}
}


function callData(param) {
	switch(param) {
		case 'market':
			getStats();
			break;
		case 'createWallet':
			createWallet();
			break;
		case 'viewWallet':
			viewWallet();
			break;
	}

} 

// Called when JS is ready + kushnote: do we have to call this if we're doing callData() anyway
Pebble.addEventListener("ready",function(e) {
	getStats();
});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",function(e) {
	console.log("Received Status: " + e.payload.status);
	callData(e.payload.req);
	//sendMessage();
});