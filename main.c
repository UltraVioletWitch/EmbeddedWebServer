#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "W5500.h"
#include "web_files.h"

uint8_t *header_types[] = {"Accept", "Accept-Charset", "Accept-Encoding", "Accept-Language", "Accept-Ranges", "Age",
    "Allow", "Authorization", "Cache-Control", "Connection", "Content-Encoding", "Content-Langauge", "Content-Length",
    "Content-Location", "Content-MD5", "Content-Range", "Content-Type", "Date", "ETag", "Expect", "Expires", "From",
    "Host", "If-Match", "If-Modified-Since", "If-None-Match", "If-Range", "If-Unmodified-Since", "Last-Modified", "Location",
    "Max-Forwards", "Pragma", "Proxy-Authenticate", "Proxy-Authorization", "Range", "Referer", "Retry-After", "Server", 
    "TE", "Trailer", "Transfer-Encoding", "Upgrade", "User-Agent", "Vary", "Via", "Warning", "WWW-Authenticate"};

uint8_t *method_types[] = {"OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT"};

uint8_t *reason_phrases[] = {"Continue", "Switching Protocols", "OK", "Created", "Accepted", "Non-Authorotative Information", 
    "No Content", "Reset Content", "Partial Content", "Multiple Choices", "Moved Permanently", "Found", "See Other", "Not Modified", 
    "Use Proxy", "Temporary Redirect", "Bad Request", "Unauthorized", "Payment Required", "Forbidden", "Not Found", "Method Not Allowed",
    "Not Acceptable", "Proxy Authentication Required", "Request Time-out", "Conflict", "Gone", "Length Required", "Precondition Failed",
    "Request Entitiy Too Large", "Request-URI Too Large", "Unsupported Media Type", "Requested range not satisfiable", 
    "Expectation Failed", "Internal Server Error", "Not Implemented", "Bad Gateway", "Service Unavailable", "Gateway Time-out",
    "HTTP Version not supported"};

struct request_line {
    uint8_t method[16];
    uint8_t uri[256];
    uint8_t http_version[16];
};

struct status_line {
    uint8_t http_version[16];
    uint8_t status_code[4];
    uint8_t reason_phrase[256];
};

struct http_header {
    uint8_t name[256];
    uint8_t value[1024];
    struct http_header *next;
};

struct http_body {
    uint8_t *body;
    uint32_t size;
};

struct http_request {
    struct request_line rq_line;
    struct http_header headers[32];
    uint8_t header_cnt;
    struct http_body body;
};

struct http_response {
    struct status_line stat_line;
    struct http_header headers[32];
    uint8_t header_cnt;
    struct http_body body;
};

typedef enum {
    VERSION,
    CODE,
    PHRASE,
    HEADER,
    BODY
} responseState;

int parseHTTP(uint8_t *message, uint32_t size, struct http_request *rq) {
    uint32_t mess_ptr = 0;
    uint8_t i = 0;
    while (message[mess_ptr] != ' ') {
        rq->rq_line.method[i] = message[mess_ptr];
        i++; mess_ptr++;
        if (i == 16) {
            return -1;
        }
    }

    rq->rq_line.method[i] = '\0';
    i = 0; mess_ptr++;

    while (message[mess_ptr] != ' ') {
        rq->rq_line.uri[i] = message[mess_ptr];
        i++; mess_ptr++;
        if (i == 256) {
            return -2;
        }
    }

    rq->rq_line.uri[i] = '\0';
    i = 0; mess_ptr++;

    while (message[mess_ptr] != '\r') {
        rq->rq_line.http_version[i] = message[mess_ptr];
        i++; mess_ptr++;
        if (i == 16) {
            return -3;
        }
    }

    rq->rq_line.http_version[i] = '\0';
    i = 0; mess_ptr += 2;
    uint8_t header_cnt = 0;
    rq->body.size = 0;

    while (1) {
        if (message[mess_ptr] == '\r') {
            mess_ptr += 2;
            break;
        } else {
            i = 0;
            while (message[mess_ptr] != ':') {
                rq->headers[header_cnt].name[i] = message[mess_ptr];
                i++; mess_ptr++;
            }

            rq->headers[header_cnt].name[i] = '\0';
            i = 0; mess_ptr++;
            while (message[mess_ptr] == ' ') {
                mess_ptr++;
            };

            while (message[mess_ptr] != '\r') {
                rq->headers[header_cnt].value[i] = message[mess_ptr];
                i++; mess_ptr++;
            }

            if (strcmp(rq->headers[header_cnt].name, "Content-Length") == 0) {
                rq->body.size = atoi(rq->headers[header_cnt].value);
            }

            rq->headers[header_cnt].value[i] = '\0';
            mess_ptr += 2;
        }

        header_cnt++;
    }

    rq->header_cnt = header_cnt;
    rq->body.body = malloc((rq->body.size + 1) * sizeof(uint8_t));

    i = 0;
    while (i < rq->body.size) {
        rq->body.body[i] = message[mess_ptr];
        i++; mess_ptr++;
    }

    rq->body.body[i] = '\0';

    return 0;
}

