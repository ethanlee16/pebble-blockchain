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
var walletArray = [{
  "address": "135wpKyGZxqQEZkPLSWcegsZivQ2ALQMMT",
  "pass": "",
  "balance": ""
}, {
  "address": "1CDdvKuzc1Zy8yCrvwPwuxyP54Y2xpDSgC",
  "pass": "",
  "balance": ""
}];

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
	var apikey = "e41ca89f-e3d4-483e-8c45-a27374631872";

	var xhrRequest = function (url, type, callback) {
  	var xhr = new XMLHttpRequest();
  	xhr.onload = function () {
    	callback(this.responseText);
  	};
  	xhr.open(type, url);
  	xhr.send();
	};

	url = 'https://blockchain.info/api/v2/create_wallet?password='+ pass+'&api_code='+apikey;


  	xhrRequest(url, 'GET', 
    function(responseText) {

      var json = JSON.parse(responseText);
      	address=json.address;

      	Pebble.sendAppMessage({
      		'ADDRESS':address
      	})

      }      
  	);

  	var newWallet = {};
  	newWallet.address = address;
  	newWallet.pass = pass
  	walletArray.push(newWallet);
  	console.log(walletArray.length + 'hi im the wallets current length');
}

function getStats() {
  var url;
  var blocksMined;
  var blocksTotal;
  var blockSize;
  var minerRevenue;

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
      	blocksMined = json.n_blocks_mined;
      	blocksTotal = json.n_blocks_total;
      	blockSize = json.blocks_size;
      	minerRevenue = json.miners_revenue_usd;
      }      
  	);

  url = 'http://blockchain.info/ticker';
  xhrRequest(url, 'GET', 
    function(responseText) {
      var json = JSON.parse(responseText);
      console.log("BUYPRICE: " + json.USD.buy);
      console.log("SELLPRICE: " + json.USD.sell);
      	Pebble.sendAppMessage({
      	  'BUYPRICE': json.USD.buy + "",
          'SELLPRICE': json.USD.sell + "",
          'BLOCKSMINED' : blocksMined + "",
          'BLOCKSTOTAL' : blocksTotal + "",
          'BLOCKSIZE' : blockSize + "",
          'MINERREVENUE' : minerRevenue + ""
      	});
      }      
  	);
}

function viewWallets() {
  var xhrRequest = function (url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
      callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
  };

  var entries = "";
  for(var i = 0; i < walletArray.length; i++) {
    (function() {
      console.log(i);
        xhrRequest("http://blockchain.info/address/" 
        + walletArray[i].address + "?format=json", 'GET', function(resp) {

            walletArray[i].balance = "$" + resp.final_balance / 100000000;
            entries += walletArray[i].toString() + ",";
        });
        console.log(JSON.stringify(walletArray[i]));
    })(i, walletArray);
    
  }

  console.log("Trying: " + entries);
  Pebble.sendAppMessage({
    'WALLETARRAY': "[" + entries.substring(0, entries.length - 1) + "]"
  });
}


function callData(param) {
	switch(param) {
		case 'market':
			getStats();
			break;
		case 'createWallet':
			createWallet();
			break;
		case 'viewWallets':
			viewWallets();
			break;
	}

} 

// Called when JS is ready + kushnote: do we have to call this if we're doing callData() anyway
Pebble.addEventListener("ready",function(e) {
	//getStats();
  console.log("ready");
  /*
  Pebble.sendAppMessage({
    'WALLETARRAY': walletArray
  })
*/

});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",function(e) {
	console.log("Received Status: " + e.payload.status);
  console.log("Received type of req: " + e.payload.REQ);
	callData(e.payload.REQ);
	//sendMessage();
});