#include "uart.h"

K_MSGQ_DEFINE(command_queue, sizeof(struct esb_payload), 10, 4);
LOG_MODULE_REGISTER(uart,4);


void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

void print_uart_payload(uint8_t *buf, uint16_t msg_len){
    uart_poll_out(uart_dev,msg_len);
    for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}
uart_msg_t msg = new_msg;
static uint8_t bytes_to_read = 0;
static uint8_t rx_buf[32];
static uint8_t rx_buf_pos = 0;

void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;
	

	if (!uart_irq_update(dev)) {
		return;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

    uart_fifo_read(dev, &c, 1);
    LOG_DBG("byte read: %c",c);
    switch (msg) {
    case new_msg:
        bytes_to_read = c ; // TODO: change back to hex read instead of char 
        msg= current_msg;
        break;
    
    case current_msg:
        rx_buf[rx_buf_pos++] = c;
        if (rx_buf_pos>=bytes_to_read-1){
            rx_buf[rx_buf_pos] = '\0';
            static struct esb_payload command;
            memcpy(command.data, rx_buf, sizeof(uint8_t)*(bytes_to_read));
            LOG_DBG("%s",(char*) command.data);
            LOG_DBG("\r\n");
            command.length = bytes_to_read-1;
            k_msgq_put(&command_queue, &command, K_NO_WAIT); //TODO If in other wait modes, doesnt continue
            bytes_to_read = 0;
            msg = new_msg;
            rx_buf_pos=0;

        }

        break;
    default:
        break;
    }
}

int uart_initialization(){
    if (!device_is_ready(uart_dev)) {
            LOG_DBG("UART device not found!");
            return 1;
        }

    uint8_t err = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);
	uart_irq_rx_enable(uart_dev);
    return err;
}

// ****************** Public API *****************

int uart_queue_send(const struct esb_payload *command) {
	return k_msgq_put(&command_queue, command, K_NO_WAIT);
}

int  uart_queue_receive(struct esb_payload *command) {
	return k_msgq_get(&command_queue, command, K_NO_WAIT);
}