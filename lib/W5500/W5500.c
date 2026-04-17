#include "W5500.h"
#include "stdio.h"

void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

void write_register(uint16_t address, uint8_t block, uint8_t *data, uint16_t len) {
    uint8_t addr[3];
    addr[0] = address >> 8;
    addr[1] = address & 0x00FF;
    addr[2] = block << 3 | 0x04;

    cs_select();
    spi_write_blocking(spi_default, addr, 3);
    spi_write_blocking(spi_default, data, len);
    cs_deselect();
}

void read_registers(uint16_t address, uint8_t block, uint8_t *buf, uint16_t len) {
    uint8_t addr[3];
    addr[0] = address >> 8;
    addr[1] = address & 0x00FF;
    addr[2] = block << 3; 
    cs_select();
    spi_write_blocking(spi_default, addr, 3);
    spi_read_blocking(spi_default, 0xFF, buf, len);
    cs_deselect();
}

void socketCommand(uint8_t sn, uint8_t com) {
    uint8_t reg = S_REGS(sn);
    write_register(Sn_CR, reg, &com, 1);
}

uint32_t receive(uint8_t sn, uint8_t *buf) {
    uint8_t reg = S_REGS(sn);
    uint8_t rx  = S_RX(sn);

    uint8_t size[2];
    read_registers(Sn_RX_RSR, reg, size, 2);
    uint16_t RSR = size[0] << 8 | size[1];

    uint8_t a[2];
    read_registers(Sn_RX_RD, reg, a, 2);
    uint16_t address = a[0] << 8 | a[1];

    read_registers(address & 0x7FF, rx, buf, RSR);

    address += RSR;
    a[0] = address >> 8;
    a[1] = address & 0xFF;
    write_register(Sn_RX_RD, reg, a, 2);

    socketCommand(sn, RECV);

    return RSR;
}

void send(uint8_t sn, uint8_t *buf, uint32_t length) {
    uint8_t reg = S_REGS(sn);
    uint8_t tx  =   S_TX(sn);
    uint32_t ptr = 0;
    while (length > 0) {

        uint8_t fsr[2];
        read_registers(Sn_TX_FSR, reg, fsr, 2);
        uint16_t l = (fsr[0] << 8) | (fsr[1]);

        if (l > length) {
            l = length;
        }

        uint8_t tx_a[2];
        read_registers(Sn_TX_WR, reg, tx_a, 2);
        uint16_t tx_address = tx_a[0] << 8 | tx_a[1];

        write_register(tx_address & 0x7FF, tx, buf + ptr, l);

        tx_address += l;
        tx_a[0] = tx_address >> 8;
        tx_a[1] = tx_address & 0xFF;
        write_register(Sn_TX_WR, reg, tx_a, 2);

        socketCommand(sn, SEND);
        ptr += l;
        length -= l;
    }
}

uint8_t readChipID() {
    uint8_t id;

    read_registers(VERSIONR, COM_REGS, &id, 1);

    return id;
}

void setGatewayIP(uint8_t *g_ip) {
    write_register(GAR, COM_REGS, g_ip, 4);
}

void setMACAddress(uint8_t *MAC) {
    write_register(SHAR, COM_REGS, MAC, 6);
}

void setSourceIP(uint8_t *s_ip) {
    write_register(SIPR, COM_REGS, s_ip, 4);
}

void setSubnetMask(uint8_t *subnet) {
    write_register(SUBR, COM_REGS, subnet, 4);
}

void setSocketMode(uint8_t sn, uint8_t mode) {
    uint8_t reg = S_REGS(sn);
    write_register(Sn_MR, reg, &mode, 1);
}

void setSocketPort(uint8_t sn, uint16_t port) {
    uint8_t reg = S_REGS(sn);
    uint8_t p[2] = {port >> 8, port & 0xFF};
    write_register(Sn_PORT, reg, p, 2);
}

uint8_t readStatusRegister(uint8_t sn) {
    uint8_t reg = S_REGS(sn);
    uint8_t status;

    read_registers(Sn_SR, reg, &status, 1);

    return status;
}

uint8_t readSocketInterrupt(uint8_t sn) {
    uint8_t reg = S_REGS(sn);
    uint8_t ir;

    read_registers(Sn_IR, reg, &ir, 1);

    return ir;
}

void clearSocketInterrupt(uint8_t sn, uint8_t ir) {
    uint8_t reg = S_REGS(sn);

    write_register(Sn_IR, reg, &ir, 1);
}
