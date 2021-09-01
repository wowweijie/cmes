#!/bin/bash

service ssh start
useradd -m -d /home/cmes_user -s /bin/bash -G sudo cmes_user
echo -e "cmes_user\ncmes_user" | passwd cmes_user