#pragma once

IPAddress pcIp;
WiFiUDP udp;


struct ChunkHeader {
  uint16_t frameId;
  uint16_t chunkIndex;
  uint16_t chunkCount;
};


bool establishUdpSocket() {
  while (pcIp == IPAddress()) {
    pcIp = MDNS.queryHost(PC_HOST_NAME, MDNS_TIMEOUT_MS);
  }
  Serial.printf("[+] PC IP is: %s\n", pcIp.toString().c_str());
  return true;
}


camera_fb_t* captureFrame() {
  return esp_camera_fb_get();
}


bool sendChunk(const ChunkHeader& header, const uint8_t* data, size_t len) {
  udp.beginPacket(pcIp, PC_UDP_PORT);
  udp.write((const uint8_t*)&header, sizeof(header));
  udp.write(data, len);
  return udp.endPacket();
}


bool sendFrame(const camera_fb_t* frame) {
  static uint16_t nextFrameId = 0;
  uint16_t frameId = nextFrameId++;
  uint16_t chunkCount = (frame->len + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE;

  for (uint16_t i = 0; i < chunkCount; i++) {
    size_t offset = i * MAX_CHUNK_SIZE;
    size_t len = min(MAX_CHUNK_SIZE, frame->len - offset);
    ChunkHeader header = {frameId, i, chunkCount};

    if (!sendChunk(header, frame->buf + offset, len)) {
      Serial.printf("[sendFrame] Chunk %u/%u failed\n", i, chunkCount);
      return false;
    }
  }
  return true;
}