#!/bin/bash
python create-tex.py && \
    ruby texify.rb && \
    git rev-parse HEAD > rev.tex && \
    date "+%B %-d, %Y" > date.tex && \
    lualatex book.tex && \
    SOURCE_DATE_EPOCH=1501250400 lualatex book.tex
