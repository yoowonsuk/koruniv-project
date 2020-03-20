#!/bin/bash

if [ "$#" -eq 1 ]; then
  psql -U postgres -d practice -a -f $1
fi
if [ "$#" -eq 2]; then
  psql -U postgres -d $1 -a -f $2
fi
