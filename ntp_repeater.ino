#include <WiFi.h>
#define waveout 23

const char ssid[] = "ctc-g-e43584";
const char password[] = "6b5f9ab54467a";

struct tm timeInfo;

char time_now[20];
char sec_now[2];

int current_month = 13;
int min_parity = 0;
int hour_parity = 0;

void send_bit(int bit);
void send_bcd(int num, int count);
int send_bcd_parity(int num, int count, int parity);
void nowtime();

void setup()
{
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (WiFi.begin(ssid, password) != WL_DISCONNECTED)
  {
    ESP.restart();
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }

  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
  // synkTime();

  pinMode(waveout, OUTPUT);
  digitalWrite(OUTPUT, LOW);

  while (timeInfo.tm_sec != 50)
  {
    nowtime();
    for (int i = 0; i < 10; i++)
    {
      getLocalTime(&timeInfo);
      if ((int)timeInfo.tm_sec == 50)
      {
        return;
      }
      delay(100);
    }
  }
}

void loop()
{
  getLocalTime(&timeInfo);

  // call currently time
  nowtime();

  // check maintenance
  if (current_month == 13)
  {
    current_month = timeInfo.tm_mon;
  }
  else if (current_month != timeInfo.tm_mon)
  {
    current_month = timeInfo.tm_mon;
    ESP.restart();
  }

  // Marker
  send_bit(-1);
  nowtime();

  // 10 minutes' BCD
  min_parity = send_bcd_parity(timeInfo.tm_min / 10, 3, min_parity);

  send_bit(0);

  // 1 minutes' BCD
  min_parity = send_bcd_parity(timeInfo.tm_min / 10, 4, min_parity);

  // P-Marker 1
  send_bit(-1);
  nowtime();
  Serial.println("1");

  // 10 hours' BCD
  send_bit(0);
  send_bit(0);
  hour_parity = send_bcd_parity(timeInfo.tm_hour / 10, 2, hour_parity);

  send_bit(0);

  // 1 hours' BCD
  hour_parity = send_bcd_parity(timeInfo.tm_hour % 10, 4, hour_parity);

  // P-Marker 2
  send_bit(-1);
  nowtime();
  Serial.println("2");

  // 100 days' BCD
  send_bit(0);
  send_bit(0);
  send_bcd(timeInfo.tm_yday / 100, 2);

  send_bit(0);

  // 10 days' BCD
  send_bcd((timeInfo.tm_yday % 100) / 10, 4);

  // P-Marker 3
  send_bit(-1);
  nowtime();
  Serial.println("3");

  // 1 days' BCD
  send_bcd(timeInfo.tm_yday % 10, 4);

  send_bit(0);
  send_bit(0);

  // Parity
  send_bit(hour_parity);
  send_bit(min_parity);

  // SU1
  send_bit(0);

  // P-Marker 4
  send_bit(-1);
  nowtime();
  Serial.println("4");

  // SU2
  send_bit(0);

  // AD 10 years' BCD
  send_bcd((timeInfo.tm_year % 100) / 10, 4);

  // AD 1 years' BCD
  send_bcd(timeInfo.tm_year % 10, 4);

  // P-Marker 5
  send_bit(-1);
  nowtime();
  Serial.println("5");

  // day of week's BCD
  send_bcd(timeInfo.tm_wday, 3);

  // LS1, 2
  send_bit(0);
  send_bit(0);

  send_bit(0);
  send_bit(0);
  send_bit(0);
  send_bit(0);

  // P-Marker 0
  send_bit(-1);
  nowtime();
  Serial.println("0");
}

void send_bit(int bit)
{
  // -1 Marker w:0.2s
  if (bit == -1)
  {
    digitalWrite(waveout, HIGH);
    delay(200);
    digitalWrite(waveout, LOW);
    delay(800);
  }

  // 0 w:0.8s
  else if (bit == 0)
  {
    digitalWrite(waveout, HIGH);
    delay(800);
    digitalWrite(waveout, LOW);
    delay(200);
  }

  // 1 w:0.5s
  else if (bit == 1)
  {
    digitalWrite(waveout, HIGH);
    delay(500);
    digitalWrite(waveout, LOW);
    delay(500);
  }
  return;
}

void send_bcd(int num, int count)
{
  int bit;
  for (int i = 0; i < count; i++)
  {
    bit = num >> ((count - 1) - i) & 0x1;
    send_bit(bit);
  }
}

int send_bcd_parity(int num, int count, int parity)
{
  int bit;
  parity = 0;
  for (int i = 0; i < count; i++)
  {
    bit = num >> ((count - 1) - i) & 0x1;
    send_bit(bit);
    parity ^= bit;
  }
  return parity;
}

void nowtime()
{
  getLocalTime(&timeInfo);
  sprintf(time_now, " %04d/%02d/%02d %02d:%02d:%02d",
          timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
          timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  Serial.println(time_now);
}
