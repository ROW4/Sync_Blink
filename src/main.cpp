#include <Arduino.h>
#include <espnow.h>
#include <ESP8266WiFi.h>

const uint8 BLINKER_PIN = 2;

typedef struct time_struct
{
  unsigned long mesh_millis;
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
unsigned long node_millis_offset = 0;

unsigned long last_millis_send = 0;
unsigned long send_interval = 1000;

bool blinker_state = 0;
int blink_interval = 645; // 1,55 Hz => T= 645 ms

bool post_init = false;

// Callback when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
  memcpy(&timeDataIncoming, incomingData, sizeof(timeDataIncoming));

  node_current_millis = millis();
  mesh_millis = node_current_millis + node_millis_offset;

  // if we receive a mesh millis that is lager than our mesh millis value

  if (timeDataIncoming.mesh_millis > mesh_millis)
  {
    // adjust to this new larger mesh millis value
    node_millis_offset = timeDataIncoming.mesh_millis - node_current_millis;
    mesh_millis = timeDataIncoming.mesh_millis;
  }
  else
  {
    // if the incoming mesh millis is significantly lower than our current mesh millis
    // let the new node know the current mesh time

    if (mesh_millis - timeDataIncoming.mesh_millis >= 10)
    {
      // if our offset is zero, we are the oldest node
      if (node_millis_offset == 0)
      {
        // reply to the new node with the current time
        esp_now_send(sync_blink_mac_address, (uint8_t *)&timeDataOutgoing, sizeof(timeDataOutgoing));
      }
      else
      {
        // we are not the oldest node, but still reply quickly in case no one else does
        // set last_millis_send to now.
        last_millis_send = node_current_millis; 
        // by setting interval to a random number between 10 and 100
        // any node that could receive the new node will answer within 100ms. 
        send_interval = random(10, 100);
      }
    }
    else
    {
      // the received mesh millis is only a few milliseconds behind
      // probably just some delay during sending or receiving
      // no need to reply immediatly
    }
  }


  // Serial.print("\treceived_millis: ");
  // Serial.print(timeDataIncoming.mesh_millis);
  // Serial.print("\tour mesh_millis: ");
  // Serial.print(node_current_millis);
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
  mesh_millis = node_current_millis + node_millis_offset;

  if (node_current_millis - last_millis_send >= send_interval)
  {
    last_millis_send = node_current_millis;

    timeDataOutgoing.mesh_millis = mesh_millis;

    esp_now_send(sync_blink_mac_address, (uint8_t *)&timeDataOutgoing, sizeof(timeDataOutgoing));

    if(node_millis_offset == 0)
    {
      if(node_current_millis < 60000)
      {
        send_interval = random(200, 300);
      }
      else
      {
        send_interval = random(800, 1200); 
      }
    }
    else
    {
      send_interval = random(10000, 12000);
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
