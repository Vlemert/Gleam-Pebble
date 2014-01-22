Pebble.addEventListener("ready",
	function(e) {
		Pebble.window.localStorage.lampConfiguration = {"bridgeid": "test"};
		
		//configuration = Pebble.window.localStorage.lampConfiguration;
		//configuration = {"bridgeid": "test"};
		
	}
);

Pebble.addEventListener("showConfiguration",
	function(e) {
		Pebble.openURL("http://lamp.vlemert.com/settings.html");
	}
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log("Configuration window returned: ", JSON.stringify(configuration));
  }
);