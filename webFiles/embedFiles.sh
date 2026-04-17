#!/bin/bash

rm ../web_files.h

for f in *.html *.css *.js; do gzip -fk "$f"; done
for f in *.gz *.pdf *.webp *.jpg; do xxd -i "$f" | sed 's/unsigned char/const unsigned char/' >>web_files.h; done
mv web_files.h ../
