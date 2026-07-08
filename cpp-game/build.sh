#!/bin/bash
# ترجمة اللعبة من C++ إلى ملف HTML واحد (WebAssembly)
# المتطلبات: Emscripten SDK  (https://emscripten.org)
#   git clone https://github.com/emscripten-core/emsdk && cd emsdk
#   ./emsdk install latest && ./emsdk activate latest && source emsdk_env.sh
set -e
cd "$(dirname "$0")"

emcc game.cpp \
  -O2 \
  -sUSE_SDL=2 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sSINGLE_FILE=1 \
  -sENVIRONMENT=web \
  --shell-file shell.html \
  -o "../سباق_الشوارع_BMW.html"

echo "تم! الملف الناتج: سباق_الشوارع_BMW.html"
