#ifndef IDB_NETWORK_H
#define IDB_NETWORK_H

void start_wifi();
void wifi_deinit();
void wifi_scan();
void ping_init();
int getLinkStatus();

#endif /* IDB_NETWORK_H */