uint32_t sendResponse(struct http_response *rs) {
    uint16_t buf_size = 2048;
    uint8_t buf[buf_size];
    uint32_t i = 0;
    uint32_t mess_ptr = 0;
    bool sent = false;
    responseState stage = VERSION;
    uint16_t size = 0;

    while (rs->stat_line.http_version[i] != '\0') {
        buf[size] = rs->stat_line.http_version[i];
        i++; mess_ptr++; size++;
    }

    buf[size] = ' ';
    i = 0; mess_ptr++; size++;

    while (rs->stat_line.status_code[i] != '\0') {
        buf[size] = rs->stat_line.status_code[i];
        i++; mess_ptr++; size++;
    }

    buf[size] = ' ';
    i = 0; mess_ptr++; size++;

    while (rs->stat_line.reason_phrase[i] != '\0') {
        buf[size] = rs->stat_line.reason_phrase[i];
        i++; mess_ptr++; size++;
    }

    buf[size] = '\r';
    mess_ptr++; size++;
    buf[size] = '\n';
    mess_ptr++; size++;

    send(0, buf, size);
    //buf[size] = '\0';
    //printf("%s", buf);

    for (int n = 0; n < rs->header_cnt; n++) {
        i = 0; size = 0;
        while (rs->headers[n].name[i] != '\0') {
            buf[size] = rs->headers[n].name[i];
            i++; mess_ptr++; size++;
        }

        buf[size] = ':';
        mess_ptr++; size++;
        buf[size] = ' ';
        mess_ptr++; size++;

        i = 0;
        while (rs->headers[n].value[i] != '\0') {
            buf[size] = rs->headers[n].value[i];
            i++; mess_ptr++; size++;
        }

        buf[size] = '\r';
        mess_ptr++; size++;
        buf[size] = '\n';
        mess_ptr++; size++;

        send(0, buf, size);
        //buf[size] = '\0';
        //printf("%s", buf);
    }

    size = 0;

    buf[size] = '\r';
    mess_ptr++; size++;
    buf[size] = '\n';
    mess_ptr++; size++;

    send(0, buf, size);
    //buf[size] = '\0';
    //printf("%s", buf);

    size = 0;

    for (uint32_t n = 0; n < rs->body.size; n++) {
        buf[size] = rs->body.body[n];
        mess_ptr++; size++;

        if (size == buf_size - 1) {
            send(0, buf, size);
            //buf[size] = '\0';
            //printf("%s", buf);
            size = 0;
        }
    }

    if (size > 0) {
        send(0, buf, size);
        //buf[size] = '\0';
        //printf("%s", buf);
    }

    return mess_ptr;
}

