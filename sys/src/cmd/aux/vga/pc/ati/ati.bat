rem this extracts modes from an ATI board

echo 320 200 8>320.8
dm -t ati -m 13 "320x200x8" >>320.8

echo 640 480 1>640.1
dm -t ati -m 12 "640x480x1" >>640.1

echo 640 480 8>640.8
dm -t ati -m 62 "640x480x8" >>640.8
mode co80

echo 800 600 1>800.1
dm -t ati -m 53 "800x600x1" >>800.1

echo 800 600 1>800.1a
dm -t ati -m 54 "800x600x1" >>800.1a

echo 800 600 8>800.8
dm -t ati -m 63 "800x600x8">>800.8
mode co80

echo 1024 768 1>1024.1
dm -t ati -m 55 "1024x768x1" >>1024.1

echo 1024 768 1>1024.1a
dm -t ati -m 65 "1024x768x1" >>1024.1a
mode co80

