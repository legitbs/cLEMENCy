#!/bin/bash
cat instruction\ database.sql | sqlite3 instructions.db
python ./create-db.py
python ./create-html.py
