# Vehicle turn signal synchronization with ESP8266 and ESP-NOW

This project enables the synchronization of vehicle turn signals using ESP8266 and ESP-NOW. By leveraging these technologies, wireless communication between vehicles can be established, ensuring synchronized turn signals.

Inspired by: [Why it's not possible to synchronize turn signals (but also absolutely is)](https://www.youtube.com/watch?v=2z5A-COlDPk) 

## Features

- **ESP8266-based:** The ESP8266 (ESP-01) platform is utilized to enable wireless communication.

- **ESP-NOW Protocol:** ESP-NOW is employed to facilitate efficient point-to-point communication between ESP8266 modules.

- **Turn Signal Synchronization:** The program allows for the synchronization of turn signals on different vehicles.

## Circuit Diagram

The circuit diagram for the setup is available in the `Circuit_Diagram.pdf` document. It outlines the basic connections and components for the project.

## Functionality

The project utilizes the ESP-NOW protocol for communication between ESP8266 nodes. 
One notable aspect is that ESP-NOW does not inherently support broadcast functionality on the ESP8266. 
However, a workaround has been implemented to achieve a broadcast-like communication within the network.

### Broadcast Simulation

ESP8266 nodes within the network share the same predefined MAC address. 
This configuration allows all nodes in the network to listen to the same MAC address, effectively enabling a form of broadcast communication. 
Broadcast messages are then sent to all ESP8266 devices simultaneously.

### Broadcast Message Contents

The broadcast message includes crucial information for node identification and synchronization. 
It contains a unique node ID derived from the hardware MAC address of each ESP8266 node. 
Additionally, the message includes a timestamp representing the time each node processed the message. 
This timestamp is based on the value obtained from the `millis()` function, providing a time reference for synchronization.

### Achieving Multi-Node Communication

With this foundation, multiple ESP8266 nodes can effectively communicate with each other through broadcast messages. 
The combination of a shared MAC address and individual node IDs ensures that broadcast messages are comprehensively received and processed by all nodes in the network. 
This approach facilitates synchronized communication between multiple ESP8266 devices within the network.

### Node Pairing and Synchronization

The pairing or synchronization of all nodes typically occurs within a few seconds. 
The master node is determined by the node that has been operational the longest. 
The "age" of a node corresponds to the `millis()` command.
A newly powered-on node is considered very young and possesses a smaller millis value. 
When a new, young node receives a message from an older node, it automatically synchronizes with the oldest node in the network.

### Vehicle Compatibility and Plug-and-Play Option

The control of the turn signals is dependent on the vehicle's electrical system.
This project is specifically designed for vehicles equipped with a traditional turn signal relay. 
In this case, the turn signal relays of three motorcycles have been replaced with ESP8266 units. 
To ensure seamless integration and to minimize work on the motorcycle, it is recommended to buy a cheap replacement turn signal relay fit for the bike. 
It should be possible to use their housing and connector and make the entire project plug and play.
Consequently, this project can be considered a plug-and-play variant, where the original turn signal relay is replaced with the ESP8266. 

### Circuit Integration and Turn Signal Control

In the attached circuit diagram, the turn signals are powered with 12V using a MOSFET. 
The switching between left, right, and hazard lights is controlled at the motorcycle handlebar. 
By utilizing a MOSFET capable of handling high currents, this project accommodates both LED and incandescent bulbs for the turn signals. 
The blink frequency is fixed in software and set to 1.55 Hz. 
Unlike conventional systems, a failure of one lamp does not result in an accelerated blink frequency. 
Therefore, it is advisable to check the functionality of the turn signals before each ride.

### Redundancy and Maintenance

Given the complexity of the ESP8266-based turn signal relay, it is recommended to carry the original turn signal relay as a backup, especially during extended journeys. 
Although the project is designed to be robust, having the original turn signal relay on hand provides a failsafe in case of any unexpected issues. 
Regular checks before departure are recommended to ensure the proper functioning of the turn signals.

## License

This project is licensed under the [MIT License](LICENSE).

© ROW4
