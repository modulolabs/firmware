#ifndef SSD1331_H
#define SSD1331_H

void SSD1331Init();
void SSD1331Refresh(uint8_t width, uint8_t height, uint8_t *data, bool flip);
void SSD1331RawWrite(bool dataMode, uint8_t x);
void SSD1331SetCurrent(uint8_t current);
void SSD1331SetContrast(uint8_t r, uint8_t g, uint8_t b);

#endif
