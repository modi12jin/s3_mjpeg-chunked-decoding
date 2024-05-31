# 本项目安装库

https://github.com/esp-arduino-libs/ESP32_JPEG/tree/feat/add_s3_block_decode

# 请使用arduino_esp32_v3版本

# ffmpeg转换代码

```
ffmpeg -i input.mp4 -vf "fps=30,scale=-1:272:flags=lanczos,crop=480:in_h:(in_w-480)/2:0" -q:v 9 480_30fps.mjpeg
```

# 注意事项

>+ 启动PSRAM

# 目前已知致命问题

>+ 播放完,会重启