# Tea

Just a simple tea timer for
- green tea: *2:00*
- black tea: *3:00*
- mint tea: *5:00*

## Live demo 
[livedemo](https://renehorstmann.github.io/tea) compiled with emscripten.

## engine
Based on [some](https://github.com/renehorstmann/some) framework.

## Compiling for Web
Using Emscripten https://emscripten.org/

Tested under Ubuntu and WSL Ubuntu.
You should have already cloned the project and `cd` to that dir:

- Create a sub directory to compile the website
```sh
mkdir web && cp index.html web && cp icon/* web && cd web
```

- Copy all resources, because emscripten may not be able to use `../res`
```sh
cp -r ../res .
```

- Compile
```sh
emcc -O3 \
-I../include/ \
-s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_MIXER=2 -s FULL_ES3=1 -s \
EXPORTED_FUNCTIONS='["_main", "_e_io_idbfs_synced", "_e_io_file_upload_done"]' \
-s EXPORTED_RUNTIME_METHODS=FS \
-s SDL2_IMAGE_FORMATS='["png"]' \
--preload-file ./res \
-s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY=1 -s EXIT_RUNTIME=1 \
-lidbfs.js \
-DOPTION_GLES -DOPTION_SDL \
../src/e/*.c ../src/p/*.c ../src/r/*.c ../src/u/*.c ../src/*.c \
-o index.js
```

- Test the website (open a browser and call localhost:8000)
```sh
python3 -m http.server --bind localhost  # [port]
```

## Author

René Horstmann

## License
This project is licensed under the MIT License - see the someLICENSE file for details
The alarm sound [sound_alarm.wav](res/sound_alarm.wav) (converted from mp3) is licensed under CC BY-NC 4.0 - see [sound_alarm_LICENSE](sound_alarm_LICENSE)