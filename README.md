Change the Wifi Credentials [based on your wifi] before burning it to ESP8266


Link to Dashboard:  https://io.adafruit.com/HMQEII_AEC/dashboards/sensor-dashboard


Connections:


Component	        Pin on ESP8266	Pin on Sensor/Module

DHT22 Sensor	         D4	             VCC (3.3V)	  
                       GND	            Ground
                      Data	              D4 
                      
LDR Sensor	           A0	             VCC (3.3V)
                      GND	              Ground
                    Output	              A0 
                    
Ultrasonic Sensor	     D5	             VCC (3.3V)	
                      D6	                GND	
                      D5	              Trig Pin	
                      D6	              Echo Pin
                      
SIM900A	            RX (D2)	          TX (SIM900A)	
                    TX (D3)	          RX (SIM900A)	

Note: SIM900A needs 5V 1A external power supply
