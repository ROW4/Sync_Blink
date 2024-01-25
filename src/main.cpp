#include <Arduino.h>
#include <espnow.h>
#include <ESP8266WiFi.h>

#define BLINKER_PIN 2

typedef struct time_struct
{
  unsigned long node_id;
  unsigned long time_millis;
} time_struct;

time_struct timeDataOutgoing; // Data to be sent
time_struct timeDataIncoming; // Data the will be received

// First byte x2 since we can consider it to be a local MAC
// 0x52=R
// 0x57=W
// 0x73=s
// 0x79=y
// 0x44=D
// 0x45=E
// LSB is 1 to resemble a group address

uint8_t sync_blink_mac_address[] = {0x52, 0x57, 0x73, 0x79, 0x44, 0x45};

unsigned long node_current_millis = 0; // Current millis of this node millis();
unsigned long mesh_millis = 0;
unsigned long mesh_millis_offset = 0; // the offset from this node and the oldest node
unsigned long node_millis_offset = 0;

unsigned long oldest_node_id = 0;

unsigned long last_millis_send = 0;

unsigned long send_interval = 1000;

bool blinker_state = 0;
int blink_interval = 645; // 1,55 Hz => T= 645 ms

bool master_node = true;
bool post_init = false;
bool single_node = true;

// Callback when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{

  memcpy(&timeDataIncoming, incomingData, sizeof(timeDataIncoming));

  node_current_millis = millis();

  // if received milllis is bigger than our own millis
  if (timeDataIncoming.time_millis > node_current_millis)
  {

    // If it just so happens that the sending node has the same node id, randomly generate a new one.
    // By putting this code here, we guarantee, that the older node will not change their node_id.
    if (timeDataIncoming.node_id == timeDataOutgoing.node_id)
    {
      timeDataOutgoing.node_id = random(2000000, 24000000);
    }

    // if bigger millis are sent from oldest node
    if (timeDataIncoming.node_id == oldest_node_id)
    {
      // re-sync with oldest node to account for drift.
      // Add 10ms to account for send time.
      mesh_millis_offset = timeDataIncoming.time_millis - node_current_millis + 10;
      mesh_millis = node_current_millis + mesh_millis_offset;
    }

    // if we receive millis from a node that is at least 20 ms larger than our current mesh-millis,
    // but not from the current oldest node,
    // sync to the new oldest node.
    else if (timeDataIncoming.time_millis > (mesh_millis - 20))
    {
      oldest_node_id = timeDataIncoming.node_id;

      // Add 10ms to account for send time.
      mesh_millis_offset = timeDataIncoming.time_millis - node_current_millis + 10;

      mesh_millis = node_current_millis + mesh_millis_offset;

      // init is over when we notice that we are not the oldest node.
      post_init = true;

      // we are not the oldest node, set master_node flag to false.
      master_node = false;
    }
  }
  else
  {
    // This is executed, if the received millis are less than our own millis.
    // Check if the timestamp comes from our previous oldest node
    // If the timestamp comes from the previous oldest node,
    // the previous node must have been rebooted.
    // We reset the mesh_millis_offset and oldest_node_id
    // and assume the we are the oldest node until we notice an older node.

    if (timeDataIncoming.node_id == oldest_node_id)
    {
      mesh_millis_offset = 0;
      oldest_node_id = 0;
      master_node = true;

      // set send_interval to 0 to send timestamp one single time instantly
      send_interval = 0;
    }

    // if we receive millis that are from a new node that is alive less than 5 seconds
    // check if we are the master node and send our timestamp one time to update the new node.
    if (timeDataIncoming.time_millis < 5000)
    {
      if (master_node || single_node)
      {
        timeDataOutgoing.time_millis = millis();
        esp_now_send(sync_blink_mac_address, (uint8_t *)&timeDataOutgoing, sizeof(timeDataOutgoing));
      }
    }
  }

  // In any case, if we receive any data,
  // we are not the only node.
  single_node = false;

  // Serial.print("\nreceived_node-id: ");
  // Serial.print(timeDataIncoming.node_id);
  // Serial.print("\treceived_millis: ");
  // Serial.print(timeDataIncoming.time_millis);
  // Serial.print("\tnode_current_millis: ");
  // Serial.print(node_current_millis);
  // Serial.print("\tmesh_millis_offset: ");
  // Serial.print(mesh_millis_offset);
  // Serial.print("\tmesh_millis_offset: ");
  // Serial.print(mesh_millis_offset);
}

void setup()
{

  // Init Serial Monitor
  // Serial.begin(9600);

  pinMode(BLINKER_PIN, OUTPUT);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  esp_now_init();

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Register peer
  esp_now_add_peer(sync_blink_mac_address, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  timeDataOutgoing.node_id = ESP.getChipId();

  // Serial.print("[OLD] ESP8266 Board MAC Address:  ");
  // Serial.println(WiFi.macAddress());

  wifi_set_macaddr(STATION_IF, &sync_blink_mac_address[0]);

  // Serial.print("[NEW] ESP8266 Board MAC Address:  ");
  // Serial.println(WiFi.macAddress());

  // Serial.print("This ESP.getChipId: ");
  // Serial.println(timeDataOutgoing.node_id);
  // Serial.println();

  // instantly send one time after setup.
  // we increase send_interval in loop anyway.
  send_interval = 0;
}

void loop()
{

  node_current_millis = millis();
  mesh_millis = node_current_millis + mesh_millis_offset;

  if (post_init == false && node_current_millis >= 60000)
  {
    // set post_init = true after 60 seconds
    post_init = true;
  }

  if (node_current_millis - last_millis_send >= send_interval)
  {
    last_millis_send = node_current_millis;

    timeDataOutgoing.time_millis = millis();

    esp_now_send(sync_blink_mac_address, (uint8_t *)&timeDataOutgoing, sizeof(timeDataOutgoing));

    if (post_init)
    {
      if (single_node)
      {
        // if we are the only node after 60 seconds, very low send rate
        send_interval = random(60000, 90000);
      }
      else if (master_node)
      {
        // if we are the oldest node among all nodes after 60 seconds, high send rate
        send_interval = random(750, 1250);
      }
      else
      {
        // we are not the only and not the oldest node, very low send rate
        send_interval = random(60000, 90000);
      }
    }
    else
    {
      if (single_node)
      {
        // if we are the only node among all nodes in the first 60 seconds after boot, low send rate
        send_interval = random(8000, 12000);
      }

      else if (master_node)
      {
        // if we are the oldest node among all nodes in the first 60 seconds after boot, high send rate
        send_interval = random(750, 1250);
      }
      else
      {
        // we are not the oldest and not the only node within the 60 seconds, very low send rate
        send_interval = random(60000, 90000);
      }
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
