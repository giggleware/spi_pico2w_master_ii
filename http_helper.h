#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H
#include "web_assets.h"
#include "cJSON.h"

volatile uint16_t slave_output = 0;
volatile uint16_t slave_input = 0;
volatile uint8_t current_led_byte = 0;
volatile uint16_t current_temp_raw = 0;

// ---------------- HTTP handler (4-arg signature) ----------------
static err_t http_handler(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    if (!p)
        return ERR_OK;

    char req[512];
    int len = p->tot_len > 511 ? 511 : p->tot_len;
    pbuf_copy_partial(p, req, len, 0);
    req[len] = 0;

    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);

    // ---- index.html ----
    if (strncmp(req, "GET / ", 6) == 0 ||
        strncmp(req, "GET /index.html", 15) == 0)
    {
        char header[256];
        int hlen = snprintf(header, sizeof(header),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: close\r\n\r\n",
                            index_html_len);

        tcp_write(pcb, header, hlen, TCP_WRITE_FLAG_COPY);
        tcp_write(pcb, index_html, index_html_len, TCP_WRITE_FLAG_COPY);
        tcp_output(pcb);
        tcp_close(pcb);
        return ERR_OK;
    }

    // ---- main.css ----
    if (strncmp(req, "GET /main.css", 13) == 0)
    {
        char header[256];
        int hlen = snprintf(header, sizeof(header),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/css\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: close\r\n\r\n",
                            main_css_len);

        tcp_write(pcb, header, hlen, TCP_WRITE_FLAG_COPY);
        tcp_write(pcb, main_css, main_css_len, TCP_WRITE_FLAG_COPY);
        tcp_output(pcb);
        tcp_close(pcb);
        return ERR_OK;
    }

    // ---- app.js ----
    if (strncmp(req, "GET /app.js", 11) == 0)
    {
        char header[256];
        int hlen = snprintf(header, sizeof(header),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/javascript\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: close\r\n\r\n",
                            app_js_len);

        tcp_write(pcb, header, hlen, TCP_WRITE_FLAG_COPY);
        tcp_write(pcb, app_js, app_js_len, TCP_WRITE_FLAG_COPY);
        tcp_output(pcb);
        tcp_close(pcb);
        return ERR_OK;
    }

    // ---- JSON value ----
    if (strncmp(req, "GET /value", strlen("GET /value")) == 0)
    {
        char json[256];
        int len = snprintf(json, sizeof(json),
                           "{"
                           "\"value\": %u,"     // original 24-bit packed value
                           "\"led_state\": %d," // NEW
                           "\"temp_raw\": %u"   // NEW
                           "}",
                           (unsigned)((current_led_byte << 16) | current_temp_raw),
                           (int)current_led_byte,
                           (unsigned)current_temp_raw);

        char header[256];
        int hlen = snprintf(header, sizeof(header),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: close\r\n\r\n",
                            len);

        tcp_write(pcb, header, hlen, TCP_WRITE_FLAG_COPY);
        tcp_write(pcb, json, len, TCP_WRITE_FLAG_COPY);
        tcp_output(pcb);
        tcp_close(pcb);
        return ERR_OK;
    }

    // ---- JSON value ----
    // ---- handle POST /control ----
    if (strncmp(req, "POST /control", strlen("POST /control")) == 0)
    {
        char *body_start = strstr(req, "\r\n\r\n");
        if (body_start)
        {
            body_start += 4;

            cJSON *json = cJSON_Parse(body_start);
            if (json)
            {
                cJSON *led = cJSON_GetObjectItemCaseSensitive(json, "led");
                if (cJSON_IsNumber(led))
                {
                    slave_input = (uint8_t)led->valueint; // outgoing command TO SLAVE
                }
                cJSON_Delete(json);
            }
        }

        // ---- POST JSON request to SLAVE to change LED. ----
        char body[256];
        int body_len = snprintf(body, sizeof(body),
                                "{"
                                "\"control\": %d,"
                                "}",
                                (int)slave_input);

        char header[256];
        int hlen = snprintf(header, sizeof(header),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: close\r\n\r\n",
                            body_len);

        tcp_write(pcb, header, hlen, TCP_WRITE_FLAG_COPY);
        tcp_write(pcb, body, body_len, TCP_WRITE_FLAG_COPY);
        tcp_output(pcb);
        tcp_close(pcb);
        return ERR_OK;
    }
}

// ---------------- accept callback ----------------
static err_t accept_callback(void *arg, struct tcp_pcb *client, err_t err)
{
    (void)arg;
    (void)err;
    tcp_arg(client, NULL);
    tcp_recv(client, http_handler);
    return ERR_OK;
}
#endif