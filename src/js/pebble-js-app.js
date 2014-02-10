/*
	Pebble library code
*/

function getLights(success) {
    var xmlhttp;

    xmlhttp = new XMLHttpRequest();
	
    xmlhttp.onreadystatechange = function() {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            success(xmlhttp.responseText);
        }
    };

    var config = JSON.parse(localStorage.gleamConfiguration);
    xmlhttp.open("GET", "http://" + config.bridgeIp + "/api/" + config.bridgeUsername + "/lights",true);
    xmlhttp.send();
}

function setLightOnState(lightId, on) {
	var xmlhttp;
	
	xmlhttp = new XMLHttpRequest();
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			console.log(xmlhttp.responseText);
		}	
	};
	
	var config = JSON.parse(localStorage.gleamConfiguration);
	
	var message = JSON.stringify({
	"on": on
	});
	
	xmlhttp.open("PUT", "http://" + config.bridgeIp + "/api/" + config.bridgeUsername + "/lights/" + lightId + "/state", true);
	xmlhttp.send(message);
}

function setAllLightsOnState(on) {
	var onState = false;
	if (on == 1) {
		onState = true;
	}
	// Get all lights, and turn them on/off
	getLights(function(e) {
		var allLights = JSON.parse(e);
		
		for (var i = 1; i < 50; i++) {
			if (!allLights[i]) {
				continue;
			}
			
			// Set on state
			setLightOnState(i, onState);
		}
	});
}

function getLightStateRecursive(lightId, success) {
	var xmlhttp;
	
	xmlhttp = new XMLHttpRequest();
	
	xmlhttp.onreadystatechange = function() {
		if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
			success(lightId, xmlhttp.responseText);
		}	
	};
	
	var config = JSON.parse(localStorage.gleamConfiguration);
	xmlhttp.open("GET", "http://" + config.bridgeIp + "/api/" + config.bridgeUsername + "/lights/" + lightId, true);
	xmlhttp.send();
}


function anyLightOnRecursive(allLights) {
    return function (lightId, e) {
        // Check if the current light is on
		var light = JSON.parse(e);
		
		console.log(e);
		
		if (light.state.on) {
			// Light is on, therefore any light is on, we can return this to the pebble
			messageAnyLightOn(1);
			return;
		} else {
			// Find the next light
			for (var i = lightId + 1; i < 51; i++) {
				// 51 means we passed the last light. No lights are on apparently.
				if (i == 51) {
					messageAnyLightOn(0);
				}
				
				if (i == 4) {
					console.log(allLights[i]);
				}
				
				if (!allLights[i]) {
					continue;
				}
				
				// Fire recursive function
				getLightStateRecursive(i, anyLightOnRecursive(allLights));
				
				// Function is recursive, therefore we only need to call it on the first light
				break;
			}
		}
    };
}

function messageAnyLightOn(on) {
	var message = {};
	
	message.anyLightOn = on;
	
	console.log('sending: ' + JSON.stringify(message));
			
	Pebble.sendAppMessage(message);	
}

function anyLightOn() {
	// Get all lights, check their on/off state, return result to pebble
	getLights(function(e) {
		var allLights = JSON.parse(e);
		
		for (var i = 1; i < 50; i++) {
			if (!allLights[i]) {
				continue;
			}
			
			// Fire recursive function
			getLightStateRecursive(i, anyLightOnRecursive(allLights));
			
			// Function is recursive, therefore we only need to call it on the first light
			break;
		}
	});
}




Pebble.addEventListener("ready",
	function(e) {
		console.log('javascript started');
		
		// Check if configuration is set and required fields are available
		if (localStorage.gleamConfiguration && JSON.parse(localStorage.gleamConfiguration).bridgeIp && JSON.parse(localStorage.gleamConfiguration).bridgeId && JSON.parse(localStorage.gleamConfiguration).bridgeUsername) {
			// Todo: test connection and add more return values (app not registered etc)
			console.log('config set');
			
			var message = {
				"javascriptReady": 1
			};
			
			var config = JSON.parse(localStorage.gleamConfiguration);
			
			message.groupCount = config.groups.length;
			
			for (var i = 0; i < config.groups.length; i++) {
				message[20+i] = config.groups[i].name;
			}
			
			anyLightOn();
			
			console.log('sending: ' + JSON.stringify(message));
			
			Pebble.sendAppMessage(message);
		} else {
			console.log('no config set');
			// Config not complete
			Pebble.sendAppMessage({
            "javascriptReady": 0});
		}
		console.log('test');
	}
);

Pebble.addEventListener("showConfiguration",
	function(e) {
		Pebble.openURL("http://lamp.vlemert.com/settings.html");
	}
);

Pebble.addEventListener("webviewclosed",
						function(e) {
							console.log("Configuration window returnedddd: " + e.response);
							localStorage.gleamConfiguration = decodeURIComponent(e.response);
							
							var config = JSON.parse(localStorage.gleamConfiguration);
	
							console.log(JSON.stringify(config));
							
							var message = {};
							
							// TODO: fill this with the preset count
							message.loadGroupMenu = config.presets.generic.length;
							
							for (var i = 0; i < config.presets.generic.length; i++) {
								message[100+i] = config.presets.generic[i].name;
							}
							
							console.log('sending: ' + JSON.stringify(message));
							
							Pebble.sendAppMessage(message);
						}
);

function loadMainMenu() {
	console.log('loadMainMenu');
	
	var config = JSON.parse(localStorage.gleamConfiguration);
	var message = {};
	
	message.groupCount = config.groups.length;
			
	for (var i = 0; i < config.groups.length; i++) {
		message[20+i] = config.groups[i].name;
	}
	
	anyLightOn();
	
	console.log('sending: ' + JSON.stringify(message));
	
	Pebble.sendAppMessage(message);
	
	/*Pebble.sendAppMessage({
            "loadMainMenu": "success"});*/
}

function loadGroupMenu() {
	console.log('load group menu');
	
	var config = JSON.parse(localStorage.gleamConfiguration);
	
	console.log(JSON.stringify(config));
	
	var message = {};
	
	// TODO: fill this with the preset count
	message.loadGroupMenu = config.presets.generic.length;
	
	for (var i = 0; i < config.presets.generic.length; i++) {
		message[100+i] = config.presets.generic[i].name;
	}
	
	// Trigger that the group menu is shown
	message.openGroupMenu = 1;
	
	console.log('sending: ' + JSON.stringify(message));
	
	Pebble.sendAppMessage(message);
}

// Set callback for appmessage events
Pebble.addEventListener("appmessage",
                        function(e) {
							console.log("message");
							console.log(JSON.stringify(e.payload));
							if (e.payload.loadMainMenu) {
								loadMainMenu();
							}
							if (e.payload.loadGroupMenu !== undefined) {
								loadGroupMenu();
							}
							if (e.payload.setAllLightsOnState !== undefined) {
								setAllLightsOnState(e.payload.setAllLightsOnState);
							}
                        });