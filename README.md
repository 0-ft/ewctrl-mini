# ewctrl mini lighting controller

This code controls a collection of [ewctrl mini](https://frdy.tw/notes/ewctrl-mini/) boards using an Arduino-compatible brain e.g. esp32.
It's not super ready-to-use but please get in touch if you do want to use it.

- Automatically discovers connected boards on the I2C bus
- Controlled (wirelessly on esp32) over websocket
- Receives a JSON map of name->pattern, where pattern is a collection of Bezier curves of power/time
- Then waits for pattern start commands (by name), and triggers playback of the corresponding pattern, writing out samples as fast as possible to ewctrl mini boards
