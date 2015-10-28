#ifndef SSD1331_H
#define SSD1331_H

void SSD1331Init();
void SSD1331Refresh(uint8_t width, uint8_t height, uint8_t *data);
void SSD1331RawWrite(bool dataMode, uint8_t x);

#endif