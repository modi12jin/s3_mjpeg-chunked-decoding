#ifndef _MJPEGCLASS_H_
#define _MJPEGCLASS_H_

#define READ_BUFFER_SIZE 1024

#include <ESP32_JPEG_Library.h>
#include <FS.h>

class MjpegClass
{
public:
  bool setup(Stream *input, uint8_t *mjpeg_buf, bool useBigEndian)
  {
    _input = input;
    _mjpeg_buf = mjpeg_buf;
    _useBigEndian = useBigEndian;
    _inputindex = 0;

    if (!_read_buf)
    {
      _read_buf = (uint8_t *)malloc(READ_BUFFER_SIZE);
    }

    if (!_read_buf)
    {
      return false;
    }

    return true;
  }

  bool readMjpegBuf()
  {
    if (_inputindex == 0)
    {
      _buf_read = _input->readBytes(_read_buf, READ_BUFFER_SIZE);
      _inputindex += _buf_read;
    }
    _mjpeg_buf_offset = 0;
    int i = 0;
    bool found_FFD8 = false;
    while ((_buf_read > 0) && (!found_FFD8))
    {
      i = 0;
      while ((i < _buf_read) && (!found_FFD8))
      {
        if ((_read_buf[i] == 0xFF) && (_read_buf[i + 1] == 0xD8)) // JPEG header
        {
          // Serial.printf("Found FFD8 at: %d.\n", i);
          found_FFD8 = true;
        }
        ++i;
      }
      if (found_FFD8)
      {
        --i;
      }
      else
      {
        _buf_read = _input->readBytes(_read_buf, READ_BUFFER_SIZE);
      }
    }
    uint8_t *_p = _read_buf + i;
    _buf_read -= i;
    bool found_FFD9 = false;
    if (_buf_read > 0)
    {
      i = 3;
      while ((_buf_read > 0) && (!found_FFD9))
      {
        if ((_mjpeg_buf_offset > 0) && (_mjpeg_buf[_mjpeg_buf_offset - 1] == 0xFF) && (_p[0] == 0xD9)) // JPEG trailer
        {
          // Serial.printf("Found FFD9 at: %d.\n", i);
          found_FFD9 = true;
        }
        else
        {
          while ((i < _buf_read) && (!found_FFD9))
          {
            if ((_p[i] == 0xFF) && (_p[i + 1] == 0xD9)) // JPEG trailer
            {
              found_FFD9 = true;
              ++i;
            }
            ++i;
          }
        }

        // Serial.printf("i: %d\n", i);
        memcpy(_mjpeg_buf + _mjpeg_buf_offset, _p, i);
        _mjpeg_buf_offset += i;
        size_t o = _buf_read - i;
        if (o > 0)
        {
          // Serial.printf("o: %d\n", o);
          memcpy(_read_buf, _p + i, o);
          _buf_read = _input->readBytes(_read_buf + o, READ_BUFFER_SIZE - o);
          _p = _read_buf;
          _inputindex += _buf_read;
          _buf_read += o;
          // Serial.printf("_buf_read: %d\n", _buf_read);
        }
        else
        {
          _buf_read = _input->readBytes(_read_buf, READ_BUFFER_SIZE);
          _p = _read_buf;
          _inputindex += _buf_read;
        }
        i = 0;
      }
      if (found_FFD9)
      {
        return true;
      }
    }

    return false;
  }

  bool decodeJpg(int (*jpegDrawCallback)(jpeg_dec_io_t *jpeg_io,jpeg_dec_header_info_t *out_info))
  {
    _remain = _mjpeg_buf_offset;

    // Generate default configuration
    jpeg_dec_config_t config = {
        .output_type = JPEG_COLOR_SPACE_RGB565_BE,
        .rotate = JPEG_ROTATE_0D,
        .block_enable = 1,
    };

    // Create jpeg_dec
    jpeg_dec = jpeg_dec_open(&config);

    // Create io_callback handle
    jpeg_io = (jpeg_dec_io_t *)calloc(1, sizeof(jpeg_dec_io_t));

    // Create out_info handle
    out_info = (jpeg_dec_header_info_t *)calloc(1, sizeof(jpeg_dec_header_info_t));

    // Set input buffer and buffer len to io_callback
    jpeg_io->inbuf = _mjpeg_buf;
    jpeg_io->inbuf_len = _remain;

    jpeg_dec_parse_header(jpeg_dec, jpeg_io, out_info);

    output_len = out_info->width * (out_info->y_factory[0] << 3) * 2;
    // Malloc output block buffer
    output_block = (unsigned char *)jpeg_malloc_align(output_len, 16);

    jpeg_io->outbuf = output_block;

    while (jpeg_io->output_line < jpeg_io->output_height)
    {
      jpeg_dec_process(jpeg_dec, jpeg_io);
      jpegDrawCallback(jpeg_io, out_info);
    }

    jpeg_dec_close(jpeg_dec);
    free(jpeg_io);
    free(out_info);
    jpeg_free_align(output_block);

    return true;
  }

private:
  Stream *_input;
  unsigned char *_mjpeg_buf;
  unsigned char *output_block = NULL;
  int output_len = 0;
  bool _useBigEndian;

  uint8_t *_read_buf;
  int32_t _mjpeg_buf_offset = 0;

  jpeg_dec_handle_t *jpeg_dec = NULL;
  jpeg_dec_io_t *jpeg_io = NULL;
  jpeg_dec_header_info_t *out_info = NULL;

  int32_t _inputindex = 0;
  int32_t _buf_read;
  int32_t _remain = 0;
};

#endif // _MJPEGCLASS_H_