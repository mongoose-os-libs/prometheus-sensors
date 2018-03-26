#include "mgos.h"

#ifdef MGOS_HAVE_DHT
#include "mgos_dht.h"
#include "mgos_config.h"
#include "mgos_prometheus_metrics.h"
#include "mgos_prometheus_sensors.h"

#define MAX_DHT 8

struct dht_sensor {
  struct mgos_dht *dht;
  float temp;
  float humidity;
  uint8_t gpio;
  uint8_t idx;
};

static struct dht_sensor *s_dht_sensor[MAX_DHT];
static int s_num_dht = 0;

static void dht_prometheus_metrics(struct mg_connection *nc, void *user_data) {
  int i;

  for (i=0; i<s_num_dht; i++) {
    mgos_prometheus_metrics_printf(nc, GAUGE,
      "temperature", "Temperature in Celcius",
      "{sensor=\"%d\",type=\"DHT\"} %f", i, s_dht_sensor[i]->temp);
    mgos_prometheus_metrics_printf(nc, GAUGE,
      "humidity", "Relative humidity percentage",
      "{sensor=\"%d\",type=\"DHT\"} %f", i, s_dht_sensor[i]->humidity);

  }

  (void) user_data;
}

static void dht_timer_cb(void *user_data) {
  struct dht_sensor *dht_sensor = (struct dht_sensor *)user_data;
  double start;
  uint32_t usecs=0;

  if (!dht_sensor) return;

  start=mgos_uptime();
  dht_sensor->temp = mgos_dht_get_temp(dht_sensor->dht);
  dht_sensor->humidity = mgos_dht_get_humidity(dht_sensor->dht);
  usecs=1000000*(mgos_uptime()-start);
  LOG(LL_DEBUG, ("DHT sensor=%u gpio=%u temp=%.2fC humidity=%.0f%% usecs=%u", dht_sensor->idx, dht_sensor->gpio, dht_sensor->temp, dht_sensor->humidity, usecs));
}

static bool dht_sensor_create(int pin, enum dht_type type) {
  struct dht_sensor *dht_sensor = calloc(1, sizeof(struct dht_sensor));
  if (s_num_dht == MAX_DHT) {
    LOG(LL_ERROR, ("No more sensor slots available (%d added)", MAX_DHT));
    free(dht_sensor);
    return false;
  }

  dht_sensor->dht=mgos_dht_create(pin, type);
  if (!dht_sensor->dht) {
    LOG(LL_ERROR, ("Could not create DHT sensor on pin %d", pin));
    free(dht_sensor);
    return false;
  }
  dht_sensor->gpio=pin;
  dht_sensor->idx=s_num_dht;
  s_dht_sensor[dht_sensor->idx] = dht_sensor;
  s_num_dht++;
  mgos_set_timer(mgos_sys_config_get_sensors_dht_period()*1000, true, dht_timer_cb, (void*)dht_sensor);
  return true;
}

float mgos_prometheus_sensors_dht_get_temp(uint8_t idx) {
  if (idx>=s_num_dht)
    return NAN;
  if (!s_dht_sensor[idx])
    return NAN;
  return s_dht_sensor[idx]->temp;
}

float mgos_prometheus_sensors_dht_get_humidity(uint8_t idx) {
  if (idx>=s_num_dht)
    return NAN;
  if (!s_dht_sensor[idx])
    return NAN;
  return s_dht_sensor[idx]->humidity;
}

void dht_drv_init() {
  char *tok;

  memset(s_dht_sensor, 0, sizeof (struct mgos_dht_sensor *) * MAX_DHT);
  tok = strtok((char *)mgos_sys_config_get_sensors_dht_gpio(), ", ");
  while (tok) {
    int gpio;
    gpio = atoi(tok);
    tok=strtok(NULL, ", ");
    dht_sensor_create(gpio, AM2302);
    mgos_msleep(250);
  }

  if (s_num_dht>0)
    mgos_prometheus_metrics_add_handler(dht_prometheus_metrics, NULL);
}

#else
void dht_drv_init() {
  LOG(LL_ERROR, ("DHT disabled, include library in mos.yml to enable"));
}
#endif