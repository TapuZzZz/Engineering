bool startMdns() {
  return MDNS.begin(MDNS_HOSTNAME);
}

bool restartMdns() {
  MDNS.end();
  return startMdns();
}