bool applySensorSettings() {
  sensor_t* sensor = esp_camera_sensor_get();
  if (sensor == nullptr) return false;

  sensor->set_saturation(sensor, 0);
  sensor->set_contrast(sensor, 0);
  sensor->set_whitebal(sensor, 1);
  sensor->set_awb_gain(sensor, 1);
  sensor->set_wb_mode(sensor, 0);
  sensor->set_gainceiling(sensor, GAINCEILING_4X);

  sensor->set_exposure_ctrl(sensor, 1);
  sensor->set_aec2(sensor, 1);
  sensor->set_gain_ctrl(sensor, 1);

  sensor->set_bpc(sensor, 1);
  sensor->set_wpc(sensor, 1);
  sensor->set_raw_gma(sensor, 1);
  sensor->set_lenc(sensor, 1);
  sensor->set_dcw(sensor, 1);

  sensor->set_vflip(sensor, 1);
  sensor->set_hmirror(sensor, 1);
  return true;
}