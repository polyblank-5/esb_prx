#include "uart.h"

K_MSGQ_DEFINE(command_queue, sizeof(struct esb_payload), 10, 4);
LOG_MODULE_REGISTER(uart);


void print_uart(char *buf)
{
	int msg_len = strlen(buf);

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

    switch (msg) {
    case new_msg:
        bytes_to_read = c ; // TODO: change back to hex read instead of char 
        msg= current_msg;
        //char msg_len[2] = {bytes_to_read+0x30,'\0'};  
        //print_uart("new msg");
        //print_uart(msg_len);
        //print_uart("\r\n");
        break;
    
    case current_msg:
        rx_buf[rx_buf_pos++] = c;
        if (rx_buf_pos>=bytes_to_read-1){
            rx_buf[rx_buf_pos] = '\0';
            static struct esb_payload command;
            memcpy(command.data, rx_buf, sizeof(uint8_t)*(bytes_to_read));
            LOG_DBG(command.data);
            LOG_DBG("\r\n");
            //char btr_len[2] = {bytes_to_read+ '0','\0'};
            //print_uart(btr_len);
            command.length = bytes_to_read-1;
            k_msgq_put(&command_queue, &command, K_NO_WAIT); //TODO If in other wait modes, doesnt continue
            //print_uart("msg seng");
            bytes_to_read = 0;
            msg = new_msg;
            rx_buf_pos=0;
            //
            //nrf_radio_task_trigger(NRF_RADIO,NRF_RADIO_TASK_RXEN);
            //nrf_radio_task_trigger(NRF_RADIO,NRF_RADIO_TASK_START);
            //print_uart("msg seng");
        }
        //print_uart("current msg");
        //char rxb_len[2] = {rx_buf_pos+ '0','\0'};
        //print_uart(rxb_len);  
        //print_uart("\r\n");
        break;
    default:
        break;
    }
}

// ****************** Public API *****************

int uart_queue_send(const struct esb_payload *command) {
	 return k_msgq_put(&command_queue, command, K_NO_WAIT);
}

int  uart_queue_receive(struct esb_payload *command) {
	return k_msgq_get(&command_queue, command, K_NO_WAIT);
}