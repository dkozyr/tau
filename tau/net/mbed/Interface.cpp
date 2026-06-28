#include <tau/net/host/Interface.h>
#include "esp_netif.h"

namespace tau::net {

Interfaces EnumerateInterfaces(bool skip_loopback, bool ipv6) {
    Interfaces result;
    esp_netif_t* netif = nullptr;
    while((netif = esp_netif_next(netif)) != nullptr) {
        esp_netif_ip_info_t ip;
        if(esp_netif_get_ip_info(netif, &ip) == ESP_OK) {
            if(ip.ip.addr != 0) {
                IpAddress local_address{ip.ip.addr, true};
                if(skip_loopback && local_address.IsLoopback()) {
                    continue;
                }

                // IF keys:
                // "WIFI_STA_DEF"   Wi-Fi Station
                // "WIFI_AP_DEF"    Wi-Fi Access Point
                // "ETH_DEF"        Ethernet
                result.push_back(Interface{
                    .name = esp_netif_get_ifkey(netif),
                    .address = local_address
                });
            }
        }
    }
    return result;
}

}
