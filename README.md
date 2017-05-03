# TUINO1_THERMOSTAT
TUINO1 Maker's Kit Thermostat Example

# TUINO CLOUD SCRIPT
One of the features of the Tuino Cloud is the ability to execute arbitrary Javascript scripts that can be attached to any node.<br/>
Once the node sends an up link the Tuino Cloud will verify if that node has a script attached and will execute the corresponding script.<br>
Here is the example script for the Thermostat demo that will decode the payload and save the temperature data in a time series that can be viewed as a graph directly from the Tuino Cloud dashbaord.


```javascript
// node_data is the variable that holds all the information from the node
var params = node_data;
var payload = node_data.f_payload_deciphered;


function hexToBytes(hex) {
    for (var bytes = [], c = 0; c < hex.length; c += 2)
        bytes.push(parseInt(hex.substr(c, 2), 16));
    return bytes;
}

// obj is the has that will be converted to JSON and saved into a time-series
var obj ={};

log("Starting");
log( payload );
data = hexToBytes( payload );
log( data );

save_time_series = {}

if (data[0] == 0x02 ) {
	var temperature;

	// read sensor temperature
	temperature = data[1] << 8;
	temperature = temperature + data[2];
					
  	log("Temperature=");
    log(temperature);
  
	obj.temperature = temperature/100; 
	
	// here we convert the obj to JSON and save it in the Tuino Cloud timer series
  	save_time_series=JSON.stringify(obj);
}

log(save_time_series);
```