void respondHTTP(struct http_request *rq, struct http_response *rs, int code) {
    strcpy(rs->stat_line.http_version, "HTTP/1.1");

    if (strcmp(rq->rq_line.method, "GET") == 0) {
        if (0) {
            strcpy(rs->stat_line.status_code, "404");
            strcpy(rs->stat_line.reason_phrase, "Not Found");
            rs->header_cnt = 3;
            strcpy(rs->headers[0].name, "Connection");
            strcpy(rs->headers[0].value, "close");
            strcpy(rs->headers[1].name, "Content-Length");
            uint8_t length[16];
            sprintf(length, "%d\0", rs->body.size);
            strcpy(rs->headers[1].value, length);
            strcpy(rs->headers[2].name, "Content-Type");
            strcpy(rs->headers[2].value, "text/html");
        } else {
            strcpy(rs->stat_line.status_code, "200");
            strcpy(rs->stat_line.reason_phrase, "OK");
            rs->header_cnt = 3;
            if (strcmp(rq->rq_line.uri, "/style.css") == 0) {
                rs->body.size = style_css_gz_len;
                rs->body.body = (uint8_t*) style_css_gz;
                strcpy(rs->headers[1].name, "Content-Type");
                strcpy(rs->headers[1].value, "text/css");
            } else if (strcmp(rq->rq_line.uri, "/modal.js") == 0) {
                rs->body.size = modal_js_gz_len;
                rs->body.body = (uint8_t*) modal_js_gz;
                strcpy(rs->headers[1].name, "Content-Type");
                strcpy(rs->headers[1].value, "text/javascript");
            } else if (strcmp(rq->rq_line.uri, "/resume.pdf") == 0) {
                rs->body.size = Violet_Kramer_Resume_pdf_len;
                rs->body.body = (uint8_t*) Violet_Kramer_Resume_pdf;
                strcpy(rs->headers[1].name, "Content-Type");
                strcpy(rs->headers[1].value, "application/pdf");
            } else if (strcmp(rq->rq_line.uri, "/") == 0 || strcmp(rq->rq_line.uri, "/index.html") == 0){
                rs->body.size = index_html_gz_len;
                rs->body.body = (uint8_t*) index_html_gz;
                strcpy(rs->headers[1].name, "Content-Type");
                strcpy(rs->headers[1].value, "text/html");
            } else if (strcmp(rq->rq_line.uri, "/template.html") == 0){
                rs->body.size = post_template_html_gz_len;
                rs->body.body = (uint8_t*) post_template_html_gz;
                strcpy(rs->headers[1].name, "Content-Type");
                strcpy(rs->headers[1].value, "text/html");
            } else if (strcmp(rq->rq_line.uri, "/about_me.html") == 0){
                rs->body.size = about_me_html_gz_len;
                rs->body.body = (uint8_t*) about_me_html_gz;
                strcpy(rs->headers[1].name, "Content-Type");
                strcpy(rs->headers[1].value, "text/html");
            } else if (strcmp(rq->rq_line.uri, "/about_this_site.html") == 0){
                rs->body.size = about_this_site_html_gz_len;
                rs->body.body = (uint8_t*) about_this_site_html_gz;
                strcpy(rs->headers[1].name, "Content-Type");
                strcpy(rs->headers[1].value, "text/html");
            } else if (strcmp(rq->rq_line.uri, "/led") == 0){
                rs->body.size = about_this_site_html_gz_len;
                rs->body.body = (uint8_t*) about_this_site_html_gz;
                strcpy(rs->headers[1].name, "Content-Type");
                strcpy(rs->headers[1].value, "text/html");
            } else {
                strcpy(rs->stat_line.status_code, "404");
                strcpy(rs->stat_line.reason_phrase, "Not Found");
                rs->body.size = four_o_four_html_gz_len;
                rs->body.body = (uint8_t*) four_o_four_html_gz;
                strcpy(rs->headers[1].name, "Content-Type");
                strcpy(rs->headers[1].value, "text/html");
            }
            strcpy(rs->headers[2].name, "Content-Encoding");
            strcpy(rs->headers[2].value, "gzip");
            strcpy(rs->headers[0].name, "Content-Length");
            uint8_t length[16];
            sprintf(length, "%d\0", rs->body.size);
            strcpy(rs->headers[0].value, length);
        }
    } else {
        strcpy(rs->stat_line.status_code, "405");
        strcpy(rs->stat_line.reason_phrase, "Method Not Allowed");
        rs->header_cnt = 2;
        rs->body.size = 0;
        rs->body.body = NULL;
        strcpy(rs->headers[0].name, "Connection");
        strcpy(rs->headers[0].value, "close");
        strcpy(rs->headers[1].name, "Content-Length");
        strcpy(rs->headers[1].value, "0");
    }
}

void printRequest(struct http_request *rq) {
    printf("%s\n", rq->rq_line.method);
    printf("%s\n", rq->rq_line.uri);
    printf("%s\n", rq->rq_line.http_version);

    for (int i = 0; i < rq->header_cnt; i++) {
        printf("%s\n", rq->headers[i].name);
        printf("%s\n", rq->headers[i].value);
    }

    if (rq->body.size > 0) {
        printf("%s\n", rq->body.body);
    }
}

int main () {
    stdio_init_all();

    sleep_ms(2000);

    printf("Reading Chip ID of W5500...\n");

    spi_init(spi_default, 5000*1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    uint8_t id = readChipID();

    printf("Chip ID is 0x%02x\n", id);
    
    printf("Setting Gateway IP address...\n");

    uint8_t g_ip[4] = {192, 168, 1, 254};
    setGatewayIP(g_ip);

    printf("Setting Chip MAC address...\n");

    uint8_t MAC[6] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12};
    setMACAddress(MAC);

    printf("Setting Source IP address...\n");

    uint8_t s_ip[4] = {192, 168, 1, 100};
    setSourceIP(s_ip);

    printf("Setting Subnet Mask...\n");

    uint8_t subnet[4] = {255, 255, 255, 0};
    setSubnetMask(subnet);

    printf("Setting up Socket 0...\n");

    // set Socket 0 to TCP Mode
    setSocketMode(0, TCP);

    // set port number to 80
    uint16_t S0_PORT_NUM = 80;
    setSocketPort(0, S0_PORT_NUM);

    uint8_t interrupt_clear = 0x1F;
    write_register(0x0002, 0x01, &interrupt_clear, 1);

    // set Socket 0 to OPEN, then LISTEN
    socketCommand(0, OPEN);
    uint8_t status;
    do {
        status = readStatusRegister(0);
    } while (status != SOCK_INIT);
    socketCommand(0, LISTEN);

    while (1) {
        uint8_t c[2048];
        uint8_t ir = readSocketInterrupt(0);

        if (ir & 0x04) {
            struct http_request rq;
            struct http_response rs;

            uint32_t size = receive(0, c);
            int code = parseHTTP(c, size, &rq);

            //printRequest(&rq);

            respondHTTP(&rq, &rs, code);
            free(rq.body.body);
            size = sendResponse(&rs);

            gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));

            clearSocketInterrupt(0, ir);
        }

        status = readStatusRegister(0);

        if (status == SOCK_CLOSE_WAIT) {
            socketCommand(0, CLOSE);
            socketCommand(0, OPEN);
            socketCommand(0, LISTEN);
        }
    }
}
