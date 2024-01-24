
/*
Stand: 20.01.2024

*/

#include <Arduino.h>

#include <espnow.h>
#include <ESP8266WiFi.h>

#define BLINKER_PIN 2

typedef struct time_struct
{
  unsigned long node_id;
  unsigned long time_millis;
} time_struct;

time_struct timeDataOutgoing; // Daten welche gesendet werden
time_struct timeDataIncoming; // Empfangene Daten

/*

   1
   ESP-01
   98:CD:AC:26:62:B8
   2515640

   2
   ESP-01
   98:CD:AC:25:5B:17
   2448151

   3
   ESP-01
   98:CD:AC:26:1A:46
   2497094

   4
   ESP-01
   98:CD:AC:23:FB:A2
   2358178




   NodeMCU
   80:7D:3A:3E:B3:96



   Besser aber eher die gesamte MAC-Adresse zu nutzen zum speichern.


 */

// REPLACE WITH YOUR ESP RECEIVER'S MAC ADDRESS
uint8_t broadcastAddress1[] = {0x98, 0xCD, 0xAC, 0x26, 0x62, 0xB8};
uint8_t broadcastAddress2[] = {0x98, 0xCD, 0xAC, 0x25, 0x5B, 0x17};
uint8_t broadcastAddress3[] = {0x98, 0xCD, 0xAC, 0x26, 0x1A, 0x46};
uint8_t broadcastAddress4[] = {0x98, 0xCD, 0xAC, 0x23, 0xFB, 0xA2};

unsigned long node_current_millis = 0; // Current millis of this node millis();
unsigned long mesh_millis = 0;
unsigned long mesh_millis_offset = 0; // the offset from this node and the oldest node
unsigned long node_millis_offset = 0;
unsigned long received_millis = 0; // value for saving received time from other nodes

unsigned long last_send_interval_change = 0; 

unsigned long oldest_node_id = 0;

unsigned long last_millis_send = 0;

unsigned long send_interval = 1000;

bool blinker_state = 0;
bool new_blinker_state = 0;
int blink_interval = 645; // 1,55 Hz => T= 645 ms

int broadcast_counter = 1;

bool post_init = false;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
  // Serial.print("Last Packet Send Status: ");
  /*
  if (sendStatus == 0)
  {
          // Serial.println("Delivery success");
  }
  else
  {
          // Serial.println("Delivery fail");
  }
  */
}

// Callback when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{

  node_current_millis = millis();

  memcpy(&timeDataIncoming, incomingData, sizeof(timeDataIncoming));

  received_millis = timeDataIncoming.time_millis;

  if (received_millis > node_current_millis)
  {

    if(timeDataIncoming.node_id == oldest_node_id)
    {
      // re-sync with oldest node to account for drift. 
      mesh_millis_offset = received_millis - node_current_millis + 10; // add 10ms to account for send_time
      mesh_millis = node_current_millis + mesh_millis_offset;
    }

    // if we receive millis from a node that is larger than our current mesh-millis, 
    // sync to the oldest node. 
    // mesh millis should always try to be as close as possible to the millis of the oldest node. 
    else if (received_millis > (mesh_millis - 20))
    {
      oldest_node_id = timeDataIncoming.node_id;
      mesh_millis_offset = received_millis - node_current_millis + 10; // add 10ms to account for send_time
      mesh_millis = node_current_millis + mesh_millis_offset;

      send_interval = random(2000, 4000); // reduce send interval as we are not the oldest node.
      post_init = 1;                      // set post_init to 1 as we are not the oldest node.

    }
  }
  else
  {
    // this is executed, if the received millis are less than our own millis.
    // check if the timestamp comes from our previous oldest node
    // if the timestamp comes from the previous oldest node,
    // the previous node must have been rebooted.
    // We reset the mesh_millis_offset and oldest_node_id
    // We now belive that we are the oldest node of the mesh.

    if (timeDataIncoming.node_id == oldest_node_id)
    {
      mesh_millis_offset = 0;
      oldest_node_id = 0;
      send_interval = random(750, 1250); // increase send interval as we are the oldest node. 

    }


  }

  Serial.print("\nreceived_millis: ");
  Serial.print(received_millis);
  Serial.print("\tnode_current_millis: ");
  Serial.print(node_current_millis);
  Serial.print("\tmesh_millis_offset: ");
  Serial.print(mesh_millis_offset);
  Serial.print("\tmesh_millis_offset: ");
  Serial.print(mesh_millis_offset);
}

void setup()
{

  // Init Serial Monitor
  Serial.begin(9600);

  pinMode(BLINKER_PIN, OUTPUT);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_add_peer(broadcastAddress1, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddress2, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddress3, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddress4, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  send_interval = random(250, 750);

  timeDataOutgoing.node_id = ESP.getChipId();

  Serial.println();
  Serial.print("This MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("This ESP.getChipId: ");
  Serial.println(timeDataOutgoing.node_id);
  Serial.println();
}

void loop()
{

  node_current_millis = millis();
  mesh_millis = node_current_millis + mesh_millis_offset;


  if (post_init == false && node_current_millis >= 60000)
  {
    post_init = true;
    send_interval = random(2000, 4000);
    // reduce send rate after init
  }

  if (node_current_millis - last_millis_send >= send_interval)
  {
    last_millis_send = node_current_millis;

    timeDataOutgoing.time_millis = millis();

    // If our own id is not equal to the id in each case, send timestamp.
    // If our ID matches the ID in the case, that means that it is our ID.
    // We don't need to send the timestamp to ourselve.

    switch (broadcast_counter)
    {
    case 1:
      if (timeDataOutgoing.node_id != 2515640)
      {
        esp_now_send(broadcastAddress1, (uint8_t *)&timeDataOutgoing, sizeof(timeDataOutgoing));
      }
      break;

    case 2:
      if (timeDataOutgoing.node_id != 2448151)
      {
        esp_now_send(broadcastAddress2, (uint8_t *)&timeDataOutgoing, sizeof(timeDataOutgoing));
      }
      break;

    case 3:
      if (timeDataOutgoing.node_id != 2497094)
      {
        esp_now_send(broadcastAddress3, (uint8_t *)&timeDataOutgoing, sizeof(timeDataOutgoing));
      }
      break;

    case 4:
      if (timeDataOutgoing.node_id != 2358178)
      {
        esp_now_send(broadcastAddress4, (uint8_t *)&timeDataOutgoing, sizeof(timeDataOutgoing));
      }
      break;

    default:
      broadcast_counter = 1;
      break;
    }

    broadcast_counter++;

    if (broadcast_counter > 4)
    {
      broadcast_counter = 1;
    }
  }

  if ((mesh_millis % blink_interval) >= (blink_interval * 0.5))
  {

    if (blinker_state == 0)
    {
      blinker_state = 1;
      digitalWrite(BLINKER_PIN, blinker_state);
    }
  }
  else
  {

    if (blinker_state == 1)
    {
      blinker_state = 0;
      digitalWrite(BLINKER_PIN, blinker_state);
    }
  }
}
