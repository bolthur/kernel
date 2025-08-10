# Notes regarding usb implementation

## Startup order

- usbd
  - setup rpc handler
  - provide /dev/usb/usbd
  - wait for /dev/usb/hcd
  - start initialization of usb by utilizing hcd
- usb hub
  - setup rpc handler
  - register handler at usbd
  - wait for rpc
- usb keyboard
  - setup rpc handler
  - register handler at usbd
  - wait for rpc
- usb mouse
  - setup rpc handler
  - register handler at usbd
  - wait for rpc
- usb storage
  - setup rpc handler
  - register handler at usbd
  - wait for rpc
- hcd
  - setup rpc handler
  - do basic initialization of dwhci
  - provide /dev/usb/hcd

## To be done

- [ ] Use interrupts in hcd server for transfer
- [ ] Add polling of keyboard to keyboard server after setup
- [ ] Implement mouse driver with polling
- [ ] Reduce usb driver output once implemented
- [ ] Add load of keyboard layout binaries from storage
  - [ ] Extend build of image to pure in english keyboard layout
  - [ ] Extend build of image to pure in default keyboard configuration containing used layout
  - [ ] Extend keyboard driver to load configuration and mapped layout
  - [ ] Add support for keyboard leds
