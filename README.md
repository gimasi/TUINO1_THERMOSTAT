# Tuino 1 Maker's Kit Thermostat Demo Application
The IoT demo application for the Maker's Kit is a thermostat. In this example we are using the LoRaWAN GMX-LR1 module for the Tuino 1.
For a full description of the Tuino 1 check it's GitHub page [TUINO 1](https://github.com/gimasi/TUINO_ONE).<br>


The Tuino 1 Maker's Kit comes with some sensors/actuators, NFC Antenna as well as a plexiglass board with screws and standoffs to put evertyhing together.<br/>
<br/>
Here is what the assembled kit looks like

![MAKERS_KIT](/docs/img/full_kit.jpg?raw=true)

These are the bundled sensors/actuators
* [Button](https://www.seeedstudio.com/Grove-Button-p-766.html)
* [Relay](https://www.seeedstudio.com/Grove-Relay-p-769.html)
* [Temperature Sensor](https://www.seeedstudio.com/Grove-Temperature-Sensor-p-774.html)
* [OLED Display](https://www.seeedstudio.com/Grove-OLED-Display-0.96%22-p-781.html)

<br>
The OLED display really ehances the overall user experience!!

![OLED1](/docs/img/oled1.jpg?raw=true)

You can use the kit with any LoRaWAN server, or use our Tuino Cloud platform, which uses [Swisscom LPN](http://lpn.swisscom.ch/e/)  or [ThingPark](https://partners.thingpark.com/) by Actility connectivity. 

# Connecting the Sensors/Actuators
The Tuino 1 has 4 Grooove connectors:
* 1 Analog - A0
* 2 Digital - D4 and D5
* 1 I2C

Connect the sensors in the following order to make everything work out of the box

* Temperature on A0
* Button on D4
* Relay on D5
* OLED Display on the I2C port

<br/>
You can connect also the NFC antenna, as seen in the picture, and with an NFC enabled smartphone you can read the paramaters of the Thermostat.<br/>
<br/>

# Payload Description
Here is a quick description of the payload we have implemented.<br>
<b>current_temperature</b> is the temperature sensed by the sensor and <b>thermostat_temperature</b> is the temperature at which the thermostat is set.<br/>

<b>Uplink</b>
```C
 temperature_int = current_temperature * 100;

 tx_buf[0] = 0x02; // packet header - multiple data
 tx_buf[1] = (temperature_int & 0xff00 ) >> 8;
 tx_buf[2] = temperature_int & 0x00ff;
 tx_buf[3] = relay_status;
 tx_buf[4] = manual_mode;

 temperature_int = thermostat_temperature * 100;
 tx_buf[5] = (temperature_int & 0xff00 ) >> 8;
 tx_buf[6] = (temperature_int & 0x00ff );
```


<b>Downlink</b>
```C

 // 0x01 controls the relay/manual status

 if (rx_buf[0] == 0x01 ) {

      if ( rx_buf[1] == 0x01 )
      {
        manual_mode = 1;
        relay_status = 1;
      }
      else
      {
        manual_mode = 0;
        relay_status = 0;
      }

 }

 // 0x02 set's the thermostat temperature

 if (rx_buf[0] == 0x02 ) {


      temperature_int = rx_buf[1];
      temperature_int = temperature_int << 8;

      temperature_int = temperature_int + rx_buf[2];
      thermostat_temperature = (float) ( temperature_int ) / 100.0;

 }

```

For instant gratification send a 0101 payload and hear the relay clicking....
<br>

# HTML Frontend
An HTML frontend is available [here](https://github.com/gimasi/TUINO-LPN-KIT-HTML-FRONTEND), with which you can control via a Web Interface the Tuino 1 Thermostat

# Tuino Cloud Script
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