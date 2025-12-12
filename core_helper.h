#ifndef CORE_HELPER_H
#define CORE_HELPER_H

// This is core_helper.h, a header file I made to handle the Core1 work.
// I put all of the cyw3 wi-fi code into Core1. The Pico 2W wi-fi management now has a whole
// core to itself. That frees up Core0 (the main cor) for routine communication with the slave.
// Core0 and Core1 also communicate with each other. Core0, the main cor, generates output values
// like ADC values (thermistor from slave), and the state of the LEDs (red = 4, yellow = 2, green = 1,
// or any three bit combination stuffed into an eight bit byte. I call this the accessory control byte.)
void core1_main()
{
    // init wifi/cyw43
    if (cyw43_arch_init())
    {
        printf("cyw43 init failed\n");
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000) ||
        cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK))
    {
        printf("Wi-Fi connect failed\n");
    }
    sleep_ms(200);
    printf("IP: %s\n", netif_default ? ip4addr_ntoa(netif_ip4_addr(netif_default)) : "0.0.0.0");

    sleep_ms(100);
    printf("MASTER READY\n");

    // start TCP server
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb)
    {
        printf("tcp_new failed\n");
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("tcp_bind failed\n");
    }
    pcb = tcp_listen(pcb);
    if (!pcb)
    {
        printf("tcp_listen failed\n");
    }
    tcp_accept(pcb, accept_callback);
    printf("Web server running at http://%s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));

    sleep_ms(200);
    printf("IP: %s\n", netif_default ? ip4addr_ntoa(netif_ip4_addr(netif_default)) : "0.0.0.0");

    sleep_ms(100);
    printf("MASTER READY\n");

    while (1)
    {
        // printf("MASTER CORE1\n");
        cyw43_arch_poll(); // MUST call frequently

        // You can wait efficiently until there's work to do
        // cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));

        sleep_ms(100);
    }
}
#endif