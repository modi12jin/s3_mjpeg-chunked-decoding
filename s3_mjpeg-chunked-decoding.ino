#include "SD_MMC.h"
#include "pins_config.h"
#include "src/lcd/nv3041a_lcd.h"
nv3041a_lcd lcd = nv3041a_lcd(TFT_QSPI_CS, TFT_QSPI_SCK, TFT_QSPI_D0, TFT_QSPI_D1, TFT_QSPI_D2, TFT_QSPI_D3, TFT_QSPI_RST);

#define MJPEG_FILENAME "/480_30fps.mjpeg"
#define MJPEG_BUFFER_SIZE (480 * 272 * 2 / 8)  // 单个JPEG帧的内存

#include "MjpegClass.h"
static MjpegClass mjpeg;

/* variables */
static int total_frames = 0;
static unsigned long total_read_video = 0;
static unsigned long total_decode_video = 0;
static unsigned long total_show_video = 0;
static unsigned long start_ms, curr_ms;

//jpeg绘制回调
static int jpegDrawCallback(jpeg_dec_io_t *jpeg_io, jpeg_dec_header_info_t *out_info) {
  lcd.draw16bitbergbbitmap(0, jpeg_io->output_line - jpeg_io->cur_line, out_info->width, jpeg_io->cur_line, (uint16_t *)jpeg_io->outbuf);
  return 1;
}

void setup() {
  Serial.begin(115200);

  lcd.begin();

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  pinMode(SDMMC_CS, OUTPUT);
  digitalWrite(SDMMC_CS, HIGH);
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0);
  if (!SD_MMC.begin("/root", true)) {
    Serial.println(F("ERROR: File System Mount Failed!"));
  } else {
    File mjpegFile = SD_MMC.open(MJPEG_FILENAME, "r");
    if (!mjpegFile || mjpegFile.isDirectory()) {
      Serial.println(F("ERROR: Failed to open " MJPEG_FILENAME " file for reading"));
    } else {
      uint8_t *mjpeg_buf = (unsigned char *)jpeg_malloc_align(MJPEG_BUFFER_SIZE, 16);
      if (!mjpeg_buf) {
        Serial.println(F("mjpeg_buf malloc failed!"));
      } else {
        Serial.println(F("MJPEG start"));

        start_ms = millis();
        curr_ms = millis();
        mjpeg.setup(&mjpegFile, mjpeg_buf, true /* useBigEndian */);
        while (mjpegFile.available() && mjpeg.readMjpegBuf()) {
          // Read video
          total_read_video += millis() - curr_ms;
          curr_ms = millis();

          // Play video
          mjpeg.decodeJpg(jpegDrawCallback);
          total_decode_video += millis() - curr_ms;

          curr_ms = millis();
          total_frames++;
        }
        int time_used = millis() - start_ms;
        Serial.println(F("MJPEG end"));
        mjpegFile.close();
        float fps = 1000.0 * total_frames / time_used;
        total_decode_video -= total_show_video;
        Serial.printf("Total frames: %d\n", total_frames);
        Serial.printf("Time used: %d ms\n", time_used);
        Serial.printf("Average FPS: %0.1f\n", fps);
        Serial.printf("Read MJPEG: %lu ms (%0.1f %%)\n", total_read_video, 100.0 * total_read_video / time_used);
        Serial.printf("Decode video: %lu ms (%0.1f %%)\n", total_decode_video, 100.0 * total_decode_video / time_used);
        Serial.printf("Show video: %lu ms (%0.1f %%)\n", total_show_video, 100.0 * total_show_video / time_used);
      }
    }
  }
}

void loop() {
}
