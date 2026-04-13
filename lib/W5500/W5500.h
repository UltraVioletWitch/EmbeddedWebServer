#ifndef W5500_H
#define W5500_H

#include "pico/stdlib.h"
#include "hardware/spi.h"

#define COM_REGS      0x00
#define S_REGS(sn)   (0x01 + 0x04 * (sn))
#define S_TX(sn)     (0x02 + 0x04 * (sn))
#define S_RX(sn)     (0x03 + 0x04 * (sn))

// Socket Modes
#define CLOSED    0x0
#define TCP       0x1
#define UDP       0x2
#define MACRAW    0x4

// Socket Commands
#define OPEN      0x01
#define LISTEN    0x02
#define CONNECT_W 0x04
#define DISCON    0x08
#define CLOSE     0x10
#define SEND      0x20
#define SEND_MAC  0x21
#define SEND_KEEP 0x22
#define RECV      0x40

// Socket Status
#define SOCK_CLOSED      0x00
#define SOCK_INIT        0x13
#define SOCK_LISTEN      0x14
#define SOCK_ESTABLISHED 0x17
#define SOCK_CLOSE_WAIT  0x1C
#define SOCK_UDP         0x22
#define SOCK_MACRAW      0x42
#define SOCK_SYNSENT     0x15
#define SOCK_SYNRECV     0x16
#define SOCK_FIN_WAIT    0x18
#define SOCK_CLOSING     0x1A
#define SOCK_TIME_WAIT   0x1B
#define SOCK_LAST_ACK    0x1D

// Common Registers
#define MR       0x00
#define GAR      0x01
#define SUBR     0x05
#define SHAR     0x09
#define SIPR     0x0F
#define INTLEVEL 0x13
#define IR       0x15
#define IMR      0x16
#define SIR      0x17
#define SIMR     0x18
#define RTR      0x19
#define RCR      0x1B
#define PTIMER   0x1C
#define PMAGIC   0x1D
#define PHAR     0x1E
#define PSID     0x24
#define PMRU     0x26
#define UIPR     0x28
#define UPORTR   0x2C
#define PHYCFGR  0x2E
#define VERSIONR 0x39

// Socket Registers
#define Sn_MR         0x00
#define Sn_CR         0x01
#define Sn_IR         0x02
#define Sn_SR         0x03
#define Sn_PORT       0x04
#define Sn_DHAR       0x06
#define Sn_DIPR       0x0C
#define Sn_DPORT      0x10
#define Sn_MSSR       0x12
#define Sn_TOS        0x15
#define Sn_TTL        0x16
#define Sn_RXBUF_SIZE 0x1E
#define Sn_TXBUF_SIZE 0x1F
#define Sn_TX_FSR     0x20
#define Sn_TX_RD      0x22
#define Sn_TX_WR      0x24
#define Sn_RX_RSR     0x26
#define Sn_RX_RD      0x28
#define Sn_RX_WR      0x2A
#define Sn_IMR        0x2C
#define Sn_FRAG       0x2D
#define Sn_KPALVTR    0x2F

void cs_select();
void cs_deselect();

void write_register(uint16_t address, uint8_t block, uint8_t *data, uint16_t len);
void read_registers(uint16_t address, uint8_t block, uint8_t *buf, uint16_t len);

uint16_t receive(uint8_t sn, uint8_t *buf);
void send(uint8_t sn, uint8_t *buf, uint16_t length);

void socketCommand(uint8_t sn, uint8_t com);

uint8_t readChipID();

void setGatewayIP(uint8_t *g_ip);
void setMACAddress(uint8_t *MAC);
void setSourceIP(uint8_t *s_ip);
void setSubnetMask(uint8_t *subnet);

void setSocketMode(uint8_t sn, uint8_t mode);
void setSocketPort(uint8_t sn, uint16_t port);

uint8_t readStatusRegister(uint8_t sn);

uint8_t readSocketInterrupt(uint8_t sn);
void clearSocketInterrupt(uint8_t sn, uint8_t ir);

#endif
