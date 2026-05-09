/* src/drivers/ata.c */
#include "ata.h"
#include "io.h"
#include "../kernel/error.h"
#include "../kernel/klog.h"
#include "../kernel/panic.h"

#define ATA_PRIMARY_IO      0x1F0
#define ATA_PRIMARY_DCR     0x3F6

#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_SECCOUNT    0x02
#define ATA_REG_LBA_LO      0x03
#define ATA_REG_LBA_MID     0x04
#define ATA_REG_LBA_HI      0x05
#define ATA_REG_DRV_HEAD    0x06
#define ATA_REG_STATUS      0x07
#define ATA_REG_COMMAND     0x07

#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_CACHE_FLUSH 0xE7

#define ATA_SR_BSY          0x80    /* Busy */
#define ATA_SR_DRDY         0x40    /* Drive ready */
#define ATA_SR_DF           0x20    /* Drive write fault */
#define ATA_SR_DSC          0x10    /* Drive seek complete */
#define ATA_SR_DRQ          0x08    /* Data request ready */
#define ATA_SR_CORR         0x04    /* Corrected data */
#define ATA_SR_IDX          0x02    /* Index */
#define ATA_SR_ERR          0x01    /* Error */

#define ATA_TIMEOUT         100000

static void ata_delay(void) {
    inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
}

static int ata_wait_bsy(void) {
    int timeout = ATA_TIMEOUT;
    while (inb(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_BSY) {
        if (--timeout == 0) return KERR_DISK_TIMEOUT;
    }
    return KERR_OK;
}

static int ata_wait_drq(void) {
    int timeout = ATA_TIMEOUT;
    while (!(inb(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_DRQ)) {
        if (inb(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_ERR) return KERR_IO_ERROR;
        if (--timeout == 0) return KERR_DISK_TIMEOUT;
    }
    return KERR_OK;
}

void ata_init(void) {
    klog_info("ATA PIO driver initialized on primary bus (0x1F0)");
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
    if (ata_wait_bsy() != KERR_OK) {
        klog_error("ATA read: timeout waiting for BSY to clear");
        return KERR_DISK_TIMEOUT;
    }
    
    outb(ATA_PRIMARY_IO + ATA_REG_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LO, (uint8_t)lba);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    if (ata_wait_bsy() != KERR_OK) {
        klog_error("ATA read: timeout waiting for command");
        return KERR_DISK_TIMEOUT;
    }
    if (ata_wait_drq() != KERR_OK) {
        klog_error("ATA read: IO error or DRQ not set");
        return KERR_IO_ERROR;
    }

    insw(ATA_PRIMARY_IO + ATA_REG_DATA, buffer, 256);
    
    ata_delay();
    return KERR_OK;
}

int ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    if (ata_wait_bsy() != KERR_OK) {
        klog_error("ATA write: timeout waiting for BSY to clear");
        return KERR_DISK_TIMEOUT;
    }
    
    outb(ATA_PRIMARY_IO + ATA_REG_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LO, (uint8_t)lba);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    if (ata_wait_bsy() != KERR_OK) {
        klog_error("ATA write: timeout waiting for command");
        return KERR_DISK_TIMEOUT;
    }
    if (ata_wait_drq() != KERR_OK) {
        klog_error("ATA write: IO error or DRQ not set");
        return KERR_IO_ERROR;
    }

    outsw(ATA_PRIMARY_IO + ATA_REG_DATA, buffer, 256);

    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    if (ata_wait_bsy() != KERR_OK) {
        klog_error("ATA write: cache flush timeout");
        return KERR_DISK_TIMEOUT;
    }

    return KERR_OK;
}
