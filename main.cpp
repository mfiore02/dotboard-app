#include "mbed.h"
#include "mDot.h"

#ifndef CHANNEL_PLAN
#define CHANNEL_PLAN CP_US915
#endif

mDot* dot;
//ChannelPlan* plan;

Serial debug(USBTX, USBRX);

// data/command port
// prints a line each packet TX & each packet RX
// RX IRQ to handle 
Serial data(UART_TX, UART_RX);
char rxBuffer[512];
int rxBufferIndex = 0;
bool gotConfig = false;
void rxIrq() {
    char byte = data.getc();
    if (byte == '\r' || byte == '\n') {
        gotConfig = true;
    }
    rxBuffer[rxBufferIndex++] = byte;
}

uint32_t payload = 0;
int tx_interval = 0;

typedef struct _miniconfig {
    char network_name[128];
    char network_phrase[128];
    uint8_t frequency_sub_band;
    uint8_t public_network;
    uint8_t ack;
    uint8_t adr;
    int tx_interval;
} miniconfig;

miniconfig config = {
    "MTS-US-64",
    "password",
    0,
    1,
    1,
    1,
    60
};

void setConfig(const miniconfig &config) {
    if (dot->setNetworkName(config.network_name) != mDot::MDOT_OK) {
        logError("failed to set new network name to %s", config.network_name);
    }
    if (dot->setNetworkPassphrase(config.network_phrase) != mDot::MDOT_OK) {
        logError("failed to set new network passphrase to %s", config.network_phrase);
    }
    if (dot->setFrequencySubBand(config.frequency_sub_band) != mDot::MDOT_OK) {
        logError("failed to set new frequency sub band to %d", config.frequency_sub_band);
    }
    if (dot->setPublicNetwork(config.public_network) != mDot::MDOT_OK) {
        logError("failed to %sable public_network", config.public_network == 1 ? "en" : "dis");
    }
    if (dot->setAck(config.ack) != mDot::MDOT_OK) {
        logError("failed to set ACKs to %d", config.ack);
    }
    if (dot->setAdr(config.adr) != mDot::MDOT_OK) {
        logError("failed to %sable ADR", config.adr == 1 ? "en" : "dis");
    }

    tx_interval = config.tx_interval;

    logInfo("Current configuration:");
    printf("\tNetwork Name:         %s\r\n", config.network_name);
    printf("\tNetwork Passphrase:   %s\r\n", config.network_phrase);
    printf("\tFrequency Sub Band:   %u\r\n", config.frequency_sub_band);
    printf("\tPublic Network:       %u\r\n", config.public_network);
    printf("\tACK:                  %u\r\n", config.ack);
    printf("\tADR:                  %u\r\n", config.adr);
    printf("\tTX Interval:          %d\r\n", config.tx_interval);
}

int main() {
    debug.baud(115200);
    data.baud(115200);
    data.attach(callback(rxIrq));

    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);

    /*
#if CHANNEL_PLAN = CP_AU915
    plan = new ChannelPlan_AU915();
#elif CHANNEL_PLAN = CP_EU868
    plan = new ChannelPlan_EU868();
#elif CHANNEL_PLAN = CP_IN865
    plan = new ChannelPlan_IN865();
#elif CHANNEL_PLAN = CP_KR920
    plan = new ChannelPlan_KR920();
#elif CHANNEL_PLAN = CP_AS923
    plan = new ChannelPlan_AS923();
#else
    plan = new ChannelPlan_US915();
#endif
    assert(plan);
    */

    //dot = mDot::getInstance(plan);
    dot = mDot::getInstance();
    assert(dot);

    // do configuration
    dot->resetConfig();
    dot->setLogLevel(mts::MTSLog::TRACE_LEVEL);

    setConfig(config);

    while (true) {
        if (gotConfig) {
            sscanf(rxBuffer, "%s,%s,%u,%u,%u,%u,%d", config.network_name, config.network_phrase, &config.frequency_sub_band, &config.public_network, &config.ack, &config.adr, &config.tx_interval);
            setConfig(config);
        }

        wait(tx_interval);
    }
}
