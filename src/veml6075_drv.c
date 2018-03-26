#include "mgos.h"

#ifdef MGOS_HAVE_VEML6075_I2C
#include "mgos_veml6075.h"
#include "mgos_config.h"
#include "mgos_prometheus_metrics.h"
#include "mgos_prometheus_sensors.h"

static struct mgos_veml6075 *s_veml6075;

static void veml6075_prometheus_metrics(struct mg_connection *nc, void *user_data) {
  mgos_prometheus_metrics_printf(nc, GAUGE,
    "UV", "Ultra Violet light intensity, in sensor counts",
    "{band=\"UVA\",type=\"VEML6075\", sensor=\"0\"} %f", mgos_veml6075_getUVA(s_veml6075));
  mgos_prometheus_metrics_printf(nc, GAUGE,
    "UV", "Ultra Violet light intensity, in sensor counts",
    "{band=\"UVB\",type=\"VEML6075\", sensor=\"0\"} %f", mgos_veml6075_getUVB(s_veml6075));
  mgos_prometheus_metrics_printf(nc, GAUGE,
    "UVIndex", "2: Low, 5.5 Moderate, 7.5 High, 10.5 Very High, else Extreme",
    "{sensor=\"0\",type=\"VEML6075\"} %f", mgos_veml6075_getUVIndex(s_veml6075));

  (void) user_data;
}

static void veml6075_timer_cb(void *user_data) {
  double start;
  uint32_t usecs=0;
  float uva, uvb, uvindex;

  start=mgos_uptime();
  uva=mgos_veml6075_getUVA(s_veml6075);
  uvb=mgos_veml6075_getUVB(s_veml6075);
  uvindex=mgos_veml6075_getUVIndex(s_veml6075);
  usecs=1000000*(mgos_uptime()-start);
  LOG(LL_INFO, ("VEML6075 sensor=0 uva=%.1f uvb=%.1f uvindex=%.2f usecs=%u", uva, uvb, uvindex, usecs));

  (void) user_data;
}

void veml6075_drv_init() {
  s_veml6075 = mgos_veml6075_create(mgos_sys_config_get_sensors_veml6075_i2caddr());
  if (s_veml6075) {
    mgos_set_timer(mgos_sys_config_get_sensors_veml6075_period()*1000, true, veml6075_timer_cb, NULL);
    mgos_prometheus_metrics_add_handler(veml6075_prometheus_metrics, NULL);
  }
}

#else
void veml6075_drv_init() {
  LOG(LL_ERROR, ("VEML6075 disabled, include library in mos.yml to enable"));
}
#endif