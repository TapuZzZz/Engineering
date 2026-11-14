#pragma once

IPAddress pcIp;

IPAddress findPcIp() {
  return MDNS.queryHost(PC_HOST_NAME, MDNS_TIMEOUT_MS);
}

void waitForPcIp() {
  while (pcIp == IPAddress()) {
    pcIp = findPcIp();
  }
  Serial.printf("[+] PC IP is: %s\n", pcIp.toString().c_str());
}

camera_fb_t* captureFrame() {
  return esp_camera_fb_get();
}