var params = node_data;
var payload = node_data.f_payload_deciphered;


function hexToBytes(hex) {
    for (var bytes = [], c = 0; c < hex.length; c += 2)
        bytes.push(parseInt(hex.substr(c, 2), 16));
    return bytes;
}

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
	
  	save_time_series=JSON.stringify(obj);
}

log(save_time_series);