#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H
struct usb_endpoint;
void config_in_packet(struct usb_endpoint *ep);
void config_out_packet(struct usb_endpoint *ep);

#endif // CONFIGURATION_MANAGER_H